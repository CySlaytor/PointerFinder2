#include "StructureHierarchyWidget.h"

#include "Models/GlobalSettings.h"
#include "UI/Common/ThemeManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <algorithm>

namespace PointerFinder2::UI {

using namespace PointerFinder2::Core;
using namespace PointerFinder2::DataModels;

namespace {
// Item delegate restricting cell editing strictly to Column 0 (Description / Name)
class NonEditableColumnDelegate : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.column() != 0) {
            return nullptr;
        }
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
};

// Extracts bracketed size/mask specifiers into a dedicated clean string (without brackets) and returns the clean description
void parseSizeAndDescription(const QString& rawDesc, QString& outSize, QString& outCleanDesc) {
    QString trimmed = rawDesc.trimmed();
    outSize.clear();
    outCleanDesc = trimmed;

    static const QRegularExpression bracketRx(R"(^\[([^\]]+)\]\s*(.*)$)");
    auto match = bracketRx.match(trimmed);
    if (match.hasMatch()) {
        QString tag = match.captured(1).trimmed();
        // Validate whether the bracket represents a memory size, data type, bitfield, mask, or byte span
        static const QRegularExpression sizePatternRx(
            R"((\d+-bit|\d+\s*x\s*\d+\s*bytes|\d+\s*bytes|float|double|byte|bit\d*|pointer|\&|lower|upper|mbf))",
            QRegularExpression::CaseInsensitiveOption);
        if (sizePatternRx.match(tag).hasMatch()) {
            outSize = tag; // Clean representation without brackets
            outCleanDesc = match.captured(2).trimmed();
        }
    }
}

// Extracts numeric bitmask from a text string (e.g. "& 0x1FFFFFFF", "& 33554431")
uint32_t extractMaskValueFromText(const QString& text) {
    static const QRegularExpression maskRx(R"(\&\s*(?:0x([0-9a-fA-F]+)|(\d+)))");
    auto match = maskRx.match(text);
    if (match.hasMatch()) {
        bool ok = false;
        if (!match.captured(1).isEmpty()) {
            uint32_t val = match.captured(1).toUInt(&ok, 16);
            if (ok && val != 0)
                return val;
        } else if (!match.captured(2).isEmpty()) {
            uint32_t val = match.captured(2).toUInt(&ok, 10);
            if (ok && val != 0)
                return val;
        }
    }
    return 0;
}

// Standard RetroAchievements memory data types (>= 8-bit allocation sizes)
static const QStringList StandardRATypes = {
    "8-bit",      "16-bit",     "24-bit",     "32-bit",
    "16-bit BE",  "24-bit BE",  "32-bit BE",  "Float",
    "Float BE",   "Double32",   "Double32 BE","MBF32",
    "MBF32 LE"
};
} // namespace

StructureHierarchyWidget::StructureHierarchyWidget(QWidget* parent) : QWidget(parent) { setupLayout(); }

StructureHierarchyWidget::~StructureHierarchyWidget() {
    saveColumnSettings();
}

QIcon StructureHierarchyWidget::getThemeIcon(const QString& resourcePath) const {
    return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
}

void StructureHierarchyWidget::updateThemeIcons() {
    m_addVariableBtn->setIcon(getThemeIcon(":/icons/bookmark_add.svg"));
    m_addPointerBtn->setIcon(getThemeIcon(":/icons/attach.svg"));
    if (m_descriptionsBtn) {
        m_descriptionsBtn->setIcon(getThemeIcon(":/icons/copy_note.svg"));
    }
    m_followPointerBtn->setIcon(getThemeIcon(":/icons/follow_pointer.svg"));
    m_removeNodeBtn->setIcon(getThemeIcon(":/icons/delete.svg"));
    m_toggleMetadataBtn->setIcon(getThemeIcon(":/icons/converter.svg"));

    m_treeCopyRaAction->setIcon(getThemeIcon(":/icons/copy_ra.svg"));
    m_treeCopyNoteAction->setIcon(getThemeIcon(":/icons/copy_note.svg"));
    m_treeCopyAddressAction->setIcon(getThemeIcon(":/icons/copy.svg"));
    m_treeCopyValueAction->setIcon(getThemeIcon(":/icons/copy_ra.svg"));
}

void StructureHierarchyWidget::setEmulatorManager(Emulators::IEmulatorManager* manager) { m_manager = manager; }

QTreeWidgetItem* StructureHierarchyWidget::currentItem() const { return m_structureTree ? m_structureTree->currentItem() : nullptr; }

QList<QTreeWidgetItem*> StructureHierarchyWidget::selectedItemsInOrder() const {
    QList<QTreeWidgetItem*> result;
    if (!m_structureTree)
        return result;

    QList<QTreeWidgetItem*> selected = m_structureTree->selectedItems();
    if (selected.isEmpty())
        return result;

    // Collect selected items in natural top-to-bottom visual hierarchy order
    QTreeWidgetItemIterator it(m_structureTree);
    while (*it) {
        if (selected.contains(*it)) {
            result.append(*it);
        }
        ++it;
    }
    return result;
}

QTreeWidget* StructureHierarchyWidget::treeWidget() const { return m_structureTree; }

void StructureHierarchyWidget::clearTree() {
    if (m_structureTree) {
        m_structureTree->blockSignals(true);
        m_structureTree->clear();
        m_structureTree->blockSignals(false);
    }
    if (m_metadataTree) {
        m_metadataTree->blockSignals(true);
        m_metadataTree->clear();
        m_metadataTree->blockSignals(false);
    }
}

QTreeWidgetItem* StructureHierarchyWidget::getTwinItem(QTreeWidgetItem* item) const {
    if (!item)
        return nullptr;
    return static_cast<QTreeWidgetItem*>(item->data(0, Qt::UserRole + 10).value<void*>());
}

void StructureHierarchyWidget::setTwinItems(QTreeWidgetItem* leftItem, QTreeWidgetItem* rightItem) {
    if (!leftItem || !rightItem)
        return;
    leftItem->setData(0, Qt::UserRole + 10, QVariant::fromValue(static_cast<void*>(rightItem)));
    rightItem->setData(0, Qt::UserRole + 10, QVariant::fromValue(static_cast<void*>(leftItem)));
}

bool StructureHierarchyWidget::isPointerItem(QTreeWidgetItem* item) const {
    if (!item)
        return false;
    QTreeWidgetItem* meta = (item->treeWidget() == m_metadataTree) ? item : getTwinItem(item);
    QString sizeText = meta ? meta->text(META_COL_SIZE) : "";
    return (item->childCount() > 0 || item->text(0).contains("Pointer", Qt::CaseInsensitive) ||
            sizeText.contains("Pointer", Qt::CaseInsensitive) || sizeText.contains("&"));
}

int StructureHierarchyWidget::getColumnMinWidth(int column) const {
    switch (column) {
    case COL_NAME:
        return 120;
    case COL_SIZE:
        return 65;
    case COL_OFFSET:
        return 55;
    case COL_RESOLVED:
    case COL_VALUE:
        return 88; // Sufficient for full 8-digit hexadecimal addresses (e.g. 0x80B4A600)
    default:
        return 50;
    }
}

void StructureHierarchyWidget::setMetadataVisible(bool visible) {
    if (!m_metadataTree || !m_innerSplitter)
        return;

    m_toggleMetadataBtn->blockSignals(true);
    m_toggleMetadataBtn->setChecked(visible);
    m_toggleMetadataBtn->blockSignals(false);

    QList<int> sizes = m_innerSplitter->sizes();
    if (visible) {
        // Position vertical scrollbar strictly on the far right of the metadata grid
        m_structureTree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_metadataTree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        int restoreW = (m_lastMetadataWidth >= 200) ? m_lastMetadataWidth : 320;
        m_metadataTree->setVisible(true);
        if (sizes.size() >= 2) {
            sizes[1] = restoreW;
            sizes[0] = std::max(120, m_innerSplitter->width() - restoreW - m_innerSplitter->handleWidth());
            m_innerSplitter->setSizes(sizes);
        }
    } else {
        // Position vertical scrollbar on the far right of the full-width structure tree
        m_structureTree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_metadataTree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        if (sizes.size() >= 2 && sizes[1] > 0) {
            m_lastMetadataWidth = sizes[1];
        }
        m_metadataTree->setVisible(false);
        if (sizes.size() >= 2) {
            sizes[0] = m_innerSplitter->width();
            sizes[1] = 0;
            m_innerSplitter->setSizes(sizes);
        }
    }
    saveColumnSettings();
}

bool StructureHierarchyWidget::isMetadataVisible() const {
    return m_metadataTree && m_metadataTree->isVisible();
}

void StructureHierarchyWidget::onToggleMetadataClicked() {
    setMetadataVisible(m_toggleMetadataBtn->isChecked());
}

