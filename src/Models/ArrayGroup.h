#pragma once

#include <cstdint>
#include <vector>

namespace PointerFinder2::DataModels {

    // Groups adjacent pointer addresses together.
    // This is used to analyze contiguous structures, such as arrays or data structures in RAM.
    struct ArrayGroup {
        uint32_t baseAddress = 0;
        uint32_t elementCount = 0;
        std::vector<uint32_t> elementAddresses;
        std::vector<uint32_t> targets;
    };

}
