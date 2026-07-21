#pragma once

#include "../Emulators/EmulatorProfile.h"

#include <QString>
#include <vector>

namespace PointerFinder2::Core {

    struct DetectedEmulatorInstance {
        Emulators::EmulatorProfile profile;
        uint32_t pid = 0;
        QString windowTitle;
    };

    class ProcessScanner {
    public:
        static std::vector<DetectedEmulatorInstance> scanForRunningEmulators();
    };

}