#pragma once

#include "../../Emulators/IEmulatorManager.h"
#include "../Widgets/MemoryViewerWidget.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSplitter>
#include <QTreeWidget>

namespace PointerFinder2::UI {

    // A standalone workspace dialog integrating an intelligent, scope-aware pointer structure editor
    // with a PCSX2-styled read-only memory viewer, eliminating manual offset calculations and level management.
    class StructureWorkspaceDialog : public QDialog {
        Q_OBJECT
    public:
        explicit StructureWorkspaceDialog(Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        ~StructureWorkspaceDialog() override;

        void refreshMemorySnapshot();

    private slots:
        void onAddressSelectedInViewer(uint32_t address);
        void onAddressDoubleClickedInViewer(uint32_t address);
        void onStructureTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
        void onStructureTreeCustomContextMenu(const QPoint& pos);
        void onAddVariableClicked();
        void onAddPointerClicked();
        void onFollowPointerClicked();
        void onRemoveNodeClicked();
        void onStructureTreeItemChanged(QTreeWidgetItem* item, int column);
        void onStructureTreeSelectionChanged();
        void onRefreshRamClicked();
        void onGoToAddressClicked();
        void onViewModeChanged();

        // RA Game Data & Dissection slots
        void onAddressComboChanged(int index);
        void onSaveUserTxtClicked();
        void onRevertToServerClicked();
        void onResetClicked();

        // Panel 3 & Synchronization slots
        void onToggleCodeNotePanelClicked();
        void onFixHierarchyClicked();
        void onHierarchyStyleChanged();
        void onCopyCodeNoteClicked();
        void onSplitterMoved(int pos, int index);

    private:
        Emulators::IEmulatorManager* m_manager = nullptr;

        // UI Controls
        QSplitter* m_mainSplitter = nullptr;

        QComboBox* m_addressSelectCombo = nullptr;
        QPushButton* m_saveUserBtn = nullptr;
        QPushButton* m_revertServerBtn = nullptr;
        QPushButton* m_resetBtn = nullptr;
        QPushButton* m_toggleCodeNoteBtn = nullptr;

        // Panel 1: Structure Hierarchy
        QWidget* m_leftContainer = nullptr;
        QTreeWidget* m_structureTree = nullptr;
        QLabel* m_offsetPreviewLabel = nullptr;
        QPushButton* m_addVariableBtn = nullptr;
        QPushButton* m_addPointerBtn = nullptr;
        QPushButton* m_followPointerBtn = nullptr;
        QPushButton* m_removeNodeBtn = nullptr;

        // Panel 2: Memory Viewer
        QWidget* m_middleContainer = nullptr;
        QLineEdit* m_goToAddressEdit = nullptr;
        QPushButton* m_goToAddressBtn = nullptr;
        QPushButton* m_refreshRamBtn = nullptr;
        QLabel* m_ramStatusLabel = nullptr;

        // Memory View Radios
        QRadioButton* m_view8BitRadio = nullptr;
        QRadioButton* m_view16BitRadio = nullptr;
        QRadioButton* m_view32BitRadio = nullptr;
        QRadioButton* m_view32BitBERadio = nullptr;
        QButtonGroup* m_viewModeGroup = nullptr;

        Widgets::MemoryViewerWidget* m_memoryViewer = nullptr;

        // Panel 3: Code Note Hierarchy Fixer
        QWidget* m_rightContainer = nullptr;
        QComboBox* m_styleCombo = nullptr;
        QCheckBox* m_indentDescCheck = nullptr;
        QPushButton* m_fixHierarchyBtn = nullptr;
        QPushButton* m_copyCodeNoteBtn = nullptr;
        QPlainTextEdit* m_codeNoteTextEdit = nullptr;

        struct RaCodeNote {
            uint32_t address = 0;
            QString addressHex;
            QString noteText;
            QString title;
            QString author;
            bool isModified = false;
            bool isLocalOnly = false;
        };

        struct RaGameData {
            QString gameId;
            QString gameTitle;
            QString raVersionLine;
            QString notesJsonPath;
            QString userTxtPath;
            std::vector<RaCodeNote> notes;
            bool isValid = false;
        };

        struct ScopeMatch {
            QTreeWidgetItem* pointerNode = nullptr;
            uint32_t baseAddress = 0;
            int32_t offset = 0;
            int depth = 0;
            bool isValid = false;
        };

        struct CodeNoteNode {
            QString content;
            std::vector<std::shared_ptr<CodeNoteNode>> children;
            int originalIndex = 0;
        };

        RaGameData m_gameData;
        uint32_t m_currentViewerSelectedAddress = 0;
        uint32_t m_currentBaseAddress = 0;

        std::vector<uint8_t> m_snapshotMemory;

        bool m_isSyncing = false;
        bool m_isLoadingSettings = false; // Guard to prevent premature settings overwriting during initialization
        int m_lastCodeNotePanelWidth = 330;
        QString m_headerComment;

        void setupLayout();
        void loadWorkspaceSettings();
        void saveWorkspaceSettings();

        // RA Data Pipeline
        QString resolveRaDataDirectory() const;
        bool loadActiveGameData();
        void populateAddressDropdown();
        void selectAddressInUI(uint32_t address);
        void populateTreeFromCodeNote(const QString& noteText);
        bool saveActiveNoteToUserTxt();
        void updateToolbarButtonStates();

        std::optional<uint32_t> readMemoryDword(uint32_t address);
        void recalculateTreeAddresses();
        uint32_t resolveNodeAddress(QTreeWidgetItem* item);
        void updateOffsetPreview();
        void updateMemoryHighlights();

        void navigateToAddress(uint32_t address);

        ScopeMatch findBestScopeForAddress(uint32_t selectedAddr);
        void insertNodeInScope(QTreeWidgetItem* parentScope, int32_t offset, const QString& description);
        QString suggestDefaultType(uint32_t address);

        void syncTreeToCodeNoteText();
        QString generateCodeNoteFromTree() const;
        QIcon getThemeIcon(const QString& resourcePath) const;

        // Hierarchy Fixer parsing, merging, and rebuilding helpers
        std::vector<std::shared_ptr<CodeNoteNode>> parseLinesToTree(const QStringList& lines);
        std::vector<std::shared_ptr<CodeNoteNode>> mergeTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes);
        std::vector<std::shared_ptr<CodeNoteNode>> sortNodes(std::vector<std::shared_ptr<CodeNoteNode>>& nodes);
        QString rebuildTextFromTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes,
            int indentLevel,
            const QString& style,
            bool indentDescriptions,
            std::vector<bool> ancestorIsLast = {});
        void populateTreeFromHierarchyNodes(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes, QTreeWidgetItem* parentItem = nullptr);
    };

}
