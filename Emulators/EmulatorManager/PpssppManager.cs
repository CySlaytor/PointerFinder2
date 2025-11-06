using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace PointerFinder2.Emulators.EmulatorManager
{
    // Added new manager for PPSSPP, using a memory scan by region size for attachment.
    public class PpssppManager : IEmulatorManager
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

        public const uint PSP_MEM_START = 0x08000000;
        public const uint PSP_MEM_END = 0x0A000000; // 0x09FFFFFF + 1
        public const uint PSP_MEM_SIZE = 32 * 1024 * 1024; // 32MB logical size
        private const long PSP_SCAN_SIZE = 31 * 1024 * 1024; // 31MB physical allocation size
        #endregion

        public string EmulatorName => "PPSSPP";
        public uint MainMemoryStart => PSP_MEM_START;
        public uint MainMemorySize => PSP_MEM_SIZE;
        public string RetroAchievementsPrefix => "X";
        public Process EmulatorProcess { get; private set; }
        public bool IsAttached => ProcessHandle != nint.Zero && MemoryBasePC != nint.Zero;
        public nint ProcessHandle { get; private set; } = nint.Zero;
        public nint MemoryBasePC { get; private set; } = nint.Zero;

        public bool Attach(Process process)
        {
            logger.Log($"[{EmulatorName}] Attempting to attach to PPSSPP...");
            EmulatorProcess = process;
            ProcessHandle = OpenProcess(ALL_ACCESS, false, EmulatorProcess.Id);

            if (ProcessHandle == nint.Zero)
            {
                logger.Log($"[{EmulatorName}] FAILURE: Could not open process handle. Run as Administrator.");
                return false;
            }

            MemoryBasePC = FindPspRamBaseBySize();

            if (MemoryBasePC == nint.Zero)
            {
                logger.Log($"[{EmulatorName}] FATAL ERROR: Could not find PSP RAM block. Ensure a game is running.");
                Detach();
                return false;
            }

            logger.Log($"[{EmulatorName}] SUCCESS! Found PSP RAM Base at PC Address: 0x{MemoryBasePC:X}");
            logger.Log($"[{EmulatorName}] Attachment complete. Ready for scanning.");
            return true;
        }

        private nint FindPspRamBaseBySize()
        {
            var foundAddresses = new List<nint>();
            nint currentAddress = nint.Zero;
            long MAX_ADDRESS = 0x7FFFFFFFFFFFL;
            logger.Log($"[{EmulatorName}] Scanning for PSP RAM block (Exact Size: {PSP_SCAN_SIZE / (1024 * 1024)}MB)...");

            while ((long)currentAddress < MAX_ADDRESS)
            {
                if (VirtualQueryEx(ProcessHandle, currentAddress, out var mbi, (uint)Marshal.SizeOf(typeof(MEMORY_BASIC_INFORMATION))) == 0)
                {
                    break;
                }

                if (mbi.State == MEM_COMMIT && (long)mbi.RegionSize == PSP_SCAN_SIZE)
                {
                    logger.Log($"  -> Found candidate region of exact size at 0x{mbi.BaseAddress:X}.");
                    foundAddresses.Add(mbi.BaseAddress);
                }
                currentAddress = mbi.BaseAddress + mbi.RegionSize;
            }

            if (foundAddresses.Count == 0)
            {
                return nint.Zero;
            }

            logger.Log($"[{EmulatorName}] Found {foundAddresses.Count} match(es). Selecting the last one as the primary memory block.");
            return foundAddresses[foundAddresses.Count - 1];
        }

        public void Detach()
        {
            if (ProcessHandle != nint.Zero)
            {
                CloseHandle(ProcessHandle);
            }
            ProcessHandle = nint.Zero;
            MemoryBasePC = nint.Zero;
            EmulatorProcess = null;
            logger.Log($"[{EmulatorName}] Detached from process.");
        }

        public byte[] ReadMemory(uint pspAddress, int count)
        {
            if (!IsAttached || !IsValidPointerTarget(pspAddress)) return null;

            nint addressInPC = IntPtr.Add(this.MemoryBasePC, (int)(pspAddress - PSP_MEM_START));

            byte[] buffer = new byte[count];
            ReadProcessMemory(ProcessHandle, addressInPC, buffer, count, out _);
            return buffer;
        }

        public uint? ReadUInt32(uint pspAddress)
        {
            byte[] buffer = ReadMemory(pspAddress, 4);
            return buffer != null ? BitConverter.ToUInt32(buffer, 0) : null;
        }

        public bool IsValidPointerTarget(uint value)
        {
            return (value & 3) == 0 && (value >= PSP_MEM_START && value < PSP_MEM_END);
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

        public string FormatDisplayAddress(uint address)
        {
            // if address >= 0x09000000, map to 0x01...
            if (address >= 0x09000000 && address < PSP_MEM_END)
            {
                return (0x01000000 + (address - 0x09000000)).ToString("X");
            }
            // if address >= 0x08000000, map to 0x00...
            if (address >= PSP_MEM_START && address < 0x09000000)
            {
                return (address - PSP_MEM_START).ToString("X");
            }
            return address.ToString("X8");
        }

        public uint UnnormalizeAddress(string address)
        {
            uint parsed = uint.Parse(address.Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);

            // if it's already a valid hardware address, use it
            if (IsValidPointerTarget(parsed))
            {
                return parsed;
            }

            // if it's an RA address in the 0x01... range
            if (parsed >= 0x01000000 && parsed < 0x02000000)
            {
                return (parsed - 0x01000000) + 0x09000000;
            }

            // else assume it's an RA address in the 0x00... range
            return parsed + PSP_MEM_START;
        }

        public (uint normalizedAddress, bool wasNormalized) NormalizeAddressForRead(uint address)
        {
            return (address, false); // No normalization needed
        }

        public bool AreAddressesEquivalent(uint addr1, uint addr2)
        {
            return addr1 == addr2;
        }

        public AppSettings GetDefaultSettings()
        {
            // Fix: Adjusted the default static range for PPSSPP.
            return new AppSettings
            {
                StaticAddressStart = "800000",
                StaticAddressEnd = "CFFFFF",
                MaxOffset = 4095,
                MaxLevel = 7,
                MaxResults = 500000,
                MaxCandidates = 10000000,
                ScanForStructureBase = false,
                MaxNegativeOffset = 1024,
                Use16ByteAlignment = true,
            };
        }

        public long GetIndexForStateDump(uint address)
        {
            if (address >= PSP_MEM_START && address < PSP_MEM_END)
            {
                return address - PSP_MEM_START;
            }
            return -1;
        }
    }
}