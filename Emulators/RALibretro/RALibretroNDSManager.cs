using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Diagnostics;
using System.Linq;

namespace PointerFinder2.Emulators.RALibretro
{
    // Manages all interaction with the RALibretro emulator for Nintendo DS cores.
    public class RALibretroNDSManager : IEmulatorManager
    {
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        // NDS Memory Layout Constants.
        public const uint NDS_RAM_START = 0x02000000;
        public const uint NDS_RAM_SIZE = 0x400000; // 4MB
        public const uint NDS_RAM_END = NDS_RAM_START + NDS_RAM_SIZE;
        public const uint NDS_STATIC_START = 0x00100000;
        public const uint NDS_STATIC_END = 0x003FFFFF + 1;

        // For attachment to RALibretro v1.8.1. Newer versions may require updating this offset.
        private const int RAM_POINTER_OFFSET = 0x212D30;
        private readonly string[] SUPPORTED_CORES = { "desmume_libretro.dll", "melondsds_libretro.dll" };

        #region Interface Implementation
        public string EmulatorName => "RALibretro (NDS)";
        public uint MainMemorySize => 4 * 1024 * 1024; // 4MB
        public string RetroAchievementsPrefix => "D";
        public Process EmulatorProcess { get; private set; }
        public bool IsAttached => ProcessHandle != IntPtr.Zero && MemoryBasePC != IntPtr.Zero;
        public nint ProcessHandle { get; private set; } = IntPtr.Zero;
        public nint MemoryBasePC { get; private set; } = IntPtr.Zero;
        private nint NdsMemoryBaseInPC { get; set; } = IntPtr.Zero;

        // Attaches by finding the loaded NDS core and then reading a pointer at a hardcoded offset.
        public bool Attach(Process process)
        {
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Attempting to attach...");

            EmulatorProcess = process;

            if (EmulatorProcess?.MainModule == null)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Process not found or main module is not accessible.");
                return false;
            }
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] SUCCESS: Attaching to process '{EmulatorProcess.ProcessName}' (ID: {EmulatorProcess.Id}).");

            // RALibretro is a 64-bit process, so this tool must be as well.
            if (!Environment.Is64BitProcess)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: This tool must be built for x64 to attach to RALibretro.");
                return false;
            }

            // Check if a supported NDS core is loaded into the process.
            var activeCoreModule = EmulatorProcess.Modules.Cast<ProcessModule>()
                .FirstOrDefault(m => SUPPORTED_CORES.Any(c => c.Equals(m.ModuleName, StringComparison.OrdinalIgnoreCase)));

            if (activeCoreModule == null)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: No supported NDS core (DeSmuME or melonDS) found loaded in RALibretro.");
                return false;
            }
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] SUCCESS: Found active core '{activeCoreModule.ModuleName}'.");


            ProcessHandle = Memory.OpenProcessHandle(EmulatorProcess);
            if (ProcessHandle == IntPtr.Zero)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Could not open process handle. Try running this tool as Administrator.");
                return false;
            }
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] SUCCESS: Process handle opened.");

            // This attachment method relies on a hardcoded offset from the emulator's main module base address.
            nint moduleBaseAddress = EmulatorProcess.MainModule.BaseAddress;
            nint pointerAddress = IntPtr.Add(moduleBaseAddress, RAM_POINTER_OFFSET);
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Reading RAM pointer from address 0x{pointerAddress:X}.");

            long? ramPtr = Memory.ReadInt64(ProcessHandle, pointerAddress);
            if (!ramPtr.HasValue || ramPtr.Value == 0)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Could not read the pointer from 0x{pointerAddress:X}. Ensure the game is fully loaded. This offset may be outdated for your RALibretro version.");
                return false;
            }

            this.MemoryBasePC = (nint)ramPtr.Value;
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] SUCCESS: NDS RAM base (0x02000000) found in PC memory at 0x{this.MemoryBasePC:X}.");

            // Calculate a base address to translate any NDS address to a PC address.
            NdsMemoryBaseInPC = IntPtr.Subtract(MemoryBasePC, (int)NDS_RAM_START);
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Attachment complete. Ready for scanning.");
            return true;
        }

        // Detaches from the emulator process and cleans up resources.
        public void Detach()
        {
            if (ProcessHandle != IntPtr.Zero)
            {
                Memory.CloseHandle(ProcessHandle);
            }
            ProcessHandle = IntPtr.Zero;
            MemoryBasePC = IntPtr.Zero;
            NdsMemoryBaseInPC = IntPtr.Zero;
            EmulatorProcess = null;
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Detached from process.");
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
            // A valid pointer must be 4-byte aligned and fall within the 4MB RAM range.
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

            // Traverse the chain of pointers and offsets.
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

            // Apply the final offset to get the target address.
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
            // If the user enters a short address, assume it's in the main RAM region.
            if (parsedAddress < NDS_RAM_SIZE)
            {
                return parsedAddress + NDS_RAM_START;
            }
            return parsedAddress;
        }

        // Provides a set of default settings specifically for NDS on RALibretro.
        public AppSettings GetDefaultSettings()
        {
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Getting default settings.");
            return new AppSettings
            {
                StaticAddressStart = "100000",
                StaticAddressEnd = "3FFFFF",
                MaxOffset = 4095,
                MaxLevel = 7,
                MaxResults = 5000,
                AnalyzeStructures = true,
                ScanForStructureBase = true,
                MaxNegativeOffset = 1024,
                Use16ByteAlignment = false // Not applicable to NDS
            };
        }
        #endregion
    }
}