void StructureHierarchyWidget::saveColumnSettings() {
    if (!m_metadataTree)
        return;

    QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
    QSettings settings(iniPath, QSettings::IniFormat);
    settings.beginGroup("DissectStructure");
    settings.setValue("colWidthName", m_structureTree ? m_structureTree->columnWidth(0) : 386);
    settings.setValue("colWidthSize", m_metadataTree->columnWidth(META_COL_SIZE));
    settings.setValue("colWidthOffset", m_metadataTree->columnWidth(META_COL_OFFSET));
    settings.setValue("colWidthResolved", m_metadataTree->columnWidth(META_COL_RESOLVED));
    settings.setValue("colWidthPointerVal", m_metadataTree->columnWidth(META_COL_VALUE));
    settings.setValue("metadataColumnsVisible", isMetadataVisible());
    settings.setValue("lastMetadataWidth", m_lastMetadataWidth);

    // Save human-readable integer splitter sizes rather than binary byte arrays
    if (m_innerSplitter) {
        QList<int> splitterSizes = m_innerSplitter->sizes();
        int splitLeft = (splitterSizes.size() >= 1) ? splitterSizes[0] : 386;
        int splitRight = (splitterSizes.size() >= 2 && splitterSizes[1] > 0) ? splitterSizes[1] : m_lastMetadataWidth;
        settings.setValue("hierarchySplitterLeft", splitLeft);
        settings.setValue("hierarchySplitterRight", splitRight);
    }
    settings.endGroup();
}

void StructureHierarchyWidget::loadColumnSettings() {
    if (!m_metadataTree)
        return;

    QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
    QSettings settings(iniPath, QSettings::IniFormat);
    settings.beginGroup("DissectStructure");

    // Default to preferred proportions when settings.ini does not exist
    int wName = settings.value("colWidthName", 386).toInt();
    int wSize = settings.value("colWidthSize", 77).toInt();
    int wOffset = settings.value("colWidthOffset", 65).toInt();
    int wResolved = settings.value("colWidthResolved", 88).toInt();
    int wValue = settings.value("colWidthPointerVal", 90).toInt();
    bool metaVisible = settings.value("metadataColumnsVisible", true).toBool();
    m_lastMetadataWidth = settings.value("lastMetadataWidth", 320).toInt();

    int splitLeft = settings.value("hierarchySplitterLeft", 386).toInt();
    int splitRight = settings.value("hierarchySplitterRight", 320).toInt();
    if (splitRight >= 200) {
        m_lastMetadataWidth = splitRight;
    }

    if (m_innerSplitter) {
        if (metaVisible) {
            m_innerSplitter->setSizes({splitLeft, splitRight});
        } else {
            m_innerSplitter->setSizes({splitLeft + splitRight, 0});
        }
    }
    settings.endGroup();

    m_isAdjustingColumns = true;
    if (m_structureTree) {
        m_structureTree->setColumnWidth(0, std::max(getColumnMinWidth(COL_NAME), wName));
    }
    m_metadataTree->setColumnWidth(META_COL_SIZE, std::max(getColumnMinWidth(COL_SIZE), wSize));
    m_metadataTree->setColumnWidth(META_COL_OFFSET, std::max(getColumnMinWidth(COL_OFFSET), wOffset));
    m_metadataTree->setColumnWidth(META_COL_RESOLVED, std::max(getColumnMinWidth(COL_RESOLVED), wResolved));
    m_metadataTree->setColumnWidth(META_COL_VALUE, std::max(getColumnMinWidth(COL_VALUE), wValue));
    m_isAdjustingColumns = false;

    setMetadataVisible(metaVisible);
}

void StructureHierarchyWidget::adjustColumnWidths() {
    if (m_isAdjustingColumns || !m_structureTree || !m_metadataTree)
        return;

    m_isAdjustingColumns = true;
    int w1 = std::max(getColumnMinWidth(COL_SIZE), m_metadataTree->columnWidth(META_COL_SIZE));
    int w2 = std::max(getColumnMinWidth(COL_OFFSET), m_metadataTree->columnWidth(META_COL_OFFSET));
    int w3 = std::max(getColumnMinWidth(COL_RESOLVED), m_metadataTree->columnWidth(META_COL_RESOLVED));
    int w4 = std::max(getColumnMinWidth(COL_VALUE), m_metadataTree->columnWidth(META_COL_VALUE));

    m_metadataTree->setColumnWidth(META_COL_SIZE, w1);
    m_metadataTree->setColumnWidth(META_COL_OFFSET, w2);
    m_metadataTree->setColumnWidth(META_COL_RESOLVED, w3);
    m_metadataTree->setColumnWidth(META_COL_VALUE, w4);
    m_isAdjustingColumns = false;
}

void StructureHierarchyWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    adjustColumnWidths();
}

