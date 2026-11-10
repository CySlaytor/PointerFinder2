#include "Memory.h"

#include <windows.h>

#include <psapi.h>

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <cstring>

namespace PointerFinder2::Core {

// Opens a safe connection handle to another running program using its Process ID (PID)
// so that our scanner can read its memory banks.
PlatformHandle Memory::openProcessHandle(uint32_t processId) {
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(processId));
    if (!hProcess) {
        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(processId));
    }
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

    BOOL success = ReadProcessMemory(static_cast<HANDLE>(handle), reinterpret_cast<LPCVOID>(address), buffer.data(),
                                     static_cast<SIZE_T>(size), &bytesRead);

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
    if (!handle)
        return 0;

    auto headerBytes = readBytes(handle, baseAddress, 4096);
    if (headerBytes.size() < sizeof(IMAGE_DOS_HEADER))
        return 0;

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(headerBytes.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    uint32_t peHeaderOffset = dosHeader->e_lfanew;
    if (peHeaderOffset + sizeof(IMAGE_NT_HEADERS32) > headerBytes.size())
        return 0;

    uint32_t rvaOffset = peHeaderOffset + (is64Bit ? 136 : 120);
    if (rvaOffset + sizeof(uint32_t) * 2 > headerBytes.size())
        return 0;

    uint32_t exportTableRVA = 0;
    uint32_t exportTableSize = 0;
    std::memcpy(&exportTableRVA, &headerBytes[rvaOffset], sizeof(uint32_t));
    std::memcpy(&exportTableSize, &headerBytes[rvaOffset + 4], sizeof(uint32_t));

    if (exportTableRVA == 0 || exportTableSize == 0)
        return 0;

    uintptr_t exportTableAddress = baseAddress + exportTableRVA;
    auto exportDirBytes = readBytes(handle, exportTableAddress, sizeof(IMAGE_EXPORT_DIRECTORY));
    if (exportDirBytes.size() < sizeof(IMAGE_EXPORT_DIRECTORY))
        return 0;

    auto* exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(exportDirBytes.data());
    uint32_t numberOfNames = exportDir->NumberOfNames;
    uint32_t functionTableRVA = exportDir->AddressOfFunctions;
    uint32_t nameTableRVA = exportDir->AddressOfNames;
    uint32_t ordinalTableRVA = exportDir->AddressOfNameOrdinals;

    auto nameTable = readBytes(handle, baseAddress + nameTableRVA, numberOfNames * 4);
    auto ordinalTable = readBytes(handle, baseAddress + ordinalTableRVA, numberOfNames * 2);
    auto functionTable = readBytes(handle, baseAddress + functionTableRVA, exportDir->NumberOfFunctions * 4);

    if (nameTable.empty() || ordinalTable.empty() || functionTable.empty())
        return 0;

    for (uint32_t i = 0; i < numberOfNames; ++i) {
        uint32_t nameRVA = 0;
        std::memcpy(&nameRVA, &nameTable[i * 4], sizeof(uint32_t));
        std::string currentExportName = readString(handle, baseAddress + nameRVA, 64);

        if (std::equal(exportName.begin(), exportName.end(), currentExportName.begin(), currentExportName.end(),
                       [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {

            uint16_t ordinal = 0;
            std::memcpy(&ordinal, &ordinalTable[i * 2], sizeof(uint16_t));
            if (ordinal >= exportDir->NumberOfFunctions)
                return 0;

            uint32_t functionRVA = 0;
            std::memcpy(&functionRVA, &functionTable[ordinal * 4], sizeof(uint32_t));
            return baseAddress + functionRVA;
        }
    }

    return 0;
}

// Locates the starting memory address of a loaded executable module inside your computer's RAM.
uintptr_t Memory::getModuleBaseAddress(PlatformHandle handle) {
    if (!handle)
        return 0;
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
    if (!handle)
        return regions;

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

bool Memory::isProcess64Bit(PlatformHandle handle) {
    if (!handle)
        return false;

    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        return false;
    }

    BOOL isWow64 = FALSE;
    if (IsWow64Process(static_cast<HANDLE>(handle), &isWow64)) {
        return !isWow64;
    }
    return sizeof(void*) == 8;
}

uintptr_t Memory::findRemoteModule(PlatformHandle handle, const std::vector<std::string>& moduleSubstrings, std::string* outModuleName) {
    if (!handle)
        return 0;

    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    HANDLE hProcess = static_cast<HANDLE>(handle);

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        int count = static_cast<int>(std::min<size_t>(cbNeeded / sizeof(HMODULE), sizeof(hMods) / sizeof(HMODULE)));
        for (int i = 0; i < count; ++i) {
            wchar_t szModPath[MAX_PATH];
            if (GetModuleFileNameExW(hProcess, hMods[i], szModPath, MAX_PATH)) {
                std::wstring wpath(szModPath);
                for (wchar_t& wc : wpath) {
                    wc = static_cast<wchar_t>(std::towlower(wc));
                }

                for (const auto& sub : moduleSubstrings) {
                    std::wstring wsub;
                    wsub.reserve(sub.length());
                    for (char c : sub) {
                        wsub.push_back(static_cast<wchar_t>(std::towlower(static_cast<unsigned char>(c))));
                    }

                    if (wpath.find(wsub) != std::wstring::npos) {
                        if (outModuleName) {
                            std::wstring rawPath(szModPath);
                            size_t lastSlash = rawPath.find_last_of(L"\\/");
                            std::wstring fileName = (lastSlash != std::wstring::npos) ? rawPath.substr(lastSlash + 1) : rawPath;

                            int len = WideCharToMultiByte(CP_UTF8, 0, fileName.c_str(), static_cast<int>(fileName.length()), NULL, 0, NULL, NULL);
                            if (len > 0) {
                                outModuleName->resize(len);
                                WideCharToMultiByte(CP_UTF8, 0, fileName.c_str(), static_cast<int>(fileName.length()), &(*outModuleName)[0], len, NULL, NULL);
                            } else {
                                outModuleName->clear();
                            }
                        }
                        return reinterpret_cast<uintptr_t>(hMods[i]);
                    }
                }
            }
        }
    }
    return 0;
}

uintptr_t Memory::callRemoteRetroGetMemoryData(PlatformHandle handle, uintptr_t coreModuleBase, uint32_t memoryId) {
    if (!handle || coreModuleBase == 0)
        return 0;

    bool is64 = isProcess64Bit(handle);
    uintptr_t funcAddr = findExportedAddress(handle, coreModuleBase, "retro_get_memory_data", is64);
    if (funcAddr == 0)
        return 0;

    HANDLE hProcess = static_cast<HANDLE>(handle);
    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteMem)
        return 0;

    uintptr_t resultAddr = reinterpret_cast<uintptr_t>(remoteMem) + 0x800;
    uint64_t zeroVal = 0;
    WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(resultAddr), &zeroVal, sizeof(zeroVal), NULL);

    std::vector<uint8_t> stub;
    if (is64) {
        uint64_t func64 = static_cast<uint64_t>(funcAddr);
        uint64_t result64 = static_cast<uint64_t>(resultAddr);

        // x64 Calling Convention:
        // sub rsp, 0x28
        stub.push_back(0x48); stub.push_back(0x83); stub.push_back(0xEC); stub.push_back(0x28);
        // mov ecx, memoryId
        stub.push_back(0xB9);
        stub.push_back(static_cast<uint8_t>(memoryId & 0xFF));
        stub.push_back(static_cast<uint8_t>((memoryId >> 8) & 0xFF));
        stub.push_back(static_cast<uint8_t>((memoryId >> 16) & 0xFF));
        stub.push_back(static_cast<uint8_t>((memoryId >> 24) & 0xFF));
        // mov rax, funcAddr
        stub.push_back(0x48); stub.push_back(0xB8);
        for (int b = 0; b < 8; ++b) {
            stub.push_back(static_cast<uint8_t>((func64 >> (b * 8)) & 0xFF));
        }
        // call rax
        stub.push_back(0xFF); stub.push_back(0xD0);
        // mov rdx, resultAddr
        stub.push_back(0x48); stub.push_back(0xBA);
        for (int b = 0; b < 8; ++b) {
            stub.push_back(static_cast<uint8_t>((result64 >> (b * 8)) & 0xFF));
        }
        // mov [rdx], rax
        stub.push_back(0x48); stub.push_back(0x89); stub.push_back(0x02);
        // add rsp, 0x28
        stub.push_back(0x48); stub.push_back(0x83); stub.push_back(0xC4); stub.push_back(0x28);
        // xor eax, eax
        stub.push_back(0x31); stub.push_back(0xC0);
        // ret
        stub.push_back(0xC3);
    } else {
        uint32_t func32 = static_cast<uint32_t>(funcAddr);
        uint32_t result32 = static_cast<uint32_t>(resultAddr);

        // x86 Calling Convention (__cdecl for retro_get_memory_data, __stdcall for ThreadProc):
        // push memoryId
        if (memoryId <= 0x7F) {
            stub.push_back(0x6A);
            stub.push_back(static_cast<uint8_t>(memoryId));
        } else {
            stub.push_back(0x68);
            stub.push_back(static_cast<uint8_t>(memoryId & 0xFF));
            stub.push_back(static_cast<uint8_t>((memoryId >> 8) & 0xFF));
            stub.push_back(static_cast<uint8_t>((memoryId >> 16) & 0xFF));
            stub.push_back(static_cast<uint8_t>((memoryId >> 24) & 0xFF));
        }
        // mov eax, funcAddr
        stub.push_back(0xB8);
        for (int b = 0; b < 4; ++b) {
            stub.push_back(static_cast<uint8_t>((func32 >> (b * 8)) & 0xFF));
        }
        // call eax
        stub.push_back(0xFF); stub.push_back(0xD0);
        // add esp, 4
        stub.push_back(0x83); stub.push_back(0xC4); stub.push_back(0x04);
        // mov edx, resultAddr
        stub.push_back(0xBA);
        for (int b = 0; b < 4; ++b) {
            stub.push_back(static_cast<uint8_t>((result32 >> (b * 8)) & 0xFF));
        }
        // mov [edx], eax
        stub.push_back(0x89); stub.push_back(0x02);
        // xor eax, eax
        stub.push_back(0x31); stub.push_back(0xC0);
        // ret 4
        stub.push_back(0xC2); stub.push_back(0x04); stub.push_back(0x00);
    }

    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(hProcess, remoteMem, stub.data(), stub.size(), &bytesWritten) || bytesWritten != stub.size()) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return 0;
    }

    FlushInstructionCache(hProcess, remoteMem, stub.size());

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteMem), NULL, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return 0;
    }

    WaitForSingleObject(hThread, 2000);
    CloseHandle(hThread);

    uintptr_t resolvedBase = 0;
    if (is64) {
        uint64_t val64 = 0;
        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(resultAddr), &val64, sizeof(val64), NULL);
        resolvedBase = static_cast<uintptr_t>(val64);
    } else {
        uint32_t val32 = 0;
        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(resultAddr), &val32, sizeof(val32), NULL);
        resolvedBase = static_cast<uintptr_t>(val32);
    }

    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    return resolvedBase;
}

} // namespace PointerFinder2::Core
