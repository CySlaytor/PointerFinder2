#pragma once

#include "Core/CodeNotes/CodeNoteHierarchyEngine.h"
#include "Core/Structure/StructureAnalyzer.h"
#include "Emulators/IEmulatorManager.h"
#include "Models/PointerPath.h"
#include "UI/Widgets/MemoryViewerWidget.h"

#include <QAction>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QWidget>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace PointerFinder2::UI {

// Encapsulates the Structure Hierarchy (Panel 1):
// tree structure representation, offset calculations, item management,
// and context menu actions.
class StructureHierarchyWidget : public QWidget {
    Q_OBJECT
  public:
    static constexpr int COL_NAME = 0;
    static constexpr int COL_SIZE = 1;
    static constexpr int COL_OFFSET = 2;
    static constexpr int COL_RESOLVED = 3;
    static constexpr int COL_VALUE = 4;

    static constexpr int META_COL_SIZE = 0;
    static constexpr int META_COL_OFFSET = 1;
    static constexpr int META_COL_RESOLVED = 2;
    static constexpr int META_COL_VALUE = 3;

    explicit StructureHierarchyWidget(QWidget* parent = nullptr);
    ~StructureHierarchyWidget() override;

    void setEmulatorManager(Emulators::IEmulatorManager* manager);

    void populateFromNodes(const std::vector<std::shared_ptr<Core::CodeNoteNode>>& nodes);
    void clearTree();

    void recalculateTreeAddresses(uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory, bool rootArr);
    uint32_t resolveNodeAddress(QTreeWidgetItem* item, uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory,
                                bool rootArr);

    void updateOffsetPreview(uint32_t selectedAddr, const Core::ScopeMatch& match, const QString& headerComment);
    void insertNodeInScope(QTreeWidgetItem* parentScope, int32_t offset, const QString& description, bool isArrayElement = false);
    void removeDocumentationAtAddress(uint32_t address, uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory,
                                      bool rootArr);

    void collectScopes(std::vector<Core::ScopeCandidate>& candidateScopes, uint32_t currentBaseAddress,
                       const std::vector<uint8_t>& snapshotMemory, bool rootArr);
    void gatherHighlights(std::unordered_map<uint32_t, Widgets::MemoryViewerWidget::HighlightInfo>& highlights,
                          std::vector<Widgets::MemoryViewerWidget::DefinedArrayInfo>& definedArrays, uint32_t currentBaseAddress,
                          bool rootArr, const std::vector<uint8_t>& snapshotMemory);

    QString generateCodeNoteFromTree(bool useUnicode, bool indentDesc, const QString& headerComment) const;
    QString generateCodeNoteFromTree(const QString& style, bool indentDesc, const QString& headerComment) const;

    QTreeWidgetItem* currentItem() const;
    QList<QTreeWidgetItem*> selectedItemsInOrder() const;
    QTreeWidget* treeWidget() const;

    std::optional<DataModels::PointerPath> getPathForTreeItem(QTreeWidgetItem* item, uint32_t currentBaseAddress,
                                                              const std::vector<uint8_t>& snapshotMemory, bool rootArr) const;
    QString formatRetroAchievementsTrigger(QTreeWidgetItem* item, uint32_t currentBaseAddress, bool rootArr,
                                           const QString& headerComment) const;

    void updateThemeIcons();
    void saveColumnSettings();
    void loadColumnSettings();

    void setMetadataVisible(bool visible);
    bool isMetadataVisible() const;

  signals:
    void addVariableClicked();
    void addPointerClicked();
    void followPointerClicked();
    void removeNodeClicked();
    void treeSelectionChanged();
    void treeDataChanged();

    void copyRaRequested();
    void copyNoteRequested();
    void copyAddressRequested();
    void copyValueRequested();
    void statusMessageRequested(const QString& message);

  protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private slots:
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onTreeCustomContextMenu(const QPoint& pos);
    void onTreeItemChanged(QTreeWidgetItem* item, int column);
    void onMetadataItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onMetadataCustomContextMenu(const QPoint& pos);
    void onToggleMetadataClicked();

  private:
    Emulators::IEmulatorManager* m_manager = nullptr;

    QSplitter* m_innerSplitter = nullptr;
    QTreeWidget* m_structureTree = nullptr; // Left hierarchy tree: Description / Name
    QTreeWidget* m_metadataTree = nullptr;  // Right pinned grid: Size, Offset, Resolved RAM, Pointer Val
    QLabel* m_offsetPreviewLabel = nullptr;
    QPushButton* m_addVariableBtn = nullptr;
    QPushButton* m_addPointerBtn = nullptr;
    QPushButton* m_followPointerBtn = nullptr;
    QPushButton* m_removeNodeBtn = nullptr;
    QPushButton* m_toggleMetadataBtn = nullptr;

    // Shared Copy Actions
    QAction* m_treeCopyRaAction = nullptr;
    QAction* m_treeCopyNoteAction = nullptr;
    QAction* m_treeCopyAddressAction = nullptr;
    QAction* m_treeCopyValueAction = nullptr;

    bool m_isAdjustingColumns = false;
    bool m_isSyncingSelection = false;
    bool m_isSyncingScroll = false;
    bool m_isSyncingExpand = false;
    int m_lastMetadataWidth = 320;

    int getColumnMinWidth(int column) const;
    void adjustColumnWidths();

    void setupLayout();
    void populateTreeFromHierarchyNodes(const std::vector<std::shared_ptr<Core::CodeNoteNode>>& nodes,
                                        QTreeWidgetItem* parentItem = nullptr,
                                        QTreeWidgetItem* metaParentItem = nullptr);
    std::optional<uint32_t> readMemoryDword(uint32_t address, const std::vector<uint8_t>& snapshotMemory);

    QTreeWidgetItem* getTwinItem(QTreeWidgetItem* item) const;
    void setTwinItems(QTreeWidgetItem* leftItem, QTreeWidgetItem* rightItem);
    bool isPointerItem(QTreeWidgetItem* item) const;
    void openSizeDropdown(QTreeWidgetItem* item, const QPoint& globalPos);
    void applySizeToItem(QTreeWidgetItem* item, const QString& newSize);

    QIcon getThemeIcon(const QString& resourcePath) const;
};

} // namespace PointerFinder2::UI
