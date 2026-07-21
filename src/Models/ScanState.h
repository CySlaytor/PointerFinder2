#pragma once

#include <cstdint>
#include <vector>

namespace PointerFinder2::DataModels {

    // Represents a static memory state captured at a single instant of target program execution,
    // pairing the raw memory buffer with the corresponding target parameter address.
    struct ScanState {
        std::vector<uint8_t> memoryDump;
        uint32_t targetAddress = 0;
    };

}
