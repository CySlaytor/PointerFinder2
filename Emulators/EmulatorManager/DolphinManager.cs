using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;

namespace PointerFinder2.Emulators.EmulatorManager
{
    // Added new manager for Dolphin, supporting both GameCube and Wii with auto-detection.
    public class DolphinManager : IEmulatorManager
    {
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        #region WinAPI
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ReadProcessMemory(nint hProcess, nint lpBaseAddress, [Out] byte[] lpBuffer, int dwSize, out int lpNumberOfBytesRead);

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

        // --- MEM1 (GameCube RAM, 24MB) ---
        public const uint MEM1_INGAME_BASE = 0x80000000;
        public const uint MEM1_SIZE = 24 * 1024 * 1024;
        public const uint MEM1_INGAME_END = MEM1_INGAME_BASE + MEM1_SIZE;
        private static readonly byte[] MEM1_SIGNATURE = { 0x7C, 0x90, 0x43, 0xA6, 0x80, 0x80, 0x00, 0xC0, 0x90, 0x64, 0x00, 0x0C, 0x7C, 0x70, 0x42, 0xA6, 0x90, 0x64, 0x00, 0x10, 0x90, 0xA4, 0x00, 0x14, 0xA0, 0x64, 0x01, 0xA2, 0x60, 0x63, 0x00, 0x02, 0xB0, 0x64, 0x01, 0xA2, 0x7C, 0x60, 0x00, 0x26, 0x90, 0x64, 0x00, 0x80, 0x7C, 0x68, 0x02, 0xA6, 0x90, 0x64, 0x00, 0x84, 0x7C, 0x69, 0x02, 0xA6, 0x90, 0x64, 0x00, 0x88, 0x7C, 0x61, 0x02, 0xA6 };
        private const int SIGNATURE_OFFSET_IN_MEM1 = 0x100;

        // --- MEM2 (Wii RAM Extension, 56MB) ---
        public const uint MEM2_INGAME_BASE = 0x90000000;
        public const uint MEM2_SIZE = 56 * 1024 * 1024; // Actually 64MB, but only ~56MB is usable by games. We use the allocation size.
        public const uint MEM2_INGAME_END = MEM2_INGAME_BASE + MEM2_SIZE;
        private static readonly byte[] MEM2_SIGNATURE = { 0x02, 0x9f, 0x00, 0x10, 0x02, 0x9f, 0x00, 0x33, 0x02, 0x9f, 0x00, 0x34, 0x02, 0x9f, 0x00, 0x35, 0x02, 0x9f, 0x00, 0x36, 0x02, 0x9f, 0x00, 0x37, 0x02, 0x9f, 0x00, 0x38, 0x02, 0x9f, 0x00, 0x39 };
        private const int SIGNATURE_OFFSET_IN_MEM2 = 0;

        // --- Wii-Specific Validation ---
        private const long MEM1_MEM2_PC_OFFSET = 0x10000000;
        #endregion

        private enum SystemType { Unknown, GameCube, Wii }
        private SystemType _detectedSystem = SystemType.Unknown;
        private nint _mem1BasePC = nint.Zero;
        private nint _mem2BasePC = nint.Zero;

        public string EmulatorName => $"Dolphin ({_detectedSystem})";
        public uint MainMemoryStart => MEM1_INGAME_BASE;
        public uint MainMemorySize => _detectedSystem == SystemType.Wii ? MEM1_SIZE + MEM2_SIZE : MEM1_SIZE;
        public string RetroAchievementsPrefix => "G"; // For Big-Endian
        public Process EmulatorProcess { get; private set; }
        public bool IsAttached => ProcessHandle != nint.Zero && _mem1BasePC != nint.Zero;
        public nint ProcessHandle { get; private set; } = nint.Zero;
        public nint MemoryBasePC => _mem1BasePC;

