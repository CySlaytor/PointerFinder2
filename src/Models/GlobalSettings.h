#pragma once

#include <QString>

namespace PointerFinder2::DataModels {

    // Manages application-wide visual theme preferences, processing thresholds,
    // performance compromises, and default notification options.
    class GlobalSettings {
    public:
        static bool useWindowsDefaultSound;
        static bool limitCpuUsage;
        static QString codeNotePrefix;
        static QString codeNoteSuffix;
        static bool codeNoteAlignSuffixes;
        static bool codeNoteSuffixOnLastLineOnly;
        static bool sortByLevelFirst;
        static QString activeTheme;
    };

}
