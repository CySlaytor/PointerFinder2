#pragma once

#include "Emulators/IEmulatorManager.h"
#include "Models/PointerPath.h"

#include <vector>

namespace PointerFinder2::Core {

    // Service dedicated to copying and exporting pointer chains in
    // RetroAchievements, custom code note, and raw address formats.
    class ExportService {
    public:
        static int copyBaseAddresses(const std::vector<DataModels::PointerPath>& selectedPaths,
                                     const Emulators::IEmulatorManager* manager);

        static bool copyAsRetroAchievementsFormat(const DataModels::PointerPath& path,
                                                  Emulators::IEmulatorManager* manager);

        static bool copyAsRetroAchievementsFormat(const std::vector<DataModels::PointerPath>& paths,
                                                  Emulators::IEmulatorManager* manager);

        static bool copyAsCodeNote(const DataModels::PointerPath& path,
                                   const Emulators::IEmulatorManager* manager);
    };

}
