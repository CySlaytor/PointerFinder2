#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

namespace PointerFinder2::Core {

    // Queries the public GitHub repository API to verify if a new release is available.
    // This class checks for software updates by contacting the online release server in the background.
    class UpdateChecker : public QObject {
        Q_OBJECT
    public:
        explicit UpdateChecker(QObject* parent = nullptr);
        ~UpdateChecker() override;

        void checkForUpdates(bool silentOnLatest = false);

    signals:
        void updateCheckFinished(bool updateAvailable, const QString& latestVersion, const QString& releaseNotes, const QString& downloadUrl, bool silentOnLatest);
        void updateCheckFailed(const QString& errorMessage, bool silentOnLatest);

    private slots:
        void onReplyFinished(QNetworkReply* reply);

    private:
        QNetworkAccessManager* m_networkManager = nullptr;
        bool m_silentOnLatest = false;

        bool isVersionNewer(const QString& current, const QString& latest);
    };

}