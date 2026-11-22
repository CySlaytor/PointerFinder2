#include "UpdateDialog.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QScreen>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace PointerFinder2::UI {

UpdateDialog::UpdateDialog(const QString& currentVer, const QString& latestVer, const QString& releaseNotes, const QString& downloadUrl,
                           QWidget* parent)
    : QDialog(parent), m_downloadUrl(downloadUrl) {
    setWindowTitle("Pointer Finder 2.0 - Software Update");
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(540, 380);
    setMinimumSize(460, 300);

    setupLayout(currentVer, latestVer, releaseNotes);
    loadPositionSettings();
}

UpdateDialog::~UpdateDialog() = default;

void UpdateDialog::loadPositionSettings() {
    QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
    QSettings settings(iniPath, QSettings::IniFormat);
    settings.beginGroup("DialogPositions");
    if (settings.contains("UpdateDialog")) {
        QString val = settings.value("UpdateDialog").toString();
        if (!val.startsWith("@ByteArray")) {
            QStringList parts = val.split(',');
            if (parts.size() >= 4) {
                bool okX = false, okY = false, okW = false, okH = false;
                int x = parts[0].trimmed().toInt(&okX);
                int y = parts[1].trimmed().toInt(&okY);
                int w = parts[2].trimmed().toInt(&okW);
                int h = parts[3].trimmed().toInt(&okH);
                if (okX && okY && okW && okH) {
                    QSize targetSize(std::max(w, 460), std::max(h, 300));
                    QPoint targetPos(x, y);
                    QRect targetRect(targetPos, targetSize);
                    bool onScreen = false;
                    for (QScreen* screen : QGuiApplication::screens()) {
                        if (screen != nullptr && screen->availableGeometry().intersects(targetRect)) {
                            onScreen = true;
                            break;
                        }
                    }
                    if (!onScreen && QGuiApplication::primaryScreen() != nullptr) {
                        QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
                        int maxX = std::max(avail.left(), avail.right() - targetSize.width());
                        int maxY = std::max(avail.top(), avail.bottom() - targetSize.height());
                        targetPos = QPoint(std::clamp(targetPos.x(), avail.left(), maxX), std::clamp(targetPos.y(), avail.top(), maxY));
                    }
                    move(targetPos);
                    resize(targetSize);
                }
            }
        }
    }
    settings.endGroup();
}

void UpdateDialog::savePositionSettings() {
    QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
    QSettings settings(iniPath, QSettings::IniFormat);
    settings.beginGroup("DialogPositions");
    settings.setValue("UpdateDialog", QString("%1, %2, %3, %4").arg(pos().x()).arg(pos().y()).arg(width()).arg(height()));
    settings.endGroup();
}

void UpdateDialog::done(int r) {
    savePositionSettings();
    QDialog::done(r);
}

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
    buttonLayout->setSpacing(8);

    m_skipUpdateButton = new QPushButton("Skip This Update", this);
    m_skipUpdateButton->setMinimumWidth(115);
    m_skipUpdateButton->setFixedHeight(26);
    m_skipUpdateButton->setAutoDefault(false);
    m_skipUpdateButton->setDefault(false);

    m_remindLaterButton = new QPushButton("Remind Me Later", this);
    m_remindLaterButton->setMinimumWidth(115);
    m_remindLaterButton->setFixedHeight(26);
    m_remindLaterButton->setAutoDefault(false);
    m_remindLaterButton->setDefault(false);

    m_updateNowButton = new QPushButton("Update Now", this);
    m_updateNowButton->setObjectName("updateNowButton");
    m_updateNowButton->setMinimumWidth(105);
    m_updateNowButton->setFixedHeight(26);
    m_updateNowButton->setDefault(true);

    buttonLayout->addWidget(m_skipUpdateButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_remindLaterButton);
    buttonLayout->addWidget(m_updateNowButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_skipUpdateButton, &QPushButton::clicked, this, &UpdateDialog::onSkipUpdateClicked);
    connect(m_remindLaterButton, &QPushButton::clicked, this, &UpdateDialog::onRemindLaterClicked);
    connect(m_updateNowButton, &QPushButton::clicked, this, &UpdateDialog::onUpdateNowClicked);
}

// Opens your computer's default web browser and redirects you directly
// to the latest software release download webpage.
void UpdateDialog::onUpdateNowClicked() {
    m_selectedAction = Action::UpdateNow;
    QDesktopServices::openUrl(QUrl(m_downloadUrl));
    accept();
}

void UpdateDialog::onRemindLaterClicked() {
    m_selectedAction = Action::RemindLater;
    reject();
}

void UpdateDialog::onSkipUpdateClicked() {
    m_selectedAction = Action::SkipUpdate;
    reject();
}

} // namespace PointerFinder2::UI
