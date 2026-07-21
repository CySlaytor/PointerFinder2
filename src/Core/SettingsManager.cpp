#include "SettingsManager.h"

#include "../Models/GlobalSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>

namespace PointerFinder2::Core {

    using namespace PointerFinder2::DataModels;

    QString SettingsManager::getSettingsFilePath() {
        return QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
    }

    QString SettingsManager::getSectionName(EmulatorTarget target) {
        switch (target) {
        case EmulatorTarget::PCSX2:         return "Scanner_PCSX2";
        case EmulatorTarget::DuckStation:   return "Scanner_DuckStation";
        case EmulatorTarget::RALibretroNDS: return "Scanner_RALibretroNDS";
        case EmulatorTarget::Dolphin:       return "Scanner_Dolphin";
        case EmulatorTarget::PPSSPP:        return "Scanner_PPSSPP";
        case EmulatorTarget::RALibretroGBA: return "Scanner_RALibretroGBA";
        }
        return "Scanner_Unknown";
    }

    // Reads global styling and layout preferences from your local settings.ini file during startup.
    void SettingsManager::initializeGlobalSettings() {
        QString path = getSettingsFilePath();
        if (!QFile::exists(path)) {
            return;
        }

        QSettings ini(path, QSettings::IniFormat);

        ini.beginGroup("Global");
        GlobalSettings::useWindowsDefaultSound = ini.value("UseWindowsDefaultSound", GlobalSettings::useWindowsDefaultSound).toBool();
        GlobalSettings::limitCpuUsage = ini.value("LimitCpuUsage", GlobalSettings::limitCpuUsage).toBool();
        GlobalSettings::sortByLevelFirst = ini.value("SortByLevelFirst", GlobalSettings::sortByLevelFirst).toBool();
        GlobalSettings::activeTheme = ini.value("ActiveTheme", GlobalSettings::activeTheme).toString();
        ini.endGroup();

        ini.beginGroup("CodeNotes");
        GlobalSettings::codeNotePrefix = ini.value("CodeNotePrefix", GlobalSettings::codeNotePrefix).toString();
        GlobalSettings::codeNoteSuffix = ini.value("CodeNoteSuffix", GlobalSettings::codeNoteSuffix).toString();
        GlobalSettings::codeNoteAlignSuffixes = ini.value("CodeNoteAlignSuffixes", GlobalSettings::codeNoteAlignSuffixes).toBool();
        GlobalSettings::codeNoteSuffixOnLastLineOnly = ini.value("CodeNoteSuffixOnLastLineOnly", GlobalSettings::codeNoteSuffixOnLastLineOnly).toBool();
        ini.endGroup();
    }

    void SettingsManager::saveGlobalSettingsOnly() {
        QSettings ini(getSettingsFilePath(), QSettings::IniFormat);

        ini.beginGroup("Global");
        ini.setValue("UseWindowsDefaultSound", GlobalSettings::useWindowsDefaultSound);
        ini.setValue("LimitCpuUsage", GlobalSettings::limitCpuUsage);
        ini.setValue("SortByLevelFirst", GlobalSettings::sortByLevelFirst);
        ini.setValue("ActiveTheme", GlobalSettings::activeTheme);
        ini.endGroup();

        ini.beginGroup("CodeNotes");
        ini.setValue("CodeNotePrefix", GlobalSettings::codeNotePrefix);
        ini.setValue("CodeNoteSuffix", GlobalSettings::codeNoteSuffix);
        ini.setValue("CodeNoteAlignSuffixes", GlobalSettings::codeNoteAlignSuffixes);
        ini.setValue("CodeNoteSuffixOnLastLineOnly", GlobalSettings::codeNoteSuffixOnLastLineOnly);
        ini.endGroup();
    }

    // Saves default search boundaries and slider values for a specific emulator profile.
    void SettingsManager::save(EmulatorTarget target, const AppSettings& settings) {
        QSettings ini(getSettingsFilePath(), QSettings::IniFormat);
        QString section = getSectionName(target);

        ini.beginGroup(section);
        ini.setValue("LastTargetAddress", settings.lastTargetAddress);
        ini.setValue("MaxOffset", settings.maxOffset);
        ini.setValue("UseLastOffsetHint", settings.useLastOffsetHint);

        if (settings.lastOffsetHint.has_value()) {
            ini.setValue("LastOffsetHint", QString::number(settings.lastOffsetHint.value(), 16).toUpper());
        }
        else {
            ini.setValue("LastOffsetHint", "");
        }

        ini.setValue("MaxLevel", settings.maxLevel);
        ini.setValue("StaticAddressStart", settings.staticAddressStart);
        ini.setValue("StaticAddressEnd", settings.staticAddressEnd);
        ini.setValue("StopOnFirstPathFound", settings.stopOnFirstPathFound);
        ini.setValue("FindAllPathLevels", settings.findAllPathLevels);
        ini.setValue("CandidatesPerLevel", settings.candidatesPerLevel);
        ini.setValue("MaxCandidates", settings.maxCandidates);
        ini.setValue("FastScanMode", settings.fastScanMode);
        ini.setValue("PrintPartialPaths", settings.printPartialPaths);
        ini.setValue("DynamicStaticDetection", settings.dynamicStaticDetection);
        ini.endGroup();

        saveGlobalSettingsOnly();
    }

