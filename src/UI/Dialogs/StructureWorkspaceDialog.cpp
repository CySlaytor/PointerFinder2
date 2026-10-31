#include "StructureWorkspaceDialog.h"

#include "../../Common/EndianUtils.h"
#include "../../Core/CodeNoteHelper.h"
#include "../../Models/CodeNoteSettings.h"
#include "../../Models/GlobalSettings.h"
#include "../Styles/ThemeManager.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <unordered_map>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::DataModels;

    StructureWorkspaceDialog::StructureWorkspaceDialog(Emulators::IEmulatorManager* manager, QWidget* parent)
        : QDialog(parent), m_manager(manager) {
        setWindowTitle("Pointer Finder 2.0 - Dissect Structure");
        resize(1300, 750);
        setMinimumSize(920, 520);

        setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

        setupLayout();
        loadWorkspaceSettings();

        if (m_manager && m_manager->isAttached()) {
            refreshMemorySnapshot();
        }
        else {
            m_ramStatusLabel->setText("Emulator not attached.");
            m_refreshRamBtn->setEnabled(false);
        }
    }

    StructureWorkspaceDialog::~StructureWorkspaceDialog() {
        saveWorkspaceSettings();
    }

    QIcon StructureWorkspaceDialog::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void StructureWorkspaceDialog::setupLayout() {
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(8, 8, 8, 8);
        rootLayout->setSpacing(6);

        // Top Toolbar
        auto* topToolbar = new QHBoxLayout();
        topToolbar->addWidget(new QLabel("Active Structure:", this));

        m_structureSelectCombo = new QComboBox(this);
        m_structureSelectCombo->setMinimumWidth(200);
        topToolbar->addWidget(m_structureSelectCombo);

        m_newStructureBtn = new QPushButton("New Structure...", this);
        m_deleteStructureBtn = new QPushButton("Delete", this);
        topToolbar->addWidget(m_newStructureBtn);
        topToolbar->addWidget(m_deleteStructureBtn);
        topToolbar->addSpacing(20);

        m_toggleCodeNoteBtn = new QPushButton("Code Note Panel", this);
        m_toggleCodeNoteBtn->setCheckable(true);
        m_toggleCodeNoteBtn->setChecked(true);
        m_toggleCodeNoteBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
        topToolbar->addWidget(m_toggleCodeNoteBtn);

        topToolbar->addStretch();
        rootLayout->addLayout(topToolbar);

        m_mainSplitter = new QSplitter(Qt::Horizontal, this);

        // --- Left Panel: Structure Editor ---
        m_leftContainer = new QWidget(m_mainSplitter);
        auto* leftLayout = new QVBoxLayout(m_leftContainer);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(6);

        auto* baseAddrRow = new QHBoxLayout();
        baseAddrRow->addWidget(new QLabel("Structure Base (Hex):", m_leftContainer));
        m_baseAddressEdit = new QLineEdit(m_leftContainer);
        m_baseAddressEdit->setPlaceholderText("e.g. 004934dc");
        baseAddrRow->addWidget(m_baseAddressEdit);
        leftLayout->addLayout(baseAddrRow);

        m_structureTree = new QTreeWidget(m_leftContainer);
        m_structureTree->setColumnCount(4);
        m_structureTree->setHeaderLabels({ "Description / Name", "Offset", "Resolved RAM", "Pointer Val" });
        m_structureTree->setContextMenuPolicy(Qt::CustomContextMenu);

        m_structureTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
        m_structureTree->header()->setSectionResizeMode(1, QHeaderView::Interactive);
        m_structureTree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
        m_structureTree->header()->setSectionResizeMode(3, QHeaderView::Interactive);
        m_structureTree->header()->setStretchLastSection(true);

        m_structureTree->setColumnWidth(0, 260);
        m_structureTree->setColumnWidth(1, 80);
        m_structureTree->setColumnWidth(2, 110);
        m_structureTree->setColumnWidth(3, 110);

        m_structureTree->setAlternatingRowColors(true);
        m_structureTree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        leftLayout->addWidget(m_structureTree, 1);

        m_offsetPreviewLabel = new QLabel("Select an address in Memory Viewer to calculate relative offset.", m_leftContainer);
        m_offsetPreviewLabel->setStyleSheet("font-weight: bold; color: #58A6FF; padding: 4px; border: 1px solid #30363D; border-radius: 4px;");
        leftLayout->addWidget(m_offsetPreviewLabel);

        auto* treeButtonsRow = new QHBoxLayout();
        m_addVariableBtn = new QPushButton("Add Variable (Enter)", m_leftContainer);
        m_addVariableBtn->setStyleSheet("font-weight: bold;");
        m_addPointerBtn = new QPushButton("Add as Pointer", m_leftContainer);
        m_followPointerBtn = new QPushButton("Follow Pointer ->", m_leftContainer);
        m_followPointerBtn->setStyleSheet("color: #FF7B72; font-weight: bold;");
        m_removeNodeBtn = new QPushButton("Remove", m_leftContainer);
        m_removeNodeBtn->setIcon(getThemeIcon(":/icons/delete.svg"));

        treeButtonsRow->addWidget(m_addVariableBtn, 2);
        treeButtonsRow->addWidget(m_addPointerBtn, 1);
        treeButtonsRow->addWidget(m_followPointerBtn, 1);
        treeButtonsRow->addWidget(m_removeNodeBtn, 1);
        leftLayout->addLayout(treeButtonsRow);

        m_mainSplitter->addWidget(m_leftContainer);

        // --- Right Panel: Memory Viewer ---
        m_middleContainer = new QWidget(m_mainSplitter);
        auto* middleLayout = new QVBoxLayout(m_middleContainer);
        middleLayout->setContentsMargins(0, 0, 0, 0);
        middleLayout->setSpacing(6);

        auto* viewerHeaderRow = new QHBoxLayout();
        m_refreshRamBtn = new QPushButton("Re-dump / Refresh RAM", m_middleContainer);
        m_refreshRamBtn->setStyleSheet("font-weight: bold;");
        viewerHeaderRow->addWidget(m_refreshRamBtn);

        m_ramStatusLabel = new QLabel("No dump loaded.", m_middleContainer);
        viewerHeaderRow->addWidget(m_ramStatusLabel);
        viewerHeaderRow->addStretch();

        viewerHeaderRow->addWidget(new QLabel("Go to (Hex):", m_middleContainer));
        m_goToAddressEdit = new QLineEdit(m_middleContainer);
        m_goToAddressEdit->setMaximumWidth(100);
        m_goToAddressBtn = new QPushButton("Go", m_middleContainer);
        viewerHeaderRow->addWidget(m_goToAddressEdit);
        viewerHeaderRow->addWidget(m_goToAddressBtn);

        middleLayout->addLayout(viewerHeaderRow);

        // Memory View Mode Selector
        auto* viewModeRow = new QHBoxLayout();
        viewModeRow->addWidget(new QLabel("Memory View:", m_middleContainer));

        m_view8BitRadio = new QRadioButton("8-bit", m_middleContainer);
        m_view16BitRadio = new QRadioButton("16-bit", m_middleContainer);
        m_view32BitRadio = new QRadioButton("32-bit", m_middleContainer);
        m_view32BitBERadio = new QRadioButton("32-bit BE", m_middleContainer);
        m_view32BitRadio->setChecked(true);

        m_viewModeGroup = new QButtonGroup(this);
        m_viewModeGroup->addButton(m_view8BitRadio, 0);
        m_viewModeGroup->addButton(m_view16BitRadio, 1);
        m_viewModeGroup->addButton(m_view32BitRadio, 2);
        m_viewModeGroup->addButton(m_view32BitBERadio, 3);

        viewModeRow->addWidget(m_view8BitRadio);
        viewModeRow->addWidget(m_view16BitRadio);
        viewModeRow->addWidget(m_view32BitRadio);
        viewModeRow->addWidget(m_view32BitBERadio);
        viewModeRow->addStretch();
        middleLayout->addLayout(viewModeRow);

        m_memoryViewer = new Widgets::MemoryViewerWidget(m_middleContainer);
        middleLayout->addWidget(m_memoryViewer, 1);

        m_mainSplitter->addWidget(m_middleContainer);

        // --- Panel 3: Code Note Hierarchy Fixer ---
        m_rightContainer = new QWidget(m_mainSplitter);
        auto* rightLayout = new QVBoxLayout(m_rightContainer);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(6);

        auto* fixerHeaderRow = new QHBoxLayout();
        fixerHeaderRow->addWidget(new QLabel("Style:", m_rightContainer));

        m_styleCombo = new QComboBox(m_rightContainer);
        m_styleCombo->addItems({
            "Unicode Tree (├─)",
            "Dots (.)",
            "Plusses (+)"
        });
        fixerHeaderRow->addWidget(m_styleCombo);

        m_indentDescCheck = new QCheckBox("Indent Descriptions", m_rightContainer);
        m_indentDescCheck->setChecked(true);
        fixerHeaderRow->addWidget(m_indentDescCheck);

        m_fixHierarchyBtn = new QPushButton("Fix Hierarchy", m_rightContainer);
        m_fixHierarchyBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
        QFont boldFont = m_fixHierarchyBtn->font();
        boldFont.setBold(true);
        m_fixHierarchyBtn->setFont(boldFont);
        fixerHeaderRow->addWidget(m_fixHierarchyBtn);

        fixerHeaderRow->addStretch();

        m_copyCodeNoteBtn = new QPushButton("Copy", m_rightContainer);
        m_copyCodeNoteBtn->setIcon(getThemeIcon(":/icons/copy.svg"));
        fixerHeaderRow->addWidget(m_copyCodeNoteBtn);

        rightLayout->addLayout(fixerHeaderRow);

        m_codeNoteTextEdit = new QPlainTextEdit(m_rightContainer);
        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        m_codeNoteTextEdit->setFont(monoFont);
        m_codeNoteTextEdit->setPlaceholderText("Code Note displays here. You can paste raw pointer chains or edit lines directly, then click 'Fix Hierarchy' to merge...");
        rightLayout->addWidget(m_codeNoteTextEdit, 1);

        m_mainSplitter->addWidget(m_rightContainer);

        m_mainSplitter->setCollapsible(0, false);
        m_mainSplitter->setCollapsible(1, false);
        m_mainSplitter->setCollapsible(2, true);

        m_mainSplitter->setStretchFactor(0, 3);
        m_mainSplitter->setStretchFactor(1, 4);
        m_mainSplitter->setStretchFactor(2, 3);
        rootLayout->addWidget(m_mainSplitter, 1);

        // Connections
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addressSelected, this, &StructureWorkspaceDialog::onAddressSelectedInViewer);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addressDoubleClicked, this, &StructureWorkspaceDialog::onAddressDoubleClickedInViewer);
        connect(m_structureTree, &QTreeWidget::itemDoubleClicked, this, &StructureWorkspaceDialog::onStructureTreeItemDoubleClicked);
        connect(m_structureTree, &QTreeWidget::customContextMenuRequested, this, &StructureWorkspaceDialog::onStructureTreeCustomContextMenu);

        connect(m_addVariableBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onAddVariableClicked);
        connect(m_addPointerBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onAddPointerClicked);
        connect(m_followPointerBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onFollowPointerClicked);
        connect(m_removeNodeBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onRemoveNodeClicked);

        connect(m_structureTree, &QTreeWidget::itemChanged, this, &StructureWorkspaceDialog::onStructureTreeItemChanged);
        connect(m_structureTree, &QTreeWidget::itemSelectionChanged, this, &StructureWorkspaceDialog::onStructureTreeSelectionChanged);
        connect(m_refreshRamBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onRefreshRamClicked);
        connect(m_goToAddressBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onGoToAddressClicked);
        connect(m_goToAddressEdit, &QLineEdit::returnPressed, this, &StructureWorkspaceDialog::onGoToAddressClicked);

        connect(m_baseAddressEdit, &QLineEdit::textChanged, this, [this]() {
            recalculateTreeAddresses();
            syncTreeToCodeNoteText();
        });

        connect(m_viewModeGroup, &QButtonGroup::idClicked, this, &StructureWorkspaceDialog::onViewModeChanged);

        connect(m_newStructureBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onNewStructureClicked);
        connect(m_deleteStructureBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onDeleteStructureClicked);
        connect(m_structureSelectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StructureWorkspaceDialog::onActiveStructureComboChanged);

        // Code note sync & panel connections
        connect(m_toggleCodeNoteBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onToggleCodeNotePanelClicked);
        connect(m_mainSplitter, &QSplitter::splitterMoved, this, &StructureWorkspaceDialog::onSplitterMoved);
        connect(m_styleCombo, &QComboBox::currentTextChanged, this, &StructureWorkspaceDialog::onHierarchyStyleChanged);
        connect(m_indentDescCheck, &QCheckBox::checkStateChanged, this, &StructureWorkspaceDialog::onHierarchyStyleChanged);
        connect(m_fixHierarchyBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onFixHierarchyClicked);
        connect(m_copyCodeNoteBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onCopyCodeNoteClicked);
    }

    void StructureWorkspaceDialog::onViewModeChanged() {
        int id = m_viewModeGroup->checkedId();
        switch (id) {
        case 0: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode8Bit); break;
        case 1: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode16Bit); break;
        case 2: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode32Bit); break;
        case 3: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode32BitBE); break;
        default: break;
        }
        saveWorkspaceSettings();
    }

    void StructureWorkspaceDialog::refreshMemorySnapshot() {
        if (!m_manager || !m_manager->isAttached()) return;

        m_ramStatusLabel->setText("Reading memory dump...");
        QApplication::processEvents();

        uint32_t start = m_manager->getMainMemoryStart();
        uint32_t size = m_manager->getMainMemorySize();
        auto dump = m_manager->readMemory(start, size);

        if (!dump.empty()) {
            bool isBE = (m_manager->getRetroAchievementsPrefix() == "G");
            m_snapshotMemory = dump;
            m_memoryViewer->setMemoryData(std::move(dump), start, isBE);
            m_ramStatusLabel->setText(QString("Snapshot active (%1 KB)").arg(size / 1024));
            recalculateTreeAddresses();
        }
        else {
            m_ramStatusLabel->setText("Failed to read memory from emulator.");
        }
    }

    void StructureWorkspaceDialog::onRefreshRamClicked() {
        refreshMemorySnapshot();
    }

    void StructureWorkspaceDialog::navigateToAddress(uint32_t address) {
        if (!m_memoryViewer->hasData() || address == 0) return;

        uint32_t nativeAddr = address;
        if (m_manager) nativeAddr = m_manager->unnormalizeAddress(QString::number(address, 16));

        if (nativeAddr >= m_memoryViewer->getMemoryBase() && 
            nativeAddr < m_memoryViewer->getMemoryBase() + m_memoryViewer->getMemorySize()) {

            m_memoryViewer->setSelectedAddress(nativeAddr);
        }
    }

    void StructureWorkspaceDialog::onGoToAddressClicked() {
        QString text = m_goToAddressEdit->text().trimmed();
        if (text.startsWith("0x", Qt::CaseInsensitive)) text = text.mid(2);

        bool ok = false;
        text.toUInt(&ok, 16);
        if (ok && m_manager) {
            uint32_t nativeAddr = m_manager->unnormalizeAddress(text);
            navigateToAddress(nativeAddr);
        }
    }

    void StructureWorkspaceDialog::onAddressSelectedInViewer(uint32_t address) {
        m_currentViewerSelectedAddress = address;
        updateOffsetPreview();
    }

    // Double-clicking in Memory Viewer: If DWORD is a pointer, traverse it; otherwise prompt to add variable
    void StructureWorkspaceDialog::onAddressDoubleClickedInViewer(uint32_t address) {
        m_currentViewerSelectedAddress = address;
        uint32_t dwordVal = m_memoryViewer->readDwordAt(address);

        if (dwordVal != 0 && m_manager && m_manager->isValidPointerTarget(dwordVal)) {
            uint32_t nativeTarget = m_manager->unnormalizeAddress(QString::number(dwordVal, 16));
            navigateToAddress(nativeTarget);
        }
        else {
            onAddVariableClicked();
        }
    }

    // Double-clicking a tree item: Column 0 edits; Columns 1-3 follow pointer to destination
    void StructureWorkspaceDialog::onStructureTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {
        if (!item) return;

        if (column == 0) {
            m_structureTree->editItem(item, 0);
        }
        else {
            onFollowPointerClicked();
        }
    }

    void StructureWorkspaceDialog::onStructureTreeCustomContextMenu(const QPoint& pos) {
        auto* item = m_structureTree->itemAt(pos);
        if (!item) return;

        QMenu menu(this);

        auto* copyResolvedAct = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Resolved RAM Address", this, [this, item]() {
            QApplication::clipboard()->setText(item->text(2));
        });
        copyResolvedAct->setProperty("iconPath", ":/icons/copy.svg");

        auto* copyValAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Pointer Value", this, [this, item]() {
            QApplication::clipboard()->setText(item->text(3));
        });
        copyValAct->setProperty("iconPath", ":/icons/copy_ra.svg");

        menu.addSeparator();
        auto* removeAct = menu.addAction(getThemeIcon(":/icons/delete.svg"), "Remove", this, &StructureWorkspaceDialog::onRemoveNodeClicked);
        removeAct->setProperty("iconPath", ":/icons/delete.svg");

        menu.exec(m_structureTree->viewport()->mapToGlobal(pos));
    }

    // Traverses the pointer directly to its pointed destination, establishing the new structure scope
    void StructureWorkspaceDialog::onFollowPointerClicked() {
        auto* current = m_structureTree->currentItem();
        if (!current) return;

        uint32_t derefTarget = current->data(0, Qt::UserRole + 4).toUInt();
        if (derefTarget != 0) {
            navigateToAddress(derefTarget);
            return;
        }

        // Check if node itself resolved to a pointer value
        uint32_t resolved = resolveNodeAddress(current);
        if (resolved != 0) {
            auto valOpt = readMemoryDword(resolved);
            if (valOpt.has_value() && m_manager && m_manager->isValidPointerTarget(valOpt.value())) {
                uint32_t target = m_manager->unnormalizeAddress(QString::number(valOpt.value(), 16));
                navigateToAddress(target);
            }
        }
    }

    void StructureWorkspaceDialog::onStructureTreeSelectionChanged() {
        updateOffsetPreview();
        updateMemoryHighlights();

        auto* current = m_structureTree->currentItem();
        if (current && m_memoryViewer->hasData()) {
            uint32_t resolved = resolveNodeAddress(current);
            if (resolved != 0) {
                navigateToAddress(resolved);
            }
        }
    }

    std::optional<uint32_t> StructureWorkspaceDialog::readMemoryDword(uint32_t address) {
        if (!m_manager) return std::nullopt;

        auto [normAddr, wasNorm] = m_manager->normalizeAddressForRead(address);

        if (!m_snapshotMemory.empty()) {
            int64_t indexInDump = m_manager->getIndexForStateDump(normAddr);
            if (indexInDump >= 0 && indexInDump + 3 < static_cast<int64_t>(m_snapshotMemory.size())) {
                uint32_t val = 0;
                std::memcpy(&val, &m_snapshotMemory[indexInDump], 4);
                if (m_manager->getRetroAchievementsPrefix() == "G") {
                    val = Core::swapEndian32(val);
                }
                return val;
            }
        }

        return m_manager->readUInt32(normAddr);
    }

    uint32_t StructureWorkspaceDialog::resolveNodeAddress(QTreeWidgetItem* item) {
        if (!item || !m_manager) return 0;

        QString baseText = m_baseAddressEdit->text().trimmed();
        uint32_t base = m_manager->unnormalizeAddress(baseText);
        if (base == 0) return 0;

        uint32_t currentAddress = base;
        auto basePtr = readMemoryDword(base);
        if (basePtr.has_value() && m_manager->isValidPointerTarget(basePtr.value())) {
            currentAddress = m_manager->unnormalizeAddress(QString::number(basePtr.value(), 16));
        }

        std::vector<QTreeWidgetItem*> lineage;
        QTreeWidgetItem* curr = item;
        while (curr) {
            lineage.push_back(curr);
            curr = curr->parent();
        }
        std::reverse(lineage.begin(), lineage.end());

        for (size_t i = 0; i < lineage.size(); ++i) {
            auto* node = lineage[i];
            int32_t offset = node->data(1, Qt::UserRole).toInt();
            currentAddress += static_cast<uint32_t>(offset);

            if (node != item && (node->childCount() > 0 || node->text(0).contains("Pointer", Qt::CaseInsensitive))) {
                auto ptrVal = readMemoryDword(currentAddress);
                if (ptrVal.has_value() && m_manager->isValidPointerTarget(ptrVal.value())) {
                    currentAddress = m_manager->unnormalizeAddress(QString::number(ptrVal.value(), 16));
                }
                else {
                    return 0;
                }
            }
        }
        return currentAddress;
    }

    void StructureWorkspaceDialog::recalculateTreeAddresses() {
        if (!m_memoryViewer->hasData() || !m_manager) return;

        QString baseText = m_baseAddressEdit->text().trimmed();
        uint32_t base = m_manager->unnormalizeAddress(baseText);

        uint32_t initialParentBase = base;
        bool initialValid = (base != 0);

        auto basePtr = readMemoryDword(base);
        if (basePtr.has_value() && m_manager->isValidPointerTarget(basePtr.value())) {
            initialParentBase = m_manager->unnormalizeAddress(QString::number(basePtr.value(), 16));
        }

        std::function<void(QTreeWidgetItem*, uint32_t, bool)> updateNode = [&](QTreeWidgetItem* node, uint32_t parentBase, bool parentValid) {
            if (!parentValid || parentBase == 0) {
                node->setText(2, "N/A");
                node->setText(3, "N/A");
                node->setForeground(2, QColor(140, 140, 140));
                node->setForeground(3, QColor(140, 140, 140));

                for (int i = 0; i < node->childCount(); ++i) {
                    updateNode(node->child(i), 0, false);
                }
                return;
            }

            int32_t offset = node->data(1, Qt::UserRole).toInt();
            uint32_t resolved = parentBase + static_cast<uint32_t>(offset);

            QString displayAddr = m_manager->formatDisplayAddress(resolved);
            node->setText(2, "0x" + displayAddr);
            node->setForeground(2, palette().color(QPalette::Text));

            auto valOpt = readMemoryDword(resolved);
            uint32_t nextBase = 0;
            bool nextValid = false;

            if (valOpt.has_value()) {
                uint32_t rawVal = valOpt.value();
                QString valHex = QString("%1").arg(rawVal, 8, 16, QChar('0')).toUpper();
                node->setText(3, "0x" + valHex);
                node->setForeground(3, palette().color(QPalette::Text));

                if (node->childCount() > 0 || node->text(0).contains("Pointer", Qt::CaseInsensitive)) {
                    if (rawVal != 0 && m_manager->isValidPointerTarget(rawVal)) {
                        nextBase = m_manager->unnormalizeAddress(QString::number(rawVal, 16));
                        nextValid = true;
                        node->setData(0, Qt::UserRole + 4, nextBase);
                    }
                    else {
                        nextValid = false;
                        node->setData(0, Qt::UserRole + 4, 0);
                    }
                }
            }
            else {
                node->setText(3, "N/A");
                node->setForeground(3, QColor(140, 140, 140));
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

        updateMemoryHighlights();
    }

    void StructureWorkspaceDialog::updateMemoryHighlights() {
        if (!m_manager) return;

        std::unordered_map<uint32_t, Widgets::MemoryViewerWidget::HighlightInfo> highlights;
        QTreeWidgetItem* activeItem = m_structureTree->currentItem();

        std::function<void(QTreeWidgetItem*)> gatherHighlights = [&](QTreeWidgetItem* item) {
            uint32_t resolved = resolveNodeAddress(item);
            if (resolved != 0) {
                uint32_t nativeAddr = m_manager->unnormalizeAddress(QString::number(resolved, 16));
                Widgets::MemoryViewerWidget::HighlightInfo info;
                info.label = item->text(0) + " (" + item->text(1) + ")";
                info.isPointer = (item->childCount() > 0 || item->text(0).contains("Pointer", Qt::CaseInsensitive));
                info.isChildOfActive = (activeItem && (item == activeItem || item->parent() == activeItem));
                info.size = 4;
                highlights[nativeAddr] = info;
            }

            for (int i = 0; i < item->childCount(); ++i) {
                gatherHighlights(item->child(i));
            }
        };

        for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
            gatherHighlights(m_structureTree->topLevelItem(i));
        }

        m_memoryViewer->setHighlightedAddresses(highlights);
    }

    StructureWorkspaceDialog::ScopeMatch StructureWorkspaceDialog::findBestScopeForAddress(uint32_t selectedAddr) {
        ScopeMatch best;
        if (!m_manager || selectedAddr == 0) return best;

        QString baseText = m_baseAddressEdit->text().trimmed();
        uint32_t base = m_manager->unnormalizeAddress(baseText);
        if (base == 0) return best;

        uint32_t initialParentBase = base;
        auto basePtr = readMemoryDword(base);
        if (basePtr.has_value() && m_manager->isValidPointerTarget(basePtr.value())) {
            initialParentBase = m_manager->unnormalizeAddress(QString::number(basePtr.value(), 16));
        }

        struct CandidateScope {
            QTreeWidgetItem* node;
            uint32_t targetBase;
            int depth;
        };

        std::vector<CandidateScope> scopes;
        scopes.push_back({ nullptr, initialParentBase, 0 });

        std::function<void(QTreeWidgetItem*, int)> collectScopes = [&](QTreeWidgetItem* item, int depth) {
            uint32_t derefBase = item->data(0, Qt::UserRole + 4).toUInt();
            if (derefBase != 0) {
                scopes.push_back({ item, derefBase, depth });
            }

            for (int i = 0; i < item->childCount(); ++i) {
                collectScopes(item->child(i), depth + 1);
            }
        };

        for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
            collectScopes(m_structureTree->topLevelItem(i), 1);
        }

        constexpr uint32_t MAX_STRUCT_SPAN = 0x10000; // 64 KB struct span
        int maxDepthMatched = -1;
        uint32_t smallestOffset = 0xFFFFFFFF;

        for (const auto& scope : scopes) {
            if (selectedAddr >= scope.targetBase) {
                uint32_t delta = selectedAddr - scope.targetBase;
                if (delta < MAX_STRUCT_SPAN) {
                    if (scope.depth > maxDepthMatched || (scope.depth == maxDepthMatched && delta < smallestOffset)) {
                        maxDepthMatched = scope.depth;
                        smallestOffset = delta;
                        best.pointerNode = scope.node;
                        best.baseAddress = scope.targetBase;
                        best.offset = static_cast<int32_t>(delta);
                        best.depth = scope.depth;
                        best.isValid = true;
                    }
                }
            }
        }

        return best;
    }

    void StructureWorkspaceDialog::updateOffsetPreview() {
        if (m_currentViewerSelectedAddress == 0) {
            m_offsetPreviewLabel->setText("Select an address in Memory Viewer to calculate relative offset.");
            return;
        }

        auto match = findBestScopeForAddress(m_currentViewerSelectedAddress);
        if (match.isValid) {
            QString scopeName = match.pointerNode ? match.pointerNode->text(0) + " (" + match.pointerNode->text(1) + ")" : "Root Base Pointer";
            QString sign = (match.offset < 0) ? "-" : "+";
            QString offsetHex = QString::number(std::abs(match.offset), 16).toUpper();

            m_offsetPreviewLabel->setText(QString("Selected: 0x%1 | Scope: %2 | Calculated Offset: %30x%4")
                .arg(QString::number(m_currentViewerSelectedAddress, 16).toUpper())
                .arg(scopeName)
                .arg(sign).arg(offsetHex));
        }
        else {
            m_offsetPreviewLabel->setText(QString("Selected: 0x%1 | Out of range of known structure scopes.")
                .arg(QString::number(m_currentViewerSelectedAddress, 16).toUpper()));
        }
    }

    QString StructureWorkspaceDialog::suggestDefaultType(uint32_t address) {
        auto valOpt = readMemoryDword(address);
        if (!valOpt.has_value()) return "[32-bit] Variable";

        uint32_t rawVal = valOpt.value();
        if (rawVal != 0 && m_manager && m_manager->isValidPointerTarget(rawVal)) {
            return m_manager->getPlatformPointerTypeDescription();
        }

        float fVal;
        std::memcpy(&fVal, &rawVal, 4);
        if (!std::isnan(fVal) && !std::isinf(fVal) && std::abs(fVal) >= 0.0001f && std::abs(fVal) <= 1000000.0f) {
            return "[Float] Variable";
        }

        return "[32-bit] Variable";
    }

    void StructureWorkspaceDialog::insertNodeInScope(QTreeWidgetItem* parentScope, int32_t offset, const QString& description) {
        QTreeWidget* tree = m_structureTree;
        int childCount = parentScope ? parentScope->childCount() : tree->topLevelItemCount();

        int insertIndex = childCount;
        for (int i = 0; i < childCount; ++i) {
            QTreeWidgetItem* existing = parentScope ? parentScope->child(i) : tree->topLevelItem(i);
            int32_t existingOffset = existing->data(1, Qt::UserRole).toInt();
            if (existingOffset > offset) {
                insertIndex = i;
                break;
            }
        }

        auto* newItem = new QTreeWidgetItem();
        newItem->setText(0, description);
        newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
        QString sign = (offset < 0) ? "-" : "+";
        newItem->setText(1, sign + "0x" + QString::number(std::abs(offset), 16).toUpper());
        newItem->setData(1, Qt::UserRole, offset);
        newItem->setData(1, Qt::UserRole + 1, "="); // Default delimiter

        if (parentScope) {
            parentScope->insertChild(insertIndex, newItem);
            parentScope->setExpanded(true);
        }
        else {
            tree->insertTopLevelItem(insertIndex, newItem);
        }

        tree->setCurrentItem(newItem);
    }

    void StructureWorkspaceDialog::onAddVariableClicked() {
        if (m_currentViewerSelectedAddress == 0) return;

        auto match = findBestScopeForAddress(m_currentViewerSelectedAddress);
        if (!match.isValid) {
            QMessageBox::warning(this, "Structure Placement", "Selected address does not fall within any active pointer scope.");
            return;
        }

        bool ok = false;
        QString defaultDesc = suggestDefaultType(m_currentViewerSelectedAddress);
        QString desc = QInputDialog::getText(this, "Add Variable", "Variable description:", QLineEdit::Normal, defaultDesc, &ok);
        if (!ok || desc.trimmed().isEmpty()) return;

        insertNodeInScope(match.pointerNode, match.offset, desc.trimmed());
        recalculateTreeAddresses();
        syncTreeToCodeNoteText();
    }

    void StructureWorkspaceDialog::onAddPointerClicked() {
        if (m_currentViewerSelectedAddress == 0) return;

        auto match = findBestScopeForAddress(m_currentViewerSelectedAddress);
        if (!match.isValid) {
            QMessageBox::warning(this, "Structure Placement", "Selected address does not fall within any active pointer scope.");
            return;
        }

        QString ptrDesc = m_manager ? m_manager->getPlatformPointerTypeDescription() : "[32-bit] Pointer";
        insertNodeInScope(match.pointerNode, match.offset, ptrDesc);
        recalculateTreeAddresses();
        syncTreeToCodeNoteText();

        // Directly navigate into the new pointer's target in RAM
        auto valOpt = readMemoryDword(m_currentViewerSelectedAddress);
        if (valOpt.has_value() && m_manager && m_manager->isValidPointerTarget(valOpt.value())) {
            uint32_t nativeTarget = m_manager->unnormalizeAddress(QString::number(valOpt.value(), 16));
            navigateToAddress(nativeTarget);
        }
    }

    void StructureWorkspaceDialog::onRemoveNodeClicked() {
        auto* current = m_structureTree->currentItem();
        if (!current) return;

        delete current;
        recalculateTreeAddresses();
        updateOffsetPreview();
        syncTreeToCodeNoteText();
    }

    void StructureWorkspaceDialog::onStructureTreeItemChanged(QTreeWidgetItem* item, int column) {
        if (column == 1) {
            QString txt = item->text(1).trimmed();
            QRegularExpression rx("^([\\+\\-]?)0x([0-9a-fA-F]+)");
            auto match = rx.match(txt);
            if (match.hasMatch()) {
                QString sign = match.captured(1);
                bool ok = false;
                int32_t val = match.captured(2).toInt(&ok, 16);
                if (sign == "-") val = -val;
                item->setData(1, Qt::UserRole, val);
                recalculateTreeAddresses();
                syncTreeToCodeNoteText();
            }
        }
        else if (column == 0) {
            syncTreeToCodeNoteText();
        }
    }

    void StructureWorkspaceDialog::onNewStructureClicked() {
        bool ok = false;
        QString name = QInputDialog::getText(this, "New Structure", "Enter structure name:", QLineEdit::Normal, "New Struct", &ok);
        if (ok && !name.trimmed().isEmpty()) {
            StructureItemData item;
            item.name = name.trimmed();
            item.baseAddress = m_baseAddressEdit->text().trimmed();
            item.headerComment = m_headerComment;
            m_structures.push_back(item);
            m_structureSelectCombo->addItem(item.name);
            m_structureSelectCombo->setCurrentIndex(m_structureSelectCombo->count() - 1);
            saveWorkspaceSettings();
        }
    }

    void StructureWorkspaceDialog::onDeleteStructureClicked() {
        if (m_structures.size() <= 1) {
            m_structureTree->clear();
            m_codeNoteTextEdit->clear();
            m_headerComment.clear();
            return;
        }

        int idx = m_structureSelectCombo->currentIndex();
        if (idx >= 0 && idx < static_cast<int>(m_structures.size())) {
            m_structures.erase(m_structures.begin() + idx);
            m_structureSelectCombo->removeItem(idx);
            saveWorkspaceSettings();
        }
    }

    void StructureWorkspaceDialog::onActiveStructureComboChanged(int index) {
        if (index < 0 || index >= static_cast<int>(m_structures.size())) return;

        // Save current active structure state before switching
        if (m_currentStructureIndex >= 0 && m_currentStructureIndex < static_cast<int>(m_structures.size()) && m_currentStructureIndex != index) {
            m_structures[m_currentStructureIndex].baseAddress = m_baseAddressEdit->text().trimmed();
            m_structures[m_currentStructureIndex].codeNoteText = m_codeNoteTextEdit->toPlainText();
            m_structures[m_currentStructureIndex].headerComment = m_headerComment;
        }

        m_currentStructureIndex = index;
        m_baseAddressEdit->setText(m_structures[index].baseAddress);
        m_headerComment = m_structures[index].headerComment;

        m_isSyncing = true;
        m_codeNoteTextEdit->setPlainText(m_structures[index].codeNoteText);
        m_isSyncing = false;

        onFixHierarchyClicked();
        saveWorkspaceSettings();
    }

    void StructureWorkspaceDialog::onToggleCodeNotePanelClicked() {
        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() < 3) return;

        if (sizes[2] > 0) {
            m_lastCodeNotePanelWidth = sizes[2];
            sizes[1] += sizes[2];
            sizes[2] = 0;
            m_mainSplitter->setSizes(sizes);
            m_toggleCodeNoteBtn->setChecked(false);
        }
        else {
            int restoreWidth = (m_lastCodeNotePanelWidth > 60) ? m_lastCodeNotePanelWidth : 330;
            sizes[1] = std::max(100, sizes[1] - restoreWidth);
            sizes[2] = restoreWidth;
            m_mainSplitter->setSizes(sizes);
            m_toggleCodeNoteBtn->setChecked(true);
        }
        saveWorkspaceSettings();
    }

    void StructureWorkspaceDialog::onSplitterMoved(int, int index) {
        if (index == 2 || index == 1) {
            QList<int> sizes = m_mainSplitter->sizes();
            if (sizes.size() > 2) {
                bool visible = (sizes[2] > 0);
                if (visible) {
                    m_lastCodeNotePanelWidth = sizes[2];
                }
                m_toggleCodeNoteBtn->blockSignals(true);
                m_toggleCodeNoteBtn->setChecked(visible);
                m_toggleCodeNoteBtn->blockSignals(false);
                saveWorkspaceSettings();
            }
        }
    }

    void StructureWorkspaceDialog::onHierarchyStyleChanged() {
        saveWorkspaceSettings();
    }

    void StructureWorkspaceDialog::onCopyCodeNoteClicked() {
        QString text = m_codeNoteTextEdit->toPlainText();
        if (text.isEmpty()) return;

        QApplication::clipboard()->setText(text);

        QString origText = m_copyCodeNoteBtn->text();
        m_copyCodeNoteBtn->setText("✓ Copied!");
        m_copyCodeNoteBtn->setEnabled(false);

        QTimer::singleShot(1500, this, [this, origText]() {
            m_copyCodeNoteBtn->setText(origText);
            m_copyCodeNoteBtn->setEnabled(true);
        });
    }

    std::vector<std::shared_ptr<StructureWorkspaceDialog::CodeNoteNode>>
    StructureWorkspaceDialog::parseLinesToTree(const QStringList& lines) {
        auto rootNodes = std::make_shared<std::vector<std::shared_ptr<CodeNoteNode>>>();

        struct StackFrame {
            int indent;
            std::vector<std::shared_ptr<CodeNoteNode>>* parentList;
        };

        std::vector<StackFrame> stack;
        stack.push_back({ -1, rootNodes.get() });
        std::shared_ptr<CodeNoteNode> lastNode = nullptr;
        int lineIndex = 0;
        bool hasEncounteredOffset = false;

        QRegularExpression offsetSearch("[\\+\\-]0x[0-9a-fA-F]+", QRegularExpression::CaseInsensitiveOption);

        for (const auto& line : lines) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.isEmpty()) {
                if (!hasEncounteredOffset) {
                    if (!m_headerComment.isEmpty()) m_headerComment.append("\n");
                }
                lineIndex++;
                continue;
            }

            int indent = 0;
            QString content;
            QString untrimmedLine = line;

            auto offsetMatch = offsetSearch.match(untrimmedLine);
            bool isOffsetLine = offsetMatch.hasMatch();

            if (isOffsetLine) {
                hasEncounteredOffset = true;
                int offsetPos = offsetMatch.capturedStart();
                QString prefix = untrimmedLine.left(offsetPos);

                bool hasBoxChar = false;
                for (QChar c : prefix) {
                    if (c == L'│' || c == L'├' || c == L'└' || c == L'┬' || c == L'─') {
                        hasBoxChar = true;
                        break;
                    }
                }

                if (hasBoxChar) {
                    indent = (prefix.length() > 0) ? (prefix.length() - 1) : 0;
                }
                else {
                    indent = prefix.length();
                }

                content = untrimmedLine.mid(offsetPos).trimmed();
            }
            else {
                indent = 0;
                content = trimmedLine;
            }

            if (indent < 0) indent = 0;

            if (!hasEncounteredOffset && !isOffsetLine) {
                if (!m_headerComment.isEmpty()) m_headerComment.append("\n");
                m_headerComment.append(trimmedLine);
                lineIndex++;
                continue;
            }

            bool isAttachedNote = !isOffsetLine && (lastNode != nullptr);

            if (isAttachedNote) {
                // Strip all leading prefix dots, pipes, or spaces for clean data representation
                QString cleanNote = trimmedLine;
                int prefixChars = 0;
                while (prefixChars < cleanNote.length() && (cleanNote[prefixChars] == '.' || cleanNote[prefixChars] == ' ' || cleanNote[prefixChars] == L'│')) {
                    prefixChars++;
                }
                cleanNote = cleanNote.mid(prefixChars).trimmed();

                if (!cleanNote.isEmpty()) {
                    auto noteNode = std::make_shared<CodeNoteNode>();
                    noteNode->content = cleanNote;
                    noteNode->originalIndex = lineIndex;
                    lastNode->children.push_back(noteNode);
                }
            }
            else {
                while (stack.back().indent >= indent) {
                    stack.pop_back();
                }

                auto node = std::make_shared<CodeNoteNode>();
                node->content = content;
                node->originalIndex = lineIndex;

                stack.back().parentList->push_back(node);
                stack.push_back({ indent, &(node->children) });
                lastNode = node;
            }
            lineIndex++;
        }
        return *rootNodes;
    }

    std::vector<std::shared_ptr<StructureWorkspaceDialog::CodeNoteNode>>
    StructureWorkspaceDialog::mergeTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
        std::vector<std::shared_ptr<CodeNoteNode>> finalNodes;
        std::map<QString, std::vector<std::shared_ptr<CodeNoteNode>>> mergeableGroups;
        std::vector<std::shared_ptr<CodeNoteNode>> nonMergeableNodes;

        QRegularExpression offsetRegex("^([\\+\\-]0x[0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);

        for (const auto& node : nodes) {
            auto match = offsetRegex.match(node->content);
            if (match.hasMatch()) {
                QString rawKey = match.captured(1);
                QString sign = rawKey.left(1);
                bool ok;
                qlonglong offsetVal = rawKey.mid(3).toLongLong(&ok, 16);
                QString key = QString("%10x%2").arg(sign).arg(QString::number(offsetVal, 16).toLower());

                mergeableGroups[key].push_back(node);
            }
            else {
                nonMergeableNodes.push_back(node);
            }
        }

        QRegularExpression contentRegex("^([\\+\\-]0x)([0-9a-fA-F]+)(.*)", QRegularExpression::CaseInsensitiveOption);

        for (const auto& [key, group] : mergeableGroups) {
            auto representative = group.front();
            for (const auto& n : group) {
                if (n->originalIndex < representative->originalIndex) {
                    representative = n;
                }
            }

            QString bestContent = representative->content;
            for (const auto& n : group) {
                if (n->content.length() > bestContent.length()) {
                    bestContent = n->content;
                }
            }

            auto contentMatch = contentRegex.match(bestContent);
            if (contentMatch.hasMatch()) {
                QString signAndPrefix = contentMatch.captured(1);
                qlonglong offsetVal = contentMatch.captured(2).toLongLong(nullptr, 16);
                QString restOfLine = contentMatch.captured(3);
                bestContent = QString("%1%2%3")
                    .arg(signAndPrefix.toLower())
                    .arg(QString::number(offsetVal, 16).toLower())
                    .arg(restOfLine);
            }

            auto newNode = std::make_shared<CodeNoteNode>();
            newNode->content = bestContent;
            newNode->originalIndex = representative->originalIndex;

            std::vector<std::shared_ptr<CodeNoteNode>> allChildren;
            for (const auto& n : group) {
                allChildren.insert(allChildren.end(), n->children.begin(), n->children.end());
            }

            if (!allChildren.empty()) {
                newNode->children = mergeTree(allChildren);
            }
            finalNodes.push_back(newNode);
        }

        for (auto node : nonMergeableNodes) {
            if (!node->children.empty()) {
                node->children = mergeTree(node->children);
            }
            finalNodes.push_back(node);
        }

        return sortNodes(finalNodes);
    }

    std::vector<std::shared_ptr<StructureWorkspaceDialog::CodeNoteNode>>
    StructureWorkspaceDialog::sortNodes(std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
        QRegularExpression offsetRegex("^[\\+\\-]0x([0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);

        std::sort(nodes.begin(), nodes.end(), [&](const std::shared_ptr<CodeNoteNode>& a, const std::shared_ptr<CodeNoteNode>& b) {
            bool aIsOffset = offsetRegex.match(a->content).hasMatch();
            bool bIsOffset = offsetRegex.match(b->content).hasMatch();

            if (aIsOffset != bIsOffset) {
                return !aIsOffset;
            }

            if (aIsOffset) {
                qlonglong aVal = offsetRegex.match(a->content).captured(1).toLongLong(nullptr, 16);
                qlonglong bVal = offsetRegex.match(b->content).captured(1).toLongLong(nullptr, 16);
                return aVal < bVal;
            }

            return a->originalIndex < b->originalIndex;
        });

        return nodes;
    }

    QString StructureWorkspaceDialog::rebuildTextFromTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes,
        int indentLevel,
        const QString& style,
        bool indentDescriptions,
        std::vector<bool> ancestorIsLast) {
        QString sb = "";

        if (indentLevel == 0 && !m_headerComment.trimmed().isEmpty()) {
            sb.append(m_headerComment.trimmed() + "\n\n");
        }

        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            bool isLastChild = (i == nodes.size() - 1);

            bool isPointerOffset = node->content.startsWith("+0x", Qt::CaseInsensitive) || node->content.startsWith("-0x", Qt::CaseInsensitive);
            bool isAttachedNote = !isPointerOffset;

            bool hasPointerChildren = false;
            for (const auto& child : node->children) {
                if (child->content.startsWith("+0x", Qt::CaseInsensitive) || child->content.startsWith("-0x", Qt::CaseInsensitive)) {
                    hasPointerChildren = true;
                    break;
                }
            }

            QString prefix = "";

            if (style.startsWith("Unicode Tree")) {
                if (isAttachedNote) {
                    if (indentDescriptions && indentLevel > 1) {
                        for (int a = 0; a < indentLevel - 2; ++a) {
                            prefix.append(ancestorIsLast[a] ? " " : "│");
                        }
                        prefix.append(ancestorIsLast[indentLevel - 2] ? "  " : "│ ");
                    }
                }
                else {
                    if (indentLevel > 0) {
                        for (int a = 0; a < indentLevel - 1; ++a) {
                            prefix.append(ancestorIsLast[a] ? " " : "│");
                        }
                        prefix.append(isLastChild ? "└" : "├");
                        prefix.append(hasPointerChildren ? "┬" : "─");
                    }
                }
            }
            else {
                QChar indentChar = style.startsWith("Plusses") ? '+' : '.';

                if (isAttachedNote) {
                    if (indentDescriptions) {
                        prefix = indentLevel > 0 ? QString(indentLevel, '.') : "";
                    }
                }
                else {
                    prefix = indentLevel > 0 ? QString(indentLevel, indentChar) : "";
                }
            }

            sb.append(QString("%1%2\n").arg(prefix).arg(node->content));

            if (!node->children.empty()) {
                std::vector<bool> nextAncestors = ancestorIsLast;
                if (indentLevel > 0) {
                    nextAncestors.push_back(isLastChild);
                }
                sb.append(rebuildTextFromTree(node->children, indentLevel + 1, style, indentDescriptions, nextAncestors));
            }
        }
        return sb;
    }

    void StructureWorkspaceDialog::populateTreeFromHierarchyNodes(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes, QTreeWidgetItem* parentItem) {
        QRegularExpression offsetRx("^([\\+\\-]0x[0-9a-fA-F]+)(?:([=\\s\\|]+)(.*))?$");

        for (const auto& node : nodes) {
            auto match = offsetRx.match(node->content.trimmed());
            if (match.hasMatch()) {
                QString rawOffset = match.captured(1);
                QString delimiter = match.captured(2);
                QString desc = match.captured(3).trimmed();

                QString sign = rawOffset.left(1);
                bool ok = false;
                int32_t offsetVal = rawOffset.mid(3).toInt(&ok, 16);
                if (sign == "-") offsetVal = -offsetVal;

                auto* newItem = new QTreeWidgetItem(parentItem ? parentItem : m_structureTree->invisibleRootItem());
                newItem->setText(0, desc);
                newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
                newItem->setText(1, sign + "0x" + QString::number(std::abs(offsetVal), 16).toUpper());
                newItem->setData(1, Qt::UserRole, offsetVal);
                newItem->setData(1, Qt::UserRole + 1, delimiter.isEmpty() ? "=" : delimiter);

                QStringList notes;
                for (const auto& child : node->children) {
                    if (!child->content.startsWith("+0x", Qt::CaseInsensitive) && !child->content.startsWith("-0x", Qt::CaseInsensitive)) {
                        notes.append(child->content);
                    }
                }
                if (!notes.empty()) {
                    newItem->setData(0, Qt::UserRole + 2, notes);
                    newItem->setToolTip(0, notes.join("\n"));
                }

                std::vector<std::shared_ptr<CodeNoteNode>> pointerChildren;
                for (const auto& child : node->children) {
                    if (child->content.startsWith("+0x", Qt::CaseInsensitive) || child->content.startsWith("-0x", Qt::CaseInsensitive)) {
                        pointerChildren.push_back(child);
                    }
                }

                if (!pointerChildren.empty()) {
                    populateTreeFromHierarchyNodes(pointerChildren, newItem);
                }
            }
        }
    }

    void StructureWorkspaceDialog::onFixHierarchyClicked() {
        if (m_isSyncing || !m_codeNoteTextEdit) return;
        m_isSyncing = true;

        QString text = m_codeNoteTextEdit->toPlainText().trimmed();
        if (text.isEmpty()) {
            m_structureTree->clear();
            m_headerComment.clear();
            m_isSyncing = false;
            return;
        }

        m_headerComment.clear();
        QStringList lines = text.split(QRegularExpression("[\r\n]+"));
        QString style = m_styleCombo->currentText();
        bool indentDesc = m_indentDescCheck->isChecked();

        auto parsedTree = parseLinesToTree(lines);
        auto mergedTree = mergeTree(parsedTree);
        QString finalResult = rebuildTextFromTree(mergedTree, 0, style, indentDesc);

        m_codeNoteTextEdit->setPlainText(finalResult.trimmed());

        m_structureTree->clear();
        populateTreeFromHierarchyNodes(mergedTree);

        m_structureTree->expandAll();
        recalculateTreeAddresses();
        updateOffsetPreview();

        if (m_currentStructureIndex >= 0 && m_currentStructureIndex < static_cast<int>(m_structures.size())) {
            m_structures[m_currentStructureIndex].codeNoteText = finalResult.trimmed();
            m_structures[m_currentStructureIndex].baseAddress = m_baseAddressEdit->text().trimmed();
            m_structures[m_currentStructureIndex].headerComment = m_headerComment;
        }

        saveWorkspaceSettings();
        m_isSyncing = false;
    }

    QString StructureWorkspaceDialog::generateCodeNoteFromTree() const {
        std::function<std::vector<std::shared_ptr<CodeNoteNode>>(QTreeWidgetItem*)> treeToNodes =
            [&](QTreeWidgetItem* parent) {
                std::vector<std::shared_ptr<CodeNoteNode>> nodes;
                int count = parent ? parent->childCount() : m_structureTree->topLevelItemCount();

                for (int i = 0; i < count; ++i) {
                    QTreeWidgetItem* item = parent ? parent->child(i) : m_structureTree->topLevelItem(i);
                    int32_t offset = item->data(1, Qt::UserRole).toInt();
                    QString sign = (offset < 0) ? "-" : "+";
                    QString offsetHex = QString::number(std::abs(offset), 16).toLower();
                    QString delimiter = item->data(1, Qt::UserRole + 1).toString();
                    if (delimiter.isEmpty()) delimiter = "=";

                    auto node = std::make_shared<CodeNoteNode>();
                    node->content = sign + "0x" + offsetHex + delimiter + item->text(0);

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
        QString style = m_styleCombo ? m_styleCombo->currentText() : "Unicode Tree (├─)";
        bool indentDesc = m_indentDescCheck ? m_indentDescCheck->isChecked() : true;

        StructureWorkspaceDialog* nonConstThis = const_cast<StructureWorkspaceDialog*>(this);
        return nonConstThis->rebuildTextFromTree(rootNodes, 0, style, indentDesc);
    }

    void StructureWorkspaceDialog::syncTreeToCodeNoteText() {
        if (m_isSyncing || !m_codeNoteTextEdit) return;
        m_isSyncing = true;

        QString newText = generateCodeNoteFromTree();
        if (m_codeNoteTextEdit->toPlainText() != newText) {
            int cursorPos = m_codeNoteTextEdit->textCursor().position();
            m_codeNoteTextEdit->setPlainText(newText);
            QTextCursor cur = m_codeNoteTextEdit->textCursor();
            cur.setPosition(std::min(cursorPos, static_cast<int>(newText.length())));
            m_codeNoteTextEdit->setTextCursor(cur);
        }

        if (m_currentStructureIndex >= 0 && m_currentStructureIndex < static_cast<int>(m_structures.size())) {
            m_structures[m_currentStructureIndex].codeNoteText = newText;
            m_structures[m_currentStructureIndex].baseAddress = m_baseAddressEdit->text().trimmed();
            m_structures[m_currentStructureIndex].headerComment = m_headerComment;
        }

        saveWorkspaceSettings();
        m_isSyncing = false;
    }

    void StructureWorkspaceDialog::loadWorkspaceSettings() {
        QSettings settings;
        settings.beginGroup("DissectStructure");
        if (settings.contains("geometry")) {
            restoreGeometry(settings.value("geometry").toByteArray());
        }
        if (settings.contains("splitterState")) {
            m_mainSplitter->restoreState(settings.value("splitterState").toByteArray());
        }
        m_lastCodeNotePanelWidth = settings.value("lastCodeNoteWidth", 330).toInt();

        int viewModeId = settings.value("viewMode", 2).toInt();
        if (auto* btn = m_viewModeGroup->button(viewModeId)) {
            btn->setChecked(true);
        }
        onViewModeChanged();

        QString style = settings.value("hierarchyStyle", "Plusses (+)").toString();
        m_styleCombo->setCurrentText(style);
        m_indentDescCheck->setChecked(settings.value("indentDescriptions", true).toBool());

        int structCount = settings.value("structureCount", 0).toInt();
        m_structures.clear();
        m_structureSelectCombo->blockSignals(true);
        m_structureSelectCombo->clear();

        if (structCount > 0) {
            for (int i = 0; i < structCount; ++i) {
                StructureItemData item;
                item.name = settings.value(QString("structure_%1_name").arg(i), QString("Structure %1").arg(i + 1)).toString();
                item.baseAddress = settings.value(QString("structure_%1_base").arg(i), "").toString();
                item.codeNoteText = settings.value(QString("structure_%1_text").arg(i), "").toString();
                item.headerComment = settings.value(QString("structure_%1_header").arg(i), "").toString();
                m_structures.push_back(item);
                m_structureSelectCombo->addItem(item.name);
            }
        }
        else {
            StructureItemData defaultStruct;
            defaultStruct.name = "Entity Struct";
            defaultStruct.baseAddress = "004934dc";
            m_structures.push_back(defaultStruct);
            m_structureSelectCombo->addItem(defaultStruct.name);
        }

        int activeIdx = settings.value("activeStructureIndex", 0).toInt();
        if (activeIdx < 0 || activeIdx >= static_cast<int>(m_structures.size())) {
            activeIdx = 0;
        }
        m_currentStructureIndex = activeIdx;
        m_structureSelectCombo->setCurrentIndex(activeIdx);
        m_structureSelectCombo->blockSignals(false);

        // Load active structure
        m_baseAddressEdit->setText(m_structures[activeIdx].baseAddress);
        m_headerComment = m_structures[activeIdx].headerComment;
        if (!m_structures[activeIdx].codeNoteText.isEmpty()) {
            m_codeNoteTextEdit->setPlainText(m_structures[activeIdx].codeNoteText);
            onFixHierarchyClicked();
        }

        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() > 2) {
            m_toggleCodeNoteBtn->setChecked(sizes[2] > 0);
        }

        settings.endGroup();
    }

    void StructureWorkspaceDialog::saveWorkspaceSettings() {
        QSettings settings;
        settings.beginGroup("DissectStructure");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("splitterState", m_mainSplitter->saveState());
        settings.setValue("lastCodeNoteWidth", m_lastCodeNotePanelWidth);

        QList<int> sizes = m_mainSplitter->sizes();
        settings.setValue("codeNotePanelVisible", (sizes.size() > 2 && sizes[2] > 0));

        settings.setValue("viewMode", m_viewModeGroup->checkedId());
        settings.setValue("hierarchyStyle", m_styleCombo->currentText());
        settings.setValue("indentDescriptions", m_indentDescCheck->isChecked());

        if (m_currentStructureIndex >= 0 && m_currentStructureIndex < static_cast<int>(m_structures.size())) {
            m_structures[m_currentStructureIndex].baseAddress = m_baseAddressEdit->text().trimmed();
            m_structures[m_currentStructureIndex].codeNoteText = m_codeNoteTextEdit->toPlainText();
            m_structures[m_currentStructureIndex].headerComment = m_headerComment;
        }

        settings.setValue("structureCount", static_cast<int>(m_structures.size()));
        settings.setValue("activeStructureIndex", m_currentStructureIndex);
        for (size_t i = 0; i < m_structures.size(); ++i) {
            settings.setValue(QString("structure_%1_name").arg(i), m_structures[i].name);
            settings.setValue(QString("structure_%1_base").arg(i), m_structures[i].baseAddress);
            settings.setValue(QString("structure_%1_text").arg(i), m_structures[i].codeNoteText);
            settings.setValue(QString("structure_%1_header").arg(i), m_structures[i].headerComment);
        }
        settings.endGroup();
    }

}