void StructureHierarchyWidget::setupLayout() {
    setMinimumWidth(280);

    auto* leftLayout = new QVBoxLayout(this);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    // Integrated Dual-Synchronized Splitter inside Structure Hierarchy Widget
    m_innerSplitter = new QSplitter(Qt::Horizontal, this);
    m_innerSplitter->setChildrenCollapsible(false);

    // 1. Left Hierarchy Tree: Houses Description / Name without scrollbar clutter
    m_structureTree = new QTreeWidget(m_innerSplitter);
    m_structureTree->setColumnCount(1);
    m_structureTree->setHeaderLabels({"Description / Name"});
    m_structureTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_structureTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_structureTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_structureTree->setAlternatingRowColors(true);
    m_structureTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_structureTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    m_structureTree->setItemDelegate(new NonEditableColumnDelegate(m_structureTree));
    m_structureTree->setUniformRowHeights(true);

    // 2. Right Metadata Grid: Houses pinned Size, Offset, Resolved RAM, and Pointer Val with horizontal scrollbar
    m_metadataTree = new QTreeWidget(m_innerSplitter);
    m_metadataTree->setColumnCount(4);
    m_metadataTree->setHeaderLabels({"Size", "Offset", "Resolved RAM", "Pointer Val"});
    m_metadataTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_metadataTree->setRootIsDecorated(false);
    m_metadataTree->setIndentation(0);
    m_metadataTree->header()->setStretchLastSection(false);
    m_metadataTree->header()->setMinimumSectionSize(50);
    m_metadataTree->header()->setSectionResizeMode(META_COL_SIZE, QHeaderView::Interactive);
    m_metadataTree->header()->setSectionResizeMode(META_COL_OFFSET, QHeaderView::Interactive);
    m_metadataTree->header()->setSectionResizeMode(META_COL_RESOLVED, QHeaderView::Interactive);
    m_metadataTree->header()->setSectionResizeMode(META_COL_VALUE, QHeaderView::Interactive);
    m_metadataTree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_metadataTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_metadataTree->setAlternatingRowColors(true);
    m_metadataTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_metadataTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_metadataTree->setUniformRowHeights(true);

    m_innerSplitter->addWidget(m_structureTree);
    m_innerSplitter->addWidget(m_metadataTree);
    m_innerSplitter->setStretchFactor(0, 1);
    m_innerSplitter->setStretchFactor(1, 0);

    leftLayout->addWidget(m_innerSplitter, 1);

    // Lockstep Vertical Scrolling Synchronization
    connect(m_structureTree->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        if (m_isSyncingScroll)
            return;
        m_isSyncingScroll = true;
        m_metadataTree->verticalScrollBar()->setValue(value);
        m_isSyncingScroll = false;
    });
    connect(m_metadataTree->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        if (m_isSyncingScroll)
            return;
        m_isSyncingScroll = true;
        m_structureTree->verticalScrollBar()->setValue(value);
        m_isSyncingScroll = false;
    });

    // Lockstep Expansion & Collapse Synchronization
    connect(m_structureTree, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* item) {
        if (m_isSyncingExpand)
            return;
        m_isSyncingExpand = true;
        if (auto* twin = getTwinItem(item)) {
            twin->setExpanded(true);
        }
        m_isSyncingExpand = false;
    });
    connect(m_structureTree, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem* item) {
        if (m_isSyncingExpand)
            return;
        m_isSyncingExpand = true;
        if (auto* twin = getTwinItem(item)) {
            twin->setExpanded(false);
        }
        m_isSyncingExpand = false;
    });

    // Lockstep Multi-Selection Synchronization
    connect(m_structureTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        if (m_isSyncingSelection)
            return;
        m_isSyncingSelection = true;
        m_metadataTree->blockSignals(true);
        m_metadataTree->clearSelection();
        for (auto* item : m_structureTree->selectedItems()) {
            if (auto* twin = getTwinItem(item)) {
                twin->setSelected(true);
            }
        }
        m_metadataTree->blockSignals(false);
        m_isSyncingSelection = false;
        emit treeSelectionChanged();
    });
    connect(m_metadataTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        if (m_isSyncingSelection)
            return;
        m_isSyncingSelection = true;
        m_structureTree->blockSignals(true);
        m_structureTree->clearSelection();
        for (auto* item : m_metadataTree->selectedItems()) {
            if (auto* twin = getTwinItem(item)) {
                twin->setSelected(true);
            }
        }
        m_structureTree->blockSignals(false);
        m_isSyncingSelection = false;
        emit treeSelectionChanged();
    });

    // Enforce individual column minimum widths during manual resize and auto-save adjustments
    connect(m_metadataTree->header(), &QHeaderView::sectionResized, this,
            [this](int logicalIndex, int /*oldSize*/, int newSize) {
                if (m_isAdjustingColumns)
                    return;

                int minWidth = getColumnMinWidth(logicalIndex + 1);
                if (newSize < minWidth) {
                    m_isAdjustingColumns = true;
                    m_metadataTree->setColumnWidth(logicalIndex, minWidth);
                    m_isAdjustingColumns = false;
                }

                saveColumnSettings();
            });

    connect(m_innerSplitter, &QSplitter::splitterMoved, this, [this]() { saveColumnSettings(); });

    // Install event filter on m_structureTree to handle navigation and hierarchy shortcuts (Enter, F, V, P, Delete, X)
    m_structureTree->installEventFilter(this);
    m_metadataTree->installEventFilter(this);

    // Setup actions and keyboard shortcuts for Structure Hierarchy
    m_treeCopyRaAction = new QAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this);
    m_treeCopyRaAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    m_treeCopyRaAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_treeCopyRaAction, &QAction::triggered, this, &StructureHierarchyWidget::copyRaRequested);

    m_treeCopyNoteAction = new QAction(getThemeIcon(":/icons/copy_note.svg"), "Copy Code Note", this);
    connect(m_treeCopyNoteAction, &QAction::triggered, this, &StructureHierarchyWidget::copyNoteRequested);

    m_treeCopyAddressAction = new QAction(getThemeIcon(":/icons/copy.svg"), "Copy Address", this);
    m_treeCopyAddressAction->setShortcut(QKeySequence::Copy);
    m_treeCopyAddressAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_treeCopyAddressAction, &QAction::triggered, this, &StructureHierarchyWidget::copyAddressRequested);

    m_treeCopyValueAction = new QAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Value", this);
    m_treeCopyValueAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
    m_treeCopyValueAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_treeCopyValueAction, &QAction::triggered, this, &StructureHierarchyWidget::copyValueRequested);

    m_structureTree->addAction(m_treeCopyRaAction);
    m_structureTree->addAction(m_treeCopyNoteAction);
    m_structureTree->addAction(m_treeCopyAddressAction);
    m_structureTree->addAction(m_treeCopyValueAction);

    m_metadataTree->addAction(m_treeCopyRaAction);
    m_metadataTree->addAction(m_treeCopyNoteAction);
    m_metadataTree->addAction(m_treeCopyAddressAction);
    m_metadataTree->addAction(m_treeCopyValueAction);

    // Hook Delete key to remove selected hierarchy node(s)
    auto* removeKeyAction = new QAction(this);
    removeKeyAction->setShortcut(QKeySequence::Delete);
    removeKeyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(removeKeyAction, &QAction::triggered, this, &StructureHierarchyWidget::removeNodeClicked);
    m_structureTree->addAction(removeKeyAction);
    m_metadataTree->addAction(removeKeyAction);

    m_offsetPreviewLabel = new QLabel("Select an address in Memory Viewer to calculate relative offset.", this);
    m_offsetPreviewLabel->setStyleSheet("font-weight: bold; color: #58A6FF; padding: 4px; border: 1px solid #30363D; border-radius: 4px;");
    leftLayout->addWidget(m_offsetPreviewLabel);

    auto* treeButtonsRow = new QHBoxLayout();
    treeButtonsRow->setSpacing(6);

    m_addVariableBtn = new QPushButton("Add Variable", this);
    m_addVariableBtn->setToolTip("Add variable at selected address (Shortcut: V)");
    m_addVariableBtn->setIcon(getThemeIcon(":/icons/bookmark_add.svg"));
    m_addVariableBtn->setFixedHeight(26);
    m_addVariableBtn->setAutoDefault(false);
    m_addVariableBtn->setDefault(false);

    m_addPointerBtn = new QPushButton("Add Pointer", this);
    m_addPointerBtn->setToolTip("Add pointer at selected address (Shortcut: P)");
    m_addPointerBtn->setIcon(getThemeIcon(":/icons/attach.svg"));
    m_addPointerBtn->setFixedHeight(26);
    m_addPointerBtn->setAutoDefault(false);
    m_addPointerBtn->setDefault(false);

    m_descriptionsBtn = new QPushButton("Descriptions", this);
    m_descriptionsBtn->setToolTip("Add or edit value descriptions for selected variable (Shortcut: D)");
    m_descriptionsBtn->setIcon(getThemeIcon(":/icons/copy_note.svg"));
    m_descriptionsBtn->setFixedHeight(26);
    m_descriptionsBtn->setAutoDefault(false);
    m_descriptionsBtn->setDefault(false);

    m_followPointerBtn = new QPushButton("Follow Pointer", this);
    m_followPointerBtn->setToolTip("Follow pointer value to destination address (Shortcut: Enter, F, or Double-click)");
    m_followPointerBtn->setIcon(getThemeIcon(":/icons/follow_pointer.svg"));
    m_followPointerBtn->setFixedHeight(26);
    m_followPointerBtn->setAutoDefault(false);
    m_followPointerBtn->setDefault(false);

    m_removeNodeBtn = new QPushButton("Remove", this);
    m_removeNodeBtn->setIcon(getThemeIcon(":/icons/delete.svg"));
    m_removeNodeBtn->setToolTip("Remove selected node (Shortcut: Delete / X)");
    m_removeNodeBtn->setFixedHeight(26);
    m_removeNodeBtn->setAutoDefault(false);
    m_removeNodeBtn->setDefault(false);

    m_toggleMetadataBtn = new QPushButton("Metadata", this);
    m_toggleMetadataBtn->setToolTip("Show/Hide Metadata Columns (Size, Offset, Resolved RAM, Pointer Val)");
    m_toggleMetadataBtn->setIcon(getThemeIcon(":/icons/converter.svg"));
    m_toggleMetadataBtn->setCheckable(true);
    m_toggleMetadataBtn->setChecked(true);
    m_toggleMetadataBtn->setFixedHeight(26);
    m_toggleMetadataBtn->setAutoDefault(false);
    m_toggleMetadataBtn->setDefault(false);

    treeButtonsRow->addWidget(m_addVariableBtn, 1);
    treeButtonsRow->addWidget(m_addPointerBtn, 1);
    treeButtonsRow->addWidget(m_descriptionsBtn, 1);
    treeButtonsRow->addWidget(m_followPointerBtn, 1);
    treeButtonsRow->addWidget(m_removeNodeBtn, 1);
    treeButtonsRow->addWidget(m_toggleMetadataBtn, 1);
    leftLayout->addLayout(treeButtonsRow);

    // Load persisted column widths and metadata visibility
    loadColumnSettings();

    connect(m_structureTree, &QTreeWidget::itemDoubleClicked, this, &StructureHierarchyWidget::onTreeItemDoubleClicked);
    connect(m_structureTree, &QTreeWidget::customContextMenuRequested, this, &StructureHierarchyWidget::onTreeCustomContextMenu);
    connect(m_structureTree, &QTreeWidget::itemChanged, this, &StructureHierarchyWidget::onTreeItemChanged);

    connect(m_metadataTree, &QTreeWidget::itemDoubleClicked, this, &StructureHierarchyWidget::onMetadataItemDoubleClicked);
    connect(m_metadataTree, &QTreeWidget::customContextMenuRequested, this, &StructureHierarchyWidget::onMetadataCustomContextMenu);

    connect(m_addVariableBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::addVariableClicked);
    connect(m_addPointerBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::addPointerClicked);
    connect(m_descriptionsBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::manageDescriptionsClicked);
    connect(m_followPointerBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::followPointerClicked);
    connect(m_removeNodeBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::removeNodeClicked);
    connect(m_toggleMetadataBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::onToggleMetadataClicked);
}

