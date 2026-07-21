#pragma once

#include <cstdint>
#include <QString>
#include <utility>
#include <vector>

namespace PointerFinder2::Emulators {
    class IEmulatorManager;
}

namespace PointerFinder2::DataModels {

    // Represents a resolved memory address chain.
    // Tracks the static entry point, structural offset list, and final target destination.
    class PointerPath {
    public:
        uint32_t baseAddress = 0;
        std::vector<int32_t> offsets;
        uint32_t finalAddress = 0;
        bool isPartial = false;

        // Maps a state slot index to the absolute address where the path broke.
        std::vector<std::pair<int, uint32_t>> brokenStateAddresses;

        QString getOffsetsString() const;
        QString toRetroAchievementsString(Emulators::IEmulatorManager* manager) const;

        bool operator==(const PointerPath& other) const;
    };

}

namespace std {
    template<>
    struct hash<PointerFinder2::DataModels::PointerPath> {
        size_t operator()(const PointerFinder2::DataModels::PointerPath& path) const {
            size_t h = 17;
            h = h * 23 + std::hash<uint32_t>{}(path.baseAddress);
            for (int32_t offset : path.offsets) {
                h = h * 23 + std::hash<int32_t>{}(offset);
            }
            return h;
        }
    };
}
