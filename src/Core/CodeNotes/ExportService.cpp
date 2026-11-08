#include "ExportService.h"

#include "../Models/CodeNoteSettings.h"
#include "CodeNoteHelper.h"

#include <QApplication>
#include <QClipboard>
#include <QStringList>

namespace PointerFinder2::Core {

using namespace PointerFinder2::DataModels;

int ExportService::copyBaseAddresses(const std::vector<PointerPath>& selectedPaths, const Emulators::IEmulatorManager* manager) {
    if (selectedPaths.empty() || !manager)
        return 0;

    QStringList addresses;
    for (const auto& path : selectedPaths) {
        addresses << manager->formatDisplayAddress(path.baseAddress);
    }

    QApplication::clipboard()->setText(addresses.join("\n"));
    return static_cast<int>(addresses.size());
}

bool ExportService::copyAsRetroAchievementsFormat(const PointerPath& path, Emulators::IEmulatorManager* manager) {
    if (!manager)
        return false;
    QApplication::clipboard()->setText(path.toRetroAchievementsString(manager));
    return true;
}

bool ExportService::copyAsRetroAchievementsFormat(const std::vector<PointerPath>& paths, Emulators::IEmulatorManager* manager) {
    if (paths.empty() || !manager)
        return false;

    QStringList raStrings;
    for (const auto& path : paths) {
        QString trigger = path.toRetroAchievementsString(manager);
        if (!trigger.isEmpty()) {
            raStrings.append(trigger);
        }
    }

    if (raStrings.isEmpty())
        return false;
    QApplication::clipboard()->setText(raStrings.join("_"));
    return true;
}

bool ExportService::copyAsCodeNote(const PointerPath& path, const Emulators::IEmulatorManager* manager) {
    CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();
    QString ptrTemplate = manager ? manager->getPlatformPointerTypeDescription() : "[32-bit] Pointer";
    QString note = CodeNoteHelper::buildCodeNote(path.offsets, settings, "", "", ptrTemplate);

    QApplication::clipboard()->setText(note);
    return true;
}

} // namespace PointerFinder2::Core
