using System;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace PointerFinder2.Core
{
    // A utility class for low-level memory operations using the Windows API (P/Invoke).
    public static class Memory
    {
        private const int PROCESS_VM_READ = 0x0010;

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern nint OpenProcess(int dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool ReadProcessMemory(nint hProcess, nint lpBaseAddress, [Out] byte[] lpBuffer, int dwSize, out int lpNumberOfBytesRead);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool CloseHandle(nint hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool IsWow64Process([In] nint processHandle, [MarshalAs(UnmanagedType.Bool)] out bool wow64Process);

        // Gets the first running process that matches the given name.
        public static Process GetProcess(string processName)
        {
            return Process.GetProcessesByName(processName).FirstOrDefault();
        }

        // Opens a handle to a process with read-only memory permissions.
        public static nint OpenProcessHandle(Process process)
        {
            if (process == null)
            {
                return IntPtr.Zero;
            }
            return OpenProcess(PROCESS_VM_READ, bInheritHandle: false, process.Id);
        }

        // Finds the memory address of an exported variable (e.g., "EEMem") by parsing the process's PE header.
        // This is the key to attaching robustly without hardcoded offsets, which would break with every new emulator version.
        public static nint FindExportedAddress(Process process, nint processHandle, string exportName)
        {
            if (process?.MainModule == null || processHandle == IntPtr.Zero) return IntPtr.Zero;
            try
            {
                nint baseAddress = process.MainModule.BaseAddress;
                byte[] peHeader = ReadBytes(processHandle, baseAddress, 4096);
                if (peHeader == null) return IntPtr.Zero;

                int peHeaderOffset = BitConverter.ToInt32(peHeader, 60);
                // The offset to the export table is different for 32-bit vs 64-bit processes.
                int rvaOffset = peHeaderOffset + (!Wow64Process(process) ? 136 : 120);

                int exportTableRVA = BitConverter.ToInt32(peHeader, rvaOffset);
                if (exportTableRVA == 0) return IntPtr.Zero;

                nint exportTableAddress = baseAddress + exportTableRVA;
                byte[] exportDir = ReadBytes(processHandle, exportTableAddress, 40);
                if (exportDir == null) return IntPtr.Zero;

                int numberOfFunctions = BitConverter.ToInt32(exportDir, 24);
                int functionTableRVA = BitConverter.ToInt32(exportDir, 28);
                int nameTableRVA = BitConverter.ToInt32(exportDir, 32);
                int ordinalTableRVA = BitConverter.ToInt32(exportDir, 36);

                byte[] nameTable = ReadBytes(processHandle, baseAddress + nameTableRVA, numberOfFunctions * 4);
                byte[] ordinalTable = ReadBytes(processHandle, baseAddress + ordinalTableRVA, numberOfFunctions * 2);
                byte[] functionTable = ReadBytes(processHandle, baseAddress + functionTableRVA, numberOfFunctions * 4);

                if (nameTable == null || ordinalTable == null || functionTable == null) return IntPtr.Zero;

                // Iterate through the export names to find a match.
                for (int i = 0; i < numberOfFunctions; i++)
                {
                    int nameRVA = BitConverter.ToInt32(nameTable, i * 4);
                    string currentExportName = ReadString(processHandle, baseAddress + nameRVA);
                    if (currentExportName.Equals(exportName, StringComparison.OrdinalIgnoreCase))
                    {
                        // Once the name matches, find its address via the ordinal table.
                        ushort ordinal = BitConverter.ToUInt16(ordinalTable, i * 2);
                        int functionRVA = BitConverter.ToInt32(functionTable, ordinal * 4);
                        return baseAddress + functionRVA;
                    }
                }
            }
            catch (Exception ex)
            {
                DebugLogForm.Instance.Log($"[Memory] ERROR finding export '{exportName}': {ex.Message}");
            }
            return IntPtr.Zero;
        }

        // Reads a specified number of bytes from a given memory address.
        public static byte[] ReadBytes(nint handle, nint address, int size)
        {
            byte[] buffer = new byte[size];
            if (!ReadProcessMemory(handle, address, buffer, size, out _))
            {
                return null;
            }
            return buffer;
        }

        // Reads a 64-bit integer from a given memory address.
        public static long? ReadInt64(nint handle, nint address)
        {
            byte[] buffer = ReadBytes(handle, address, 8);
            if (buffer == null)
            {
                return null;
            }
            return BitConverter.ToInt64(buffer, 0);
        }

        // Reads a null-terminated ASCII string from a given memory address.
        private static string ReadString(nint handle, nint address)
        {
            byte[] buffer = new byte[64];
            if (!ReadProcessMemory(handle, address, buffer, buffer.Length, out _))
            {
                return string.Empty;
            }
            return Encoding.ASCII.GetString(buffer).Split('\0')[0];
        }

        // Determines if the target process is a 32-bit application.
        private static bool Wow64Process(Process process)
        {
            if (Environment.Is64BitOperatingSystem && Environment.Is64BitProcess)
            {
                return IsWow64Process(process.Handle, out bool isWow64) && isWow64;
            }
            return IsWow64Process(process.Handle, out bool _isWow64) && _isWow64;
        }
    }
}