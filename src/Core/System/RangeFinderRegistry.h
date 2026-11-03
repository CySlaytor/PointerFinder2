#pragma once

#include "Emulators/IEmulatorManager.h"
#include "Models/EmulatorTarget.h"

#include <functional>
#include <unordered_map>
#include <QDialog>
#include <QWidget>

namespace PointerFinder2::Core {

    // Decouples static range finder dialog instantiation from UI windows,
    // providing a polymorphic factory mapped by target emulator type.
    class RangeFinderRegistry {
    public:
        using Factory = std::function<QDialog*(Emulators::IEmulatorManager*, QWidget*)>;

        static void registerFactory(DataModels::EmulatorTarget target, Factory factory);
        static QDialog* createDialog(DataModels::EmulatorTarget target, Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        static bool hasFactory(DataModels::EmulatorTarget target);

    private:
        static void ensureInitialized();
        static std::unordered_map<DataModels::EmulatorTarget, Factory> s_factories;
    };

}
