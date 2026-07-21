#pragma once

#include "../Models/SessionData.h"

#include <optional>

namespace PointerFinder2::Core {

    // Handles writing and loading of scan sessions, bookmarks, and array configurations to file.
    // This class handles loading and saving your work, writing found paths,
    // parameters, and bookmarks to file.
    class SessionManager {
    public:
        SessionManager();
        ~SessionManager();

        bool saveSession(const DataModels::SessionData& sessionData, const QString& filePath);
        std::optional<DataModels::SessionData> loadSession(const QString& filePath);
    };

}