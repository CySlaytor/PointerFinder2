#pragma once

#include "../../Emulators/IEmulatorManager.h"

#include <memory>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>

namespace PointerFinder2::UI {

    // This tool converts RetroAchievements cheat triggers into readable multi-level 
    // text code notes, or translates code notes back into raw cheat code strings.
    class CodeNoteConverterDialog : public QDialog {
        Q_OBJECT
    public:
        explicit CodeNoteConverterDialog(Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        ~CodeNoteConverterDialog() override;

        void processTrigger(const QString& trigger);

    private slots:
        void onConvertClicked();
        void onReconvertClicked();
        void onCopyToClipboardClicked();
        void onCopyTriggerClicked();
        void onPointerPrefixChanged(int index);

    private:
        QTabWidget* m_tabWidget = nullptr;
        QWidget* m_toNoteTab = nullptr;
        QWidget* m_toTriggerTab = nullptr;

        QLineEdit* m_triggerInput = nullptr;
        QPushButton* m_convertButton = nullptr;
        QComboBox* m_memorySizeCombo = nullptr;
        QLineEdit* m_descriptionEdit = nullptr;
        QPlainTextEdit* m_codeNoteOutput = nullptr;
        QPushButton* m_copyToClipboardButton = nullptr;

        QPlainTextEdit* m_codeNoteInput = nullptr;
        QLineEdit* m_baseAddressEdit = nullptr;
        QComboBox* m_pointerPrefixCombo = nullptr;
        QCheckBox* m_useMaskCheck = nullptr;
        QPushButton* m_reconvertButton = nullptr;
        QLineEdit* m_triggerOutput = nullptr;
        QPushButton* m_copyTriggerButton = nullptr;

        QPushButton* m_closeButton = nullptr;

        Emulators::IEmulatorManager* m_manager = nullptr;
        std::vector<int32_t> m_lastOffsets;

        static QString s_lastTriggerInput;
        static QString s_lastCodeNoteInput;
        static QString s_lastBaseAddress;
        static int s_lastPrefixIndex;
        static bool s_lastUseMask;
        static QString s_lastDescription;

        void setupLayout();
        void applyEmulatorDefaults();
        void updateNotePreview();
        void saveGeometrySettings();
        void loadGeometrySettings();
    };

}
