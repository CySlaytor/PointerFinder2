#include "SessionManager.h"

#include <QFile>
#include <QJsonDocument>

namespace PointerFinder2::Core {

    SessionManager::SessionManager() = default;
    SessionManager::~SessionManager() = default;

    // Converts your active bookmarks and search results into a saved workspace file.
    bool SessionManager::saveSession(const DataModels::SessionData& sessionData, const QString& filePath) {
        if (filePath.isEmpty()) {
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }

        QJsonObject json = sessionData.toJson();
        QJsonDocument doc(json);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    // Loads and resumes a previously saved progress file.
    std::optional<DataModels::SessionData> SessionManager::loadSession(const QString& filePath) {
        if (filePath.isEmpty()) {
            return std::nullopt;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return std::nullopt;
        }

        QByteArray fileData = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(fileData);
        if (doc.isNull() || !doc.isObject()) {
            return std::nullopt;
        }

        return DataModels::SessionData::fromJson(doc.object());
    }

}