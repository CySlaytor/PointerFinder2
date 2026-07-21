#pragma once

#include "StateBasedScannerStrategyBase.h"

namespace PointerFinder2::Emulators::StateBased {

    // This scanner runs Wii/GameCube specific scans, translating Big-Endian 
    // byte alignments as it indexes pointers.
    class DolphinStateBasedScanner : public StateBasedScannerStrategyBase {
    protected:
        void buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) override;
        SourceResults findSourcesForValue(uint32_t value) const override;
    };

}
