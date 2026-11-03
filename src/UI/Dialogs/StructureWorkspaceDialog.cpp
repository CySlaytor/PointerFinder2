#define NOMINMAX
#include "StructureWorkspaceDialog.h"

#include "../../Common/EndianUtils.h"
#include "../../Core/CodeNoteHelper.h"
#include "../../Core/ExportService.h"
#include "../../Models/CodeNoteSettings.h"
#include "../../Models/GlobalSettings.h"
#include "../Styles/ThemeManager.h"

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::DataModels;
    using namespace PointerFinder2::Core;

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
            loadActiveGameData();
            populateAddressDropdown();
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

    void StructureWorkspaceDialog::keyPressEvent(QKeyEvent* event) {
        // Allow Backspace / Alt+Left and Alt+Right navigation history shortcuts anywhere in
        // the workspace, unless the user is actively typing inside a text input field.
        QWidget* fw = focusWidget();
        bool isTextInput = fw && (qobject_cast<QLineEdit*>(fw) || qobject_cast<QPlainTextEdit*>(fw) || qobject_cast<QTextEdit*>(fw));

        if (!isTextInput) {
            if (event->key() == Qt::Key_Backspace || (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_Left)) {
                if (m_memoryViewer && m_memoryViewer->canNavigateBack()) {
                    event->accept();
                    m_memoryViewer->navigateBack();
                    return;
                }
            }
            if (event->modifiers() & Qt::AltModifier && event->key() == Qt::Key_Right) {
                if (m_memoryViewer && m_memoryViewer->canNavigateForward()) {
                    event->accept();
                    m_memoryViewer->navigateForward();
                    return;
                }
            }
        }
        QDialog::keyPressEvent(event);
    }

    void StructureWorkspaceDialog::setupLayout() {
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(8, 8, 8, 8);
        rootLayout->setSpacing(6);

        // Top Toolbar
        auto* topToolbar = new QHBoxLayout();
        topToolbar->setSpacing(6);
        topToolbar->addWidget(new QLabel("Dissect Address:", this));

        m_addressSelectCombo = new QComboBox(this);
        m_addressSelectCombo->setMinimumWidth(280);
        m_addressSelectCombo->setFixedHeight(26);

        // Model and filter proxy setup for real-time dropdown shrinking
        m_addressSourceModel = new QStandardItemModel(this);
        m_addressProxyModel = new AddressFilterProxyModel(this);
        m_addressProxyModel->setSourceModel(m_addressSourceModel);
        m_addressSelectCombo->setModel(m_addressProxyModel);

        // Win32 classic compact dropdown appearance configuration
        m_addressSelectCombo->setMaxVisibleItems(14);
        auto* popupView = new QListView(m_addressSelectCombo);
        popupView->setUniformItemSizes(true);
        popupView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        bool isDark = (GlobalSettings::activeTheme.compare("Light", Qt::CaseInsensitive) != 0);
        popupView->setStyleSheet(QString(
            "QListView {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: 1px solid %3;"
            "    padding: 1px;"
            "    outline: 0;"
            "    font-family: 'Consolas', 'Segoe UI', monospace;"
            "    font-size: 11px;"
            "}"
            "QListView::item {"
            "    height: 19px;"
            "    min-height: 19px;"
            "    padding: 1px 4px;"
            "}"
            "QListView::item:hover {"
            "    background-color: %6;"
            "}"
            "QListView::item:selected {"
            "    background-color: %4;"
            "    color: %5;"
            "}"
        ).arg(isDark ? "#1a1a1a" : "#ffffff")
         .arg(isDark ? "#ffffff" : "#2b2b2b")
         .arg(isDark ? "#383838" : "#c0c0c0")
         .arg(isDark ? "#3b4252" : "#d2def5")
         .arg(isDark ? "#ffffff" : "#1e1e1e")
         .arg(isDark ? "#2c2c2c" : "#f0f0f0"));

        m_addressSelectCombo->setView(popupView);
        topToolbar->addWidget(m_addressSelectCombo);

        m_addressSearchEdit = new QLineEdit(this);
        m_addressSearchEdit->setPlaceholderText("Search note or address...");
        m_addressSearchEdit->setClearButtonEnabled(true);
        m_addressSearchEdit->setFixedWidth(170);
        m_addressSearchEdit->setFixedHeight(26);
        topToolbar->addWidget(m_addressSearchEdit);

        m_newAddressBtn = new QPushButton("New Address...", this);
        m_newAddressBtn->setIcon(getThemeIcon(":/icons/bookmark_add.svg"));
        m_newAddressBtn->setToolTip("Create a new structure or array at a manual address (Ctrl+N)");
        m_newAddressBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
        m_newAddressBtn->setFixedHeight(26);
        m_newAddressBtn->setAutoDefault(false);
        m_newAddressBtn->setDefault(false);
        topToolbar->addWidget(m_newAddressBtn);

        m_deleteAddressBtn = new QPushButton("Remove Address", this);
        m_deleteAddressBtn->setIcon(getThemeIcon(":/icons/delete.svg"));
        m_deleteAddressBtn->setToolTip("Delete this manually created address from the list and User.txt");
        m_deleteAddressBtn->setEnabled(false);
        m_deleteAddressBtn->setFixedHeight(26);
        m_deleteAddressBtn->setAutoDefault(false);
        m_deleteAddressBtn->setDefault(false);
        topToolbar->addWidget(m_deleteAddressBtn);

        // Right-align configuration and panel control buttons
        topToolbar->addStretch();

        m_saveUserBtn = new QPushButton("Save to User.txt", this);
        m_saveUserBtn->setIcon(getThemeIcon(":/icons/save.svg"));
        m_saveUserBtn->setShortcut(QKeySequence::Save);
        m_saveUserBtn->setToolTip("Save active note override to User.txt (Ctrl+S)");
        m_saveUserBtn->setFixedHeight(26);
        m_saveUserBtn->setAutoDefault(false);
        m_saveUserBtn->setDefault(false);
        topToolbar->addWidget(m_saveUserBtn);

        m_revertServerBtn = new QPushButton("Revert to Server", this);
        m_revertServerBtn->setIcon(getThemeIcon(":/icons/undo.svg"));
        m_revertServerBtn->setToolTip("Remove local override from User.txt and restore original server note");
        m_revertServerBtn->setEnabled(false);
        m_revertServerBtn->setFixedHeight(26);
        m_revertServerBtn->setAutoDefault(false);
        m_revertServerBtn->setDefault(false);
        topToolbar->addWidget(m_revertServerBtn);

        m_resetBtn = new QPushButton("Reset", this);
        m_resetBtn->setIcon(getThemeIcon(":/icons/refresh.svg"));
        m_resetBtn->setToolTip("Reload Notes.json and User.txt from disk if modified externally");
        m_resetBtn->setFixedHeight(26);
        m_resetBtn->setAutoDefault(false);
        m_resetBtn->setDefault(false);
        topToolbar->addWidget(m_resetBtn);

        m_toggleCodeNoteBtn = new QPushButton("Code Note Panel", this);
        m_toggleCodeNoteBtn->setCheckable(true);
        m_toggleCodeNoteBtn->setChecked(true);
        m_toggleCodeNoteBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
        m_toggleCodeNoteBtn->setFixedHeight(26);
        m_toggleCodeNoteBtn->setAutoDefault(false);
        m_toggleCodeNoteBtn->setDefault(false);
        topToolbar->addWidget(m_toggleCodeNoteBtn);

        rootLayout->addLayout(topToolbar);

        m_mainSplitter = new QSplitter(Qt::Horizontal, this);

        // --- Left Panel: Structure Editor ---
        m_leftContainer = new QWidget(m_mainSplitter);
        auto* leftLayout = new QVBoxLayout(m_leftContainer);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(6);

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

        // Setup actions and keyboard shortcuts for Structure Hierarchy
        m_treeCopyRaAction = new QAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this);
        m_treeCopyRaAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        m_treeCopyRaAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_treeCopyRaAction, &QAction::triggered, this, &StructureWorkspaceDialog::onCopySelectedAsRaFormat);

        m_treeCopyNoteAction = new QAction(getThemeIcon(":/icons/copy_note.svg"), "Copy Code Note", this);
        connect(m_treeCopyNoteAction, &QAction::triggered, this, &StructureWorkspaceDialog::onCopySelectedAsCodeNote);

        m_treeCopyAddressAction = new QAction(getThemeIcon(":/icons/copy.svg"), "Copy Address", this);
        m_treeCopyAddressAction->setShortcut(QKeySequence::Copy);
        m_treeCopyAddressAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_treeCopyAddressAction, &QAction::triggered, this, &StructureWorkspaceDialog::onCopySelectedAddress);

        m_treeCopyValueAction = new QAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Value", this);
        m_treeCopyValueAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
        m_treeCopyValueAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_treeCopyValueAction, &QAction::triggered, this, &StructureWorkspaceDialog::onCopySelectedValue);

        m_structureTree->addAction(m_treeCopyRaAction);
        m_structureTree->addAction(m_treeCopyNoteAction);
        m_structureTree->addAction(m_treeCopyAddressAction);
        m_structureTree->addAction(m_treeCopyValueAction);

        leftLayout->addWidget(m_structureTree, 1);

        m_offsetPreviewLabel = new QLabel("Select an address in Memory Viewer to calculate relative offset.", m_leftContainer);
        m_offsetPreviewLabel->setStyleSheet("font-weight: bold; color: #58A6FF; padding: 4px; border: 1px solid #30363D; border-radius: 4px;");
        leftLayout->addWidget(m_offsetPreviewLabel);

        auto* treeButtonsRow = new QHBoxLayout();
        treeButtonsRow->setSpacing(6);

        m_addVariableBtn = new QPushButton("Add Variable", m_leftContainer);
        m_addVariableBtn->setToolTip("Add variable at selected address (Shortcut: V)");
        m_addVariableBtn->setIcon(getThemeIcon(":/icons/bookmark_add.svg"));
        m_addVariableBtn->setFixedHeight(26);
        m_addVariableBtn->setAutoDefault(false);
        m_addVariableBtn->setDefault(false);

        m_addPointerBtn = new QPushButton("Add as Pointer", m_leftContainer);
        m_addPointerBtn->setToolTip("Add pointer at selected address (Shortcut: P)");
        m_addPointerBtn->setIcon(getThemeIcon(":/icons/attach.svg"));
        m_addPointerBtn->setFixedHeight(26);
        m_addPointerBtn->setAutoDefault(false);
        m_addPointerBtn->setDefault(false);

        m_followPointerBtn = new QPushButton("Follow Pointer", m_leftContainer);
        m_followPointerBtn->setToolTip("Follow pointer value to destination address (Shortcut: Enter, F, or Double-click)");
        m_followPointerBtn->setIcon(getThemeIcon(":/icons/follow_pointer.svg"));
        m_followPointerBtn->setFixedHeight(26);
        m_followPointerBtn->setAutoDefault(false);
        m_followPointerBtn->setDefault(false);

        m_removeNodeBtn = new QPushButton("Remove", m_leftContainer);
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

        m_mainSplitter->addWidget(m_leftContainer);

        // --- Right Panel: Memory Viewer ---
        m_middleContainer = new QWidget(m_mainSplitter);
        auto* middleLayout = new QVBoxLayout(m_middleContainer);
        middleLayout->setContentsMargins(0, 0, 0, 0);
        middleLayout->setSpacing(6);

        // Top Header Row for Memory Viewer: Re-dump / Refresh RAM and Snapshot Info
        auto* viewerTopRow = new QHBoxLayout();
        viewerTopRow->setContentsMargins(0, 0, 0, 0);
        viewerTopRow->setSpacing(6);

        m_refreshRamBtn = new QPushButton("Re-dump / Refresh RAM", m_middleContainer);
        m_refreshRamBtn->setIcon(getThemeIcon(":/icons/refresh.svg"));
        m_refreshRamBtn->setFixedHeight(26);
        m_refreshRamBtn->setAutoDefault(false);
        m_refreshRamBtn->setDefault(false);
        viewerTopRow->addWidget(m_refreshRamBtn);

        m_ramStatusLabel = new QLabel("No dump loaded.", m_middleContainer);
        m_ramStatusLabel->setStyleSheet("color: #8B949E; font-size: 11px;");
        viewerTopRow->addWidget(m_ramStatusLabel);
        viewerTopRow->addStretch();

        middleLayout->addLayout(viewerTopRow);

        // Instantiate Memory Viewer widget
        m_memoryViewer = new Widgets::MemoryViewerWidget(m_middleContainer);
        middleLayout->addWidget(m_memoryViewer, 1);

        // Memory View Mode Selector (positioned at bottom)
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

        // Memory Controls Bar (positioned at bottom)
        auto* viewerBottomRow = new QHBoxLayout();
        viewerBottomRow->setSpacing(6);

        m_navBackBtn = new QPushButton(this);
        m_navBackBtn->setIcon(getThemeIcon(":/icons/undo.svg"));
        m_navBackBtn->setToolTip("Navigate Back (Backspace / Alt+Left)");
        m_navBackBtn->setFixedHeight(26);
        m_navBackBtn->setFixedWidth(28);
        m_navBackBtn->setEnabled(false);
        m_navBackBtn->setAutoDefault(false);
        m_navBackBtn->setDefault(false);
        viewerBottomRow->addWidget(m_navBackBtn);

        m_navForwardBtn = new QPushButton(this);
        m_navForwardBtn->setIcon(getThemeIcon(":/icons/redo.svg"));
        m_navForwardBtn->setToolTip("Navigate Forward (Alt+Right)");
        m_navForwardBtn->setFixedHeight(26);
        m_navForwardBtn->setFixedWidth(28);
        m_navForwardBtn->setEnabled(false);
        m_navForwardBtn->setAutoDefault(false);
        m_navForwardBtn->setDefault(false);
        viewerBottomRow->addWidget(m_navForwardBtn);

        m_navHistoryLabel = new QLabel("0/0", m_middleContainer);
        m_navHistoryLabel->setObjectName("navHistoryLabel");
        m_navHistoryLabel->setStyleSheet("color: #8B949E; font-size: 11px; font-weight: bold; padding: 0 4px;");
        viewerBottomRow->addWidget(m_navHistoryLabel);

        viewerBottomRow->addStretch();

        viewerBottomRow->addWidget(new QLabel("Go to (Hex):", m_middleContainer));
        m_goToAddressEdit = new QLineEdit(m_middleContainer);
        m_goToAddressEdit->setMinimumWidth(90);
        m_goToAddressEdit->setMaximumWidth(120);
        m_goToAddressEdit->setFixedHeight(26);
        m_goToAddressBtn = new QPushButton("Go", m_middleContainer);
        m_goToAddressBtn->setIcon(getThemeIcon(":/icons/follow_pointer.svg"));
        m_goToAddressBtn->setFixedHeight(26);
        m_goToAddressBtn->setAutoDefault(false);
        m_goToAddressBtn->setDefault(false);
        viewerBottomRow->addWidget(m_goToAddressEdit);
        viewerBottomRow->addWidget(m_goToAddressBtn);

        middleLayout->addLayout(viewerBottomRow);

        m_mainSplitter->addWidget(m_middleContainer);

        // --- Panel 3: Code Note Hierarchy Fixer ---
        m_rightContainer = new QWidget(m_mainSplitter);
        auto* rightLayout = new QVBoxLayout(m_rightContainer);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(6);

        m_codeNoteTextEdit = new QPlainTextEdit(m_rightContainer);
        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        m_codeNoteTextEdit->setFont(monoFont);
        m_codeNoteTextEdit->setPlaceholderText("Code Note displays here. You can paste raw pointer chains or edit lines directly, then click 'Fix Hierarchy' to merge...");
        rightLayout->addWidget(m_codeNoteTextEdit, 1);

        // Hierarchy Controls Bar (positioned at bottom)
        auto* fixerBottomRow = new QHBoxLayout();
        fixerBottomRow->addWidget(new QLabel("Style:", m_rightContainer));

        m_styleCombo = new QComboBox(m_rightContainer);
        m_styleCombo->addItems({
            "Plusses (+)",
            "Dots (.)",
            "Unicode Tree (├─)"
        });
        m_styleCombo->setFixedHeight(26);
        fixerBottomRow->addWidget(m_styleCombo);

        m_indentDescCheck = new QCheckBox("Indent Descriptions", m_rightContainer);
        m_indentDescCheck->setChecked(true);
        fixerBottomRow->addWidget(m_indentDescCheck);

        m_fixHierarchyBtn = new QPushButton("Fix Hierarchy", m_rightContainer);
        m_fixHierarchyBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
        m_fixHierarchyBtn->setFixedHeight(26);
        m_fixHierarchyBtn->setAutoDefault(false);
        m_fixHierarchyBtn->setDefault(false);
        fixerBottomRow->addWidget(m_fixHierarchyBtn);

        fixerBottomRow->addStretch();

        m_copyCodeNoteBtn = new QPushButton("Copy", m_rightContainer);
        m_copyCodeNoteBtn->setIcon(getThemeIcon(":/icons/copy.svg"));
        m_copyCodeNoteBtn->setFixedHeight(26);
        m_copyCodeNoteBtn->setAutoDefault(false);
        m_copyCodeNoteBtn->setDefault(false);
        fixerBottomRow->addWidget(m_copyCodeNoteBtn);

        rightLayout->addLayout(fixerBottomRow);

        m_mainSplitter->addWidget(m_rightContainer);

        m_mainSplitter->setCollapsible(0, false);
        m_mainSplitter->setCollapsible(1, false);
        m_mainSplitter->setCollapsible(2, true);

        // Prioritize horizontal space for Structure Hierarchy while keeping Memory Viewer sized to content
        m_mainSplitter->setStretchFactor(0, 1);
        m_mainSplitter->setStretchFactor(1, 0);
        m_mainSplitter->setStretchFactor(2, 0);
        rootLayout->addWidget(m_mainSplitter, 1);

        // Connections
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addressSelected, this, &StructureWorkspaceDialog::onAddressSelectedInViewer);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addressDoubleClicked, this, &StructureWorkspaceDialog::onAddressDoubleClickedInViewer);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::contextMenuRequested, this, &StructureWorkspaceDialog::onMemoryViewerContextMenu);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::defineArrayRequested, this, &StructureWorkspaceDialog::onDefineArrayRequested);
        connect(m_structureTree, &QTreeWidget::itemDoubleClicked, this, &StructureWorkspaceDialog::onStructureTreeItemDoubleClicked);
        connect(m_structureTree, &QTreeWidget::customContextMenuRequested, this, &StructureWorkspaceDialog::onStructureTreeCustomContextMenu);

        // Connect navigation history controls
        connect(m_navBackBtn, &QPushButton::clicked, m_memoryViewer, &Widgets::MemoryViewerWidget::navigateBack);
        connect(m_navForwardBtn, &QPushButton::clicked, m_memoryViewer, &Widgets::MemoryViewerWidget::navigateForward);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::navigationHistoryChanged, this,
            [this](bool canBack, bool canForward, int currentIndex, int totalCount) {
                m_navBackBtn->setEnabled(canBack);
                m_navForwardBtn->setEnabled(canForward);
                if (totalCount > 0) {
                    m_navHistoryLabel->setText(QString("%1/%2").arg(currentIndex).arg(totalCount));
                }
                else {
                    m_navHistoryLabel->setText("0/0");
                }
            });

        // Connect focus-scoped memory viewer shortcut signals
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addVariableTriggered, this, &StructureWorkspaceDialog::onAddVariableClicked);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addPointerTriggered, this, &StructureWorkspaceDialog::onAddPointerClicked);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::followPointerTriggered, this, &StructureWorkspaceDialog::onFollowPointerClicked);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::removeDocumentationTriggered, this, &StructureWorkspaceDialog::onRemoveDocumentationAtAddress);

        // Connect clipboard copy signals from Memory Viewer
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyAddressTriggered, this, &StructureWorkspaceDialog::onCopySelectedAddress);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyValueTriggered, this, &StructureWorkspaceDialog::onCopySelectedValue);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyRaTriggered, this, &StructureWorkspaceDialog::onCopySelectedAsRaFormat);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyCodeNoteTriggered, this, &StructureWorkspaceDialog::onCopySelectedAsCodeNote);

        connect(m_addVariableBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onAddVariableClicked);
        connect(m_addPointerBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onAddPointerClicked);
        connect(m_followPointerBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onFollowPointerClicked);
        connect(m_removeNodeBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onRemoveNodeClicked);

        connect(m_structureTree, &QTreeWidget::itemChanged, this, &StructureWorkspaceDialog::onStructureTreeItemChanged);
        connect(m_structureTree, &QTreeWidget::itemSelectionChanged, this, &StructureWorkspaceDialog::onStructureTreeSelectionChanged);
        connect(m_refreshRamBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onRefreshRamClicked);
        connect(m_goToAddressBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onGoToAddressClicked);
        connect(m_goToAddressEdit, &QLineEdit::returnPressed, this, &StructureWorkspaceDialog::onGoToAddressClicked);

        connect(m_viewModeGroup, &QButtonGroup::idClicked, this, &StructureWorkspaceDialog::onViewModeChanged);

        // RA Data controls
        connect(m_newAddressBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onNewAddressClicked);
        connect(m_deleteAddressBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onDeleteAddressClicked);
        connect(m_addressSelectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StructureWorkspaceDialog::onAddressComboChanged);
        connect(m_addressSearchEdit, &QLineEdit::textChanged, this, &StructureWorkspaceDialog::onAddressSearchTextChanged);
        connect(m_saveUserBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onSaveUserTxtClicked);
        connect(m_revertServerBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onRevertServerClicked);
        connect(m_resetBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onResetClicked);

        // Code note sync & panel connections
        connect(m_toggleCodeNoteBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onToggleCodeNotePanelClicked);
        connect(m_mainSplitter, &QSplitter::splitterMoved, this, &StructureWorkspaceDialog::onSplitterMoved);
        connect(m_styleCombo, &QComboBox::currentTextChanged, this, &StructureWorkspaceDialog::onHierarchyStyleChanged);
        connect(m_indentDescCheck, &QCheckBox::checkStateChanged, this, &StructureWorkspaceDialog::onHierarchyStyleChanged);
        connect(m_fixHierarchyBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onFixHierarchyClicked);
        connect(m_copyCodeNoteBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onCopyCodeNoteClicked);
    }

    bool StructureWorkspaceDialog::loadActiveGameData() {
        if (!m_manager || !m_manager->isAttached()) return false;
        return RaStorageService::loadActiveGameData(m_manager->getProcessHandle(), m_gameData);
    }

    void StructureWorkspaceDialog::populateAddressDropdown(bool autoNavigate) {
        m_addressSelectCombo->blockSignals(true);
        m_addressSourceModel->clear();

        if (!m_gameData.isValid || m_gameData.notes.empty()) {
            auto* emptyItem = new QStandardItem("No active game code notes detected.");
            m_addressSourceModel->appendRow(emptyItem);
            m_addressSelectCombo->blockSignals(false);
            return;
        }

        // Sort by address
        std::sort(m_gameData.notes.begin(), m_gameData.notes.end(), [](const RaCodeNote& a, const RaCodeNote& b) {
            return a.address < b.address;
        });

        int selectSourceRow = 0;

        for (size_t i = 0; i < m_gameData.notes.size(); ++i) {
            const auto& n = m_gameData.notes[i];
            QString tag = n.isLocalOnly ? "(Local)" : (n.isModified ? "(Modified)" : "(Server)");
            QString itemText = QString("0x%1 - %2 %3")
                .arg(QString::number(n.address, 16).toUpper().rightJustified(8, '0'))
                .arg(n.title)
                .arg(tag);

            auto* item = new QStandardItem(itemText);
            item->setData(n.address, Qt::UserRole);
            m_addressSourceModel->appendRow(item);

            if (m_currentBaseAddress != 0 && n.address == m_currentBaseAddress) {
                selectSourceRow = static_cast<int>(i);
            }
        }

        m_addressProxyModel->setSearchFilter(m_addressSearchEdit ? m_addressSearchEdit->text() : "");

        QModelIndex srcIndex = m_addressSourceModel->index(selectSourceRow, 0);
        QModelIndex proxyIndex = m_addressProxyModel->mapFromSource(srcIndex);
        if (proxyIndex.isValid()) {
            m_addressSelectCombo->setCurrentIndex(proxyIndex.row());
        }
        else if (m_addressProxyModel->rowCount() > 0) {
            m_addressSelectCombo->setCurrentIndex(0);
        }

        m_addressSelectCombo->blockSignals(false);

        if (m_addressSelectCombo->count() > 0 && m_currentBaseAddress != 0) {
            selectAddressInUI(m_currentBaseAddress, autoNavigate);
        }
        else if (m_addressSelectCombo->count() > 0) {
            QVariant comboVal = m_addressSelectCombo->currentData();
            if (comboVal.isValid()) {
                selectAddressInUI(comboVal.toUInt(), autoNavigate);
            }
        }
    }

    void StructureWorkspaceDialog::onNewAddressClicked() {
        uint32_t defaultAddr = (m_currentViewerSelectedAddress != 0) ? m_currentViewerSelectedAddress : m_currentBaseAddress;
        NewAddressDialog dialog(m_manager, defaultAddr, this);

        if (dialog.exec() == QDialog::Accepted) {
            uint32_t address = dialog.getAddress();
            if (address == 0) return;

            QString initialNote = dialog.getInitialCodeNote();

            // Check if address already exists in code notes
            auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
                return n.address == address;
            });

            if (it != m_gameData.notes.end()) {
                auto reply = QMessageBox::question(
                    this,
                    "Address Exists",
                    QString("A code note for address 0x%1 already exists (%2).\n\nWould you like to switch to it?")
                        .arg(QString::number(address, 16).toUpper().rightJustified(8, '0'), it->title),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes
                );
                if (reply == QMessageBox::Yes) {
                    selectAddressInUI(address, true);
                }
                return;
            }

            RaCodeNote newNote;
            newNote.address = address;
            newNote.addressHex = QString("0x%1").arg(address, 8, 16, QChar('0')).toUpper();
            newNote.noteText = initialNote;
            newNote.title = RaStorageService::extractTitleFromNote(initialNote);
            newNote.author = "Local";
            newNote.isModified = false;
            newNote.isLocalOnly = true;

            m_gameData.notes.push_back(newNote);

            populateAddressDropdown(true);
            selectAddressInUI(address, true);
        }
    }

    void StructureWorkspaceDialog::onDeleteAddressClicked() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0) return;

        QString addrStr = m_manager ? m_manager->formatDisplayAddress(activeAddr) : QString::number(activeAddr, 16).toUpper();

        auto reply = QMessageBox::question(
            this,
            "Remove Address",
            QString("Are you sure you want to remove address 0x%1 from the address list and User.txt?").arg(addrStr),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (reply != QMessageBox::Yes) return;

        RaStorageService::deleteLocalNote(m_gameData, activeAddr);

        m_currentBaseAddress = 0;
        populateAddressDropdown(true);

        if (m_addressSelectCombo->count() > 0) {
            QVariant val = m_addressSelectCombo->currentData();
            if (val.isValid()) {
                selectAddressInUI(val.toUInt(), true);
            }
        }
        else {
            m_structureTree->blockSignals(true);
            m_structureTree->clear();
            m_structureTree->blockSignals(false);
            m_codeNoteTextEdit->clear();
            m_offsetPreviewLabel->setText("Select an address in Memory Viewer to calculate relative offset.");
            updateToolbarButtonStates();
        }
    }

    void StructureWorkspaceDialog::onAddressSearchTextChanged(const QString& text) {
        m_addressSelectCombo->blockSignals(true);
        m_addressProxyModel->setSearchFilter(text);
        m_addressSelectCombo->blockSignals(false);

        if (m_addressProxyModel->rowCount() > 0) {
            // Retain active address selection if it is present within filtered results
            bool currentStillVisible = false;
            for (int i = 0; i < m_addressProxyModel->rowCount(); ++i) {
                QModelIndex proxyIdx = m_addressProxyModel->index(i, 0);
                if (proxyIdx.data(Qt::UserRole).toUInt() == m_currentBaseAddress) {
                    m_addressSelectCombo->setCurrentIndex(i);
                    currentStillVisible = true;
                    break;
                }
            }
            if (!currentStillVisible) {
                m_addressSelectCombo->setCurrentIndex(0);
                QVariant matchedVal = m_addressSelectCombo->currentData();
                if (matchedVal.isValid()) {
                    selectAddressInUI(matchedVal.toUInt(), true);
                }
            }
        }
    }

    void StructureWorkspaceDialog::selectAddressInUI(uint32_t address, bool navigate) {
        auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
            return n.address == address;
        });

        if (it == m_gameData.notes.end()) return;

        m_currentBaseAddress = address;

        m_isSyncing = true;
        m_codeNoteTextEdit->setPlainText(it->noteText);
        m_isSyncing = false;

        // Populate tree non-destructively without altering or rewriting original user note text
        populateTreeFromCodeNote(it->noteText);
        if (navigate) {
            navigateToAddress(address);
        }
        updateToolbarButtonStates();
    }

    void StructureWorkspaceDialog::populateTreeFromCodeNote(const QString& noteText) {
        QString text = noteText.trimmed();
        if (text.isEmpty()) {
            m_structureTree->blockSignals(true);
            m_structureTree->clear();
            m_structureTree->blockSignals(false);
            m_headerComment.clear();
            return;
        }

        m_headerComment.clear();
        QStringList lines = text.split(QRegularExpression("[\r\n]+"));
        auto parsedTree = CodeNoteHierarchyEngine::parseLinesToTree(lines, m_headerComment);
        auto mergedTree = CodeNoteHierarchyEngine::mergeTree(parsedTree);

        // Block tree signals to prevent redundant settings.ini and sync updates during population
        m_structureTree->blockSignals(true);
        m_structureTree->clear();
        populateTreeFromHierarchyNodes(mergedTree);

        m_structureTree->expandAll();
        recalculateTreeAddresses();
        updateOffsetPreview();
        m_structureTree->blockSignals(false);
    }

    void StructureWorkspaceDialog::onAddressComboChanged(int index) {
        if (index < 0 || index >= m_addressSelectCombo->count()) return;
        QVariant comboData = m_addressSelectCombo->itemData(index);
        if (!comboData.isValid()) return;

        uint32_t addr = comboData.toUInt();
        selectAddressInUI(addr, true);
    }

    bool StructureWorkspaceDialog::saveActiveNoteToUserTxt() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0) return false;

        // Capture current memory viewer location and cursor to prevent jumping
        uint32_t savedCursor = (m_currentViewerSelectedAddress != 0) ? m_currentViewerSelectedAddress : m_memoryViewer->getSelectedAddress();
        int savedScroll = m_memoryViewer->verticalScrollBar()->value();

        QString currentNote = m_codeNoteTextEdit->toPlainText();
        bool ok = RaStorageService::saveActiveNoteToUserTxt(m_gameData, activeAddr, currentNote);
        if (ok) {
            populateAddressDropdown(false);
            updateToolbarButtonStates();

            // Restore exact cursor and scroll state in Memory Viewer
            if (savedCursor != 0) {
                m_memoryViewer->setSelectedAddress(savedCursor, false, false);
                m_memoryViewer->verticalScrollBar()->setValue(savedScroll);
            }
        }
        return ok;
    }

    void StructureWorkspaceDialog::onSaveUserTxtClicked() {
        if (saveActiveNoteToUserTxt()) {
            QString origText = m_saveUserBtn->text();
            m_saveUserBtn->setText("✓ Saved!");
            m_saveUserBtn->setEnabled(false);
            QTimer::singleShot(1500, this, [this, origText]() {
                m_saveUserBtn->setText(origText);
                m_saveUserBtn->setEnabled(true);
            });
        }
    }

    void StructureWorkspaceDialog::onRevertServerClicked() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0) return;

        // Capture current memory viewer location and cursor to prevent jumping
        uint32_t savedCursor = (m_currentViewerSelectedAddress != 0) ? m_currentViewerSelectedAddress : m_memoryViewer->getSelectedAddress();
        int savedScroll = m_memoryViewer->verticalScrollBar()->value();

        if (RaStorageService::revertServerNote(m_gameData, activeAddr)) {
            loadActiveGameData();
            populateAddressDropdown(false);
            selectAddressInUI(activeAddr, false);

            // Restore exact cursor and scroll state in Memory Viewer
            if (savedCursor != 0) {
                m_memoryViewer->setSelectedAddress(savedCursor, false, false);
                m_memoryViewer->verticalScrollBar()->setValue(savedScroll);
            }
        }
    }

    void StructureWorkspaceDialog::onResetClicked() {
        uint32_t currentAddr = m_currentBaseAddress;

        // Capture current memory viewer location and cursor to prevent jumping
        uint32_t savedCursor = (m_currentViewerSelectedAddress != 0) ? m_currentViewerSelectedAddress : m_memoryViewer->getSelectedAddress();
        int savedScroll = m_memoryViewer->verticalScrollBar()->value();

        loadActiveGameData();
        populateAddressDropdown(false);
        if (currentAddr != 0) {
            selectAddressInUI(currentAddr, false);

            // Restore exact cursor and scroll state in Memory Viewer
            if (savedCursor != 0) {
                m_memoryViewer->setSelectedAddress(savedCursor, false, false);
                m_memoryViewer->verticalScrollBar()->setValue(savedScroll);
            }
        }

        QString origText = m_resetBtn->text();
        m_resetBtn->setText("✓ Refreshed!");
        m_resetBtn->setEnabled(false);
        QTimer::singleShot(1500, this, [this, origText]() {
            m_resetBtn->setText(origText);
            m_resetBtn->setEnabled(true);
        });
    }

    void StructureWorkspaceDialog::updateToolbarButtonStates() {
        uint32_t activeAddr = m_currentBaseAddress;

        bool canRevert = false;
        bool canDelete = false;

        if (activeAddr != 0 && m_gameData.isValid) {
            auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
                return n.address == activeAddr;
            });
            if (it != m_gameData.notes.end()) {
                if (it->isModified) {
                    canRevert = true;
                }
                if (it->isLocalOnly) {
                    canDelete = true;
                }
            }
        }
        m_revertServerBtn->setEnabled(canRevert);
        m_deleteAddressBtn->setEnabled(canDelete);
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

            m_memoryViewer->setSelectedAddress(nativeAddr, true, true);
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
        updateMemoryHighlights();
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

    std::optional<DataModels::PointerPath> StructureWorkspaceDialog::getPathForTreeItem(QTreeWidgetItem* item) const {
        if (!item || m_currentBaseAddress == 0) return std::nullopt;

        std::vector<QTreeWidgetItem*> lineage;
        for (QTreeWidgetItem* curr = item; curr != nullptr; curr = curr->parent()) {
            lineage.push_back(curr);
        }
        std::reverse(lineage.begin(), lineage.end());

        DataModels::PointerPath path;
        path.baseAddress = m_currentBaseAddress;
        for (auto* node : lineage) {
            path.offsets.push_back(node->data(1, Qt::UserRole).toInt());
        }
        path.finalAddress = const_cast<StructureWorkspaceDialog*>(this)->resolveNodeAddress(item);
        return path;
    }

    std::optional<DataModels::PointerPath> StructureWorkspaceDialog::getPathForViewerAddress(uint32_t address) const {
        if (address == 0 || m_currentBaseAddress == 0 || !m_manager) return std::nullopt;

        auto match = const_cast<StructureWorkspaceDialog*>(this)->findBestScopeForAddress(address);
        if (!match.isValid) return std::nullopt;

        DataModels::PointerPath path;
        path.baseAddress = m_currentBaseAddress;

        if (match.pointerNode) {
            std::vector<QTreeWidgetItem*> lineage;
            for (QTreeWidgetItem* curr = match.pointerNode; curr != nullptr; curr = curr->parent()) {
                lineage.push_back(curr);
            }
            std::reverse(lineage.begin(), lineage.end());

            for (auto* node : lineage) {
                path.offsets.push_back(node->data(1, Qt::UserRole).toInt());
            }
        }

        path.offsets.push_back(match.offset);
        path.finalAddress = address;
        return path;
    }

    void StructureWorkspaceDialog::onCopySelectedAsRaFormat() {
        std::optional<PointerPath> pathOpt;
        if (m_structureTree && m_structureTree->hasFocus() && m_structureTree->currentItem()) {
            pathOpt = getPathForTreeItem(m_structureTree->currentItem());
        }
        else if (m_currentViewerSelectedAddress != 0) {
            pathOpt = getPathForViewerAddress(m_currentViewerSelectedAddress);
        }
        else if (m_structureTree && m_structureTree->currentItem()) {
            pathOpt = getPathForTreeItem(m_structureTree->currentItem());
        }

        if (pathOpt.has_value() && m_manager) {
            ExportService::copyAsRetroAchievementsFormat(pathOpt.value(), m_manager);
        }
    }

    void StructureWorkspaceDialog::onCopySelectedAsCodeNote() {
        std::optional<PointerPath> pathOpt;
        if (m_structureTree && m_structureTree->hasFocus() && m_structureTree->currentItem()) {
            pathOpt = getPathForTreeItem(m_structureTree->currentItem());
        }
        else if (m_currentViewerSelectedAddress != 0) {
            pathOpt = getPathForViewerAddress(m_currentViewerSelectedAddress);
        }
        else if (m_structureTree && m_structureTree->currentItem()) {
            pathOpt = getPathForTreeItem(m_structureTree->currentItem());
        }

        if (pathOpt.has_value() && m_manager) {
            ExportService::copyAsCodeNote(pathOpt.value(), m_manager);
        }
    }

    void StructureWorkspaceDialog::onCopySelectedAddress() {
        uint32_t addr = 0;
        if (m_structureTree && m_structureTree->hasFocus() && m_structureTree->currentItem()) {
            addr = resolveNodeAddress(m_structureTree->currentItem());
        }
        else if (m_currentViewerSelectedAddress != 0) {
            addr = m_currentViewerSelectedAddress;
        }
        else if (m_structureTree && m_structureTree->currentItem()) {
            addr = resolveNodeAddress(m_structureTree->currentItem());
        }

        if (addr != 0 && m_manager) {
            QString addrStr = m_manager->formatDisplayAddress(addr);
            QApplication::clipboard()->setText("0x" + addrStr);
        }
    }

    void StructureWorkspaceDialog::onCopySelectedValue() {
        uint32_t addr = 0;
        if (m_structureTree && m_structureTree->hasFocus() && m_structureTree->currentItem()) {
            addr = resolveNodeAddress(m_structureTree->currentItem());
        }
        else if (m_currentViewerSelectedAddress != 0) {
            addr = m_currentViewerSelectedAddress;
        }
        else if (m_structureTree && m_structureTree->currentItem()) {
            addr = resolveNodeAddress(m_structureTree->currentItem());
        }

        if (addr != 0 && m_manager) {
            uint32_t dwordVal = m_memoryViewer ? m_memoryViewer->readDwordAt(addr) : 0;
            if (dwordVal == 0) {
                auto valOpt = readMemoryDword(addr);
                if (valOpt.has_value()) dwordVal = valOpt.value();
            }
            QApplication::clipboard()->setText("0x" + QString("%1").arg(dwordVal, 8, 16, QChar('0')).toUpper());
        }
    }

    void StructureWorkspaceDialog::onMemoryViewerContextMenu(uint32_t address, const QPoint& globalPos) {
        m_currentViewerSelectedAddress = address;
        updateOffsetPreview();

        QMenu menu(this);

        auto* addVarAct = menu.addAction(getThemeIcon(":/icons/bookmark_add.svg"), "Add Variable", this, &StructureWorkspaceDialog::onAddVariableClicked);
        addVarAct->setProperty("iconPath", ":/icons/bookmark_add.svg");

        auto* addPtrAct = menu.addAction(getThemeIcon(":/icons/attach.svg"), "Add as Pointer", this, &StructureWorkspaceDialog::onAddPointerClicked);
        addPtrAct->setProperty("iconPath", ":/icons/attach.svg");

        uint32_t dwordVal = m_memoryViewer->readDwordAt(address);
        bool isPointer = (dwordVal != 0 && m_manager && m_manager->isValidPointerTarget(dwordVal));

        auto* followPtrAct = menu.addAction(getThemeIcon(":/icons/follow_pointer.svg"), "Follow Pointer", this, [this, dwordVal]() {
            if (m_manager) {
                uint32_t nativeTarget = m_manager->unnormalizeAddress(QString::number(dwordVal, 16));
                navigateToAddress(nativeTarget);
            }
        });
        followPtrAct->setProperty("iconPath", ":/icons/follow_pointer.svg");
        followPtrAct->setEnabled(isPointer);

        if (m_memoryViewer->isAddressDocumented(address)) {
            menu.addSeparator();
            auto* removeDocAct = menu.addAction(getThemeIcon(":/icons/delete.svg"), "Remove Documentation", this, [this, address]() {
                onRemoveDocumentationAtAddress(address);
            });
            removeDocAct->setProperty("iconPath", ":/icons/delete.svg");
        }

        menu.addSeparator();

        // Standardized clipboard copy choices consistent with main results view
        auto* copyRaAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this, &StructureWorkspaceDialog::onCopySelectedAsRaFormat);
        copyRaAct->setProperty("iconPath", ":/icons/copy_ra.svg");
        copyRaAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        copyRaAct->setEnabled(getPathForViewerAddress(address).has_value());

        auto* copyNoteAct = menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Copy Code Note", this, &StructureWorkspaceDialog::onCopySelectedAsCodeNote);
        copyNoteAct->setProperty("iconPath", ":/icons/copy_note.svg");
        copyNoteAct->setEnabled(getPathForViewerAddress(address).has_value());

        auto* copyAddrAct = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Address", this, &StructureWorkspaceDialog::onCopySelectedAddress);
        copyAddrAct->setProperty("iconPath", ":/icons/copy.svg");
        copyAddrAct->setShortcut(QKeySequence::Copy);

        auto* copyValAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Value", this, &StructureWorkspaceDialog::onCopySelectedValue);
        copyValAct->setProperty("iconPath", ":/icons/copy_ra.svg");
        copyValAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));

        menu.addSeparator();

        QString displayAddrStr = m_manager ? m_manager->formatDisplayAddress(address) : QString("%1").arg(address, 8, 16, QChar('0')).toUpper();

        auto* newAddrAct = menu.addAction(getThemeIcon(":/icons/bookmark_add.svg"), QString("Create New Structure at 0x%1...").arg(displayAddrStr), this, [this, address]() {
            NewAddressDialog dialog(m_manager, address, this);
            if (dialog.exec() == QDialog::Accepted) {
                uint32_t newAddr = dialog.getAddress();
                if (newAddr == 0) return;
                QString initialNote = dialog.getInitialCodeNote();

                RaCodeNote newNote;
                newNote.address = newAddr;
                newNote.addressHex = QString("0x%1").arg(newAddr, 8, 16, QChar('0')).toUpper();
                newNote.noteText = initialNote;
                newNote.title = RaStorageService::extractTitleFromNote(initialNote);
                newNote.author = "Local";
                newNote.isModified = false;
                newNote.isLocalOnly = true;

                m_gameData.notes.push_back(newNote);
                populateAddressDropdown();
                selectAddressInUI(newAddr);
            }
        });
        newAddrAct->setProperty("iconPath", ":/icons/bookmark_add.svg");

        menu.exec(globalPos);
    }

    void StructureWorkspaceDialog::onRemoveDocumentationAtAddress(uint32_t address) {
        if (!m_manager || address == 0) return;

        QTreeWidgetItem* targetItem = nullptr;
        std::function<void(QTreeWidgetItem*)> findItem = [&](QTreeWidgetItem* item) {
            if (targetItem) return;
            uint32_t res = resolveNodeAddress(item);
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
            recalculateTreeAddresses();
            updateOffsetPreview();
            syncTreeToCodeNoteText();
            updateMemoryHighlights();
        }
    }

    void StructureWorkspaceDialog::onDefineArrayRequested(uint32_t startAddress, uint32_t endAddress, uint32_t byteCount) {
        Q_UNUSED(endAddress);
        if (byteCount == 0) return;

        // Locate the pointer node whose dereferenced target address matches the start of the array range
        QTreeWidgetItem* targetNode = nullptr;

        std::function<void(QTreeWidgetItem*)> findTargetNode = [&](QTreeWidgetItem* item) {
            if (targetNode) return;
            uint32_t derefBase = item->data(0, Qt::UserRole + 4).toUInt();
            if (derefBase == startAddress) {
                targetNode = item;
                return;
            }
            for (int i = 0; i < item->childCount(); ++i) {
                findTargetNode(item->child(i));
            }
        };

        if (m_structureTree->currentItem()) {
            uint32_t curDeref = m_structureTree->currentItem()->data(0, Qt::UserRole + 4).toUInt();
            if (curDeref == startAddress) {
                targetNode = m_structureTree->currentItem();
            }
        }

        if (!targetNode) {
            for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
                findTargetNode(m_structureTree->topLevelItem(i));
            }
        }

        if (targetNode) {
            // Standardized array formatting: ++0x78=[128 bytes] Pointer to Array
            QString arrayFormatText = QString("[%1 bytes] Pointer to Array").arg(byteCount);

            targetNode->setText(0, arrayFormatText);
            targetNode->setData(0, Qt::UserRole + 5, byteCount); // Array size in bytes
            targetNode->setData(0, Qt::UserRole + 6, true);      // Array scope flag
            m_memoryViewer->clearRangeSelection();
            recalculateTreeAddresses();
            syncTreeToCodeNoteText();
        }
        else {
            auto match = findBestScopeForAddress(startAddress);
            if (match.isValid) {
                // If defined at the base of the dissected structure itself
                QString arrayFormatText = QString("[%1 bytes] Array").arg(byteCount);
                insertNodeInScope(match.pointerNode, match.offset, arrayFormatText, match.isArrayScope);
                m_memoryViewer->clearRangeSelection();
                recalculateTreeAddresses();
                syncTreeToCodeNoteText();
            }
            else {
                // If range is outside known scopes, offer to create a new Root Array Structure at this address
                QString displayAddrStr = m_manager ? m_manager->formatDisplayAddress(startAddress) : QString::number(startAddress, 16).toUpper();
                auto reply = QMessageBox::question(
                    this,
                    "Create Array Structure",
                    QString("Selected range is outside known structure scopes.\n\nWould you like to create a new Root Array Structure at 0x%1 (%2 bytes)?")
                        .arg(displayAddrStr)
                        .arg(byteCount),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes
                );

                if (reply == QMessageBox::Yes) {
                    uint32_t normalizedAddr = m_manager ? m_manager->unnormalizeAddress(displayAddrStr) : startAddress;
                    QString initialNote = QString("[%1 bytes] Array").arg(byteCount);

                    RaCodeNote newNote;
                    newNote.address = normalizedAddr;
                    newNote.addressHex = QString("0x%1").arg(normalizedAddr, 8, 16, QChar('0')).toUpper();
                    newNote.noteText = initialNote;
                    newNote.title = RaStorageService::extractTitleFromNote(initialNote);
                    newNote.author = "Local";
                    newNote.isModified = false;
                    newNote.isLocalOnly = true;

                    m_gameData.notes.push_back(newNote);
                    populateAddressDropdown();
                    selectAddressInUI(normalizedAddr);
                }
            }
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

        // Standardized clipboard copy choices matching main results view
        auto* copyRaAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this, &StructureWorkspaceDialog::onCopySelectedAsRaFormat);
        copyRaAct->setProperty("iconPath", ":/icons/copy_ra.svg");
        copyRaAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        copyRaAct->setEnabled(getPathForTreeItem(item).has_value());

        auto* copyNoteAct = menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Copy Code Note", this, &StructureWorkspaceDialog::onCopySelectedAsCodeNote);
        copyNoteAct->setProperty("iconPath", ":/icons/copy_note.svg");
        copyNoteAct->setEnabled(getPathForTreeItem(item).has_value());

        auto* copyAddrAct = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Address", this, &StructureWorkspaceDialog::onCopySelectedAddress);
        copyAddrAct->setProperty("iconPath", ":/icons/copy.svg");
        copyAddrAct->setShortcut(QKeySequence::Copy);

        auto* copyValAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Value", this, &StructureWorkspaceDialog::onCopySelectedValue);
        copyValAct->setProperty("iconPath", ":/icons/copy_ra.svg");
        copyValAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));

        menu.addSeparator();

        bool hasPointerVal = !item->text(3).trimmed().isEmpty() && item->text(3) != "N/A" && item->text(3) != "0x00000000";
        if (hasPointerVal) {
            auto* followAct = menu.addAction(getThemeIcon(":/icons/follow_pointer.svg"), "Follow Pointer", this, &StructureWorkspaceDialog::onFollowPointerClicked);
            followAct->setProperty("iconPath", ":/icons/follow_pointer.svg");
            menu.addSeparator();
        }

        auto* removeAct = menu.addAction(getThemeIcon(":/icons/delete.svg"), "Remove", this, &StructureWorkspaceDialog::onRemoveNodeClicked);
        removeAct->setProperty("iconPath", ":/icons/delete.svg");

        menu.exec(m_structureTree->viewport()->mapToGlobal(pos));
    }

    // Traverses the pointer directly to its pointed destination, establishing the new structure scope
    void StructureWorkspaceDialog::onFollowPointerClicked() {
        // If an address is selected in Memory Viewer and holds a valid pointer, follow it directly
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            uint32_t dwordVal = m_memoryViewer->readDwordAt(m_currentViewerSelectedAddress);
            if (dwordVal != 0 && m_manager->isValidPointerTarget(dwordVal)) {
                uint32_t target = m_manager->unnormalizeAddress(QString::number(dwordVal, 16));
                navigateToAddress(target);
                return;
            }
        }

        auto* current = m_structureTree->currentItem();
        if (!current) return;

        uint32_t derefTarget = current->data(0, Qt::UserRole + 4).toUInt();
        if (derefTarget != 0) {
            navigateToAddress(derefTarget);
            return;
        }

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
        return StructureAnalyzer::readMemoryDword(address, m_manager, m_snapshotMemory);
    }

    uint32_t StructureWorkspaceDialog::resolveNodeAddress(QTreeWidgetItem* item) {
        if (!item || !m_manager || m_currentBaseAddress == 0) return 0;

        uint32_t base = m_manager->unnormalizeAddress(QString::number(m_currentBaseAddress, 16));
        if (base == 0) return 0;

        uint32_t currentAddress = base;

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        if (!rootArr) {
            auto basePtr = readMemoryDword(base);
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
                auto ptrVal = readMemoryDword(currentAddress);
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

    void StructureWorkspaceDialog::recalculateTreeAddresses() {
        if (!m_memoryViewer->hasData() || !m_manager || m_currentBaseAddress == 0) return;

        uint32_t base = m_manager->unnormalizeAddress(QString::number(m_currentBaseAddress, 16));

        uint32_t initialParentBase = base;
        bool initialValid = (base != 0);

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        if (!rootArr) {
            auto basePtr = readMemoryDword(base);
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
        std::vector<Widgets::MemoryViewerWidget::DefinedArrayInfo> definedArrays;
        QTreeWidgetItem* activeItem = m_structureTree->currentItem();

        // Add root array highlight to Memory Viewer if the dissected note declares an array
        uint32_t rootArrSize = 0;
        if (isRootArray(&rootArrSize) && rootArrSize > 0 && m_currentBaseAddress != 0) {
            uint32_t base = m_manager->unnormalizeAddress(QString::number(m_currentBaseAddress, 16));
            if (base != 0) {
                Widgets::MemoryViewerWidget::DefinedArrayInfo arrInfo;
                arrInfo.name = m_headerComment.isEmpty() ? QString("[%1 bytes] Array").arg(rootArrSize) : m_headerComment.trimmed();
                arrInfo.startAddress = base;
                arrInfo.size = rootArrSize;
                arrInfo.endAddress = base + rootArrSize - 1;
                definedArrays.push_back(arrInfo);
            }
        }

        std::function<void(QTreeWidgetItem*)> gatherHighlights = [&](QTreeWidgetItem* item) {
            uint32_t resolved = resolveNodeAddress(item);
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
                gatherHighlights(item->child(i));
            }
        };

        for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
            gatherHighlights(m_structureTree->topLevelItem(i));
        }

        m_memoryViewer->setHighlightedAddresses(highlights);
        m_memoryViewer->setDefinedArrays(definedArrays);
    }

    bool StructureWorkspaceDialog::isRootArray(uint32_t* outSize) const {
        QString header = m_headerComment.trimmed();
        if (header.isEmpty() && m_codeNoteTextEdit) {
            QString firstLine = m_codeNoteTextEdit->toPlainText().split('\n').value(0).trimmed();
            header = firstLine;
        }

        static const QRegularExpression arrayRx(R"(\[(?:(\d+)\s*x\s*(\d+)|(\d+))\s*bytes\])", QRegularExpression::CaseInsensitiveOption);
        auto match = arrayRx.match(header);
        if (match.hasMatch()) {
            if (outSize) {
                if (!match.captured(1).isEmpty() && !match.captured(2).isEmpty()) {
                    *outSize = match.captured(1).toUInt() * match.captured(2).toUInt();
                }
                else if (!match.captured(3).isEmpty()) {
                    *outSize = match.captured(3).toUInt();
                }
            }
            return true;
        }
        return false;
    }

    StructureWorkspaceDialog::ScopeMatch StructureWorkspaceDialog::findBestScopeForAddress(uint32_t selectedAddr) {
        ScopeMatch best;
        if (!m_manager || selectedAddr == 0 || m_currentBaseAddress == 0) return best;

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        std::vector<ScopeCandidate> candidateScopes;

        std::function<void(QTreeWidgetItem*, int)> collectScopes = [&](QTreeWidgetItem* item, int depth) {
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
                uint32_t nodeAddr = resolveNodeAddress(item);
                if (nodeAddr != 0) {
                    candidateScopes.push_back({ item, nodeAddr, depth, true, arraySize });
                }
            }

            for (int i = 0; i < item->childCount(); ++i) {
                collectScopes(item->child(i), depth + 1);
            }
        };

        for (int i = 0; i < m_structureTree->topLevelItemCount(); ++i) {
            collectScopes(m_structureTree->topLevelItem(i), 1);
        }

        auto match = StructureAnalyzer::findBestScopeForAddress(
            selectedAddr,
            m_currentBaseAddress,
            candidateScopes,
            m_manager,
            m_snapshotMemory,
            rootArr,
            rootArrSize
        );

        best.pointerNode = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
        best.baseAddress = match.baseAddress;
        best.offset = match.offset;
        best.depth = match.depth;
        best.isValid = match.isValid;
        best.isArrayScope = match.isArrayScope;

        return best;
    }

    void StructureWorkspaceDialog::updateOffsetPreview() {
        if (m_currentViewerSelectedAddress == 0) {
            m_offsetPreviewLabel->setText("Select an address in Memory Viewer to calculate relative offset.");
            return;
        }

        auto match = findBestScopeForAddress(m_currentViewerSelectedAddress);
        if (match.isValid) {
            QString scopeName;
            if (match.pointerNode) {
                scopeName = match.pointerNode->text(0) + " (" + match.pointerNode->text(1) + ")";
            }
            else if (match.isArrayScope) {
                scopeName = m_headerComment.isEmpty() ? "Root Array" : m_headerComment.trimmed();
            }
            else {
                scopeName = "Root Base Pointer";
            }

            QString sign = (match.offset < 0) ? "-" : "+";
            QString offsetHex = QString("%1").arg(std::abs(match.offset), 2, 16, QChar('0')).toUpper();

            QString prefix = match.isArrayScope ? "|" : sign;
            QString offsetDisplay = prefix + (match.isArrayScope && match.offset < 0 ? "-" : "") + "0x" + offsetHex;
            m_offsetPreviewLabel->setText(QString("Selected: 0x%1 | Scope: %2 | Calculated Offset: %3")
                .arg(QString::number(m_currentViewerSelectedAddress, 16).toUpper())
                .arg(scopeName)
                .arg(offsetDisplay));
        }
        else {
            m_offsetPreviewLabel->setText(QString("Selected: 0x%1 | Out of range of known structure scopes.")
                .arg(QString::number(m_currentViewerSelectedAddress, 16).toUpper()));
        }
    }

    QString StructureWorkspaceDialog::suggestDefaultType(uint32_t address) {
        return StructureAnalyzer::suggestDefaultType(address, m_manager, m_snapshotMemory);
    }

    void StructureWorkspaceDialog::insertNodeInScope(QTreeWidgetItem* parentScope, int32_t offset, const QString& description, bool isArrayElement) {
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

    void StructureWorkspaceDialog::onAddVariableClicked() {
        if (m_currentViewerSelectedAddress == 0) return;

        // Align address based on current active view mode
        uint32_t alignedAddr = m_currentViewerSelectedAddress;
        if (m_view32BitRadio->isChecked() || m_view32BitBERadio->isChecked()) {
            alignedAddr &= ~3U;
        }
        else if (m_view16BitRadio->isChecked()) {
            alignedAddr &= ~1U;
        }

        auto match = findBestScopeForAddress(alignedAddr);
        if (!match.isValid) {
            QMessageBox::warning(this, "Structure Placement", "Selected address does not fall within any active pointer scope.");
            return;
        }

        bool ok = false;
        QString defaultDesc = suggestDefaultType(alignedAddr);
        QString desc = QInputDialog::getText(this, "Add Variable", "Variable description:", QLineEdit::Normal, defaultDesc, &ok);
        if (!ok || desc.trimmed().isEmpty()) return;

        insertNodeInScope(match.pointerNode, match.offset, desc.trimmed(), match.isArrayScope);
        recalculateTreeAddresses();
        syncTreeToCodeNoteText();
    }

    void StructureWorkspaceDialog::onAddPointerClicked() {
        if (m_currentViewerSelectedAddress == 0) return;

        // Pointers are always 4-byte aligned on target platforms
        uint32_t alignedAddr = m_currentViewerSelectedAddress & ~3U;

        auto match = findBestScopeForAddress(alignedAddr);
        if (!match.isValid) {
            QMessageBox::warning(this, "Structure Placement", "Selected address does not fall within any active pointer scope.");
            return;
        }

        QString defaultPtrDesc = m_manager ? m_manager->getPlatformPointerTypeDescription() : "[32-bit] Pointer";
        bool ok = false;
        QString ptrDesc = QInputDialog::getText(this, "Add as Pointer", "Pointer description:", QLineEdit::Normal, defaultPtrDesc, &ok);
        if (!ok || ptrDesc.trimmed().isEmpty()) return;

        insertNodeInScope(match.pointerNode, match.offset, ptrDesc.trimmed(), match.isArrayScope);
        recalculateTreeAddresses();
        syncTreeToCodeNoteText();

        // Directly navigate into the new pointer's target in RAM
        auto valOpt = readMemoryDword(alignedAddr);
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
            QRegularExpression rx("^(\\|?[\\+\\-]?)0x([0-9a-fA-F]+)");
            auto match = rx.match(txt);
            if (match.hasMatch()) {
                QString prefix = match.captured(1);
                bool isArrayElem = (prefix == "|");
                bool ok = false;
                int32_t val = match.captured(2).toInt(&ok, 16);
                if (prefix.contains('-')) val = -val;
                item->setData(1, Qt::UserRole, val);
                item->setData(1, Qt::UserRole + 3, isArrayElem);
                recalculateTreeAddresses();
                syncTreeToCodeNoteText();
            }
        }
        else if (column == 0) {
            syncTreeToCodeNoteText();
        }
    }

    void StructureWorkspaceDialog::onToggleCodeNotePanelClicked() {
        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() < 3) return;

        if (sizes[2] > 0) {
            m_lastCodeNotePanelWidth = sizes[2];
            sizes[0] += sizes[2]; // Reallocate space to Structure Hierarchy
            sizes[2] = 0;
            m_mainSplitter->setSizes(sizes);
            m_toggleCodeNoteBtn->setChecked(false);
        }
        else {
            int restoreWidth = (m_lastCodeNotePanelWidth > 60) ? m_lastCodeNotePanelWidth : 330;
            sizes[0] = std::max(100, sizes[0] - restoreWidth);
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

    void StructureWorkspaceDialog::populateTreeFromHierarchyNodes(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes, QTreeWidgetItem* parentItem) {
        QRegularExpression offsetRx("^(\\|?[\\+\\-]?0x[0-9a-fA-F]+)(?:([=\\s\\|]+)(.*))?$");

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

    void StructureWorkspaceDialog::onFixHierarchyClicked() {
        if (m_isSyncing || !m_codeNoteTextEdit) return;
        m_isSyncing = true;

        QString text = m_codeNoteTextEdit->toPlainText().trimmed();
        if (text.isEmpty()) {
            m_structureTree->blockSignals(true);
            m_structureTree->clear();
            m_structureTree->blockSignals(false);
            m_headerComment.clear();
            m_isSyncing = false;
            return;
        }

        m_headerComment.clear();
        QStringList lines = text.split(QRegularExpression("[\r\n]+"));
        QString style = m_styleCombo->currentText();
        bool indentDesc = m_indentDescCheck->isChecked();

        auto parsedTree = CodeNoteHierarchyEngine::parseLinesToTree(lines, m_headerComment);
        auto mergedTree = CodeNoteHierarchyEngine::mergeTree(parsedTree);
        QString finalResult = CodeNoteHierarchyEngine::rebuildTextFromTree(mergedTree, 0, style, indentDesc, m_headerComment);

        m_codeNoteTextEdit->setPlainText(finalResult.trimmed());

        m_structureTree->blockSignals(true);
        m_structureTree->clear();
        populateTreeFromHierarchyNodes(mergedTree);

        m_structureTree->expandAll();
        recalculateTreeAddresses();
        updateOffsetPreview();
        m_structureTree->blockSignals(false);

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
        QString style = m_styleCombo ? m_styleCombo->currentText() : "Plusses (+)";
        bool indentDesc = m_indentDescCheck ? m_indentDescCheck->isChecked() : true;

        return CodeNoteHierarchyEngine::rebuildTextFromTree(rootNodes, 0, style, indentDesc, m_headerComment);
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

        saveWorkspaceSettings();
        m_isSyncing = false;
    }

    void StructureWorkspaceDialog::loadWorkspaceSettings() {
        m_isLoadingSettings = true;

        QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.beginGroup("DissectStructure");

        if (settings.contains("geometry")) {
            restoreGeometry(settings.value("geometry").toByteArray());
        }

        m_lastCodeNotePanelWidth = settings.value("lastCodeNoteWidth", 330).toInt();

        if (settings.contains("splitterState")) {
            m_mainSplitter->restoreState(settings.value("splitterState").toByteArray());
        }
        else {
            // Default layout sizing favoring Structure Hierarchy over Memory Viewer dead space
            m_mainSplitter->setSizes({ 500, 540, m_lastCodeNotePanelWidth });
        }

        int viewModeId = settings.value("viewMode", 2).toInt();
        if (auto* btn = m_viewModeGroup->button(viewModeId)) {
            btn->setChecked(true);
        }
        onViewModeChanged();

        m_styleCombo->blockSignals(true);
        m_indentDescCheck->blockSignals(true);

        QString style = settings.value("hierarchyStyle", "Plusses (+)").toString();
        m_styleCombo->setCurrentText(style);
        m_indentDescCheck->setChecked(settings.value("indentDescriptions", true).toBool());

        m_styleCombo->blockSignals(false);
        m_indentDescCheck->blockSignals(false);

        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() > 2) {
            m_toggleCodeNoteBtn->setChecked(sizes[2] > 0);
        }

        settings.endGroup();

        m_isLoadingSettings = false;
    }

    void StructureWorkspaceDialog::saveWorkspaceSettings() {
        if (m_isLoadingSettings) return;

        QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.beginGroup("DissectStructure");

        settings.setValue("geometry", saveGeometry());
        settings.setValue("splitterState", m_mainSplitter->saveState());
        settings.setValue("lastCodeNotePanelWidth", m_lastCodeNotePanelWidth);

        QList<int> sizes = m_mainSplitter->sizes();
        settings.setValue("codeNotePanelVisible", (sizes.size() > 2 && sizes[2] > 0));

        settings.setValue("viewMode", m_viewModeGroup->checkedId());
        settings.setValue("hierarchyStyle", m_styleCombo->currentText());
        settings.setValue("indentDescriptions", m_indentDescCheck->isChecked());

        settings.endGroup();
    }

}