    // Loads saved search ranges and options for a specific emulator from the settings.ini file.
    AppSettings SettingsManager::load(EmulatorTarget target, const AppSettings& defaultSettings) {
        QString path = getSettingsFilePath();
        if (!QFile::exists(path)) {
            return defaultSettings;
        }

        QSettings ini(path, QSettings::IniFormat);
        QString section = getSectionName(target);
        AppSettings settings;

        ini.beginGroup(section);
        settings.lastTargetAddress = ini.value("LastTargetAddress", defaultSettings.lastTargetAddress).toString();
        settings.maxOffset = ini.value("MaxOffset", defaultSettings.maxOffset).toInt();
        settings.useLastOffsetHint = ini.value("UseLastOffsetHint", defaultSettings.useLastOffsetHint).toBool();

        QString hintStr = ini.value("LastOffsetHint", "").toString();
        if (!hintStr.isEmpty()) {
            bool ok;
            int parsedHint = hintStr.toInt(&ok, 16);
            if (ok) {
                settings.lastOffsetHint = parsedHint;
            }
            else {
                settings.lastOffsetHint = std::nullopt;
            }
        }
        else {
            settings.lastOffsetHint = std::nullopt;
        }

        settings.maxLevel = ini.value("MaxLevel", defaultSettings.maxLevel).toInt();
        if (settings.maxLevel < 1) {
            settings.maxLevel = defaultSettings.maxLevel;
        }

        settings.staticAddressStart = ini.value("StaticAddressStart", defaultSettings.staticAddressStart).toString();
        settings.staticAddressEnd = ini.value("StaticAddressEnd", defaultSettings.staticAddressEnd).toString();
        settings.stopOnFirstPathFound = ini.value("StopOnFirstPathFound", defaultSettings.stopOnFirstPathFound).toBool();
        settings.findAllPathLevels = ini.value("FindAllPathLevels", defaultSettings.findAllPathLevels).toBool();

        settings.candidatesPerLevel = ini.value("CandidatesPerLevel", defaultSettings.candidatesPerLevel).toInt();
        if (settings.candidatesPerLevel < 1) {
            settings.candidatesPerLevel = defaultSettings.candidatesPerLevel;
        }

        settings.maxCandidates = ini.value("MaxCandidates", defaultSettings.maxCandidates).toInt();
        if (settings.maxCandidates < 1) {
            settings.maxCandidates = defaultSettings.maxCandidates;
        }

        settings.fastScanMode = ini.value("FastScanMode", defaultSettings.fastScanMode).toBool();
        settings.printPartialPaths = ini.value("PrintPartialPaths", defaultSettings.printPartialPaths).toBool();
        settings.dynamicStaticDetection = ini.value("DynamicStaticDetection", defaultSettings.dynamicStaticDetection).toBool();
        ini.endGroup();

        ini.beginGroup("Global");
        GlobalSettings::useWindowsDefaultSound = ini.value("UseWindowsDefaultSound", GlobalSettings::useWindowsDefaultSound).toBool();
        GlobalSettings::limitCpuUsage = ini.value("LimitCpuUsage", GlobalSettings::limitCpuUsage).toBool();
        GlobalSettings::sortByLevelFirst = ini.value("SortByLevelFirst", GlobalSettings::sortByLevelFirst).toBool();
        GlobalSettings::activeTheme = ini.value("ActiveTheme", GlobalSettings::activeTheme).toString();
        ini.endGroup();

        ini.beginGroup("CodeNotes");
        GlobalSettings::codeNotePrefix = ini.value("CodeNotePrefix", GlobalSettings::codeNotePrefix).toString();
        GlobalSettings::codeNoteSuffix = ini.value("CodeNoteSuffix", GlobalSettings::codeNoteSuffix).toString();
        GlobalSettings::codeNoteAlignSuffixes = ini.value("CodeNoteAlignSuffixes", GlobalSettings::codeNoteAlignSuffixes).toBool();
        GlobalSettings::codeNoteSuffixOnLastLineOnly = ini.value("CodeNoteSuffixOnLastLineOnly", GlobalSettings::codeNoteSuffixOnLastLineOnly).toBool();
        ini.endGroup();

        return settings;
    }

}