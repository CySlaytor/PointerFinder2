#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>

namespace PointerFinder2::UI {

    // This window displays settings options, letting users configure visual themes,
    // CPU limitation caps, notification sound alerts, and custom developer note layouts.
    class SettingsDialog : public QDialog {
        Q_OBJECT
    public:
        explicit SettingsDialog(QWidget* parent = nullptr);
        ~SettingsDialog() override;

    private slots:
        void onResetAllClicked();
        void onGeneralSettingChanged();
        void onCodeNoteSettingChanged();

    private:
        QTabWidget* m_tabWidget = nullptr;

        QComboBox* m_themeCombo = nullptr;
        QCheckBox* m_useDefaultSoundsCheck = nullptr;
        QCheckBox* m_limitCpuCheck = nullptr;
        QCheckBox* m_sortByLevelCheck = nullptr;
        QPushButton* m_resetAllButton = nullptr;

        QLineEdit* m_prefixEdit = nullptr;
        QLineEdit* m_suffixEdit = nullptr;
        QCheckBox* m_alignCheck = nullptr;
        QCheckBox* m_suffixLastLineCheck = nullptr;
        QPlainTextEdit* m_previewEdit = nullptr;

        QPushButton* m_closeButton = nullptr;
        bool m_isInitializing = true;

        void setupLayout();
        void setupGeneralTab();
        void setupCodeNotesTab();

        void updateCodeNotePreview();
        void triggerApplicationSelfRestart();
    };

}
