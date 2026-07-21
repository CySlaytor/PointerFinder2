#pragma once

#include <cstdint>

namespace PointerFinder2::Core {

// Swaps the byte order of a 32 - bit unsigned integer. 
// Uses platform intrinsics where available. This is necessary because some systems (like Wii/GameCube) 
// read memory backwards compared to standard Windows PCs.
inline uint32_t swapEndian32(uint32_t val) {
#if defined(_MSC_VER)
        return _byteswap_ulong(val);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(val);
#else
        return ((val >> 24) & 0x000000FF) |
            ((val >> 8) & 0x0000FF00) |
            ((val << 8) & 0x00FF0000) |
            ((val << 24) & 0xFF000000);
#endif
    }

}
