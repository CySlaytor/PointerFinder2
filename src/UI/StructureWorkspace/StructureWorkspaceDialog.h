#pragma once

#include "Core/CodeNotes/CodeNoteHierarchyEngine.h"
#include "Core/Storage/RaStorageService.h"
#include "Core/Structure/StructureAnalyzer.h"
#include "Emulators/IEmulatorManager.h"
#include "Models/PointerPath.h"
#include "UI/Widgets/MemoryViewerWidget.h"
#include "CodeNoteEditorWidget.h"
#include "DissectAddressBar.h"
#include "UI/Dialogs/NewAddressDialog.h"
#include "StructureHierarchyWidget.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>

namespace PointerFinder2::UI {

    // A standalone workspace dialog integrating an intelligent, scope-aware pointer structure editor
    // with a PCSX2-styled read-only memory viewer, eliminating manual offset calculations and level management.
    class StructureWorkspaceDialog : public QDialog {
        Q_OBJECT
    public:
        explicit StructureWorkspaceDialog(Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        ~StructureWorkspaceDialog() override;

        void refreshMemorySnapshot();

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    private slots:
        void onAddressSelectedInViewer(uint32_t address);
        void onAddressDoubleClickedInViewer(uint32_t address);
        void onMemoryViewerContextMenu(uint32_t address, const QPoint& globalPos);
        void onDefineArrayRequested(uint32_t startAddress, uint32_t endAddress, uint32_t byteCount);
        void onAddVariableClicked();
        void onAddPointerClicked();
        void onFollowPointerClicked();
        void onRemoveNodeClicked();
        void onRefreshRamClicked();
        void onGoToAddressClicked();
        void onViewModeChanged();

        // RA Game Data & Dissection slots
        void onNewAddressClicked();
        void onDeleteAddressClicked();
        void onAddressSelectedFromBar(uint32_t address, bool autoNavigate);
        void onSaveUserTxtClicked();
        void onRevertServerClicked();
        void onResetClicked();

        // Clipboard copy slots for Tree & Viewer
        void onCopySelectedAsRaFormat();
        void onCopySelectedAsCodeNote();
        void onCopySelectedAddress();
        void onCopySelectedValue();

        // Panel 3 & Synchronization slots
        void onToggleCodeNotePanelClicked();
        void onFixHierarchyClicked();
        void onHierarchyStyleChanged();
        void onTreeDataChanged();
        void onTreeSelectionChanged();
        void onSplitterMoved(int pos, int index);

    private:
        Emulators::IEmulatorManager* m_manager = nullptr;

        // UI Controls
        QSplitter* m_mainSplitter = nullptr;

        // Top Toolbar Component
        DissectAddressBar* m_addressBar = nullptr;

        // Panel 1: Structure Hierarchy Component
        StructureHierarchyWidget* m_hierarchyWidget = nullptr;

        // Panel 2: Memory Viewer
        QWidget* m_middleContainer = nullptr;
        QPushButton* m_navBackBtn = nullptr;
        QPushButton* m_navForwardBtn = nullptr;
        QLabel* m_navHistoryLabel = nullptr;
        QLineEdit* m_goToAddressEdit = nullptr;
        QPushButton* m_goToAddressBtn = nullptr;
        QPushButton* m_refreshRamBtn = nullptr;
        QLabel* m_ramStatusLabel = nullptr;

        // Memory View Mode Combo
        QComboBox* m_viewModeCombo = nullptr;
        Widgets::MemoryViewerWidget* m_memoryViewer = nullptr;

        // Panel 3: Code Note Hierarchy Fixer Component
        CodeNoteEditorWidget* m_codeNoteEditor = nullptr;

        using RaCodeNote = Core::RaCodeNote;
        using RaGameData = Core::RaGameData;
        using CodeNoteNode = Core::CodeNoteNode;

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
        bool loadActiveGameData();
        void populateAddressDropdown(bool autoNavigate = true);
        void selectAddressInUI(uint32_t address, bool navigate = true);
        void populateTreeFromCodeNote(const QString& noteText);
        bool saveActiveNoteToUserTxt();
        void updateToolbarButtonStates();

        void navigateToAddress(uint32_t address);
        bool isRootArray(uint32_t* outSize = nullptr) const;
        Core::ScopeMatch findBestScopeForAddress(uint32_t selectedAddr);
        QString suggestDefaultType(uint32_t address);

        std::optional<DataModels::PointerPath> getPathForViewerAddress(uint32_t address) const;
        void syncTreeToCodeNoteText();
        void updateOffsetPreview();
        void updateMemoryHighlights();

        QIcon getThemeIcon(const QString& resourcePath) const;
    };

}