        public bool Attach(Process process)
        {
            logger.Log($"[{EmulatorName}] Attempting to attach to Dolphin...");
            EmulatorProcess = process;
            ProcessHandle = OpenProcess(ALL_ACCESS, false, EmulatorProcess.Id);

            if (ProcessHandle == nint.Zero)
            {
                logger.Log($"[{EmulatorName}] FAILURE: Could not open process handle. Run as Administrator.");
                return false;
            }

            // --- Find Memory Candidates ---
            var mem1Candidates = FindCandidateAddresses("MEM1", MEM1_SIGNATURE, SIGNATURE_OFFSET_IN_MEM1, MEM1_SIZE);
            var mem2Candidates = FindCandidateAddresses("MEM2", MEM2_SIGNATURE, SIGNATURE_OFFSET_IN_MEM2, MEM2_SIZE);

            if (!mem1Candidates.Any())
            {
                logger.Log($"[{EmulatorName}] FATAL ERROR: Could not find MEM1. Ensure game is running.");
                Detach();
                return false;
            }

            _mem1BasePC = mem1Candidates.Last(); // Primary heuristic: the last found block is the correct one.
            logger.Log($"[{EmulatorName}] SUCCESS! Found MEM1 at PC Address: 0x{_mem1BasePC:X}");

            // --- Auto-Detect System (GC vs Wii) ---
            if (mem2Candidates.Any())
            {
                nint lastMem2Candidate = mem2Candidates.Last();
                logger.Log($"[{EmulatorName}] Found MEM2 candidate at 0x{lastMem2Candidate:X}. Validating layout...");

                // Secondary heuristic: for Wii, MEM2 must be at a fixed offset from MEM1.
                if (lastMem2Candidate == _mem1BasePC + MEM1_MEM2_PC_OFFSET)
                {
                    _mem2BasePC = lastMem2Candidate;
                    _detectedSystem = SystemType.Wii;
                    logger.Log($"[{EmulatorName}] SUCCESS! MEM2 validated. System is Wii.");
                }
                else
                {
                    _detectedSystem = SystemType.GameCube;
                    logger.Log($"[{EmulatorName}] WARNING: MEM2 found but layout is incorrect. Falling back to GameCube mode. This can happen if RAIntegration is active.");
                }
            }
            else
            {
                _detectedSystem = SystemType.GameCube;
                logger.Log($"[{EmulatorName}] INFO: MEM2 not found. System is GameCube.");
            }

            logger.Log($"[{EmulatorName}] Attachment complete. Ready for scanning.");
            return true;
        }

        public void Detach()
        {
            if (ProcessHandle != nint.Zero)
            {
                CloseHandle(ProcessHandle);
            }
            ProcessHandle = nint.Zero;
            _mem1BasePC = nint.Zero;
            _mem2BasePC = nint.Zero;
            EmulatorProcess = null;
            _detectedSystem = SystemType.Unknown;
            logger.Log($"[{EmulatorName}] Detached from process.");
        }

        public byte[] ReadMemory(uint address, int count)
        {
            if (!IsAttached) return null;

            // --- State-based Scan Dump for Wii ---
            if (_detectedSystem == SystemType.Wii && count == (MEM1_SIZE + MEM2_SIZE) && address == MEM1_INGAME_BASE)
            {
                logger.Log($"[{EmulatorName}] Performing combined MEM1+MEM2 memory dump for state-based scan...");
                byte[] mem1Dump = ReadMemoryBlock(_mem1BasePC, (int)MEM1_SIZE);
                byte[] mem2Dump = ReadMemoryBlock(_mem2BasePC, (int)MEM2_SIZE);
                if (mem1Dump == null || mem2Dump == null) return null;

                byte[] combined = new byte[count];
                Buffer.BlockCopy(mem1Dump, 0, combined, 0, (int)MEM1_SIZE);
                Buffer.BlockCopy(mem2Dump, 0, combined, (int)MEM1_SIZE, (int)MEM2_SIZE);
                return combined;
            }

            // --- Standard Memory Reads ---
            if (address >= MEM1_INGAME_BASE && address < MEM1_INGAME_END)
            {
                // Use IntPtr.Add for safe pointer arithmetic and cast the offset to int.
                nint pcAddress = IntPtr.Add(_mem1BasePC, (int)(address - MEM1_INGAME_BASE));
                return ReadMemoryBlock(pcAddress, count);
            }

            if (_detectedSystem == SystemType.Wii && address >= MEM2_INGAME_BASE && address < MEM2_INGAME_END)
            {
                // Use IntPtr.Add for safe pointer arithmetic and cast the offset to int.
                nint pcAddress = IntPtr.Add(_mem2BasePC, (int)(address - MEM2_INGAME_BASE));
                return ReadMemoryBlock(pcAddress, count);
            }

            return null;
        }

        private byte[] ReadMemoryBlock(nint pcAddress, int count)
        {
            byte[] buffer = new byte[count];
            ReadProcessMemory(ProcessHandle, pcAddress, buffer, count, out _);
            return buffer;
        }

        public uint? ReadUInt32(uint address)
        {
            byte[] buffer = ReadMemory(address, 4);
            if (buffer == null) return null;
            // Convert from Big-Endian to Little-Endian (system native)
            Array.Reverse(buffer);
            return BitConverter.ToUInt32(buffer, 0);
        }

        public bool IsValidPointerTarget(uint value)
        {
            if ((value & 3) != 0) return false;
            bool inMem1 = value >= MEM1_INGAME_BASE && value < MEM1_INGAME_END;
            if (inMem1) return true;
            if (_detectedSystem == SystemType.Wii)
            {
                return value >= MEM2_INGAME_BASE && value < MEM2_INGAME_END;
            }
            return false;
        }

