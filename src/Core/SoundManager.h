#pragma once

namespace PointerFinder2::Core {

    // Handles sound effect playback for scanning status notifications.
    // This class triggers alert sounds, playing custom audio alerts when searches succeed, fail, or stop.
    class SoundManager {
    public:
        static void playSuccess();
        static void playFail();
        static void playNotify();
    };

}