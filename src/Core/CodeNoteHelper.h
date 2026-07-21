#pragma once

#include "../Models/CodeNoteSettings.h"

#include <optional>
#include <QString>
#include <utility>
#include <vector>

namespace PointerFinder2::Core {

    // This class assists with formatting and parsing plain-text code notes 
    // and RetroAchievements trigger blocks.
    class CodeNoteHelper {
    public:
        // Parses a RetroAchievements trigger string into a vector of offsets and the final memory size descriptor.
        static std::pair<std::vector<int32_t>, QString> parseTrigger(const QString& trigger);

        // Reconstructs a RetroAchievements trigger expression from a multiline human-readable code note.
        static QString generateTriggerFromCodeNote(const QString& codeNote, const QString& baseAddress, const QString& pointerPrefix, bool useMask);

        // Builds a formatted multi-level code note representation from a list of offsets.
        static QString buildCodeNote(const std::vector<int32_t>& offsets, const DataModels::CodeNoteSettings& settings, const QString& finalMemorySize = "", const QString& finalDescription = "");
    };

}
