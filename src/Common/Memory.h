#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace PointerFinder2::Core {

using PlatformHandle = void*;

struct MemoryRegion {
    uintptr_t baseAddress = 0;
    size_t size = 0;
    bool isCommitted = false;
};

// This class provides low-level tools used to open connection handles to other
// running programs, allowing us to safely read memory blocks and system labels from their RAM.
class Memory {
  public:
    static PlatformHandle openProcessHandle(uint32_t processId);

    static void closeHandle(PlatformHandle handle);

    static std::vector<uint8_t> readBytes(PlatformHandle handle, uintptr_t address, size_t size);

    static std::optional<int64_t> readInt64(PlatformHandle handle, uintptr_t address);

    static std::string readString(PlatformHandle handle, uintptr_t address, size_t maxLen = 64);

    // Traverses the export directory of a module loaded inside a remote process to resolve an export signature address.
    static uintptr_t findExportedAddress(PlatformHandle handle, uintptr_t baseAddress, const std::string& exportName, bool is64Bit);

    static uintptr_t getModuleBaseAddress(PlatformHandle handle);

    static void forceGarbageCollection();

    // Gathers active system virtual memory mapping assignments via OS-specific APIs.
    static std::vector<MemoryRegion> queryMemoryRegions(PlatformHandle handle);

    // Determines whether the target process is running as a 64-bit process.
    static bool isProcess64Bit(PlatformHandle handle);

    // Enumerates loaded modules in the target process and returns the base address of the first module matching any of the given substrings.
    static uintptr_t findRemoteModule(PlatformHandle handle, const std::vector<std::string>& moduleSubstrings, std::string* outModuleName = nullptr);

    // Executes retro_get_memory_data(memoryId) in the target process via a remote execution stub and returns the resolved pointer address.
    static uintptr_t callRemoteRetroGetMemoryData(PlatformHandle handle, uintptr_t coreModuleBase, uint32_t memoryId = 2);
};

} // namespace PointerFinder2::Core
