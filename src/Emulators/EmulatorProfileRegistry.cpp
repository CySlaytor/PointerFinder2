#include "EmulatorProfileRegistry.h"

#include "Managers/DolphinManager.h"
#include "Managers/DuckStationManager.h"
#include "Managers/Pcsx2Manager.h"
#include "Managers/PpssppManager.h"
#include "Managers/RALibretroNDSManager.h"
#include "Managers/RALibretroGBAManager.h"
#include "Strategies/DolphinStateBasedScanner.h"
#include "Strategies/DuckStationStateBasedScanner.h"
#include "Strategies/Pcsx2StateBasedScanner.h"
#include "Strategies/PpssppStateBasedScanner.h"
#include "Strategies/RALibretroNDSStateBasedScanner.h"
#include "Strategies/RALibretroGBAStateBasedScanner.h"

namespace PointerFinder2::Emulators {

    using namespace PointerFinder2::DataModels;
    using namespace PointerFinder2::Emulators::EmulatorManager;
    using namespace PointerFinder2::Emulators::StateBased;

    // Returns the master registry list, detailing process names and custom scanning 
    // procedures for each supported system (PCSX2, DuckStation, Dolphin, PPSSPP, and RALibretro).
    const std::vector<EmulatorProfile>& EmulatorProfileRegistry::getProfiles() {
        static const std::vector<EmulatorProfile> profiles = {
            {
                "PCSX2",
                EmulatorTarget::PCSX2,
                { "pcsx2-qt", "pcsx2" },
                []() { return std::make_unique<Pcsx2Manager>(); },
                []() { return std::make_unique<Pcsx2StateBasedScanner>(); },
                true
            },
            {
                "DuckStation",
                EmulatorTarget::DuckStation,
                { "duckstation-qt-x64-ReleaseLTCG" },
                []() { return std::make_unique<DuckStationManager>(); },
                []() { return std::make_unique<DuckStationStateBasedScanner>(); },
                false
            },
            {
                "RALibretro (NDS)",
                EmulatorTarget::RALibretroNDS,
                { "RALibretro" },
                []() { return std::make_unique<RALibretroNDSManager>(); },
                []() { return std::make_unique<RALibretroNDSStateBasedScanner>(); },
                true
            },
            {
                "Dolphin",
                EmulatorTarget::Dolphin,
                { "Dolphin" },
                []() { return std::make_unique<DolphinManager>(); },
                []() { return std::make_unique<DolphinStateBasedScanner>(); },
                true
            },
            {
                "PPSSPP",
                EmulatorTarget::PPSSPP,
                { "PPSSPPWindows64" },
                []() { return std::make_unique<PpssppManager>(); },
                []() { return std::make_unique<PpssppStateBasedScanner>(); },
                true
            },
            {
                "RALibretro (GBA)",
                EmulatorTarget::RALibretroGBA,
                { "RALibretro" },
                []() { return std::make_unique<RALibretroGBAManager>(); },
                []() { return std::make_unique<RALibretroGBAStateBasedScanner>(); },
                false
            }
        };
        return profiles;
    }

}