bool StructureHierarchyWidget::eventFilter(QObject* obj, QEvent* event) {
    if ((obj == m_structureTree || obj == m_metadataTree) && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        // Do not intercept hotkeys if an item cell editor is actively open
        QWidget* focusW = QApplication::focusWidget();
        bool isEditing = focusW && (focusW != m_structureTree) && (focusW != m_structureTree->viewport()) &&
                         (focusW != m_metadataTree) && (focusW != m_metadataTree->viewport()) &&
                         (m_structureTree->isAncestorOf(focusW) || m_metadataTree->isAncestorOf(focusW));

        if (!isEditing) {
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_F) {
                keyEvent->accept();
                emit followPointerClicked();
                return true;
            }
            if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_X) {
                keyEvent->accept();
                emit removeNodeClicked();
                return true;
            }
            if (keyEvent->key() == Qt::Key_V) {
                keyEvent->accept();
                emit addVariableClicked();
                return true;
            }
            if (keyEvent->key() == Qt::Key_P) {
                keyEvent->accept();
                emit addPointerClicked();
                return true;
            }
            if (keyEvent->key() == Qt::Key_D) {
                keyEvent->accept();
                emit manageDescriptionsClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void StructureHierarchyWidget::populateFromNodes(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
    m_structureTree->blockSignals(true);
    m_metadataTree->blockSignals(true);
    m_structureTree->clear();
    m_metadataTree->clear();

    populateTreeFromHierarchyNodes(nodes);

    m_structureTree->expandAll();
    m_metadataTree->expandAll();
    m_structureTree->blockSignals(false);
    m_metadataTree->blockSignals(false);
}

void StructureHierarchyWidget::populateTreeFromHierarchyNodes(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes,
                                                              QTreeWidgetItem* parentItem, QTreeWidgetItem* metaParentItem) {
    QRegularExpression offsetRx("^((?:\\|[\\+\\-]?|[\\+\\-])0x[0-9a-fA-F]+)(?:([=\\s\\|]+)(.*))?$");

    for (const auto& node : nodes) {
        auto match = offsetRx.match(node->content.trimmed());
        if (match.hasMatch()) {
            QString rawOffset = match.captured(1);
            QString delimiter = match.captured(2);
            QString desc = match.captured(3).trimmed();

            bool isArrayChild = rawOffset.startsWith("|+") || rawOffset.startsWith("|-");
            bool isArrayElem = !isArrayChild && rawOffset.startsWith('|');
            if (isArrayChild) {
                rawOffset = rawOffset.mid(1);
            } else if (isArrayElem) {
                rawOffset = rawOffset.mid(1);
            }

            QString sign = rawOffset.startsWith('-') ? "-" : (rawOffset.startsWith('+') ? "+" : "");
            QString hexPart = (rawOffset.startsWith('-') || rawOffset.startsWith('+')) ? rawOffset.mid(1) : rawOffset;
            if (hexPart.startsWith("0x", Qt::CaseInsensitive)) {
                hexPart = hexPart.mid(2);
            }

            bool ok = false;
            int32_t offsetVal = hexPart.toInt(&ok, 16);
            if (sign == "-")
                offsetVal = -offsetVal;

            QString sizeStr;
            QString cleanDesc;
            parseSizeAndDescription(desc, sizeStr, cleanDesc);

            // Left Tree Item: Clean Description / Name
            auto* leftItem = new QTreeWidgetItem(parentItem ? parentItem : m_structureTree->invisibleRootItem());
            leftItem->setText(0, cleanDesc);
            leftItem->setFlags(leftItem->flags() | Qt::ItemIsEditable);

            // Right Grid Item: Pinned Size, Offset, Resolved RAM, Pointer Val
            auto* rightItem = new QTreeWidgetItem(metaParentItem ? metaParentItem : m_metadataTree->invisibleRootItem());
            rightItem->setText(META_COL_SIZE, sizeStr);

            QString offsetHex = QString("%1").arg(std::abs(offsetVal), 2, 16, QChar('0')).toUpper();
            QString displayOffset;
            if (isArrayChild) {
                displayOffset = "|+" + (offsetVal < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
            } else if (isArrayElem) {
                displayOffset = "|" + (offsetVal < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
            } else {
                displayOffset = (offsetVal < 0 ? "-0x" : "+0x") + offsetHex;
            }

            rightItem->setText(META_COL_OFFSET, displayOffset);

            // Bidirectional $O(1)$ Twin Linking
            setTwinItems(leftItem, rightItem);

            // Backwards-compatible data role storage
            leftItem->setData(1, Qt::UserRole, offsetVal);
            leftItem->setData(2, Qt::UserRole, offsetVal);
            leftItem->setData(1, Qt::UserRole + 1, delimiter.isEmpty() ? "=" : delimiter);
            leftItem->setData(2, Qt::UserRole + 1, delimiter.isEmpty() ? "=" : delimiter);
            leftItem->setData(1, Qt::UserRole + 3, isArrayElem);
            leftItem->setData(2, Qt::UserRole + 3, isArrayElem);

            rightItem->setData(1, Qt::UserRole, offsetVal);
            rightItem->setData(2, Qt::UserRole, offsetVal);
            rightItem->setData(1, Qt::UserRole + 1, delimiter.isEmpty() ? "=" : delimiter);
            rightItem->setData(2, Qt::UserRole + 1, delimiter.isEmpty() ? "=" : delimiter);
            rightItem->setData(1, Qt::UserRole + 3, isArrayElem);
            rightItem->setData(2, Qt::UserRole + 3, isArrayElem);

            QStringList notes;
            for (const auto& child : node->children) {
                bool isChildOffset =
                    child->content.startsWith("+0x", Qt::CaseInsensitive) || child->content.startsWith("-0x", Qt::CaseInsensitive) ||
                    child->content.startsWith("|0x", Qt::CaseInsensitive) || child->content.startsWith("|+0x", Qt::CaseInsensitive) ||
                    child->content.startsWith("|-0x", Qt::CaseInsensitive);
                if (!isChildOffset) {
                    notes.append(child->content);
                }
            }
            if (!notes.empty()) {
                leftItem->setData(0, Qt::UserRole + 2, notes);
                leftItem->setToolTip(0, notes.join("\n"));
            }

            std::vector<std::shared_ptr<CodeNoteNode>> pointerChildren;
            for (const auto& child : node->children) {
                bool isChildOffset =
                    child->content.startsWith("+0x", Qt::CaseInsensitive) || child->content.startsWith("-0x", Qt::CaseInsensitive) ||
                    child->content.startsWith("|0x", Qt::CaseInsensitive) || child->content.startsWith("|+0x", Qt::CaseInsensitive) ||
                    child->content.startsWith("|-0x", Qt::CaseInsensitive);
                if (isChildOffset) {
                    pointerChildren.push_back(child);
                }
            }

            if (!pointerChildren.empty()) {
                populateTreeFromHierarchyNodes(pointerChildren, leftItem, rightItem);
            }
        }
    }
}

std::optional<uint32_t> StructureHierarchyWidget::readMemoryDword(uint32_t address, const std::vector<uint8_t>& snapshotMemory) {
    return StructureAnalyzer::readMemoryDword(address, m_manager, snapshotMemory);
}

uint32_t StructureHierarchyWidget::resolveNodeAddress(QTreeWidgetItem* item, uint32_t currentBaseAddress,
                                                      const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
    if (!item || !m_manager || currentBaseAddress == 0)
        return 0;

    // Resolve base from emulator manager
    uint32_t base = m_manager->unnormalizeAddress(QString::number(currentBaseAddress, 16));
    if (base == 0)
        return 0;

    uint32_t currentAddress = base;

    if (!rootArr) {
        auto basePtr = readMemoryDword(base, snapshotMemory);
        if (basePtr.has_value() && m_manager->isValidPointerTarget(basePtr.value())) {
            currentAddress = m_manager->unnormalizeAddress(QString::number(basePtr.value(), 16));
        }
    }

    QTreeWidgetItem* hierItem = (item->treeWidget() == m_metadataTree) ? getTwinItem(item) : item;
    if (!hierItem)
        return 0;

    std::vector<QTreeWidgetItem*> lineage;
    QTreeWidgetItem* curr = hierItem;
    while (curr) {
        lineage.push_back(curr);
        curr = curr->parent();
    }
    std::reverse(lineage.begin(), lineage.end());

    for (size_t i = 0; i < lineage.size(); ++i) {
        auto* node = lineage[i];
        int32_t offset = node->data(2, Qt::UserRole).isValid() ? node->data(2, Qt::UserRole).toInt() : node->data(1, Qt::UserRole).toInt();
        currentAddress += static_cast<uint32_t>(offset);

        if (node != hierItem && isPointerItem(node)) {
            auto ptrVal = readMemoryDword(currentAddress, snapshotMemory);
            if (ptrVal.has_value() && m_manager && m_manager->isValidPointerTarget(ptrVal.value())) {
                currentAddress = m_manager->unnormalizeAddress(QString::number(ptrVal.value(), 16));
            } else {
                return 0;
            }
        }
    }
    return currentAddress;
}

QTreeWidgetItem* StructureHierarchyWidget::findItemForAddress(uint32_t address, uint32_t currentBaseAddress,
                                                              const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
    if (!m_manager || address == 0 || currentBaseAddress == 0)
        return nullptr;

    QTreeWidgetItem* targetItem = nullptr;
    std::function<void(QTreeWidgetItem*)> search = [&](QTreeWidgetItem* item) {
        if (targetItem)
            return;
        uint32_t resolved = resolveNodeAddress(item, currentBaseAddress, snapshotMemory, rootArr);
        if (resolved == address) {
            targetItem = item;
            return;
        }
        for (int i = 0; i < item->childCount(); ++i) {
            search(item->child(i));
        }
    };

    for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
        search(m_structureTree->topLevelItem(i));
        if (targetItem)
            break;
    }
    return targetItem;
}

bool StructureHierarchyWidget::locateAndSelectAddress(uint32_t address, uint32_t currentBaseAddress,
                                                      const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
    if (!m_manager || address == 0 || currentBaseAddress == 0)
        return false;

    QTreeWidgetItem* targetItem = findItemForAddress(address, currentBaseAddress, snapshotMemory, rootArr);

    if (targetItem) {
        // Expand all ancestors to make the item visible in the tree
        QTreeWidgetItem* p = targetItem->parent();
        while (p) {
            p->setExpanded(true);
            if (auto* metaTwin = getTwinItem(p)) {
                metaTwin->setExpanded(true);
            }
            p = p->parent();
        }

        // Preserve horizontal scroll position across navigation to prevent unexpected view shifts
        int savedTreeHScroll = m_structureTree->horizontalScrollBar() ? m_structureTree->horizontalScrollBar()->value() : 0;
        int savedMetaHScroll = m_metadataTree->horizontalScrollBar() ? m_metadataTree->horizontalScrollBar()->value() : 0;

        m_structureTree->setCurrentItem(targetItem);
        if (auto* metaItem = getTwinItem(targetItem)) {
            m_metadataTree->setCurrentItem(metaItem);
        }
        m_structureTree->scrollToItem(targetItem, QAbstractItemView::PositionAtCenter);
        m_structureTree->setFocus();

        if (m_structureTree->horizontalScrollBar()) {
            m_structureTree->horizontalScrollBar()->setValue(savedTreeHScroll);
        }
        if (m_metadataTree->horizontalScrollBar()) {
            m_metadataTree->horizontalScrollBar()->setValue(savedMetaHScroll);
        }
        return true;
    }
    return false;
}

void StructureHierarchyWidget::recalculateTreeAddresses(uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory,
                                                        bool rootArr) {
    if (!m_manager || currentBaseAddress == 0)
        return;

    // Block tree signals during internal address and pointer calculations to prevent spurious itemChanged events
    m_structureTree->blockSignals(true);
    m_metadataTree->blockSignals(true);

    uint32_t base = m_manager->unnormalizeAddress(QString::number(currentBaseAddress, 16));

    uint32_t initialParentBase = base;
    bool initialValid = (base != 0);

    if (!rootArr) {
        auto basePtr = readMemoryDword(base, snapshotMemory);
        if (basePtr.has_value() && m_manager->isValidPointerTarget(basePtr.value())) {
            initialParentBase = m_manager->unnormalizeAddress(QString::number(basePtr.value(), 16));
        }
    }

    std::function<void(QTreeWidgetItem*, uint32_t, bool)> updateNode = [&](QTreeWidgetItem* node, uint32_t parentBase, bool parentValid) {
        QTreeWidgetItem* meta = getTwinItem(node);

        if (!parentValid || parentBase == 0) {
            if (meta) {
                meta->setText(META_COL_RESOLVED, "N/A");
                meta->setText(META_COL_VALUE, "N/A");
                meta->setForeground(META_COL_RESOLVED, QColor(140, 140, 140));
                meta->setForeground(META_COL_VALUE, QColor(140, 140, 140));
            }

            for (int i = 0; i < node->childCount(); ++i) {
                updateNode(node->child(i), 0, false);
            }
            return;
        }

        int32_t offset = node->data(2, Qt::UserRole).isValid() ? node->data(2, Qt::UserRole).toInt() : node->data(1, Qt::UserRole).toInt();
        uint32_t resolved = parentBase + static_cast<uint32_t>(offset);

        QString displayAddr = m_manager->formatDisplayAddress(resolved);
        if (meta) {
            meta->setText(META_COL_RESOLVED, "0x" + displayAddr);
            meta->setForeground(META_COL_RESOLVED, palette().color(QPalette::Text));
        }

        auto valOpt = readMemoryDword(resolved, snapshotMemory);
        uint32_t nextBase = 0;
        bool nextValid = false;

        if (valOpt.has_value()) {
            uint32_t rawVal = valOpt.value();
            QString valHex = QString("%1").arg(rawVal, 8, 16, QChar('0')).toUpper();
            if (meta) {
                meta->setText(META_COL_VALUE, "0x" + valHex);
                meta->setForeground(META_COL_VALUE, palette().color(QPalette::Text));
            }

            if (isPointerItem(node)) {
                if (rawVal != 0 && m_manager->isValidPointerTarget(rawVal)) {
                    nextBase = m_manager->unnormalizeAddress(QString::number(rawVal, 16));
                    nextValid = true;
                    node->setData(0, Qt::UserRole + 4, nextBase);
                } else {
                    nextValid = false;
                    node->setData(0, Qt::UserRole + 4, 0);
                }
            }
        } else {
            if (meta) {
                meta->setText(META_COL_VALUE, "N/A");
                meta->setForeground(META_COL_VALUE, QColor(140, 140, 140));
            }
            nextValid = false;
            node->setData(0, Qt::UserRole + 4, 0);
        }

        for (int i = 0; i < node->childCount(); ++i) {
            updateNode(node->child(i), nextBase, nextValid);
        }
    };

    for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
        updateNode(m_structureTree->topLevelItem(i), initialParentBase, initialValid);
    }

    m_structureTree->blockSignals(false);
    m_metadataTree->blockSignals(false);
}

void StructureHierarchyWidget::collectScopes(std::vector<ScopeCandidate>& candidateScopes, uint32_t currentBaseAddress,
                                             const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
    std::function<void(QTreeWidgetItem*, int)> collect = [&](QTreeWidgetItem* item, int depth) {
        uint32_t derefBase = item->data(0, Qt::UserRole + 4).toUInt();
        QString desc = item->text(0).trimmed();
        QTreeWidgetItem* meta = getTwinItem(item);
        QString sizeTag = meta ? meta->text(META_COL_SIZE).trimmed() : "";

        static const QRegularExpression arrayTagRx(R"(\[(?:(\d+)\s*x\s*(\d+)|(\d+))\s*bytes\])", QRegularExpression::CaseInsensitiveOption);
        auto match = arrayTagRx.match(desc);
        if (!match.hasMatch()) {
            match = arrayTagRx.match(sizeTag);
        }

        bool isArray = item->data(0, Qt::UserRole + 6).toBool() || match.hasMatch();
        uint32_t arraySize = item->data(0, Qt::UserRole + 5).toUInt();
        if (arraySize == 0 && match.hasMatch()) {
            if (!match.captured(1).isEmpty() && !match.captured(2).isEmpty()) {
                arraySize = match.captured(1).toUInt() * match.captured(2).toUInt();
            } else if (!match.captured(3).isEmpty()) {
                arraySize = match.captured(3).toUInt();
            }
        }

        if (derefBase != 0) {
            candidateScopes.push_back({item, derefBase, depth, isArray, arraySize});
        } else if (isArray && arraySize > 0) {
            uint32_t nodeAddr = resolveNodeAddress(item, currentBaseAddress, snapshotMemory, rootArr);
            if (nodeAddr != 0) {
                candidateScopes.push_back({item, nodeAddr, depth, true, arraySize});
            }
        }

        for (int i = 0; i < item->childCount(); ++i) {
            collect(item->child(i), depth + 1);
        }
    };

    for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
        collect(m_structureTree->topLevelItem(i), 1);
    }
}

void StructureHierarchyWidget::gatherHighlights(std::unordered_map<uint32_t, Widgets::MemoryViewerWidget::HighlightInfo>& highlights,
                                                std::vector<Widgets::MemoryViewerWidget::DefinedArrayInfo>& definedArrays,
                                                uint32_t currentBaseAddress, bool rootArr, const std::vector<uint8_t>& snapshotMemory,
                                                const QString& headerComment) {
    if (!m_manager)
        return;

    QTreeWidgetItem* activeItem = m_structureTree->currentItem();

    std::function<void(QTreeWidgetItem*)> gather = [&](QTreeWidgetItem* item) {
        uint32_t resolved = resolveNodeAddress(item, currentBaseAddress, snapshotMemory, rootArr);
        if (resolved != 0) {
            uint32_t nativeAddr = m_manager->unnormalizeAddress(QString::number(resolved, 16));
            Widgets::MemoryViewerWidget::HighlightInfo info;

            QTreeWidgetItem* meta = getTwinItem(item);
            QString sizeTag = meta ? meta->text(META_COL_SIZE).trimmed() : "";
            if (sizeTag.isEmpty()) {
                sizeTag = item->text(0).trimmed();
            }

            uint32_t varSize = 4;
            QString tagLower = sizeTag.toLower();
            if (tagLower.startsWith('[') && tagLower.endsWith(']')) {
                tagLower = tagLower.mid(1, tagLower.length() - 2).trimmed();
            }

            if (tagLower.startsWith("8-bit") || tagLower == "8bit" || tagLower == "byte" || tagLower.startsWith("bit") ||
                tagLower.contains("bitflag") || tagLower == "lower4" || tagLower == "upper4") {
                varSize = 1;
            } else if (tagLower.startsWith("16-bit") || tagLower == "16bit" || tagLower == "short") {
                varSize = 2;
            } else if (tagLower.startsWith("24-bit") || tagLower == "24bit") {
                varSize = 3;
            } else if (tagLower.startsWith("64-bit") || tagLower == "64bit" || tagLower.contains("double")) {
                varSize = 8;
            } else if (tagLower.startsWith("32-bit") || tagLower == "32bit" || tagLower.contains("float") || tagLower.contains("pointer")) {
                varSize = 4;
            }

            info.name = item->text(0).trimmed();
            info.offset = meta ? meta->text(META_COL_OFFSET).trimmed() : "";
            info.label = info.name + " (" + info.offset + ")";
            info.startAddress = nativeAddr;
            info.size = varSize;
            info.isPointer = isPointerItem(item);
            info.isChildOfActive = (activeItem && (item == activeItem || item->parent() == activeItem));

            // Resolve pointer mask following the 3 priority levels:
            // 1. Pointer-specific mask from node's size tag or description
            // 2. Header mask from the top-level structure declaration
            // 3. Fallback to full address (0xFFFFFFFF)
            if (info.isPointer) {
                uint32_t mask = extractMaskValueFromText(meta ? meta->text(META_COL_SIZE) : "");
                if (mask == 0) {
                    mask = extractMaskValueFromText(item->text(0));
                }
                if (mask == 0 && !headerComment.isEmpty()) {
                    mask = extractMaskValueFromText(headerComment);
                }
                info.pointerMask = (mask != 0) ? mask : 0xFFFFFFFF;
            }

            for (uint32_t b = 0; b < varSize; ++b) {
                highlights[nativeAddr + b] = info;
            }
        }

        // Identify defined array boundaries across the hierarchy
        QString desc = item->text(0).trimmed();
        QTreeWidgetItem* meta = getTwinItem(item);
        QString sizeTagStr = meta ? meta->text(META_COL_SIZE).trimmed() : "";
        static const QRegularExpression arrayTagRx(R"(\[(?:(\d+)\s*x\s*(\d+)|(\d+))\s*bytes\])", QRegularExpression::CaseInsensitiveOption);
        auto arrayMatch = arrayTagRx.match(desc);
        if (!arrayMatch.hasMatch()) {
            arrayMatch = arrayTagRx.match(sizeTagStr);
        }

        bool isArray = item->data(0, Qt::UserRole + 6).toBool() || arrayMatch.hasMatch();
        uint32_t arraySize = item->data(0, Qt::UserRole + 5).toUInt();
        if (arraySize == 0 && arrayMatch.hasMatch()) {
            if (!arrayMatch.captured(1).isEmpty() && !arrayMatch.captured(2).isEmpty()) {
                arraySize = arrayMatch.captured(1).toUInt() * arrayMatch.captured(2).toUInt();
            } else if (!arrayMatch.captured(3).isEmpty()) {
                arraySize = arrayMatch.captured(3).toUInt();
            }
        }

        if (isArray && arraySize > 0) {
            uint32_t targetBase = item->data(0, Qt::UserRole + 4).toUInt();
            if (targetBase == 0) {
                targetBase = resolved;
            }

            if (targetBase != 0) {
                Widgets::MemoryViewerWidget::DefinedArrayInfo arrInfo;
                arrInfo.name = desc;
                arrInfo.startAddress = targetBase;
                arrInfo.size = arraySize;
                arrInfo.endAddress = targetBase + arraySize - 1;
                definedArrays.push_back(arrInfo);
            }
        }

        for (int i = 0; i < item->childCount(); ++i) {
            gather(item->child(i));
        }
    };

    for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
        gather(m_structureTree->topLevelItem(i));
    }
}

