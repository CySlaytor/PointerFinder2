#pragma once

#include "StateBasedScannerStrategyBase.h"

namespace PointerFinder2::Emulators::StateBased {

    // This scanner runs PCSX2 scans, managing virtual mapping boundaries unique to PS2 structures.
    class Pcsx2StateBasedScanner : public StateBasedScannerStrategyBase {
    protected:
        void buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) override;
        SourceResults findSourcesForValue(uint32_t value) const override;
    };

}
