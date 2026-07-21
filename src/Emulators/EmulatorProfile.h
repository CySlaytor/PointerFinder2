#pragma once

#include "../Models/EmulatorTarget.h"
#include "IEmulatorManager.h"
#include "IPointerScannerStrategy.h"

#include <functional>
#include <memory>
#include <QString>
#include <QStringList>

namespace PointerFinder2::Emulators {

    // This structure maps an emulator profile name (e.g. "PCSX2") to its corresponding 
    // process name, target platform type, range finder dialog, and scanning procedures.
    struct EmulatorProfile {
        QString name;
        DataModels::EmulatorTarget target;
        QStringList processNames;

        std::function<std::unique_ptr<IEmulatorManager>()> managerFactory;
        std::function<std::unique_ptr<IPointerScannerStrategy>()> stateBasedScannerFactory;

        bool supportsStaticRangeFinder = false;
    };

}