        public uint? RecalculateFinalAddress(PointerPath path, uint expectedFinalAddress)
        {
            uint? currentAddress = ReadUInt32(path.BaseAddress);
            if (!currentAddress.HasValue || !IsValidPointerTarget(currentAddress.Value)) return null;

            for (int i = 0; i < path.Offsets.Count - 1; i++)
            {
                uint nextAddressToRead = currentAddress.Value + (uint)path.Offsets[i];
                currentAddress = ReadUInt32(nextAddressToRead);
                if (!currentAddress.HasValue || !IsValidPointerTarget(currentAddress.Value)) return null;
            }
            return currentAddress.Value + (uint)path.Offsets.Last();
        }

        // Updated address formatting to use RAIntegration's memory mapping for Wii MEM2.
        public string FormatDisplayAddress(uint address)
        {
            // Handle MEM1 (0x8... physical) -> 0x... offset
            if (address >= MEM1_INGAME_BASE && address < MEM1_INGAME_END)
            {
                return (address - MEM1_INGAME_BASE).ToString("X");
            }

            // Handle MEM2 (0x9... physical) -> 0x1... offset for Wii
            if (_detectedSystem == SystemType.Wii && address >= MEM2_INGAME_BASE && address < MEM2_INGAME_END)
            {
                // This is the RAIntegration mapping
                return (0x10000000 + (address - MEM2_INGAME_BASE)).ToString("X");
            }

            // Fallback for any other address (shouldn't happen for valid paths)
            return address.ToString("X8");
        }

        public uint UnnormalizeAddress(string address)
        {
            uint parsed = uint.Parse(address.Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);

            // 1. If the user provides a full, valid game address, use it directly.
            if (IsValidPointerTarget(parsed))
            {
                return parsed;
            }

            // 2. Handle RAIntegration's mapping for Wii's MEM2 (0x1... -> 0x9...).
            if (_detectedSystem == SystemType.Wii && (parsed >= 0x10000000 && parsed < 0x20000000))
            {
                return (parsed - 0x10000000) + MEM2_INGAME_BASE;
            }

            // 3. Fallback: Treat as an offset from MEM1. This handles RA's 0x0... addresses
            //    and simple user-entered offsets like "54D2C0".
            return parsed + MEM1_INGAME_BASE;
        }

        public (uint normalizedAddress, bool wasNormalized) NormalizeAddressForRead(uint address)
        {
            return (address, false); // No normalization needed for Dolphin
        }

        public bool AreAddressesEquivalent(uint addr1, uint addr2)
        {
            return addr1 == addr2; // No special comparison needed
        }

        public AppSettings GetDefaultSettings()
        {
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
                Use16ByteAlignment = false, // Dolphin memory is not typically 16-byte aligned like PS2
            };
        }

        public long GetIndexForStateDump(uint address)
        {
            if (address >= MEM1_INGAME_BASE && address < MEM1_INGAME_END)
            {
                return address - MEM1_INGAME_BASE;
            }
            if (_detectedSystem == SystemType.Wii && address >= MEM2_INGAME_BASE && address < MEM2_INGAME_END)
            {
                return (address - MEM2_INGAME_BASE) + MEM1_SIZE;
            }
            return -1; // Invalid address
        }

        private List<nint> FindCandidateAddresses(string blockName, byte[] signature, int signatureOffset, uint exactRegionSize)
        {
            var foundAddresses = new List<nint>();
            nint currentAddress = nint.Zero;
            logger.Log($"[{EmulatorName}] Scanning for {blockName} block (Exact Size: {exactRegionSize / (1024 * 1024)}MB)...");

            while ((long)currentAddress < 0x7FFFFFFFFFFFL)
            {
                if (VirtualQueryEx(ProcessHandle, currentAddress, out var mbi, (uint)Marshal.SizeOf(typeof(MEMORY_BASIC_INFORMATION))) == 0)
                {
                    break;
                }

                if (mbi.State == MEM_COMMIT && (long)mbi.RegionSize == exactRegionSize)
                {
                    logger.Log($"  -> Found candidate region of exact size at 0x{mbi.BaseAddress:X}. Verifying signature...");
                    byte[] regionBuffer = ReadMemoryBlock(mbi.BaseAddress, signature.Length + signatureOffset);

                    if (regionBuffer != null)
                    {
                        var signatureSlice = new Span<byte>(regionBuffer, signatureOffset, signature.Length);
                        if (signatureSlice.SequenceEqual(signature))
                        {
                            nint baseAddress = mbi.BaseAddress;
                            logger.Log($"  --> Signature VALID. Calculated base: 0x{baseAddress:X}");
                            foundAddresses.Add(baseAddress);
                        }
                    }
                }
                currentAddress = mbi.BaseAddress + mbi.RegionSize;
            }
            return foundAddresses;
        }
    }
}