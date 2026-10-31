#pragma once

#include "../../Emulators/IEmulatorManager.h"
#include "../Widgets/MemoryViewerWidget.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
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
        void onNavHistoryBack();
        void onNavHistoryForward();
        void onExportCodeNoteClicked();
        void onImportCodeNoteClicked();
        void onActiveStructureComboChanged(int index);
        void onNewStructureClicked();
        void onDeleteStructureClicked();
        void onViewModeChanged();

    private:
        Emulators::IEmulatorManager* m_manager = nullptr;

        // UI Controls
        QComboBox* m_structureSelectCombo = nullptr;
        QPushButton* m_newStructureBtn = nullptr;
        QPushButton* m_deleteStructureBtn = nullptr;

        QTreeWidget* m_structureTree = nullptr;
        QLabel* m_offsetPreviewLabel = nullptr;
        QPushButton* m_addVariableBtn = nullptr;
        QPushButton* m_addPointerBtn = nullptr;
        QPushButton* m_followPointerBtn = nullptr;
        QPushButton* m_removeNodeBtn = nullptr;

        QPushButton* m_importNoteBtn = nullptr;
        QPushButton* m_exportNoteBtn = nullptr;

        QLineEdit* m_baseAddressEdit = nullptr;
        QPushButton* m_historyBackBtn = nullptr;
        QPushButton* m_historyForwardBtn = nullptr;
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

        struct StructureItemData {
            QString name;
            uint32_t baseAddress = 0;
        };

        struct ScopeMatch {
            QTreeWidgetItem* pointerNode = nullptr;
            uint32_t baseAddress = 0;
            int32_t offset = 0;
            int depth = 0;
            bool isValid = false;
        };

        std::vector<StructureItemData> m_structures;
        int m_currentStructureIndex = 0;
        uint32_t m_currentViewerSelectedAddress = 0;

        // Navigation History Stack (Cheat Engine style back/forward trail)
        std::vector<uint32_t> m_navHistory;
        int m_navHistoryIndex = -1;

        std::vector<uint8_t> m_snapshotMemory;

        void setupLayout();
        void loadWorkspaceSettings();
        void saveWorkspaceSettings();

        std::optional<uint32_t> readMemoryDword(uint32_t address);
        void recalculateTreeAddresses();
        uint32_t resolveNodeAddress(QTreeWidgetItem* item);
        void updateOffsetPreview();
        void updateMemoryHighlights();

        void navigateToAddress(uint32_t address, bool recordHistory = true);
        void updateHistoryButtonStates();

        ScopeMatch findBestScopeForAddress(uint32_t selectedAddr);
        void insertNodeInScope(QTreeWidgetItem* parentScope, int32_t offset, const QString& description);
        QString suggestDefaultType(uint32_t address);
    };

}
