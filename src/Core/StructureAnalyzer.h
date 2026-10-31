#pragma once

#include "../Emulators/IEmulatorManager.h"

#include <cstdint>
#include <optional>
#include <vector>
#include <QString>

namespace PointerFinder2::Core {

    struct ScopeCandidate {
        void* identifier = nullptr;
        uint32_t targetBase = 0;
        int depth = 0;
    };

    struct ScopeMatch {
        void* scopeIdentifier = nullptr;
        uint32_t baseAddress = 0;
        int32_t offset = 0;
        int depth = 0;
        bool isValid = false;
    };

    // Service providing memory dereferencing, parent pointer scope resolution,
    // and data-type deduction heuristics for structured memory viewing.
    class StructureAnalyzer {
    public:
        static std::optional<uint32_t> readMemoryDword(
            uint32_t address,
            Emulators::IEmulatorManager* manager,
            const std::vector<uint8_t>& snapshotMemory
        );

        static ScopeMatch findBestScopeForAddress(
            uint32_t selectedAddr,
            uint32_t rootBaseAddress,
            const std::vector<ScopeCandidate>& scopes,
            Emulators::IEmulatorManager* manager,
            const std::vector<uint8_t>& snapshotMemory
        );

        static QString suggestDefaultType(
            uint32_t address,
            Emulators::IEmulatorManager* manager,
            const std::vector<uint8_t>& snapshotMemory
        );
    };

}
