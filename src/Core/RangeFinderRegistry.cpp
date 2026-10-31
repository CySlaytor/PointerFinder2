#include "RangeFinderRegistry.h"

#include "../UI/StaticRangeFinders/DolphinFileRangeFinderDialog.h"
#include "../UI/StaticRangeFinders/NdsStaticRangeFinderDialog.h"
#include "../UI/StaticRangeFinders/Pcsx2RamScanRangeFinderDialog.h"
#include "../UI/StaticRangeFinders/PpssppRamScanRangeFinderDialog.h"

namespace PointerFinder2::Core {

    std::unordered_map<DataModels::EmulatorTarget, RangeFinderRegistry::Factory> RangeFinderRegistry::s_factories;

    void RangeFinderRegistry::registerFactory(DataModels::EmulatorTarget target, Factory factory) {
        s_factories[target] = std::move(factory);
    }

    void RangeFinderRegistry::ensureInitialized() {
        if (!s_factories.empty()) return;

        s_factories[DataModels::EmulatorTarget::PCSX2] = [](Emulators::IEmulatorManager* mgr, QWidget* parent) -> QDialog* {
            return new UI::StaticRangeFinders::Pcsx2RamScanRangeFinderDialog(mgr, parent);
        };
        s_factories[DataModels::EmulatorTarget::RALibretroNDS] = [](Emulators::IEmulatorManager* mgr, QWidget* parent) -> QDialog* {
            return new UI::StaticRangeFinders::NdsStaticRangeFinderDialog(mgr, parent);
        };
        s_factories[DataModels::EmulatorTarget::Dolphin] = [](Emulators::IEmulatorManager* mgr, QWidget* parent) -> QDialog* {
            return new UI::StaticRangeFinders::DolphinFileRangeFinderDialog(mgr, parent);
        };
        s_factories[DataModels::EmulatorTarget::PPSSPP] = [](Emulators::IEmulatorManager* mgr, QWidget* parent) -> QDialog* {
            return new UI::StaticRangeFinders::PpssppRamScanRangeFinderDialog(mgr, parent);
        };
    }

    QDialog* RangeFinderRegistry::createDialog(DataModels::EmulatorTarget target, Emulators::IEmulatorManager* manager, QWidget* parent) {
        ensureInitialized();
        auto it = s_factories.find(target);
        if (it != s_factories.end()) {
            return it->second(manager, parent);
        }
        return nullptr;
    }

    bool RangeFinderRegistry::hasFactory(DataModels::EmulatorTarget target) {
        ensureInitialized();
        return s_factories.find(target) != s_factories.end();
    }

}
