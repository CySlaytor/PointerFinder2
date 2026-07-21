#pragma once

#include "ScanState.h"

#include <optional>
#include <vector>

namespace PointerFinder2::DataModels {

    // Aggregates structural settings, limits, base ranges, and comparative states
    // required by background scanning strategies to perform path searches.
    struct ScanParameters {
        uint32_t targetAddress = 0;
        int maxLevel = 7;
        int maxOffset = 4095;
        uint32_t staticBaseStart = 0;
        uint32_t staticBaseEnd = 0;
        bool limitCpuUsage = false;
        bool stopOnFirstPathFound = false;
        bool findAllPathLevels = false;
        int candidatesPerLevel = 10;
        int maxCandidates = 500000;
        uint32_t finalAddressTarget = 0;
        bool fastScanMode = true;
        bool printPartialPaths = false;
        std::optional<int> lastOffsetHint = std::nullopt;
        std::vector<ScanState> capturedStates;

        bool detectArrays = false;
        uint32_t arraySearchRange = 0x80;
        bool dynamicStaticDetection = false;

        ScanParameters cloneWithoutStates() const {
            ScanParameters copy = *this;
            copy.capturedStates.clear();
            return copy;
        }
    };

}