void StructureHierarchyWidget::updateOffsetPreview(uint32_t selectedAddr, const ScopeMatch& match, const QString& headerComment) {
    if (selectedAddr == 0) {
        m_offsetPreviewLabel->setText("Select an address in Memory Viewer to calculate relative offset.");
        return;
    }

    if (match.isValid) {
        QString scopeName;
        auto* node = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
        if (node) {
            QTreeWidgetItem* hierItem = (node->treeWidget() == m_metadataTree) ? getTwinItem(node) : node;
            QTreeWidgetItem* metaItem = (node->treeWidget() == m_metadataTree) ? node : getTwinItem(node);
            QString offStr = metaItem ? metaItem->text(META_COL_OFFSET) : "";
            scopeName = (hierItem ? hierItem->text(0) : "") + " (" + offStr + ")";
        } else if (match.isArrayScope) {
            scopeName = headerComment.isEmpty() ? "Root Array" : headerComment.trimmed();
        } else {
            scopeName = "Root Base Pointer";
        }

        QString sign = (match.offset < 0) ? "-" : "+";
        QString offsetHex = QString("%1").arg(std::abs(match.offset), 2, 16, QChar('0')).toUpper();

        QString prefix = match.isArrayScope ? "|" : sign;
        QString offsetDisplay = prefix + (match.isArrayScope && match.offset < 0 ? "-" : "") + "0x" + offsetHex;
        m_offsetPreviewLabel->setText(QString("Selected: 0x%1 | Scope: %2 | Calculated Offset: %3")
                                          .arg(QString::number(selectedAddr, 16).toUpper())
                                          .arg(scopeName)
                                          .arg(offsetDisplay));
    } else {
        m_offsetPreviewLabel->setText(
            QString("Selected: 0x%1 | Out of range of known structure scopes.").arg(QString::number(selectedAddr, 16).toUpper()));
    }
}

