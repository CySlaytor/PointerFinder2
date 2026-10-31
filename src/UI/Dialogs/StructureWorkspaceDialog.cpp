#include "StructureWorkspaceDialog.h"

#include "../../Common/EndianUtils.h"
#include "../../Core/CodeNoteHelper.h"
#include "../../Models/CodeNoteSettings.h"

#include <algorithm>
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
#include <QTimer>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::DataModels;

    StructureWorkspaceDialog::StructureWorkspaceDialog(Emulators::IEmulatorManager* manager, QWidget* parent)
        : QDialog(parent), m_manager(manager) {
        setWindowTitle("Pointer Finder 2.0 - Structure Workspace & Memory Viewer");
        resize(1220, 750);
        setMinimumSize(880, 520);

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

        m_importNoteBtn = new QPushButton("Import Code Note...", this);
        m_exportNoteBtn = new QPushButton("Export as Code Note", this);
        QFont boldFont = m_exportNoteBtn->font();
        boldFont.setBold(true);
        m_exportNoteBtn->setFont(boldFont);
        topToolbar->addWidget(m_importNoteBtn);
        topToolbar->addWidget(m_exportNoteBtn);

        topToolbar->addStretch();
        rootLayout->addLayout(topToolbar);

        auto* splitter = new QSplitter(Qt::Horizontal, this);

        // --- Left Panel: Structure Editor ---
        auto* leftContainer = new QWidget(splitter);
        auto* leftLayout = new QVBoxLayout(leftContainer);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(6);

        auto* baseAddrRow = new QHBoxLayout();
        baseAddrRow->addWidget(new QLabel("Structure Base (Hex):", leftContainer));
        m_baseAddressEdit = new QLineEdit(leftContainer);
        m_baseAddressEdit->setPlaceholderText("e.g. 004934dc");
        baseAddrRow->addWidget(m_baseAddressEdit);
        leftLayout->addLayout(baseAddrRow);

        m_structureTree = new QTreeWidget(leftContainer);
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

        m_offsetPreviewLabel = new QLabel("Select an address in Memory Viewer to calculate relative offset.", leftContainer);
        m_offsetPreviewLabel->setStyleSheet("font-weight: bold; color: #58A6FF; padding: 4px; border: 1px solid #30363D; border-radius: 4px;");
        leftLayout->addWidget(m_offsetPreviewLabel);

        auto* treeButtonsRow = new QHBoxLayout();
        m_addVariableBtn = new QPushButton("Add Variable (Enter)", leftContainer);
        m_addVariableBtn->setStyleSheet("font-weight: bold;");
        m_addPointerBtn = new QPushButton("Add as Pointer", leftContainer);
        m_followPointerBtn = new QPushButton("Follow Pointer ->", leftContainer);
        m_followPointerBtn->setStyleSheet("color: #FF7B72; font-weight: bold;");
        m_removeNodeBtn = new QPushButton("Remove", leftContainer);

        treeButtonsRow->addWidget(m_addVariableBtn, 2);
        treeButtonsRow->addWidget(m_addPointerBtn, 1);
        treeButtonsRow->addWidget(m_followPointerBtn, 1);
        treeButtonsRow->addWidget(m_removeNodeBtn, 1);
        leftLayout->addLayout(treeButtonsRow);

        splitter->addWidget(leftContainer);

        // --- Right Panel: Memory Viewer ---
        auto* rightContainer = new QWidget(splitter);
        auto* rightLayout = new QVBoxLayout(rightContainer);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(6);

        auto* viewerHeaderRow = new QHBoxLayout();
        m_refreshRamBtn = new QPushButton("Re-dump / Refresh RAM", rightContainer);
        m_refreshRamBtn->setStyleSheet("font-weight: bold;");
        viewerHeaderRow->addWidget(m_refreshRamBtn);

        m_ramStatusLabel = new QLabel("No dump loaded.", rightContainer);
        viewerHeaderRow->addWidget(m_ramStatusLabel);
        viewerHeaderRow->addStretch();

        // Browser/Cheat-Engine-style Navigation History controls
        m_historyBackBtn = new QPushButton("<", rightContainer);
        m_historyBackBtn->setToolTip("Navigate Back (Alt+Left)");
        m_historyBackBtn->setFixedWidth(28);
        m_historyBackBtn->setEnabled(false);

        m_historyForwardBtn = new QPushButton(">", rightContainer);
        m_historyForwardBtn->setToolTip("Navigate Forward (Alt+Right)");
        m_historyForwardBtn->setFixedWidth(28);
        m_historyForwardBtn->setEnabled(false);

        viewerHeaderRow->addWidget(m_historyBackBtn);
        viewerHeaderRow->addWidget(m_historyForwardBtn);

        viewerHeaderRow->addWidget(new QLabel("Go to (Hex):", rightContainer));
        m_goToAddressEdit = new QLineEdit(rightContainer);
        m_goToAddressEdit->setMaximumWidth(100);
        m_goToAddressBtn = new QPushButton("Go", rightContainer);
        viewerHeaderRow->addWidget(m_goToAddressEdit);
        viewerHeaderRow->addWidget(m_goToAddressBtn);

        rightLayout->addLayout(viewerHeaderRow);

        // Memory View Mode Selector
        auto* viewModeRow = new QHBoxLayout();
        viewModeRow->addWidget(new QLabel("Memory View:", rightContainer));

        m_view8BitRadio = new QRadioButton("8-bit", rightContainer);
        m_view16BitRadio = new QRadioButton("16-bit", rightContainer);
        m_view32BitRadio = new QRadioButton("32-bit", rightContainer);
        m_view32BitBERadio = new QRadioButton("32-bit BE", rightContainer);
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
        rightLayout->addLayout(viewModeRow);

        m_memoryViewer = new Widgets::MemoryViewerWidget(rightContainer);
        rightLayout->addWidget(m_memoryViewer, 1);

        splitter->addWidget(rightContainer);

        splitter->setStretchFactor(0, 4);
        splitter->setStretchFactor(1, 6);
        rootLayout->addWidget(splitter, 1);

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
        connect(m_historyBackBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onNavHistoryBack);
        connect(m_historyForwardBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onNavHistoryForward);
        connect(m_baseAddressEdit, &QLineEdit::textChanged, this, [this]() { recalculateTreeAddresses(); });

        connect(m_viewModeGroup, &QButtonGroup::idClicked, this, &StructureWorkspaceDialog::onViewModeChanged);

        connect(m_newStructureBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onNewStructureClicked);
        connect(m_deleteStructureBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onDeleteStructureClicked);
        connect(m_structureSelectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StructureWorkspaceDialog::onActiveStructureComboChanged);
        connect(m_exportNoteBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onExportCodeNoteClicked);
        connect(m_importNoteBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onImportCodeNoteClicked);
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

    void StructureWorkspaceDialog::navigateToAddress(uint32_t address, bool recordHistory) {
        if (!m_memoryViewer->hasData() || address == 0) return;

        uint32_t nativeAddr = address;
        if (m_manager) nativeAddr = m_manager->unnormalizeAddress(QString::number(address, 16));

        if (nativeAddr >= m_memoryViewer->getMemoryBase() && 
            nativeAddr < m_memoryViewer->getMemoryBase() + m_memoryViewer->getMemorySize()) {

            if (recordHistory) {
                if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size())) {
                    m_navHistory.erase(m_navHistory.begin() + m_navHistoryIndex + 1, m_navHistory.end());
                }
                m_navHistory.push_back(nativeAddr);
                m_navHistoryIndex = static_cast<int>(m_navHistory.size()) - 1;
                updateHistoryButtonStates();
            }

            m_memoryViewer->setSelectedAddress(nativeAddr);
        }
    }

    void StructureWorkspaceDialog::updateHistoryButtonStates() {
        m_historyBackBtn->setEnabled(m_navHistoryIndex > 0);
        m_historyForwardBtn->setEnabled(m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size()) - 1);
    }

    void StructureWorkspaceDialog::onNavHistoryBack() {
        if (m_navHistoryIndex > 0) {
            m_navHistoryIndex--;
            navigateToAddress(m_navHistory[m_navHistoryIndex], false);
            updateHistoryButtonStates();
        }
    }

    void StructureWorkspaceDialog::onNavHistoryForward() {
        if (m_navHistoryIndex >= 0 && m_navHistoryIndex < static_cast<int>(m_navHistory.size()) - 1) {
            m_navHistoryIndex++;
            navigateToAddress(m_navHistory[m_navHistoryIndex], false);
            updateHistoryButtonStates();
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
        uint32_t derefTarget = item->data(0, Qt::UserRole + 4).toUInt();

        if (derefTarget != 0) {
            QString followText = QString("Follow Pointer -> (0x%1)").arg(m_manager ? m_manager->formatDisplayAddress(derefTarget) : QString::number(derefTarget, 16).toUpper());
            menu.addAction(followText, this, &StructureWorkspaceDialog::onFollowPointerClicked);
            menu.addSeparator();
        }

        uint32_t resolved = resolveNodeAddress(item);
        if (resolved != 0) {
            QString locText = QString("Go to Pointer Address (0x%1)").arg(m_manager ? m_manager->formatDisplayAddress(resolved) : QString::number(resolved, 16).toUpper());
            menu.addAction(locText, this, [this, resolved]() {
                navigateToAddress(resolved);
            });
        }

        menu.addAction("Copy Resolved RAM Address", this, [this, item]() {
            QApplication::clipboard()->setText(item->text(2));
        });

        menu.addAction("Copy Pointer Value", this, [this, item]() {
            QApplication::clipboard()->setText(item->text(3));
        });

        menu.addSeparator();
        menu.addAction("Remove", this, &StructureWorkspaceDialog::onRemoveNodeClicked);

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
            }
        }
    }

    void StructureWorkspaceDialog::onNewStructureClicked() {
        bool ok = false;
        QString name = QInputDialog::getText(this, "New Structure", "Enter structure name:", QLineEdit::Normal, "New Struct", &ok);
        if (ok && !name.trimmed().isEmpty()) {
            StructureItemData item;
            item.name = name.trimmed();
            m_structures.push_back(item);
            m_structureSelectCombo->addItem(item.name);
            m_structureSelectCombo->setCurrentIndex(m_structureSelectCombo->count() - 1);
        }
    }

    void StructureWorkspaceDialog::onDeleteStructureClicked() {
        if (m_structures.size() <= 1) {
            m_structureTree->clear();
            return;
        }

        int idx = m_structureSelectCombo->currentIndex();
        if (idx >= 0 && idx < static_cast<int>(m_structures.size())) {
            m_structures.erase(m_structures.begin() + idx);
            m_structureSelectCombo->removeItem(idx);
        }
    }

    void StructureWorkspaceDialog::onActiveStructureComboChanged(int index) {
        if (index < 0 || index >= static_cast<int>(m_structures.size())) return;
        m_currentStructureIndex = index;
    }

    void StructureWorkspaceDialog::onExportCodeNoteClicked() {
        QStringList outputLines;
        CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();

        std::function<void(QTreeWidgetItem*, int)> exportItem = [&](QTreeWidgetItem* item, int level) {
            int32_t offset = item->data(1, Qt::UserRole).toInt();
            QChar prefixChar = settings.Prefix.isEmpty() ? QChar('+') : settings.Prefix.at(0);
            QString prefix = (level == 0) ? QString() : QString(level, prefixChar);
            QString sign = (offset < 0) ? "-" : "+";
            QString offsetHex = QString::number(std::abs(offset), 16).toUpper();

            QString line = prefix + sign + "0x" + offsetHex + settings.Suffix + (item->text(0).isEmpty() ? "" : item->text(0));
            outputLines << line;

            QStringList valueNotes = item->data(0, Qt::UserRole + 2).toStringList();
            for (const QString& vNote : valueNotes) {
                outputLines << vNote;
            }

            for (int i = 0; i < item->childCount(); ++i) {
                exportItem(item->child(i), level + 1);
            }
        };

        for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
            exportItem(m_structureTree->topLevelItem(i), 0);
        }

        if (outputLines.isEmpty()) {
            QMessageBox::warning(this, "Export Empty", "No nodes to export in the structure tree.");
            return;
        }

        QString noteText = outputLines.join("\n");
        QApplication::clipboard()->setText(noteText);

        QString origText = m_exportNoteBtn->text();
        m_exportNoteBtn->setText("✓ Copied Code Note!");
        m_exportNoteBtn->setEnabled(false);

        QTimer::singleShot(1500, this, [this, origText]() {
            m_exportNoteBtn->setText(origText);
            m_exportNoteBtn->setEnabled(true);
        });
    }

    void StructureWorkspaceDialog::onImportCodeNoteClicked() {
        bool ok = false;
        QString text = QInputDialog::getMultiLineText(this, "Import Code Note", "Paste Code Note text to parse into structure:", "", &ok);
        if (!ok || text.trimmed().isEmpty()) return;

        m_structureTree->clear();
        QStringList lines = text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

        std::map<int, QTreeWidgetItem*> levelParents;
        QTreeWidgetItem* lastVariableItem = nullptr;

        for (const QString& rawLine : lines) {
            QString untrimmed = rawLine;
            QString trimmed = rawLine.trimmed();
            if (trimmed.isEmpty()) continue;

            int prefixLength = 0;
            while (prefixLength < untrimmed.length()) {
                QChar c = untrimmed[prefixLength];
                if (c == '+' || c == '.' || c == ' ') {
                    prefixLength++;
                }
                else {
                    break;
                }
            }

            int indent = prefixLength;
            QString content = untrimmed.mid(prefixLength).trimmed();

            if (prefixLength > 0 && untrimmed[prefixLength - 1] == '+') {
                if (!content.startsWith('+') && !content.startsWith('-')) {
                    content.prepend('+');
                    indent = prefixLength - 1;
                }
            }

            QRegularExpression offsetRx("^([\\+\\-]0x[0-9a-fA-F]+)(?:[=\\s\\|]*)(.*)");
            auto match = offsetRx.match(content);

            if (match.hasMatch()) {
                QString rawOffset = match.captured(1);
                QString desc = match.captured(2).trimmed();

                QString sign = rawOffset.left(1);
                bool parseOk = false;
                int32_t offsetVal = rawOffset.mid(3).toInt(&parseOk, 16);
                if (sign == "-") offsetVal = -offsetVal;

                QTreeWidgetItem* parentItem = nullptr;
                for (int lvl = indent - 1; lvl >= 0; --lvl) {
                    if (levelParents.count(lvl)) {
                        parentItem = levelParents[lvl];
                        break;
                    }
                }

                auto* newItem = new QTreeWidgetItem(parentItem ? parentItem : m_structureTree->invisibleRootItem());
                newItem->setText(0, desc);
                newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
                newItem->setText(1, sign + "0x" + QString::number(std::abs(offsetVal), 16).toUpper());
                newItem->setData(1, Qt::UserRole, offsetVal);
                newItem->setData(0, Qt::UserRole + 3, indent);

                levelParents[indent] = newItem;

                auto it = levelParents.upper_bound(indent);
                levelParents.erase(it, levelParents.end());

                lastVariableItem = newItem;
            }
            else {
                if (lastVariableItem) {
                    QStringList notes = lastVariableItem->data(0, Qt::UserRole + 2).toStringList();
                    notes.append(rawLine);
                    lastVariableItem->setData(0, Qt::UserRole + 2, notes);
                    lastVariableItem->setToolTip(0, notes.join("\n"));
                }
                else if (trimmed.startsWith("[") && trimmed.contains("Pointer", Qt::CaseInsensitive)) {
                    m_baseAddressEdit->setPlaceholderText(trimmed);
                }
            }
        }

        m_structureTree->expandAll();
        recalculateTreeAddresses();
    }

    void StructureWorkspaceDialog::loadWorkspaceSettings() {
        QSettings settings;
        settings.beginGroup("StructureWorkspace");
        if (settings.contains("geometry")) {
            restoreGeometry(settings.value("geometry").toByteArray());
        }

        if (m_structures.empty()) {
            StructureItemData defaultStruct;
            defaultStruct.name = "Entity Struct";
            m_structures.push_back(defaultStruct);
            m_structureSelectCombo->addItem(defaultStruct.name);
        }
        settings.endGroup();
    }

    void StructureWorkspaceDialog::saveWorkspaceSettings() {
        QSettings settings;
        settings.beginGroup("StructureWorkspace");
        settings.setValue("geometry", saveGeometry());
        settings.endGroup();
    }

}
