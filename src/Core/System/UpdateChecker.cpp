#include "UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

#include <algorithm>

#ifndef VERSION_STRING
#define VERSION_STRING "0.0.0"
#endif

namespace PointerFinder2::Core {

const QString CURRENT_VERSION = VERSION_STRING;

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {}

UpdateChecker::~UpdateChecker() = default;

// Queries the online release server to check if a newer version of the program is available.
void UpdateChecker::checkForUpdates(bool silentOnLatest) {
    if (!silentOnLatest && m_manualReply) {
        return;
    }

    QUrl url("https://api.github.com/repos/CySlaytor/PointerFinder2/releases/latest");
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "PointerFinder2-UpdateChecker");
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("silentOnLatest", silentOnLatest);
    if (!silentOnLatest) {
        m_manualReply = reply;
    }

    // Connecting directly to the reply's finished signal prevents multiple redundant slots
    // from binding and exhausting network resources on sequential updates.
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { this->onReplyFinished(reply); });
}

void UpdateChecker::cancelManualCheck() {
    if (m_manualReply) {
        m_manualReply->abort();
        m_manualReply = nullptr;
    }
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply) {
    const bool silentOnLatest = reply->property("silentOnLatest").toBool();
    if (!silentOnLatest && m_manualReply == reply) {
        m_manualReply = nullptr;
    }

    const QNetworkReply::NetworkError err = reply->error();
    const QString errString = reply->errorString();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray responseData = reply->readAll();
    reply->deleteLater();

    if (err == QNetworkReply::OperationCanceledError) {
        return;
    }

    if (err != QNetworkReply::NoError) {
        QString message = errString;
        if (statusCode > 0) {
            message = QString("%1 (HTTP %2)").arg(errString).arg(statusCode);
        }
        emit updateCheckFailed(message, silentOnLatest);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        emit updateCheckFailed("Invalid JSON response.", silentOnLatest);
        return;
    }

    QJsonObject obj = doc.object();

    if (!obj.contains("tag_name") || !obj.contains("body") || !obj.contains("html_url")) {
        emit updateCheckFailed("Malformed server payload.", silentOnLatest);
        return;
    }

    QString latestTag = obj["tag_name"].toString();
    QString releaseNotes = obj["body"].toString();
    QString downloadUrl = obj["html_url"].toString();

    if (latestTag.isEmpty()) {
        emit updateCheckFailed("Missing tag_name.", silentOnLatest);
        return;
    }

    bool updateAvailable = isVersionNewer(CURRENT_VERSION, latestTag);

    emit updateCheckFinished(updateAvailable, latestTag, releaseNotes, downloadUrl, silentOnLatest);
}

bool UpdateChecker::isVersionNewer(const QString& current, const QString& latest) {
    auto stripPrefixAndSuffix = [](const QString& ver, bool* hasPreRelease) {
        QString clean = ver.trimmed();
        if (clean.startsWith('v', Qt::CaseInsensitive))
            clean.remove(0, 1);
        int dashIdx = clean.indexOf('-');
        if (dashIdx != -1) {
            if (hasPreRelease != nullptr)
                *hasPreRelease = true;
            clean = clean.left(dashIdx);
        } else if (hasPreRelease != nullptr) {
            *hasPreRelease = false;
        }
        return clean;
    };

    bool curPre = false;
    bool latPre = false;
    QStringList curParts = stripPrefixAndSuffix(current, &curPre).split('.');
    QStringList latParts = stripPrefixAndSuffix(latest, &latPre).split('.');

    qsizetype maxComponents = std::max(curParts.size(), latParts.size());
    for (qsizetype i = 0; i < maxComponents; ++i) {
        int curVal = (i < curParts.size()) ? curParts[i].toInt() : 0;
        int latVal = (i < latParts.size()) ? latParts[i].toInt() : 0;

        if (latVal > curVal)
            return true;
        if (curVal > latVal)
            return false;
    }

    return curPre && !latPre;
}

} // namespace PointerFinder2::Core