void StructureHierarchyWidget::insertNodeInScope(QTreeWidgetItem* parentScope, int32_t offset, const QString& description,
                                                 bool isArrayElement) {
    QTreeWidgetItem* hierParent = parentScope ? ((parentScope->treeWidget() == m_metadataTree) ? getTwinItem(parentScope) : parentScope) : nullptr;
    QTreeWidgetItem* metaParent = hierParent ? getTwinItem(hierParent) : nullptr;

    QTreeWidget* tree = m_structureTree;
    int childCount = hierParent ? hierParent->childCount() : tree->topLevelItemCount();

    int insertIndex = childCount;
    for (int i = 0; i < childCount; ++i) {
        QTreeWidgetItem* existing = hierParent ? hierParent->child(i) : tree->topLevelItem(i);
        int32_t existingOffset = existing->data(2, Qt::UserRole).isValid() ? existing->data(2, Qt::UserRole).toInt()
                                                                           : existing->data(1, Qt::UserRole).toInt();
        if (existingOffset > offset) {
            insertIndex = i;
            break;
        }
    }

    QString sizeStr;
    QString cleanDesc;
    parseSizeAndDescription(description, sizeStr, cleanDesc);

    auto* leftItem = new QTreeWidgetItem();
    leftItem->setText(0, cleanDesc);
    leftItem->setFlags(leftItem->flags() | Qt::ItemIsEditable);

    auto* rightItem = new QTreeWidgetItem();
    rightItem->setText(META_COL_SIZE, sizeStr);

    QString sign = (offset < 0) ? "-" : "";
    QString offsetHex = QString("%1").arg(std::abs(offset), 2, 16, QChar('0')).toUpper();

    bool parentIsArrayElem = hierParent && (hierParent->data(2, Qt::UserRole + 3).isValid()
                                                ? hierParent->data(2, Qt::UserRole + 3).toBool()
                                                : hierParent->data(1, Qt::UserRole + 3).toBool());

    // Standardized array offset display (|0x.. without redundant '+')
    QString displayOffset;
    if (isArrayElement) {
        displayOffset = "|" + sign + "0x" + offsetHex;
    } else if (parentIsArrayElem) {
        displayOffset = "|+" + sign + "0x" + offsetHex;
    } else {
        displayOffset = (offset < 0 ? "-0x" : "+0x") + offsetHex;
    }

    rightItem->setText(META_COL_OFFSET, displayOffset);

    setTwinItems(leftItem, rightItem);

    leftItem->setData(1, Qt::UserRole, offset);
    leftItem->setData(2, Qt::UserRole, offset);
    leftItem->setData(1, Qt::UserRole + 1, "="); // Default delimiter
    leftItem->setData(2, Qt::UserRole + 1, "=");
    leftItem->setData(1, Qt::UserRole + 3, isArrayElement);
    leftItem->setData(2, Qt::UserRole + 3, isArrayElement);

    rightItem->setData(1, Qt::UserRole, offset);
    rightItem->setData(2, Qt::UserRole, offset);
    rightItem->setData(1, Qt::UserRole + 1, "=");
    rightItem->setData(2, Qt::UserRole + 1, "=");
    rightItem->setData(1, Qt::UserRole + 3, isArrayElement);
    rightItem->setData(2, Qt::UserRole + 3, isArrayElement);

    if (hierParent) {
        hierParent->insertChild(insertIndex, leftItem);
        hierParent->setExpanded(true);
        if (metaParent) {
            metaParent->insertChild(insertIndex, rightItem);
            metaParent->setExpanded(true);
        }
    } else {
        tree->insertTopLevelItem(insertIndex, leftItem);
        m_metadataTree->insertTopLevelItem(insertIndex, rightItem);
    }

    tree->setCurrentItem(leftItem);
}

void StructureHierarchyWidget::updateNodeDescription(QTreeWidgetItem* item, const QString& description) {
    if (!item)
        return;

    QTreeWidgetItem* hierItem = (item->treeWidget() == m_metadataTree) ? getTwinItem(item) : item;
    QTreeWidgetItem* metaItem = (item->treeWidget() == m_metadataTree) ? item : getTwinItem(item);

    QString sizeStr;
    QString cleanDesc;
    parseSizeAndDescription(description, sizeStr, cleanDesc);

    if (hierItem) {
        hierItem->setText(0, cleanDesc);
    }
    if (metaItem) {
        metaItem->setText(META_COL_SIZE, sizeStr);
    }
}

void StructureHierarchyWidget::removeDocumentationAtAddress(uint32_t address, uint32_t currentBaseAddress,
                                                            const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
    if (!m_manager || address == 0)
        return;

    QTreeWidgetItem* targetItem = findItemForAddress(address, currentBaseAddress, snapshotMemory, rootArr);

    if (targetItem) {
        QTreeWidgetItem* meta = getTwinItem(targetItem);
        delete targetItem;
        if (meta) {
            delete meta;
        }
        emit treeDataChanged();
    }
}

