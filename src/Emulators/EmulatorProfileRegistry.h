#pragma once

#include "EmulatorProfile.h"

#include <vector>

namespace PointerFinder2::Emulators {

    // This class maintains a master register list of all supported emulators. It acts 
    // as a single access directory whenever the application needs to identify connected emulators.
    class EmulatorProfileRegistry {
    public:
        static const std::vector<EmulatorProfile>& getProfiles();
    };

}
