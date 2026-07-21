#include "Memory.h"

#include <windows.h>
#include <algorithm>
#include <cstring>
#include <psapi.h>

namespace PointerFinder2::Core {

    // Opens a safe connection handle to another running program using its Process ID (PID)
    // so that our scanner can read its memory banks.
    PlatformHandle Memory::openProcessHandle(uint32_t processId) {
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(processId));
        return static_cast<PlatformHandle>(hProcess);
    }

    // Safely closes a connection handle to a running program, releasing active system resources.
    void Memory::closeHandle(PlatformHandle handle) {
        if (handle && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(static_cast<HANDLE>(handle));
        }
    }

    std::vector<uint8_t> Memory::readBytes(PlatformHandle handle, uintptr_t address, size_t size) {
        // Cap allocation size to prevent memory exhaustion from corrupt size arguments.
        constexpr size_t MAX_SAFE_ALLOCATION_SIZE = 128 * 1024 * 1024;
        if (!handle || size == 0 || size > MAX_SAFE_ALLOCATION_SIZE) {
            return {};
        }

        std::vector<uint8_t> buffer(size);
        SIZE_T bytesRead = 0;

        BOOL success = ReadProcessMemory(
            static_cast<HANDLE>(handle),
            reinterpret_cast<LPCVOID>(address),
            buffer.data(),
            static_cast<SIZE_T>(size),
            &bytesRead
        );

        if (!success || bytesRead != size) {
            return {};
        }
        return buffer;
    }

    std::optional<int64_t> Memory::readInt64(PlatformHandle handle, uintptr_t address) {
        auto bytes = readBytes(handle, address, sizeof(int64_t));
        if (bytes.size() < sizeof(int64_t)) {
            return std::nullopt;
        }
        int64_t value;
        std::memcpy(&value, bytes.data(), sizeof(int64_t));
        return value;
    }

    // Reads a standard text string (such as a Game ID or path label) directly from another program's memory.
    std::string Memory::readString(PlatformHandle handle, uintptr_t address, size_t maxLen) {
        if (maxLen == 0 || maxLen > 4096) {
            maxLen = 64;
        }

        auto bytes = readBytes(handle, address, maxLen);
        if (bytes.empty()) {
            return "";
        }

        std::string str;
        for (size_t i = 0; i < bytes.size(); ++i) {
            if (bytes[i] == '\0') {
                break;
            }
            str.push_back(static_cast<char>(bytes[i]));
        }
        return str;
    }

    uintptr_t Memory::findExportedAddress(PlatformHandle handle, uintptr_t baseAddress, const std::string& exportName, bool is64Bit) {
        if (!handle) return 0;

        auto headerBytes = readBytes(handle, baseAddress, 4096);
        if (headerBytes.size() < sizeof(IMAGE_DOS_HEADER)) return 0;

        auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(headerBytes.data());
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            return 0;
        }

        uint32_t peHeaderOffset = dosHeader->e_lfanew;
        if (peHeaderOffset + sizeof(IMAGE_NT_HEADERS32) > headerBytes.size()) return 0;

        uint32_t rvaOffset = peHeaderOffset + (is64Bit ? 136 : 120);
        if (rvaOffset + sizeof(uint32_t) * 2 > headerBytes.size()) return 0;

        uint32_t exportTableRVA = 0;
        uint32_t exportTableSize = 0;
        std::memcpy(&exportTableRVA, &headerBytes[rvaOffset], sizeof(uint32_t));
        std::memcpy(&exportTableSize, &headerBytes[rvaOffset + 4], sizeof(uint32_t));

        if (exportTableRVA == 0 || exportTableSize == 0) return 0;

        uintptr_t exportTableAddress = baseAddress + exportTableRVA;
        auto exportDirBytes = readBytes(handle, exportTableAddress, sizeof(IMAGE_EXPORT_DIRECTORY));
        if (exportDirBytes.size() < sizeof(IMAGE_EXPORT_DIRECTORY)) return 0;

        auto* exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(exportDirBytes.data());
        uint32_t numberOfNames = exportDir->NumberOfNames;
        uint32_t functionTableRVA = exportDir->AddressOfFunctions;
        uint32_t nameTableRVA = exportDir->AddressOfNames;
        uint32_t ordinalTableRVA = exportDir->AddressOfNameOrdinals;

        auto nameTable = readBytes(handle, baseAddress + nameTableRVA, numberOfNames * 4);
        auto ordinalTable = readBytes(handle, baseAddress + ordinalTableRVA, numberOfNames * 2);
        auto functionTable = readBytes(handle, baseAddress + functionTableRVA, exportDir->NumberOfFunctions * 4);

        if (nameTable.empty() || ordinalTable.empty() || functionTable.empty()) return 0;

        for (uint32_t i = 0; i < numberOfNames; ++i) {
            uint32_t nameRVA = 0;
            std::memcpy(&nameRVA, &nameTable[i * 4], sizeof(uint32_t));
            std::string currentExportName = readString(handle, baseAddress + nameRVA, 64);

            if (std::equal(exportName.begin(), exportName.end(), currentExportName.begin(), currentExportName.end(),
                [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {

                uint16_t ordinal = 0;
                std::memcpy(&ordinal, &ordinalTable[i * 2], sizeof(uint16_t));
                if (ordinal >= exportDir->NumberOfFunctions) return 0;

                uint32_t functionRVA = 0;
                std::memcpy(&functionRVA, &functionTable[ordinal * 4], sizeof(uint32_t));
                return baseAddress + functionRVA;
            }
        }

        return 0;
    }

    // Locates the starting memory address of a loaded executable module inside your computer's RAM.
    uintptr_t Memory::getModuleBaseAddress(PlatformHandle handle) {
        if (!handle) return 0;
        HMODULE hMods[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(static_cast<HANDLE>(handle), hMods, sizeof(hMods), &cbNeeded)) {
            return reinterpret_cast<uintptr_t>(hMods[0]);
        }
        return 0;
    }

    // Instructs Windows to immediately optimize our program's memory footprint, purging unused memory allocations.
    void Memory::forceGarbageCollection() {
        HANDLE hProcess = GetCurrentProcess();
        SetProcessWorkingSetSize(hProcess, static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
    }

    // Queries Windows for a list of all active memory pages assigned to a running program so we can scan them.
    std::vector<MemoryRegion> Memory::queryMemoryRegions(PlatformHandle handle) {
        std::vector<MemoryRegion> regions;
        if (!handle) return regions;

        uintptr_t currentAddress = 0;
        uintptr_t maxAddress = 0x7FFFFFFFFFFFULL;

        while (currentAddress < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(static_cast<HANDLE>(handle), reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
                break;
            }

            if (mbi.RegionSize == 0) {
                break;
            }

            MemoryRegion region;
            region.baseAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            region.size = mbi.RegionSize;
            region.isCommitted = (mbi.State == MEM_COMMIT);
            regions.push_back(region);

            currentAddress = region.baseAddress + region.size;
        }

        return regions;
    }

}