QString StructureHierarchyWidget::generateCodeNoteFromTree(bool useUnicode, bool indentDesc, const QString& headerComment) const {
    std::function<std::vector<std::shared_ptr<CodeNoteNode>>(QTreeWidgetItem*)> treeToNodes = [&](QTreeWidgetItem* parent) {
        std::vector<std::shared_ptr<CodeNoteNode>> nodes;
        int count = parent ? parent->childCount() : m_structureTree->topLevelItemCount();

        for (int i = 0; i < count; ++i) {
            QTreeWidgetItem* item = parent ? parent->child(i) : m_structureTree->topLevelItem(i);
            int32_t offset = item->data(2, Qt::UserRole).isValid() ? item->data(2, Qt::UserRole).toInt() : item->data(1, Qt::UserRole).toInt();
            QString offsetHex = QString("%1").arg(std::abs(offset), 2, 16, QChar('0')).toLower();
            QString delimiter = item->data(2, Qt::UserRole + 1).isValid() ? item->data(2, Qt::UserRole + 1).toString()
                                                                           : item->data(1, Qt::UserRole + 1).toString();
            if (delimiter.isEmpty())
                delimiter = "=";

            bool isArrayElem = item->data(2, Qt::UserRole + 3).isValid() ? item->data(2, Qt::UserRole + 3).toBool()
                                                                         : item->data(1, Qt::UserRole + 3).toBool();
            bool parentIsArrayElem = parent && (parent->data(2, Qt::UserRole + 3).isValid()
                                                    ? parent->data(2, Qt::UserRole + 3).toBool()
                                                    : parent->data(1, Qt::UserRole + 3).toBool());

            QString offsetStr;
            if (isArrayElem) {
                offsetStr = "|" + (offset < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
            } else if (parentIsArrayElem) {
                offsetStr = "|+" + (offset < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
            } else {
                offsetStr = (offset < 0 ? QString("-") : QString("+")) + "0x" + offsetHex;
            }

            QTreeWidgetItem* meta = getTwinItem(item);
            QString sizeStr = meta ? meta->text(META_COL_SIZE).trimmed() : "";
            QString descStr = item->text(0).trimmed();

            // Seamlessly wrap in brackets for RetroAchievements QA/CR standards compliance
            QString sizeWithBrackets;
            if (!sizeStr.isEmpty()) {
                sizeWithBrackets = sizeStr.startsWith('[') ? sizeStr : ("[" + sizeStr + "]");
            }

            QString fullDesc;
            if (!sizeWithBrackets.isEmpty() && !descStr.isEmpty()) {
                fullDesc = sizeWithBrackets + " " + descStr;
            } else if (!sizeWithBrackets.isEmpty()) {
                fullDesc = sizeWithBrackets;
            } else {
                fullDesc = descStr;
            }

            auto node = std::make_shared<CodeNoteNode>();
            node->content = offsetStr + delimiter + fullDesc;

            QStringList valueNotes = item->data(0, Qt::UserRole + 2).toStringList();
            for (const auto& vNote : valueNotes) {
                auto noteChild = std::make_shared<CodeNoteNode>();
                noteChild->content = vNote;
                node->children.push_back(noteChild);
            }

            auto subChildren = treeToNodes(item);
            for (const auto& sub : subChildren) {
                node->children.push_back(sub);
            }

            nodes.push_back(node);
        }
        return nodes;
    };

    auto rootNodes = treeToNodes(nullptr);
    QString delimiter = DataModels::GlobalSettings::codeNoteSuffix;
    bool autoPopulate = DataModels::GlobalSettings::codeNoteAutoPopulatePointers;
    QString ptrTemplate = m_manager ? m_manager->getPlatformPointerTypeDescription() : "[32-bit] Pointer";
    QString indentPrefix = DataModels::GlobalSettings::codeNotePrefix;

    return CodeNoteHierarchyEngine::rebuildTextFromTree(rootNodes, 0, useUnicode, indentDesc, headerComment, {}, 0, delimiter, autoPopulate,
                                                        ptrTemplate, indentPrefix);
}

QString StructureHierarchyWidget::generateCodeNoteFromTree(const QString& style, bool indentDesc, const QString& headerComment) const {
    bool useUnicode = style.startsWith("Unicode Tree", Qt::CaseInsensitive);
    return generateCodeNoteFromTree(useUnicode, indentDesc, headerComment);
}

std::optional<PointerPath> StructureHierarchyWidget::getPathForTreeItem(QTreeWidgetItem* item, uint32_t currentBaseAddress,
                                                                        const std::vector<uint8_t>& snapshotMemory, bool rootArr) const {
    if (!item || currentBaseAddress == 0)
        return std::nullopt;

    QTreeWidgetItem* hierItem = (item->treeWidget() == m_metadataTree) ? getTwinItem(item) : item;
    if (!hierItem)
        return std::nullopt;

    std::vector<QTreeWidgetItem*> lineage;
    for (QTreeWidgetItem* curr = hierItem; curr != nullptr; curr = curr->parent()) {
        lineage.push_back(curr);
    }
    std::reverse(lineage.begin(), lineage.end());

    PointerPath path;

    // In static array structures, the top-level element offset represents a direct offset in static RAM,
    // not an additional indirect pointer chain level.
    if (rootArr && !lineage.empty()) {
        int32_t elemOffset = lineage[0]->data(2, Qt::UserRole).isValid() ? lineage[0]->data(2, Qt::UserRole).toInt()
                                                                         : lineage[0]->data(1, Qt::UserRole).toInt();
        path.baseAddress = currentBaseAddress + static_cast<uint32_t>(elemOffset);

        // Append offsets within the pointed instance
        for (size_t i = 1; i < lineage.size(); ++i) {
            path.offsets.push_back(lineage[i]->data(2, Qt::UserRole).isValid() ? lineage[i]->data(2, Qt::UserRole).toInt()
                                                                               : lineage[i]->data(1, Qt::UserRole).toInt());
        }

        // If the element itself is selected, target offset is 0
        if (path.offsets.empty()) {
            path.offsets.push_back(0);
        }
    } else {
        path.baseAddress = currentBaseAddress;
        for (auto* node : lineage) {
            path.offsets.push_back(node->data(2, Qt::UserRole).isValid() ? node->data(2, Qt::UserRole).toInt()
                                                                         : node->data(1, Qt::UserRole).toInt());
        }
    }

    path.finalAddress = const_cast<StructureHierarchyWidget*>(this)->resolveNodeAddress(hierItem, currentBaseAddress, snapshotMemory, rootArr);
    return path;
}

QString StructureHierarchyWidget::formatRetroAchievementsTrigger(QTreeWidgetItem* item, uint32_t currentBaseAddress, bool rootArr,
                                                                 const QString& headerComment) const {
    if (!item || currentBaseAddress == 0 || !m_manager)
        return QString();

    QTreeWidgetItem* hierItem = (item->treeWidget() == m_metadataTree) ? getTwinItem(item) : item;
    if (!hierItem)
        return QString();

    std::vector<QTreeWidgetItem*> lineage;
    for (QTreeWidgetItem* curr = hierItem; curr != nullptr; curr = curr->parent()) {
        lineage.push_back(curr);
    }
    std::reverse(lineage.begin(), lineage.end());

    QString sizePrefix = "X";
    QString defaultPointerMask = "";
    QString raPrefix = m_manager->getRetroAchievementsPrefix();
    QString emuName = m_manager->getEmulatorName();

    if (raPrefix == "W" || raPrefix == "D" || raPrefix == "O") {
        sizePrefix = "W";
    } else if (raPrefix == "G") {
        sizePrefix = "G";
        defaultPointerMask = "&536870911"; // 0x1FFFFFFF mask
    } else if (emuName.contains("PPSSPP", Qt::CaseInsensitive)) {
        sizePrefix = "X";
        defaultPointerMask = "&33554431"; // 0x01FFFFFF mask
    }

    bool isPs2 = (raPrefix == "X" && emuName.contains("PCSX2", Qt::CaseInsensitive));

    auto extractMaskFromText = [](const QString& text) -> QString {
        static const QRegularExpression maskRx(R"(\&\s*(?:0x([0-9a-fA-F]+)|(\d+)))");
        auto match = maskRx.match(text);
        if (match.hasMatch()) {
            if (!match.captured(1).isEmpty()) {
                bool ok = false;
                uint32_t val = match.captured(1).toUInt(&ok, 16);
                if (ok)
                    return QString("&%1").arg(val);
            } else if (!match.captured(2).isEmpty()) {
                return QString("&%1").arg(match.captured(2));
            }
        }
        return "";
    };

    bool rootIsPointer = false;
    if (!rootArr) {
        QString cleanHeader = headerComment.trimmed();
        rootIsPointer = cleanHeader.contains("Pointer", Qt::CaseInsensitive);
    }

    uint32_t currentStaticAddr = currentBaseAddress;
    bool isStatic = !rootIsPointer;

    struct StepInfo {
        bool isIndirect = true;
        uint32_t value = 0;
        QString mask = "";
    };
    std::vector<StepInfo> steps;

    if (rootIsPointer) {
        QString mask = extractMaskFromText(headerComment);
        if (mask.isEmpty()) {
            if (isPs2 && m_manager->isAttached()) {
                auto [normAddr, _] = m_manager->normalizeAddressForRead(currentBaseAddress);
                auto dwordOpt = m_manager->readUInt32(normAddr);
                if (dwordOpt.has_value() && (dwordOpt.value() >> 28) == 0x2) {
                    mask = "&536870911";
                }
            } else {
                mask = defaultPointerMask;
            }
        }
        steps.push_back({true, currentBaseAddress, mask});
    }

    uint32_t accumulatedOffset = 0;

    for (size_t i = 0; i < lineage.size(); ++i) {
        QTreeWidgetItem* node = lineage[i];
        QTreeWidgetItem* meta = getTwinItem(node);
        int32_t offset = node->data(2, Qt::UserRole).isValid() ? node->data(2, Qt::UserRole).toInt() : node->data(1, Qt::UserRole).toInt();
        bool isPtr = isPointerItem(node);
        bool isLast = (i == lineage.size() - 1);

        if (isStatic) {
            currentStaticAddr += static_cast<uint32_t>(offset);
            if (isPtr) {
                QString mask = extractMaskFromText(meta ? meta->text(META_COL_SIZE) : "");
                if (mask.isEmpty()) {
                    mask = extractMaskFromText(node->text(0));
                }
                if (mask.isEmpty()) {
                    if (isPs2 && m_manager->isAttached()) {
                        auto [normAddr, _] = m_manager->normalizeAddressForRead(currentStaticAddr);
                        auto dwordOpt = m_manager->readUInt32(normAddr);
                        if (dwordOpt.has_value() && (dwordOpt.value() >> 28) == 0x2) {
                            mask = "&536870911";
                        }
                    } else {
                        mask = defaultPointerMask;
                    }
                }
                steps.push_back({true, currentStaticAddr, mask});
                isStatic = false;
                accumulatedOffset = 0;
            }
        } else {
            accumulatedOffset += static_cast<uint32_t>(offset);
            if (isPtr && !isLast) {
                QString mask = extractMaskFromText(meta ? meta->text(META_COL_SIZE) : "");
                if (mask.isEmpty()) {
                    mask = extractMaskFromText(node->text(0));
                }
                if (mask.isEmpty()) {
                    if (isPs2 && m_manager->isAttached()) {
                        uint32_t nodeAddr =
                            const_cast<StructureHierarchyWidget*>(this)->resolveNodeAddress(node, currentBaseAddress, {}, rootArr);
                        if (nodeAddr != 0) {
                            auto [normAddr, _] = m_manager->normalizeAddressForRead(nodeAddr);
                            auto dwordOpt = m_manager->readUInt32(normAddr);
                            if (dwordOpt.has_value() && (dwordOpt.value() >> 28) == 0x2) {
                                mask = "&536870911";
                            }
                        }
                    } else {
                        mask = defaultPointerMask;
                    }
                }
                steps.push_back({true, accumulatedOffset, mask});
                accumulatedOffset = 0;
            }
        }
    }

    // Case 1: Pure static memory variable with no pointer dereferencing involved
    if (isStatic && steps.empty()) {
        QString norm = m_manager->formatDisplayAddress(currentStaticAddr);
        return QString("I:0x%1%2").arg(sizePrefix, norm.rightJustified(8, '0').toLower());
    }

    // Case 2 & 3: Pointer chain with one or more indirect hops
    QString sb;
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        QString addrStr = (i == 0) ? m_manager->formatDisplayAddress(step.value).rightJustified(8, '0').toLower()
                                   : QString("%1").arg(step.value, 8, 16, QChar('0')).toLower();

        if (i == 0) {
            sb.append(QString("I:0x%1%2%3").arg(sizePrefix, addrStr, step.mask));
        } else {
            sb.append(QString("_I:0x%1%2%3").arg(sizePrefix, addrStr, step.mask));
        }
    }

    if (accumulatedOffset != 0 || lineage.back()->childCount() == 0) {
        QString offStr = QString("%1").arg(accumulatedOffset, 8, 16, QChar('0')).toLower();
        sb.append(QString("_0x%1%2=1").arg(sizePrefix, offStr));
    }

    return sb;
}

// Double-clicking a tree item: Navigates to target address in Memory Viewer
void StructureHierarchyWidget::onTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (!item)
        return;

    // Double-clicking follows the pointer / navigates to entry's target in Memory Viewer
    emit followPointerClicked();
}

