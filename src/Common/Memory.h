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
    };

}
