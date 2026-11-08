#pragma once

#include "Core/CodeNotes/CodeNoteHierarchyEngine.h"
#include "Core/Structure/StructureAnalyzer.h"
#include "Emulators/IEmulatorManager.h"
#include "Models/PointerPath.h"
#include "UI/Widgets/MemoryViewerWidget.h"

#include <QAction>
#include <QLabel>
#include <QPushButton>
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

  protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

  private slots:
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onTreeCustomContextMenu(const QPoint& pos);
    void onTreeItemChanged(QTreeWidgetItem* item, int column);

  private:
    Emulators::IEmulatorManager* m_manager = nullptr;

    QTreeWidget* m_structureTree = nullptr;
    QLabel* m_offsetPreviewLabel = nullptr;
    QPushButton* m_addVariableBtn = nullptr;
    QPushButton* m_addPointerBtn = nullptr;
    QPushButton* m_followPointerBtn = nullptr;
    QPushButton* m_removeNodeBtn = nullptr;

    // Shared Copy Actions
    QAction* m_treeCopyRaAction = nullptr;
    QAction* m_treeCopyNoteAction = nullptr;
    QAction* m_treeCopyAddressAction = nullptr;
    QAction* m_treeCopyValueAction = nullptr;

    void setupLayout();
    void populateTreeFromHierarchyNodes(const std::vector<std::shared_ptr<Core::CodeNoteNode>>& nodes,
                                        QTreeWidgetItem* parentItem = nullptr);
    std::optional<uint32_t> readMemoryDword(uint32_t address, const std::vector<uint8_t>& snapshotMemory);

    QIcon getThemeIcon(const QString& resourcePath) const;
};

} // namespace PointerFinder2::UI
