#pragma once

#include "../Common/Memory.h"

#include <cstdint>
#include <QString>
#include <vector>

namespace PointerFinder2::Core {

    struct RaCodeNote {
        uint32_t address = 0;
        QString addressHex;
        QString noteText;
        QString title;
        QString author;
        bool isModified = false;
        bool isLocalOnly = false;
    };

    struct RaGameData {
        QString gameId;
        QString gameTitle;
        QString raVersionLine;
        QString notesJsonPath;
        QString userTxtPath;
        std::vector<RaCodeNote> notes;
        bool isValid = false;
    };

    // Service dedicated to discovering RetroAchievements data directories,
    // loading Notes.json, and atomically managing User.txt note overrides.
    class RaStorageService {
    public:
        static QString resolveRaDataDirectory(PlatformHandle processHandle);
        static bool loadActiveGameData(PlatformHandle processHandle, RaGameData& outData);
        static bool saveActiveNoteToUserTxt(RaGameData& gameData, uint32_t activeAddr, const QString& currentNote);
        static bool revertServerNote(RaGameData& gameData, uint32_t activeAddr);
        static bool deleteLocalNote(RaGameData& gameData, uint32_t activeAddr);

        static QString extractTitleFromNote(const QString& noteText);
        static QString unescapeUserTxt(const QString& in);
        static QString escapeUserTxt(const QString& in);
        static void clearDirectoryCache();
    };

}
