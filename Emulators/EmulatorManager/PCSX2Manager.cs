using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Diagnostics;
using System.Linq;

namespace PointerFinder2.Emulators.EmulatorManager
{
    public class Pcsx2Manager : IEmulatorManager
    {
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        // PS2 Memory Layout Constants.
        public const uint PS2_EEMEM_START = 0x20000000;
        public const uint PS2_EEMEM_END = 0x21FFFFFF + 1; // 32MB of EE RAM
        public const uint PS2_GAME_CODE_START = 0x00100000;
        public const uint PS2_GAME_CODE_END = 0x01FFFFFF + 1; // Main game code region

        #region Interface Implementation
        public string EmulatorName => "PCSX2";
        public uint MainMemoryStart => PS2_EEMEM_START;
        public uint MainMemorySize => 32 * 1024 * 1024; // 32MB
        public string RetroAchievementsPrefix => "X";
        public Process EmulatorProcess { get; private set; }
        public bool IsAttached => ProcessHandle != IntPtr.Zero && MemoryBasePC != IntPtr.Zero;
        public nint ProcessHandle { get; private set; } = IntPtr.Zero;
        public nint MemoryBasePC { get; private set; } = IntPtr.Zero;
        private nint Ps2MemoryBaseInPC { get; set; } = IntPtr.Zero;

        // Attaches by finding PCSX2's exported "EEMem" variable, which points to the emulated RAM.
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

            nint eeMemBasePC = Memory.FindExportedAddress(EmulatorProcess, ProcessHandle, "EEMem");
            if (eeMemBasePC == IntPtr.Zero)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Could not find the 'EEMem' export. Ensure the game is fully loaded.");
                return false;
            }

            long? eeMemPtr = Memory.ReadInt64(ProcessHandle, eeMemBasePC);
            if (!eeMemPtr.HasValue)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] FAILURE: Could not read the pointer from the 'EEMem' export address.");
                return false;
            }
            MemoryBasePC = (nint)eeMemPtr.Value;
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] SUCCESS: EEMem base found in PC memory at 0x{MemoryBasePC:X}.");

            Ps2MemoryBaseInPC = IntPtr.Subtract(MemoryBasePC, (int)PS2_EEMEM_START);
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
            Ps2MemoryBaseInPC = IntPtr.Zero;
            EmulatorProcess = null;
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Detached from process.");
        }

        // Reads a block of memory from the emulated PS2 RAM.
        public byte[] ReadMemory(uint ps2Address, int count)
        {
            if (!IsAttached) return null;
            nint address = IntPtr.Add(Ps2MemoryBaseInPC, (int)ps2Address);
            return Memory.ReadBytes(ProcessHandle, address, count);
        }

        // Reads a 32-bit unsigned integer from a PS2 memory address.
        public uint? ReadUInt32(uint ps2Address)
        {
            byte[] buffer = ReadMemory(ps2Address, 4);
            if (buffer == null)
            {
                return null;
            }
            return BitConverter.ToUInt32(buffer, 0);
        }

        // Checks if a value is a valid pointer target within the PS2's main memory regions.
        public bool IsValidPointerTarget(uint value)
        {
            if ((value & 3) != 0) return false; // Must be 4-byte aligned.
            bool isGameCode = value >= PS2_GAME_CODE_START && value < PS2_GAME_CODE_END;
            bool isEeMem = value >= PS2_EEMEM_START && value < PS2_EEMEM_END;
            return isGameCode || isEeMem;
        }

        // Traverses a pointer path to verify its final address. Used for filtering.
        public uint? RecalculateFinalAddress(PointerPath path, uint expectedFinalAddress)
        {
            bool shouldLog = DebugSettings.LogFilterValidation;

            (expectedFinalAddress, _) = NormalizeAddressForRead(expectedFinalAddress);

            if (shouldLog)
            {
                logger.Log("--------------------------------------------------");
                logger.Log($"[{EmulatorName}] Validating Path: {path.BaseAddress:X8} -> {path.GetOffsetsString()} (Expecting: 0x{expectedFinalAddress:X8})");
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
                if (shouldLog) logger.Log($"  -> Calculating next address to read: 0x{currentAddress.Value:X8} + 0x{path.Offsets[i]:X} = 0x{nextAddressToRead:X8}");

                (nextAddressToRead, bool wasNorm) = NormalizeAddressForRead(nextAddressToRead);
                if (wasNorm && shouldLog) logger.Log($"  -> Normalizing address to 0x{nextAddressToRead:X8}");

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

            (finalAddress, bool finalNorm) = NormalizeAddressForRead(finalAddress);
            if (finalNorm && shouldLog) logger.Log($"  -> Normalizing FINAL address to 0x{finalAddress:X8}");

            if (shouldLog)
            {
                bool isValid = finalAddress == expectedFinalAddress;
                logger.Log($"[{EmulatorName}] Comparison: Calculated (0x{finalAddress:X8}) == Expected (0x{expectedFinalAddress:X8}) -> {isValid.ToString().ToUpper()}");
            }

            return finalAddress;
        }

        // Formats a full PS2 address into a shorter format relative to EE RAM for display.
        public string FormatDisplayAddress(uint address)
        {
            if (address >= 0x20000000 && address < 0x22000000)
            {
                return (address - 0x20000000).ToString("X");
            }
            return address.ToString("X8");
        }

        // Converts a user-entered address into a full PS2 memory address.
        public uint UnnormalizeAddress(string address)
        {
            uint parsedAddress = uint.Parse(address.Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);
            if (parsedAddress < 0x2000000)
            {
                return parsedAddress + 0x20000000;
            }
            return parsedAddress;
        }

        // Handles PS2's address mirroring (e.g., 0x0... -> 0x20...).
        public (uint normalizedAddress, bool wasNormalized) NormalizeAddressForRead(uint address)
        {
            if (address < PS2_EEMEM_START)
            {
                return (address + PS2_EEMEM_START, true);
            }
            return (address, false);
        }

        // Compares two PS2 addresses, accounting for mirroring.
        public bool AreAddressesEquivalent(uint addr1, uint addr2)
        {
            uint normAddr1 = (addr1 < PS2_EEMEM_START) ? addr1 + PS2_EEMEM_START : addr1;
            uint normAddr2 = (addr2 < PS2_EEMEM_START) ? addr2 + PS2_EEMEM_START : addr2;
            return normAddr1 == normAddr2;
        }

        // Provides a set of default settings specifically for PCSX2.
        public AppSettings GetDefaultSettings()
        {
            if (DebugSettings.LogLiveScan) logger.Log($"[{EmulatorName}] Getting default settings.");
            return new AppSettings
            {
                StaticAddressStart = "100000",
                StaticAddressEnd = "7FFFFF",
                MaxOffset = 4095,
                MaxLevel = 7,
                MaxResults = 500000,
                // Provide default for new MaxCandidates setting.
                MaxCandidates = 10000000,
                ScanForStructureBase = false,
                MaxNegativeOffset = 1024,
                Use16ByteAlignment = true,
                StopOnFirstPathFound = false,
                CandidatesPerLevel = 10
            };
        }

        // Added implementation for the new interface method.
        public long GetIndexForStateDump(uint address)
        {
            (uint normalizedAddress, _) = NormalizeAddressForRead(address);
            if (normalizedAddress >= MainMemoryStart && normalizedAddress < (MainMemoryStart + MainMemorySize))
            {
                return normalizedAddress - MainMemoryStart;
            }
            return -1;
        }
        #endregion
    }
}