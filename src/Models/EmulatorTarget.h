#pragma once

namespace PointerFinder2::DataModels {

    // Identifies the active target hardware platform or software engine profile.
    enum class EmulatorTarget {
        PCSX2,
        DuckStation,
        RALibretroNDS,
        Dolphin,
        PPSSPP,
        RALibretroGBA
    };

}