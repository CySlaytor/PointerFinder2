using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;

namespace PointerFinder2.Emulators.EmulatorManager
{
    public class RALibretroNDSManager : IEmulatorManager
    {
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        public const uint NDS_RAM_START = 0x02000000;
        public const uint NDS_RAM_SIZE = 0x400000; // 4MB
        public const uint NDS_RAM_END = NDS_RAM_START + NDS_RAM_SIZE;
        public const uint NDS_STATIC_START = 0x00100000;
        public const uint NDS_STATIC_END = 0x003FFFFF + 1;

        // Replaced fragile hardcoded offset with a robust signature scanning method.
        #region WinAPI
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern nint OpenProcess(int dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(nint hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern int VirtualQueryEx(nint hProcess, nint lpAddress, out MEMORY_BASIC_INFORMATION lpBuffer, uint dwLength);

        [StructLayout(LayoutKind.Sequential)]
        private struct MEMORY_BASIC_INFORMATION
        {
            public nint BaseAddress;
            public nint AllocationBase;
            public uint AllocationProtect;
            public ushort PartitionId;
            public nint RegionSize;
            public uint State;
            public uint Protect;
            public uint Type;
        }
        #endregion

        #region Constants
        private const int PROCESS_QUERY_INFORMATION = 0x0400;
        private const int PROCESS_VM_READ = 0x0010;
        private const int PROCESS_VM_WRITE = 0x0020;
        private const int PROCESS_VM_OPERATION = 0x0008;
        private const int ALL_ACCESS = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
        private const uint MEM_COMMIT = 0x1000;
        #endregion

        // Added a struct to define core-specific memory signatures.
        private struct CoreProfile
        {
            public string Name;
            public long SearchSize;
            public int Offset;
        }

        private static readonly CoreProfile[] CoreProfiles =
        {
            new CoreProfile { Name = "DeSmuME", SearchSize = 4456448, Offset = 0x112BF40 },
            new CoreProfile { Name = "melonDS", SearchSize = 17760256, Offset = 0x0 }
        };
        private string _foundCoreName = "Unknown";


        #region Interface Implementation
        // Updated EmulatorName to be dynamic based on the detected core.
        public string EmulatorName => $"RALibretro (NDS - {_foundCoreName})";
        public uint MainMemoryStart => NDS_RAM_START;
        public uint MainMemorySize => 4 * 1024 * 1024; // 4MB
        public string RetroAchievementsPrefix => "D";
        public Process EmulatorProcess { get; private set; }
        public bool IsAttached => ProcessHandle != IntPtr.Zero && MemoryBasePC != IntPtr.Zero;
        public nint ProcessHandle { get; private set; } = IntPtr.Zero;
        public nint MemoryBasePC { get; private set; } = IntPtr.Zero;
        private nint NdsMemoryBaseInPC { get; set; } = IntPtr.Zero;

        // Rewrote the Attach method to use memory scanning instead of a hardcoded offset.
        public bool Attach(Process process)
        {
            logger.Log($"[{EmulatorName}] Attempting to attach...");

            EmulatorProcess = process;

            if (EmulatorProcess?.MainModule == null)
            {
                logger.Log($"[{EmulatorName}] FAILURE: Process not found or main module is not accessible.");
                return false;
            }
            logger.Log($"[{EmulatorName}] SUCCESS: Attaching to process '{EmulatorProcess.ProcessName}' (ID: {EmulatorProcess.Id}).");

            if (!Environment.Is64BitProcess)
            {
                logger.Log($"[{EmulatorName}] FAILURE: This tool must be built for x64 to attach to RALibretro.");
                return false;
            }

            ProcessHandle = OpenProcess(ALL_ACCESS, false, EmulatorProcess.Id);
            if (ProcessHandle == IntPtr.Zero)
            {
                logger.Log($"[{EmulatorName}] FAILURE: Could not open process handle. Try running this tool as Administrator.");
                return false;
            }
            logger.Log($"[{EmulatorName}] SUCCESS: Process handle opened. Searching for core memory signatures...");

            nint parentBlockBase = nint.Zero;
            foreach (var profile in CoreProfiles)
            {
                logger.Log($"  -> Searching for {profile.Name} signature (Region size: {profile.SearchSize} bytes)...");
                parentBlockBase = FindBaseAddressBySize(profile.SearchSize);
                if (parentBlockBase != nint.Zero)
                {
                    _foundCoreName = profile.Name;
                    this.MemoryBasePC = parentBlockBase + profile.Offset;
                    logger.Log($"\n[{EmulatorName}] SUCCESS! Detected {_foundCoreName} core.");
                    logger.Log($"  -> Found parent block at: 0x{parentBlockBase:X}");
                    logger.Log($"  -> Applying offset:       + 0x{profile.Offset:X}");
                    logger.Log("--------------------------------------------------");
                    break;
                }
            }

            if (MemoryBasePC == nint.Zero)
            {
                logger.Log($"\n[{EmulatorName}] FATAL ERROR: Could not find memory signature for any known core (DeSmuME, melonDS).");
                logger.Log("Please ensure a DS game is fully loaded and running in RALibretro.");
                Detach();
                return false;
            }

            logger.Log($"[{EmulatorName}] Calculated DS Main RAM Base at PC Address: 0x{this.MemoryBasePC:X}.");

            NdsMemoryBaseInPC = IntPtr.Subtract(MemoryBasePC, (int)NDS_RAM_START);
            logger.Log($"[{EmulatorName}] Attachment complete. Ready for scanning.");
            return true;
        }

        // Detaches from the emulator process and cleans up resources.
        public void Detach()
        {
            if (ProcessHandle != IntPtr.Zero)
            {
                CloseHandle(ProcessHandle);
            }
            ProcessHandle = IntPtr.Zero;
            MemoryBasePC = IntPtr.Zero;
            NdsMemoryBaseInPC = IntPtr.Zero;
            EmulatorProcess = null;
            // Reset the found core name on detach.
            _foundCoreName = "Unknown";
            logger.Log($"[{EmulatorName}] Detached from process.");
        }

        // Reads a block of memory from the emulated NDS system.
        public byte[] ReadMemory(uint ndsAddress, int count)
        {
            if (!IsAttached) return null;
            nint addressInPC = IntPtr.Add(NdsMemoryBaseInPC, (int)ndsAddress);
            return Memory.ReadBytes(this.ProcessHandle, addressInPC, count);
        }

        // Reads a 32-bit unsigned integer from an NDS memory address.
        public uint? ReadUInt32(uint ndsAddress)
        {
            byte[] buffer = ReadMemory(ndsAddress, 4);
            return buffer != null ? BitConverter.ToUInt32(buffer, 0) : null;
        }

        // Checks if a value is a valid pointer target within the NDS's RAM.
        public bool IsValidPointerTarget(uint value)
        {
            return (value & 3) == 0 && (value >= NDS_RAM_START && value < NDS_RAM_END);
        }

        // Traverses a pointer path to verify its final address. Used for filtering.
        public uint? RecalculateFinalAddress(PointerPath path, uint expectedFinalAddress)
        {
            bool shouldLog = DebugSettings.LogFilterValidation;
            if (shouldLog)
            {
                logger.Log("--------------------------------------------------");
                logger.Log($"[{EmulatorName}] Validating Path: {FormatDisplayAddress(path.BaseAddress)} -> {path.GetOffsetsString()} (Expecting: {FormatDisplayAddress(expectedFinalAddress)})");
            }

            if (path == null || !path.Offsets.Any())
            {
                if (shouldLog) logger.Log($"[{EmulatorName}] FAILURE: Path is null or has no offsets.");
                return null;
            }

            if (shouldLog) logger.Log($"[{EmulatorName}] Step 0: Reading base address [0x{path.BaseAddress:X8}]");
            uint? currentAddress = ReadUInt32(path.BaseAddress);
            if (!currentAddress.HasValue)
            {
                if (shouldLog) logger.Log($"[{EmulatorName}] FAILURE: Could not read base address.");
                return null;
            }
            if (!IsValidPointerTarget(currentAddress.Value))
            {
                if (shouldLog) logger.Log($"[{EmulatorName}] FAILURE: Base address points to invalid memory 0x{currentAddress.Value:X8}.");
                return null;
            }
            if (shouldLog) logger.Log($"  -> Value: 0x{currentAddress.Value:X8}");

            for (int i = 0; i < path.Offsets.Count - 1; i++)
            {
                if (shouldLog) logger.Log($"[{EmulatorName}] Step {i + 1}: Applying Offset #{i + 1} ({path.Offsets[i]:+X;-X})");
                uint nextAddressToRead = currentAddress.Value + (uint)path.Offsets[i];
                if (shouldLog) logger.Log($"  -> Reading from [0x{nextAddressToRead:X8}]");
                currentAddress = ReadUInt32(nextAddressToRead);

                if (!currentAddress.HasValue)
                {
                    if (shouldLog) logger.Log($"[{EmulatorName}] FAILURE: Could not read memory at 0x{nextAddressToRead:X8}. Chain is broken.");
                    return null;
                }
                if (!IsValidPointerTarget(currentAddress.Value))
                {
                    if (shouldLog) logger.Log($"[{EmulatorName}] FAILURE: Intermediate pointer is invalid memory 0x{currentAddress.Value:X8}.");
                    return null;
                }
                if (shouldLog) logger.Log($"  -> Value: 0x{currentAddress.Value:X8}");
            }

            if (shouldLog) logger.Log($"[{EmulatorName}] Step {path.Offsets.Count}: Applying Final Offset ({path.Offsets.Last():+X;-X})");
            uint finalAddress = currentAddress.Value + (uint)path.Offsets.Last();
            if (shouldLog) logger.Log($"  -> Final Calculation: 0x{currentAddress.Value:X8} + 0x{path.Offsets.Last():X} = 0x{finalAddress:X8}");

            if (shouldLog)
            {
                bool isValid = finalAddress == expectedFinalAddress;
                logger.Log($"[{EmulatorName}] Comparison: Calculated (0x{finalAddress:X8}) == Expected (0x{expectedFinalAddress:X8}) -> {isValid.ToString().ToUpper()}");
            }

            return finalAddress;
        }

        // Formats a full NDS address into a shorter, user-friendly format.
        public string FormatDisplayAddress(uint address)
        {
            if (address >= NDS_RAM_START && address < NDS_RAM_END)
            {
                return (address - NDS_RAM_START).ToString("X");
            }
            return address.ToString("X8");
        }

        // Converts a user-entered address into a full NDS address.
        public uint UnnormalizeAddress(string address)
        {
            uint parsedAddress = uint.Parse(address.Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);
            if (parsedAddress < NDS_RAM_SIZE)
            {
                return parsedAddress + NDS_RAM_START;
            }
            return parsedAddress;
        }

        // No normalization needed for NDS.
        public (uint normalizedAddress, bool wasNormalized) NormalizeAddressForRead(uint address)
        {
            return (address, false);
        }

        // No special comparison needed for NDS.
        public bool AreAddressesEquivalent(uint addr1, uint addr2)
        {
            return addr1 == addr2;
        }

        // Provides a set of default settings specifically for NDS on RALibretro.
        public AppSettings GetDefaultSettings()
        {
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Getting default settings.");
            return new AppSettings
            {
                StaticAddressStart = "100000",
                StaticAddressEnd = "1FFFFF",
                MaxOffset = 4095,
                MaxLevel = 7,
                // Provide default for new MaxCandidates setting.
                MaxCandidates = 10000000,
                UseSliderRange = true,
                StopOnFirstPathFound = false,
                CandidatesPerLevel = 10
            };
        }

        // Added implementation for the new interface method.
        public long GetIndexForStateDump(uint address)
        {
            if (address >= MainMemoryStart && address < (MainMemoryStart + MainMemorySize))
            {
                return address - MainMemoryStart;
            }
            return -1;
        }
        #endregion

        // Added a helper method to scan process memory for a region of a specific size.
        private nint FindBaseAddressBySize(long exactRegionSizeBytes)
        {
            var foundAddresses = new List<nint>();
            nint currentAddress = nint.Zero;
            long MAX_ADDRESS = 0x7FFFFFFFFFFFL; // Max user-mode address for 64-bit processes

            while ((long)currentAddress < MAX_ADDRESS)
            {
                if (VirtualQueryEx(ProcessHandle, currentAddress, out var mbi, (uint)Marshal.SizeOf(typeof(MEMORY_BASIC_INFORMATION))) == 0)
                {
                    break;
                }

                if (mbi.State == MEM_COMMIT && (long)mbi.RegionSize == exactRegionSizeBytes)
                {
                    foundAddresses.Add(mbi.BaseAddress);
                }
                currentAddress = mbi.BaseAddress + mbi.RegionSize;
            }

            // The Python script implies the last found block is the correct one.
            return foundAddresses.Any() ? foundAddresses.Last() : nint.Zero;
        }
    }
}