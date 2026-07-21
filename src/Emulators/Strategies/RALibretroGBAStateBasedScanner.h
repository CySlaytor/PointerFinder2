#pragma once

#include "StateBasedScannerStrategyBase.h"

namespace PointerFinder2::Emulators::StateBased {

    class RALibretroGBAStateBasedScanner : public StateBasedScannerStrategyBase {
    protected:
        void buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) override;
        SourceResults findSourcesForValue(uint32_t value) const override;
    };

}