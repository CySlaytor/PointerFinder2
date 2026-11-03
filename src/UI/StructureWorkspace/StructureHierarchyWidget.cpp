#include "StructureHierarchyWidget.h"

#include "Models/GlobalSettings.h"
#include "UI/Common/ThemeManager.h"

#include <algorithm>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::DataModels;

    StructureHierarchyWidget::StructureHierarchyWidget(QWidget* parent)
        : QWidget(parent) {
        setupLayout();
    }

    StructureHierarchyWidget::~StructureHierarchyWidget() = default;

    QIcon StructureHierarchyWidget::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void StructureHierarchyWidget::updateThemeIcons() {
        m_addVariableBtn->setIcon(getThemeIcon(":/icons/bookmark_add.svg"));
        m_addPointerBtn->setIcon(getThemeIcon(":/icons/attach.svg"));
        m_followPointerBtn->setIcon(getThemeIcon(":/icons/follow_pointer.svg"));
        m_removeNodeBtn->setIcon(getThemeIcon(":/icons/delete.svg"));

        m_treeCopyRaAction->setIcon(getThemeIcon(":/icons/copy_ra.svg"));
        m_treeCopyNoteAction->setIcon(getThemeIcon(":/icons/copy_note.svg"));
        m_treeCopyAddressAction->setIcon(getThemeIcon(":/icons/copy.svg"));
        m_treeCopyValueAction->setIcon(getThemeIcon(":/icons/copy_ra.svg"));
    }

    void StructureHierarchyWidget::setEmulatorManager(Emulators::IEmulatorManager* manager) {
        m_manager = manager;
    }

    QTreeWidgetItem* StructureHierarchyWidget::currentItem() const {
        return m_structureTree ? m_structureTree->currentItem() : nullptr;
    }

    QTreeWidget* StructureHierarchyWidget::treeWidget() const {
        return m_structureTree;
    }

    void StructureHierarchyWidget::clearTree() {
        if (m_structureTree) {
            m_structureTree->blockSignals(true);
            m_structureTree->clear();
            m_structureTree->blockSignals(false);
        }
    }

    void StructureHierarchyWidget::setupLayout() {
        auto* leftLayout = new QVBoxLayout(this);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(6);

        m_structureTree = new QTreeWidget(this);
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

        leftLayout->addWidget(m_structureTree, 1);

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

        m_addPointerBtn = new QPushButton("Add as Pointer", this);
        m_addPointerBtn->setToolTip("Add pointer at selected address (Shortcut: P)");
        m_addPointerBtn->setIcon(getThemeIcon(":/icons/attach.svg"));
        m_addPointerBtn->setFixedHeight(26);
        m_addPointerBtn->setAutoDefault(false);
        m_addPointerBtn->setDefault(false);

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

        treeButtonsRow->addWidget(m_addVariableBtn, 1);
        treeButtonsRow->addWidget(m_addPointerBtn, 1);
        treeButtonsRow->addWidget(m_followPointerBtn, 1);
        treeButtonsRow->addWidget(m_removeNodeBtn, 1);
        leftLayout->addLayout(treeButtonsRow);

        connect(m_structureTree, &QTreeWidget::itemDoubleClicked, this, &StructureHierarchyWidget::onTreeItemDoubleClicked);
        connect(m_structureTree, &QTreeWidget::customContextMenuRequested, this, &StructureHierarchyWidget::onTreeCustomContextMenu);
        connect(m_structureTree, &QTreeWidget::itemChanged, this, &StructureHierarchyWidget::onTreeItemChanged);
        connect(m_structureTree, &QTreeWidget::itemSelectionChanged, this, &StructureHierarchyWidget::treeSelectionChanged);

        connect(m_addVariableBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::addVariableClicked);
        connect(m_addPointerBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::addPointerClicked);
        connect(m_followPointerBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::followPointerClicked);
        connect(m_removeNodeBtn, &QPushButton::clicked, this, &StructureHierarchyWidget::removeNodeClicked);
    }

    void StructureHierarchyWidget::populateFromNodes(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
        m_structureTree->blockSignals(true);
        m_structureTree->clear();
        populateTreeFromHierarchyNodes(nodes);
        m_structureTree->expandAll();
        m_structureTree->blockSignals(false);
    }

    void StructureHierarchyWidget::populateTreeFromHierarchyNodes(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes, QTreeWidgetItem* parentItem) {
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
                }
                else if (isArrayElem) {
                    rawOffset = rawOffset.mid(1);
                }

                QString sign = rawOffset.startsWith('-') ? "-" : (rawOffset.startsWith('+') ? "+" : "");
                QString hexPart = (rawOffset.startsWith('-') || rawOffset.startsWith('+')) ? rawOffset.mid(1) : rawOffset;
                if (hexPart.startsWith("0x", Qt::CaseInsensitive)) {
                    hexPart = hexPart.mid(2);
                }

                bool ok = false;
                int32_t offsetVal = hexPart.toInt(&ok, 16);
                if (sign == "-") offsetVal = -offsetVal;

                auto* newItem = new QTreeWidgetItem(parentItem ? parentItem : m_structureTree->invisibleRootItem());
                newItem->setText(0, desc);
                newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
                QString offsetHex = QString("%1").arg(std::abs(offsetVal), 2, 16, QChar('0')).toUpper();

                QString displayOffset;
                if (isArrayChild) {
                    displayOffset = "|+" + (offsetVal < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
                }
                else if (isArrayElem) {
                    displayOffset = "|" + (offsetVal < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
                }
                else {
                    displayOffset = (offsetVal < 0 ? "-0x" : "+0x") + offsetHex;
                }

                newItem->setText(1, displayOffset);
                newItem->setData(1, Qt::UserRole, offsetVal);
                newItem->setData(1, Qt::UserRole + 1, delimiter.isEmpty() ? "=" : delimiter);
                newItem->setData(1, Qt::UserRole + 3, isArrayElem);

                QStringList notes;
                for (const auto& child : node->children) {
                    bool isChildOffset = child->content.startsWith("+0x", Qt::CaseInsensitive) || 
                                         child->content.startsWith("-0x", Qt::CaseInsensitive) ||
                                         child->content.startsWith("|0x", Qt::CaseInsensitive) ||
                                         child->content.startsWith("|+0x", Qt::CaseInsensitive) ||
                                         child->content.startsWith("|-0x", Qt::CaseInsensitive);
                    if (!isChildOffset) {
                        notes.append(child->content);
                    }
                }
                if (!notes.empty()) {
                    newItem->setData(0, Qt::UserRole + 2, notes);
                    newItem->setToolTip(0, notes.join("\n"));
                }

                std::vector<std::shared_ptr<CodeNoteNode>> pointerChildren;
                for (const auto& child : node->children) {
                    bool isChildOffset = child->content.startsWith("+0x", Qt::CaseInsensitive) || 
                                         child->content.startsWith("-0x", Qt::CaseInsensitive) ||
                                         child->content.startsWith("|0x", Qt::CaseInsensitive) ||
                                         child->content.startsWith("|+0x", Qt::CaseInsensitive) ||
                                         child->content.startsWith("|-0x", Qt::CaseInsensitive);
                    if (isChildOffset) {
                        pointerChildren.push_back(child);
                    }
                }

                if (!pointerChildren.empty()) {
                    populateTreeFromHierarchyNodes(pointerChildren, newItem);
                }
            }
        }
    }

    std::optional<uint32_t> StructureHierarchyWidget::readMemoryDword(uint32_t address, const std::vector<uint8_t>& snapshotMemory) {
        return StructureAnalyzer::readMemoryDword(address, m_manager, snapshotMemory);
    }

    uint32_t StructureHierarchyWidget::resolveNodeAddress(QTreeWidgetItem* item, uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
        if (!item || !m_manager || currentBaseAddress == 0) return 0;

        uint32_t base = m_manager->unnormalizeAddress(QString::number(currentBaseAddress, 16));
        if (base == 0) return 0;

        uint32_t currentAddress = base;

        if (!rootArr) {
            auto basePtr = readMemoryDword(base, snapshotMemory);
            if (basePtr.has_value() && m_manager->isValidPointerTarget(basePtr.value())) {
                currentAddress = m_manager->unnormalizeAddress(QString::number(basePtr.value(), 16));
            }
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
                auto ptrVal = readMemoryDword(currentAddress, snapshotMemory);
                if (ptrVal.has_value() && m_manager && m_manager->isValidPointerTarget(ptrVal.value())) {
                    currentAddress = m_manager->unnormalizeAddress(QString::number(ptrVal.value(), 16));
                }
                else {
                    return 0;
                }
            }
        }
        return currentAddress;
    }

    void StructureHierarchyWidget::recalculateTreeAddresses(uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
        if (!m_manager || currentBaseAddress == 0) return;

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

            auto valOpt = readMemoryDword(resolved, snapshotMemory);
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
    }

    void StructureHierarchyWidget::collectScopes(std::vector<ScopeCandidate>& candidateScopes, uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
        std::function<void(QTreeWidgetItem*, int)> collect = [&](QTreeWidgetItem* item, int depth) {
            uint32_t derefBase = item->data(0, Qt::UserRole + 4).toUInt();
            QString desc = item->text(0).trimmed();
            static const QRegularExpression arrayTagRx(R"(\[(?:(\d+)\s*x\s*(\d+)|(\d+))\s*bytes\])", QRegularExpression::CaseInsensitiveOption);
            auto match = arrayTagRx.match(desc);

            bool isArray = item->data(0, Qt::UserRole + 6).toBool() || match.hasMatch();
            uint32_t arraySize = item->data(0, Qt::UserRole + 5).toUInt();
            if (arraySize == 0 && match.hasMatch()) {
                if (!match.captured(1).isEmpty() && !match.captured(2).isEmpty()) {
                    arraySize = match.captured(1).toUInt() * match.captured(2).toUInt();
                }
                else if (!match.captured(3).isEmpty()) {
                    arraySize = match.captured(3).toUInt();
                }
            }

            if (derefBase != 0) {
                candidateScopes.push_back({ item, derefBase, depth, isArray, arraySize });
            }
            else if (isArray && arraySize > 0) {
                uint32_t nodeAddr = resolveNodeAddress(item, currentBaseAddress, snapshotMemory, rootArr);
                if (nodeAddr != 0) {
                    candidateScopes.push_back({ item, nodeAddr, depth, true, arraySize });
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
                                                   uint32_t currentBaseAddress,
                                                   bool rootArr,
                                                   const std::vector<uint8_t>& snapshotMemory) {
        if (!m_manager) return;

        QTreeWidgetItem* activeItem = m_structureTree->currentItem();

        std::function<void(QTreeWidgetItem*)> gather = [&](QTreeWidgetItem* item) {
            uint32_t resolved = resolveNodeAddress(item, currentBaseAddress, snapshotMemory, rootArr);
            if (resolved != 0) {
                uint32_t nativeAddr = m_manager->unnormalizeAddress(QString::number(resolved, 16));
                Widgets::MemoryViewerWidget::HighlightInfo info;

                // Extract clean name by removing redundant size/format tag like [32-bit], [Float], etc.
                QString rawDesc = item->text(0).trimmed();
                static const QRegularExpression sizeTagRegex("^\\s*\\[([^\\]]+)\\]\\s*");
                auto match = sizeTagRegex.match(rawDesc);

                uint32_t varSize = 4;
                if (match.hasMatch()) {
                    QString tag = match.captured(1).trimmed().toLower();
                    if (tag.startsWith("8-bit") || tag == "8bit" || tag == "byte" ||
                        tag.startsWith("bit") || tag.contains("bitflag") ||
                        tag == "lower4" || tag == "upper4") {
                        varSize = 1;
                    }
                    else if (tag.startsWith("16-bit") || tag == "16bit" || tag == "short") {
                        varSize = 2;
                    }
                    else if (tag.startsWith("24-bit") || tag == "24bit") {
                        varSize = 3;
                    }
                    else if (tag.startsWith("64-bit") || tag == "64bit" || tag.contains("double")) {
                        varSize = 8;
                    }
                    else if (tag.startsWith("32-bit") || tag == "32bit" || tag.contains("float") || tag.contains("pointer")) {
                        varSize = 4;
                    }
                }

                QString cleanName = rawDesc;
                cleanName.remove(sizeTagRegex);
                cleanName = cleanName.trimmed();
                if (cleanName.isEmpty()) {
                    cleanName = rawDesc;
                }

                info.name = cleanName;
                info.offset = item->text(1).trimmed();
                info.label = cleanName + " (" + info.offset + ")";
                info.startAddress = nativeAddr;
                info.size = varSize;
                info.isPointer = (item->childCount() > 0 || item->text(0).contains("Pointer", Qt::CaseInsensitive));
                info.isChildOfActive = (activeItem && (item == activeItem || item->parent() == activeItem));

                for (uint32_t b = 0; b < varSize; ++b) {
                    highlights[nativeAddr + b] = info;
                }
            }

            // Identify defined array boundaries across the hierarchy
            QString desc = item->text(0).trimmed();
            static const QRegularExpression arrayTagRx(R"(\[(?:(\d+)\s*x\s*(\d+)|(\d+))\s*bytes\])", QRegularExpression::CaseInsensitiveOption);
            auto arrayMatch = arrayTagRx.match(desc);

            bool isArray = item->data(0, Qt::UserRole + 6).toBool() || arrayMatch.hasMatch();
            uint32_t arraySize = item->data(0, Qt::UserRole + 5).toUInt();
            if (arraySize == 0 && arrayMatch.hasMatch()) {
                if (!arrayMatch.captured(1).isEmpty() && !arrayMatch.captured(2).isEmpty()) {
                    arraySize = arrayMatch.captured(1).toUInt() * arrayMatch.captured(2).toUInt();
                }
                else if (!arrayMatch.captured(3).isEmpty()) {
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
                scopeName = node->text(0) + " (" + node->text(1) + ")";
            }
            else if (match.isArrayScope) {
                scopeName = headerComment.isEmpty() ? "Root Array" : headerComment.trimmed();
            }
            else {
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
        }
        else {
            m_offsetPreviewLabel->setText(QString("Selected: 0x%1 | Out of range of known structure scopes.")
                .arg(QString::number(selectedAddr, 16).toUpper()));
        }
    }

    void StructureHierarchyWidget::insertNodeInScope(QTreeWidgetItem* parentScope, int32_t offset, const QString& description, bool isArrayElement) {
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
        QString sign = (offset < 0) ? "-" : "";
        QString offsetHex = QString("%1").arg(std::abs(offset), 2, 16, QChar('0')).toUpper();

        bool parentIsArrayElem = parentScope && parentScope->data(1, Qt::UserRole + 3).toBool();

        // Standardized array offset display (|0x.. without redundant '+')
        QString displayOffset;
        if (isArrayElement) {
            displayOffset = "|" + sign + "0x" + offsetHex;
        }
        else if (parentIsArrayElem) {
            displayOffset = "|+" + sign + "0x" + offsetHex;
        }
        else {
            displayOffset = (offset < 0 ? "-0x" : "+0x") + offsetHex;
        }

        newItem->setText(1, displayOffset);
        newItem->setData(1, Qt::UserRole, offset);
        newItem->setData(1, Qt::UserRole + 1, "="); // Default delimiter
        newItem->setData(1, Qt::UserRole + 3, isArrayElement);

        if (parentScope) {
            parentScope->insertChild(insertIndex, newItem);
            parentScope->setExpanded(true);
        }
        else {
            tree->insertTopLevelItem(insertIndex, newItem);
        }

        tree->setCurrentItem(newItem);
    }

    void StructureHierarchyWidget::removeDocumentationAtAddress(uint32_t address, uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory, bool rootArr) {
        if (!m_manager || address == 0) return;

        QTreeWidgetItem* targetItem = nullptr;
        std::function<void(QTreeWidgetItem*)> findItem = [&](QTreeWidgetItem* item) {
            if (targetItem) return;
            uint32_t res = resolveNodeAddress(item, currentBaseAddress, snapshotMemory, rootArr);
            if (res == address) {
                targetItem = item;
                return;
            }
            for (int i = 0; i < item->childCount(); ++i) {
                findItem(item->child(i));
            }
        };

        for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
            findItem(m_structureTree->topLevelItem(i));
        }

        if (targetItem) {
            delete targetItem;
            emit treeDataChanged();
        }
    }

    QString StructureHierarchyWidget::generateCodeNoteFromTree(const QString& style, bool indentDesc, const QString& headerComment) const {
        std::function<std::vector<std::shared_ptr<CodeNoteNode>>(QTreeWidgetItem*)> treeToNodes =
            [&](QTreeWidgetItem* parent) {
                std::vector<std::shared_ptr<CodeNoteNode>> nodes;
                int count = parent ? parent->childCount() : m_structureTree->topLevelItemCount();

                for (int i = 0; i < count; ++i) {
                    QTreeWidgetItem* item = parent ? parent->child(i) : m_structureTree->topLevelItem(i);
                    int32_t offset = item->data(1, Qt::UserRole).toInt();
                    QString offsetHex = QString("%1").arg(std::abs(offset), 2, 16, QChar('0')).toLower();
                    QString delimiter = item->data(1, Qt::UserRole + 1).toString();
                    if (delimiter.isEmpty()) delimiter = "=";

                    bool isArrayElem = item->data(1, Qt::UserRole + 3).toBool();
                    bool parentIsArrayElem = parent && parent->data(1, Qt::UserRole + 3).toBool();

                    QString offsetStr;
                    if (isArrayElem) {
                        offsetStr = "|" + (offset < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
                    }
                    else if (parentIsArrayElem) {
                        offsetStr = "|+" + (offset < 0 ? QString("-") : QString("")) + "0x" + offsetHex;
                    }
                    else {
                        offsetStr = (offset < 0 ? QString("-") : QString("+")) + "0x" + offsetHex;
                    }

                    auto node = std::make_shared<CodeNoteNode>();
                    node->content = offsetStr + delimiter + item->text(0);

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
        return CodeNoteHierarchyEngine::rebuildTextFromTree(rootNodes, 0, style, indentDesc, headerComment);
    }

    std::optional<PointerPath> StructureHierarchyWidget::getPathForTreeItem(QTreeWidgetItem* item, uint32_t currentBaseAddress, const std::vector<uint8_t>& snapshotMemory, bool rootArr) const {
        if (!item || currentBaseAddress == 0) return std::nullopt;

        std::vector<QTreeWidgetItem*> lineage;
        for (QTreeWidgetItem* curr = item; curr != nullptr; curr = curr->parent()) {
            lineage.push_back(curr);
        }
        std::reverse(lineage.begin(), lineage.end());

        PointerPath path;
        path.baseAddress = currentBaseAddress;
        for (auto* node : lineage) {
            path.offsets.push_back(node->data(1, Qt::UserRole).toInt());
        }
        path.finalAddress = const_cast<StructureHierarchyWidget*>(this)->resolveNodeAddress(item, currentBaseAddress, snapshotMemory, rootArr);
        return path;
    }

    // Double-clicking a tree item: Column 0 edits; Columns 1-3 follow pointer to destination
    void StructureHierarchyWidget::onTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {
        if (!item) return;

        if (column == 0) {
            m_structureTree->editItem(item, 0);
        }
        else {
            emit followPointerClicked();
        }
    }

    void StructureHierarchyWidget::onTreeCustomContextMenu(const QPoint& pos) {
        auto* item = m_structureTree->itemAt(pos);
        if (!item) return;

        QMenu menu(this);

        // Standardized clipboard copy choices matching main results view
        auto* copyRaAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this, &StructureHierarchyWidget::copyRaRequested);
        copyRaAct->setProperty("iconPath", ":/icons/copy_ra.svg");
        copyRaAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));

        auto* copyNoteAct = menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Copy Code Note", this, &StructureHierarchyWidget::copyNoteRequested);
        copyNoteAct->setProperty("iconPath", ":/icons/copy_note.svg");

        auto* copyAddrAct = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Address", this, &StructureHierarchyWidget::copyAddressRequested);
        copyAddrAct->setProperty("iconPath", ":/icons/copy.svg");
        copyAddrAct->setShortcut(QKeySequence::Copy);

        auto* copyValAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Value", this, &StructureHierarchyWidget::copyValueRequested);
        copyValAct->setProperty("iconPath", ":/icons/copy_ra.svg");
        copyValAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));

        menu.addSeparator();

        bool hasPointerVal = !item->text(3).trimmed().isEmpty() && item->text(3) != "N/A" && item->text(3) != "0x00000000";
        if (hasPointerVal) {
            auto* followAct = menu.addAction(getThemeIcon(":/icons/follow_pointer.svg"), "Follow Pointer", this, &StructureHierarchyWidget::followPointerClicked);
            followAct->setProperty("iconPath", ":/icons/follow_pointer.svg");
            menu.addSeparator();
        }

        auto* removeAct = menu.addAction(getThemeIcon(":/icons/delete.svg"), "Remove", this, &StructureHierarchyWidget::removeNodeClicked);
        removeAct->setProperty("iconPath", ":/icons/delete.svg");

        menu.exec(m_structureTree->viewport()->mapToGlobal(pos));
    }

    void StructureHierarchyWidget::onTreeItemChanged(QTreeWidgetItem* item, int column) {
        if (column == 1) {
            QString txt = item->text(1).trimmed();
            QRegularExpression rx("^((?:\\|[\\+\\-]?|[\\+\\-])0x)([0-9a-fA-F]+)");
            auto match = rx.match(txt);
            if (match.hasMatch()) {
                QString prefix = match.captured(1);
                bool isArrayElem = (prefix.startsWith('|') && !prefix.startsWith("|+") && !prefix.startsWith("|-"));
                bool ok = false;
                int32_t val = match.captured(2).toInt(&ok, 16);
                if (prefix.contains('-')) val = -val;
                item->setData(1, Qt::UserRole, val);
                item->setData(1, Qt::UserRole + 3, isArrayElem);
                emit treeDataChanged();
            }
        }
        else if (column == 0) {
            emit treeDataChanged();
        }
    }

}
