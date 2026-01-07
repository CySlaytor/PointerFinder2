using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Diagnostics;
using System.Linq;

namespace PointerFinder2.Emulators.EmulatorManager
{
    // Manages all interaction with the DuckStation emulator.
    public class DuckStationManager : IEmulatorManager
    {
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        public const uint PS1_RAM_START = 0x80000000;
        public const uint PS1_RAM_SIZE = 0x200000; // 2MB
        public const uint PS1_RAM_END = PS1_RAM_START + PS1_RAM_SIZE;

        #region Interface Implementation
        public string EmulatorName => "DuckStation";
        public uint MainMemoryStart => PS1_RAM_START;
        public uint MainMemorySize => 2 * 1024 * 1024; // 2MB
        public string RetroAchievementsPrefix => "W";
        public Process EmulatorProcess { get; private set; }
        public bool IsAttached => ProcessHandle != IntPtr.Zero && MemoryBasePC != IntPtr.Zero;
        public nint ProcessHandle { get; private set; } = IntPtr.Zero;
        public nint MemoryBasePC { get; private set; } = IntPtr.Zero;

        // Attaches by finding DuckStation's exported "RAM" variable, which points to the emulated RAM.
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

            ProcessHandle = Memory.OpenProcessHandle(EmulatorProcess);
            if (ProcessHandle == IntPtr.Zero)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Could not open process handle. Try running this tool as Administrator.");
                return false;
            }
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] SUCCESS: Process handle opened.");

            nint ramExportAddress = Memory.FindExportedAddress(EmulatorProcess, ProcessHandle, "RAM");
            if (ramExportAddress == IntPtr.Zero)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Could not find the 'RAM' export. Ensure the game is fully loaded.");
                return false;
            }

            long? ramPtr = Memory.ReadInt64(ProcessHandle, ramExportAddress);
            if (!ramPtr.HasValue)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Could not read the pointer from the 'RAM' export address.");
                return false;
            }

            this.MemoryBasePC = (nint)ramPtr.Value;
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] SUCCESS: PS1 RAM base found in PC memory at 0x{this.MemoryBasePC:X}.");
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
            EmulatorProcess = null;
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Detached from process.");
        }

        // Reads a block of memory from the emulated PS1 RAM.
        public byte[] ReadMemory(uint ps1Address, int count)
        {
            if (!IsAttached || ps1Address < PS1_RAM_START || ps1Address >= PS1_RAM_END) return null;

            nint addressInPC = IntPtr.Add(this.MemoryBasePC, (int)(ps1Address - PS1_RAM_START));

            return Memory.ReadBytes(this.ProcessHandle, addressInPC, count);
        }

        // Reads a 32-bit unsigned integer from a PS1 memory address.
        public uint? ReadUInt32(uint ps1Address)
        {
            byte[] buffer = ReadMemory(ps1Address, 4);
            return buffer != null ? BitConverter.ToUInt32(buffer, 0) : null;
        }

        // Checks if a value is a valid pointer target within the PS1's RAM.
        public bool IsValidPointerTarget(uint value)
        {
            return (value & 3) == 0 && (value >= PS1_RAM_START && value < PS1_RAM_END);
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

        // Formats a full PS1 address into a shorter, user-friendly format (e.g., "1B4A0").
        public string FormatDisplayAddress(uint address)
        {
            if (address >= PS1_RAM_START && address < PS1_RAM_END)
            {
                return (address - PS1_RAM_START).ToString("X");
            }
            return address.ToString("X8");
        }

        // Converts a user-entered address (e.g., "1B4A0") into a full PS1 address.
        public uint UnnormalizeAddress(string address)
        {
            uint parsedAddress = uint.Parse(address.Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);
            if (parsedAddress < PS1_RAM_SIZE)
            {
                return parsedAddress + PS1_RAM_START;
            }
            return parsedAddress;
        }

        // No normalization needed for DuckStation.
        public (uint normalizedAddress, bool wasNormalized) NormalizeAddressForRead(uint address)
        {
            return (address, false);
        }

        // No special comparison needed for DuckStation.
        public bool AreAddressesEquivalent(uint addr1, uint addr2)
        {
            return addr1 == addr2;
        }

        // Provides a set of default settings specifically for DuckStation.
        public AppSettings GetDefaultSettings()
        {
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Getting default settings.");
            return new AppSettings
            {
                StaticAddressStart = "10000",
                StaticAddressEnd = "7FFFF",
                MaxOffset = 4095,
                MaxLevel = 7,
                // Provide default for new MaxCandidates setting.
                MaxCandidates = 10000000,
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
    }
}