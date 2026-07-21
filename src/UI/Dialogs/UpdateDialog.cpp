#include "UpdateDialog.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QUrl>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    UpdateDialog::UpdateDialog(const QString& currentVer, const QString& latestVer, const QString& releaseNotes, const QString& downloadUrl, QWidget* parent)
        : QDialog(parent), m_downloadUrl(downloadUrl) {
        setWindowTitle("Pointer Finder 2.0 - Software Update");
        setModal(true);
        resize(520, 360);

        setupLayout(currentVer, latestVer, releaseNotes);
    }

    UpdateDialog::~UpdateDialog() = default;

    void UpdateDialog::setupLayout(const QString& currentVer, const QString& latestVer, const QString& releaseNotes) {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(10);

        m_statusLabel = new QLabel("A new version of Pointer Finder 2.0 is available!", this);
        QFont statusFont = m_statusLabel->font();
        statusFont.setPointSize(11);
        statusFont.setBold(true);
        m_statusLabel->setFont(statusFont);
        mainLayout->addWidget(m_statusLabel);

        m_versionLabel = new QLabel(QString("Current Version: %1  |  Latest Version: %2").arg(currentVer, latestVer), this);
        mainLayout->addWidget(m_versionLabel);

        mainLayout->addWidget(new QLabel("Release Notes:", this));

        m_notesBrowser = new QTextBrowser(this);
        m_notesBrowser->setPlainText(releaseNotes);
        m_notesBrowser->setFrameShape(QFrame::StyledPanel);
        mainLayout->addWidget(m_notesBrowser, 1);

        auto* buttonLayout = new QHBoxLayout();
        m_downloadButton = new QPushButton("Download Update", this);
        m_downloadButton->setStyleSheet("font-weight: bold; min-width: 120px;");
        m_closeButton = new QPushButton("Close", this);
        m_closeButton->setObjectName("cancelButton");

        buttonLayout->addStretch();
        buttonLayout->addWidget(m_downloadButton);
        buttonLayout->addWidget(m_closeButton);
        mainLayout->addLayout(buttonLayout);

        connect(m_downloadButton, &QPushButton::clicked, this, &UpdateDialog::onDownloadClicked);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    // Opens your computer's default web browser and redirects you directly 
    // to the latest software release download webpage.
    void UpdateDialog::onDownloadClicked() {
        QDesktopServices::openUrl(QUrl(m_downloadUrl));
        accept();
    }

}
