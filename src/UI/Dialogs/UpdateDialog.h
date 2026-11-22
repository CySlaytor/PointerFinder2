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
    enum class Action { None, RemindLater, SkipUpdate, UpdateNow };

    UpdateDialog(const QString& currentVer, const QString& latestVer, const QString& releaseNotes, const QString& downloadUrl,
                 QWidget* parent = nullptr);
    ~UpdateDialog() override;

    Action selectedAction() const { return m_selectedAction; }

  protected:
    void done(int r) override;

  private slots:
    void onUpdateNowClicked();
    void onRemindLaterClicked();
    void onSkipUpdateClicked();

  private:
    QString m_downloadUrl;
    Action m_selectedAction = Action::None;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_versionLabel = nullptr;
    QTextBrowser* m_notesBrowser = nullptr;
    QPushButton* m_skipUpdateButton = nullptr;
    QPushButton* m_remindLaterButton = nullptr;
    QPushButton* m_updateNowButton = nullptr;

    void setupLayout(const QString& currentVer, const QString& latestVer, const QString& releaseNotes);
    void loadPositionSettings();
    void savePositionSettings();
};

} // namespace PointerFinder2::UI
