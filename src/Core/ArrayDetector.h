#pragma once

#include "../Emulators/IEmulatorManager.h"
#include "../Models/ArrayGroup.h"
#include "../Models/PointerPath.h"
#include "../Models/ScanState.h"
#include "../UI/Widgets/ArrayDetectionWidget.h"

#include <cstdint>
#include <vector>

namespace PointerFinder2::Core {

    // Handles identification of contiguous arrays in memory dumps using platform-specific heuristics.
    class ArrayDetector {
    public:
        static std::vector<DataModels::ArrayGroup> detectArraysInDump(
            const std::vector<uint8_t>& memory,
            const std::vector<DataModels::PointerPath>& paths,
            uint32_t range,
            Emulators::IEmulatorManager* manager
        );

        static UI::Controls::ArrayDetectionResult performArrayDetectionAndMatching(
            const std::vector<uint8_t>& memory,
            const std::vector<DataModels::PointerPath>& paths,
            uint32_t range,
            Emulators::IEmulatorManager* manager,
            const DataModels::ScanState& state0
        );
    };

}
