#include "UpdateChecker.h"

#include "../UI/Dialogs/UpdateDialog.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

#ifndef VERSION_STRING
#define VERSION_STRING "0.0.0"
#endif

namespace PointerFinder2::Core {

    const QString CURRENT_VERSION = VERSION_STRING;

    UpdateChecker::UpdateChecker(QObject* parent)
        : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {
    }

    UpdateChecker::~UpdateChecker() = default;

    // Queries the online release server to check if a newer version of the program is available.
    void UpdateChecker::checkForUpdates(bool silentOnLatest) {
        m_silentOnLatest = silentOnLatest;

        QUrl url("https://api.github.com/repos/CySlaytor/PointerFinder2/releases/latest");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, "PointerFinder2-UpdateChecker");

        QNetworkReply* reply = m_networkManager->get(request);

        // Connecting directly to the reply's finished signal prevents multiple redundant slots
        // from binding and exhausting network resources on sequential updates.
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            this->onReplyFinished(reply);
            });
    }

    void UpdateChecker::onReplyFinished(QNetworkReply* reply) {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit updateCheckFailed(reply->errorString(), m_silentOnLatest);
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isNull() || !doc.isObject()) {
            emit updateCheckFailed("Invalid JSON response.", m_silentOnLatest);
            return;
        }

        QJsonObject obj = doc.object();

        if (!obj.contains("tag_name") || !obj.contains("body") || !obj.contains("html_url")) {
            emit updateCheckFailed("Malformed server payload.", m_silentOnLatest);
            return;
        }

        QString latestTag = obj["tag_name"].toString();
        QString releaseNotes = obj["body"].toString();
        QString downloadUrl = obj["html_url"].toString();

        if (latestTag.isEmpty()) {
            emit updateCheckFailed("Missing tag_name.", m_silentOnLatest);
            return;
        }

        bool updateAvailable = isVersionNewer(CURRENT_VERSION, latestTag);

        emit updateCheckFinished(updateAvailable, latestTag, releaseNotes, downloadUrl, m_silentOnLatest);
    }

    bool UpdateChecker::isVersionNewer(const QString& current, const QString& latest) {
        QString cleanCur = current.trimmed().toLower();
        if (cleanCur.startsWith('v')) cleanCur.remove(0, 1);

        QString cleanLat = latest.trimmed().toLower();
        if (cleanLat.startsWith('v')) cleanLat.remove(0, 1);

        QStringList curParts = cleanCur.split('.');
        QStringList latParts = cleanLat.split('.');

        int maxComponents = std::max(curParts.size(), latParts.size());
        for (int i = 0; i < maxComponents; ++i) {
            int curVal = (i < curParts.size()) ? curParts[i].toInt() : 0;
            int latVal = (i < latParts.size()) ? latParts[i].toInt() : 0;

            if (latVal > curVal) return true;
            if (curVal > latVal) return false;
        }
        return false;
    }

}