void StructureHierarchyWidget::openSizeDropdown(QTreeWidgetItem* item, const QPoint& globalPos) {
    if (!item)
        return;

    QTreeWidgetItem* metaItem = (item->treeWidget() == m_metadataTree) ? item : getTwinItem(item);
    if (!metaItem)
        return;

    QString currentSize = metaItem->text(META_COL_SIZE).trimmed();

    QMenu menu(this);
    menu.setStyleSheet("QMenu { font-family: 'Consolas', 'Segoe UI', monospace; font-size: 11px; }");

    for (const auto& type : StandardRATypes) {
        QAction* act = menu.addAction(type);
        act->setCheckable(true);
        if (type.compare(currentSize, Qt::CaseInsensitive) == 0) {
            act->setChecked(true);
        }

        connect(act, &QAction::triggered, this, [this, type]() {
            // Apply selected size across selection, safely skipping pointer nodes
            QList<QTreeWidgetItem*> selected = m_structureTree->selectedItems();
            if (selected.isEmpty()) {
                if (auto* cur = m_structureTree->currentItem()) {
                    selected.append(cur);
                }
            }

            int updatedCount = 0;
            for (auto* itm : selected) {
                if (!isPointerItem(itm)) {
                    applySizeToItem(itm, type);
                    updatedCount++;
                }
            }

            if (updatedCount > 0) {
                emit treeDataChanged();
            }
        });
    }

    menu.exec(globalPos);
}

void StructureHierarchyWidget::applySizeToItem(QTreeWidgetItem* item, const QString& newSize) {
    if (!item)
        return;

    QTreeWidgetItem* metaItem = (item->treeWidget() == m_metadataTree) ? item : getTwinItem(item);

    if (metaItem) {
        metaItem->setText(META_COL_SIZE, newSize);
    }
}

void StructureHierarchyWidget::onMetadataItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (!item)
        return;

    QTreeWidgetItem* hierItem = getTwinItem(item);
    if (!hierItem)
        hierItem = item;

    if (column == META_COL_SIZE) {
        if (isPointerItem(hierItem)) {
            QString emuName = m_manager ? m_manager->getEmulatorName() : "Emulator";
            QString ptrDesc = m_manager ? m_manager->getPlatformPointerTypeDescription() : "[32-bit] Pointer";
            emit statusMessageRequested(QString("Pointer size is locked to active architecture (%1: %2)").arg(emuName, ptrDesc));
            return;
        }

        QRect rect = m_metadataTree->visualItemRect(item);
        QPoint globalPos = m_metadataTree->viewport()->mapToGlobal(QPoint(rect.left(), rect.bottom()));
        openSizeDropdown(hierItem, globalPos);
    } else {
        // Double-clicking Offset, Resolved RAM, or Pointer Val navigates in Memory Viewer
        emit followPointerClicked();
    }
}

void StructureHierarchyWidget::onMetadataCustomContextMenu(const QPoint& pos) {
    QTreeWidgetItem* clickedItem = m_metadataTree->itemAt(pos);
    if (!clickedItem)
        return;

    QTreeWidgetItem* hierItem = getTwinItem(clickedItem);
    if (!hierItem)
        hierItem = clickedItem;

    if (!m_structureTree->selectedItems().contains(hierItem)) {
        m_structureTree->setCurrentItem(hierItem);
    }

    onTreeCustomContextMenu(m_structureTree->viewport()->mapFromGlobal(m_metadataTree->viewport()->mapToGlobal(pos)));
}

void StructureHierarchyWidget::onTreeCustomContextMenu(const QPoint& pos) {
    auto* clickedItem = m_structureTree->itemAt(pos);
    if (!clickedItem)
        return;

    // If clicked item is not part of current multi-selection, select it exclusively
    if (!m_structureTree->selectedItems().contains(clickedItem)) {
        m_structureTree->setCurrentItem(clickedItem);
    }

    QMenu menu(this);

    // Standardized clipboard copy choices matching main results view
    auto* copyRaAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this,
                                     &StructureHierarchyWidget::copyRaRequested);
    copyRaAct->setProperty("iconPath", ":/icons/copy_ra.svg");
    copyRaAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));

    auto* copyNoteAct =
        menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Copy Code Note", this, &StructureHierarchyWidget::copyNoteRequested);
    copyNoteAct->setProperty("iconPath", ":/icons/copy_note.svg");

    auto* copyAddrAct =
        menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Address", this, &StructureHierarchyWidget::copyAddressRequested);
    copyAddrAct->setProperty("iconPath", ":/icons/copy.svg");
    copyAddrAct->setShortcut(QKeySequence::Copy);

    auto* copyValAct =
        menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Value", this, &StructureHierarchyWidget::copyValueRequested);
    copyValAct->setProperty("iconPath", ":/icons/copy_ra.svg");
    copyValAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));

    menu.addSeparator();

    // Context menu option to add or edit value descriptions for non-pointer variables
    if (!isPointerItem(clickedItem)) {
        auto* descAct = menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Value Descriptions... (D)", this,
                                       &StructureHierarchyWidget::manageDescriptionsClicked);
        descAct->setProperty("iconPath", ":/icons/copy_note.svg");
        menu.addSeparator();
    }

    QTreeWidgetItem* meta = getTwinItem(clickedItem);
    bool hasPointerVal = meta && !meta->text(META_COL_VALUE).trimmed().isEmpty() &&
                         meta->text(META_COL_VALUE) != "N/A" && meta->text(META_COL_VALUE) != "0x00000000";

    if (hasPointerVal) {
        auto* followAct = menu.addAction(getThemeIcon(":/icons/follow_pointer.svg"), "Follow Pointer", this,
                                         &StructureHierarchyWidget::followPointerClicked);
        followAct->setProperty("iconPath", ":/icons/follow_pointer.svg");
        menu.addSeparator();
    }

    auto* removeAct = menu.addAction(getThemeIcon(":/icons/delete.svg"), "Remove", this, &StructureHierarchyWidget::removeNodeClicked);
    removeAct->setProperty("iconPath", ":/icons/delete.svg");

    menu.exec(m_structureTree->viewport()->mapToGlobal(pos));
}

void StructureHierarchyWidget::onTreeItemChanged(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item);
    if (m_structureTree->signalsBlocked())
        return;

    if (column == 0) {
        emit treeDataChanged();
    }
}

} // namespace PointerFinder2::UI
