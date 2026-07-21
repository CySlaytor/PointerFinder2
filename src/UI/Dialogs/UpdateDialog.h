#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>

namespace PointerFinder2::UI {

    // This alert popup appears when a newer version of the program is found online,
    // showing update descriptions and a direct download link.
    class UpdateDialog : public QDialog {
        Q_OBJECT
    public:
        UpdateDialog(const QString& currentVer, const QString& latestVer, const QString& releaseNotes, const QString& downloadUrl, QWidget* parent = nullptr);
        ~UpdateDialog() override;

    private slots:
        void onDownloadClicked();

    private:
        QString m_downloadUrl;

        QLabel* m_statusLabel = nullptr;
        QLabel* m_versionLabel = nullptr;
        QTextBrowser* m_notesBrowser = nullptr;
        QPushButton* m_downloadButton = nullptr;
        QPushButton* m_closeButton = nullptr;

        void setupLayout(const QString& currentVer, const QString& latestVer, const QString& releaseNotes);
    };

}
