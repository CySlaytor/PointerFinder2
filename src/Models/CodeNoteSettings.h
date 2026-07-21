#pragma once

#include "GlobalSettings.h"

#include <QString>

namespace PointerFinder2::DataModels {

    // Configures formatting guidelines (such as prefix/suffix choices and alignment)
    // used when converting raw pointer chains into readable code documentation notes.
    struct CodeNoteSettings {
        QString Prefix;
        QString Suffix;
        bool AlignSuffixes = true;
        bool SuffixOnLastLineOnly = false;

        static CodeNoteSettings getFromGlobalSettings() {
            CodeNoteSettings s;
            s.Prefix = GlobalSettings::codeNotePrefix;
            s.Suffix = GlobalSettings::codeNoteSuffix;
            s.AlignSuffixes = GlobalSettings::codeNoteAlignSuffixes;
            s.SuffixOnLastLineOnly = GlobalSettings::codeNoteSuffixOnLastLineOnly;
            return s;
        }
    };

}
