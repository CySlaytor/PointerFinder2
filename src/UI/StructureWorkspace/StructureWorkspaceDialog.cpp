#define NOMINMAX
#include "StructureWorkspaceDialog.h"

#include "Common/EndianUtils.h"
#include "Core/CodeNotes/CodeNoteHelper.h"
#include "Core/CodeNotes/ExportService.h"
#include "Models/CodeNoteSettings.h"
#include "Models/GlobalSettings.h"
#include "UI/Common/ThemeManager.h"

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
            m_hierarchyWidget->setEmulatorManager(m_manager);
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
        m_addressBar = new DissectAddressBar(this);
        rootLayout->addWidget(m_addressBar);

        m_mainSplitter = new QSplitter(Qt::Horizontal, this);

        // --- Left Panel: Structure Editor ---
        m_hierarchyWidget = new StructureHierarchyWidget(m_mainSplitter);
        m_mainSplitter->addWidget(m_hierarchyWidget);

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

        m_memoryViewer = new Widgets::MemoryViewerWidget(m_middleContainer);
        middleLayout->addWidget(m_memoryViewer, 1);

        // Memory View Mode Selector (positioned at bottom)
        auto* viewModeRow = new QHBoxLayout();
        viewModeRow->addWidget(new QLabel("Memory View:", m_middleContainer));

        m_viewModeCombo = new QComboBox(m_middleContainer);
        m_viewModeCombo->addItems({ "8-bit", "16-bit", "32-bit", "32-bit BE", "Float", "Float BE" });
        m_viewModeCombo->setCurrentIndex(2);
        m_viewModeCombo->setFixedHeight(24);
        m_viewModeCombo->setMinimumWidth(110);
        viewModeRow->addWidget(m_viewModeCombo);
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
        m_codeNoteEditor = new CodeNoteEditorWidget(m_mainSplitter);
        m_mainSplitter->addWidget(m_codeNoteEditor);

        m_mainSplitter->setCollapsible(0, false);
        m_mainSplitter->setCollapsible(1, false);
        m_mainSplitter->setCollapsible(2, true);

        // Prioritize horizontal space for Structure Hierarchy while keeping Memory Viewer sized to content
        m_mainSplitter->setStretchFactor(0, 1);
        m_mainSplitter->setStretchFactor(1, 0);
        m_mainSplitter->setStretchFactor(2, 0);
        rootLayout->addWidget(m_mainSplitter, 1);

        // Memory Viewer Connections
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addressSelected, this, &StructureWorkspaceDialog::onAddressSelectedInViewer);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::addressDoubleClicked, this, &StructureWorkspaceDialog::onAddressDoubleClickedInViewer);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::contextMenuRequested, this, &StructureWorkspaceDialog::onMemoryViewerContextMenu);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::defineArrayRequested, this, &StructureWorkspaceDialog::onDefineArrayRequested);

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
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::removeDocumentationTriggered, this, [this](uint32_t address) {
            uint32_t rootArrSize = 0;
            bool rootArr = isRootArray(&rootArrSize);
            m_hierarchyWidget->removeDocumentationAtAddress(address, m_currentBaseAddress, m_snapshotMemory, rootArr);
        });

        // Connect clipboard copy signals from Memory Viewer
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyAddressTriggered, this, &StructureWorkspaceDialog::onCopySelectedAddress);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyValueTriggered, this, &StructureWorkspaceDialog::onCopySelectedValue);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyRaTriggered, this, &StructureWorkspaceDialog::onCopySelectedAsRaFormat);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyCodeNoteTriggered, this, &StructureWorkspaceDialog::onCopySelectedAsCodeNote);

        // Hierarchy Widget Connections
        connect(m_hierarchyWidget, &StructureHierarchyWidget::addVariableClicked, this, &StructureWorkspaceDialog::onAddVariableClicked);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::addPointerClicked, this, &StructureWorkspaceDialog::onAddPointerClicked);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::followPointerClicked, this, &StructureWorkspaceDialog::onFollowPointerClicked);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::removeNodeClicked, this, &StructureWorkspaceDialog::onRemoveNodeClicked);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::treeDataChanged, this, &StructureWorkspaceDialog::onTreeDataChanged);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::treeSelectionChanged, this, &StructureWorkspaceDialog::onTreeSelectionChanged);

        connect(m_hierarchyWidget, &StructureHierarchyWidget::copyRaRequested, this, &StructureWorkspaceDialog::onCopySelectedAsRaFormat);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::copyNoteRequested, this, &StructureWorkspaceDialog::onCopySelectedAsCodeNote);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::copyAddressRequested, this, &StructureWorkspaceDialog::onCopySelectedAddress);
        connect(m_hierarchyWidget, &StructureHierarchyWidget::copyValueRequested, this, &StructureWorkspaceDialog::onCopySelectedValue);

        // Memory Controls
        connect(m_refreshRamBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onRefreshRamClicked);
        connect(m_goToAddressBtn, &QPushButton::clicked, this, &StructureWorkspaceDialog::onGoToAddressClicked);
        connect(m_goToAddressEdit, &QLineEdit::returnPressed, this, &StructureWorkspaceDialog::onGoToAddressClicked);
        connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StructureWorkspaceDialog::onViewModeChanged);

        // Address Bar Connections
        connect(m_addressBar, &DissectAddressBar::newAddressClicked, this, &StructureWorkspaceDialog::onNewAddressClicked);
        connect(m_addressBar, &DissectAddressBar::deleteAddressClicked, this, &StructureWorkspaceDialog::onDeleteAddressClicked);
        connect(m_addressBar, &DissectAddressBar::addressSelected, this, &StructureWorkspaceDialog::onAddressSelectedFromBar);
        connect(m_addressBar, &DissectAddressBar::saveUserTxtClicked, this, &StructureWorkspaceDialog::onSaveUserTxtClicked);
        connect(m_addressBar, &DissectAddressBar::revertServerClicked, this, &StructureWorkspaceDialog::onRevertServerClicked);
        connect(m_addressBar, &DissectAddressBar::resetClicked, this, &StructureWorkspaceDialog::onResetClicked);
        connect(m_addressBar, &DissectAddressBar::toggleCodeNotePanelClicked, this, &StructureWorkspaceDialog::onToggleCodeNotePanelClicked);

        // Code Note Panel Connections
        connect(m_codeNoteEditor, &CodeNoteEditorWidget::fixHierarchyClicked, this, &StructureWorkspaceDialog::onFixHierarchyClicked);
        connect(m_codeNoteEditor, &CodeNoteEditorWidget::hierarchyStyleChanged, this, &StructureWorkspaceDialog::onHierarchyStyleChanged);

        connect(m_mainSplitter, &QSplitter::splitterMoved, this, &StructureWorkspaceDialog::onSplitterMoved);
    }

    bool StructureWorkspaceDialog::loadActiveGameData() {
        if (!m_manager || !m_manager->isAttached()) return false;
        return RaStorageService::loadActiveGameData(m_manager->getProcessHandle(), m_gameData);
    }

    void StructureWorkspaceDialog::populateAddressDropdown(bool autoNavigate) {
        m_addressBar->populateAddressDropdown(m_gameData, m_currentBaseAddress, autoNavigate);
    }

    void StructureWorkspaceDialog::onAddressSelectedFromBar(uint32_t address, bool autoNavigate) {
        selectAddressInUI(address, autoNavigate);
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

        uint32_t nextAddr = m_addressBar->getSelectedAddress();
        if (nextAddr != 0) {
            selectAddressInUI(nextAddr, true);
        }
        else {
            m_hierarchyWidget->clearTree();
            m_codeNoteEditor->clear();
            updateOffsetPreview();
            updateToolbarButtonStates();
        }
    }

    void StructureWorkspaceDialog::selectAddressInUI(uint32_t address, bool navigate) {
        auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
            return n.address == address;
        });

        if (it == m_gameData.notes.end()) return;

        m_currentBaseAddress = address;
        m_addressBar->selectAddressInUI(address);

        m_isSyncing = true;
        m_codeNoteEditor->setPlainText(it->noteText);
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
            m_hierarchyWidget->clearTree();
            m_headerComment.clear();
            return;
        }

        m_headerComment.clear();
        QStringList lines = text.split(QRegularExpression("[\r\n]+"));
        auto parsedTree = CodeNoteHierarchyEngine::parseLinesToTree(lines, m_headerComment);
        auto mergedTree = CodeNoteHierarchyEngine::mergeTree(parsedTree);

        // Block tree signals to prevent redundant settings.ini and sync updates during population
        m_hierarchyWidget->populateFromNodes(mergedTree);

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        updateOffsetPreview();
        updateMemoryHighlights();
    }

    bool StructureWorkspaceDialog::saveActiveNoteToUserTxt() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0) return false;

        // Capture current memory viewer location and cursor to prevent jumping
        uint32_t savedCursor = (m_currentViewerSelectedAddress != 0) ? m_currentViewerSelectedAddress : m_memoryViewer->getSelectedAddress();
        int savedScroll = m_memoryViewer->verticalScrollBar()->value();

        QString currentNote = m_codeNoteEditor->toPlainText();
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
            m_addressBar->showSaveFeedback();
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

        m_addressBar->showResetFeedback();
    }

    void StructureWorkspaceDialog::updateToolbarButtonStates() {
        m_addressBar->updateToolbarButtonStates(m_gameData, m_currentBaseAddress);
    }

    void StructureWorkspaceDialog::onViewModeChanged() {
        int id = m_viewModeCombo ? m_viewModeCombo->currentIndex() : 2;
        switch (id) {
        case 0: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode8Bit); break;
        case 1: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode16Bit); break;
        case 2: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode32Bit); break;
        case 3: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode32BitBE); break;
        case 4: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::ModeFloat); break;
        case 5: m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::ModeFloatBE); break;
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

            uint32_t rootArrSize = 0;
            bool rootArr = isRootArray(&rootArrSize);
            m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
            updateMemoryHighlights();
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

    std::optional<PointerPath> StructureWorkspaceDialog::getPathForViewerAddress(uint32_t address) const {
        if (address == 0 || m_currentBaseAddress == 0 || !m_manager) return std::nullopt;

        auto match = const_cast<StructureWorkspaceDialog*>(this)->findBestScopeForAddress(address);
        if (!match.isValid) return std::nullopt;

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        PointerPath path;

        auto* ptrNode = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
        if (ptrNode) {
            std::vector<QTreeWidgetItem*> lineage;
            for (QTreeWidgetItem* curr = ptrNode; curr != nullptr; curr = curr->parent()) {
                lineage.push_back(curr);
            }
            std::reverse(lineage.begin(), lineage.end());

            if (rootArr && !lineage.empty()) {
                int32_t elemOffset = lineage[0]->data(1, Qt::UserRole).toInt();
                path.baseAddress = m_currentBaseAddress + static_cast<uint32_t>(elemOffset);
                for (size_t i = 1; i < lineage.size(); ++i) {
                    path.offsets.push_back(lineage[i]->data(1, Qt::UserRole).toInt());
                }
            }
            else {
                path.baseAddress = m_currentBaseAddress;
                for (auto* node : lineage) {
                    path.offsets.push_back(node->data(1, Qt::UserRole).toInt());
                }
            }
        }
        else {
            if (rootArr && match.isArrayScope) {
                path.baseAddress = m_currentBaseAddress + static_cast<uint32_t>(match.offset);
            }
            else {
                path.baseAddress = m_currentBaseAddress;
                path.offsets.push_back(match.offset);
            }
        }

        if (ptrNode) {
            path.offsets.push_back(match.offset);
        }
        else if (path.offsets.empty()) {
            path.offsets.push_back(0);
        }

        path.finalAddress = address;
        return path;
    }

    void StructureWorkspaceDialog::onCopySelectedAsRaFormat() {
        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        // Prioritize hierarchy tree selection if items are selected
        auto selectedTreeItems = m_hierarchyWidget->selectedItemsInOrder();
        if (!selectedTreeItems.isEmpty() && m_manager) {
            QStringList raStrings;
            for (auto* item : selectedTreeItems) {
                QString trigger = m_hierarchyWidget->formatRetroAchievementsTrigger(item, m_currentBaseAddress, rootArr, m_headerComment);
                if (!trigger.isEmpty()) {
                    raStrings.append(trigger);
                }
            }
            if (!raStrings.isEmpty()) {
                QApplication::clipboard()->setText(raStrings.join("\n"));
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            auto pathOpt = getPathForViewerAddress(m_currentViewerSelectedAddress);
            if (pathOpt.has_value()) {
                ExportService::copyAsRetroAchievementsFormat(pathOpt.value(), m_manager);
            }
        }
    }

    void StructureWorkspaceDialog::onCopySelectedAsCodeNote() {
        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        // Prioritize hierarchy tree selection if items are selected
        auto selectedTreeItems = m_hierarchyWidget->selectedItemsInOrder();
        if (!selectedTreeItems.isEmpty() && m_manager) {
            QStringList noteStrings;
            CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();
            QString ptrTemplate = m_manager->getPlatformPointerTypeDescription();

            for (auto* item : selectedTreeItems) {
                auto pathOpt = m_hierarchyWidget->getPathForTreeItem(item, m_currentBaseAddress, m_snapshotMemory, rootArr);
                if (pathOpt.has_value()) {
                    noteStrings.append(CodeNoteHelper::buildCodeNote(pathOpt->offsets, settings, "", item->text(0), ptrTemplate));
                }
            }
            if (!noteStrings.isEmpty()) {
                QApplication::clipboard()->setText(noteStrings.join("\n"));
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            auto pathOpt = getPathForViewerAddress(m_currentViewerSelectedAddress);
            if (pathOpt.has_value()) {
                ExportService::copyAsCodeNote(pathOpt.value(), m_manager);
            }
        }
    }

    void StructureWorkspaceDialog::onCopySelectedAddress() {
        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        // Prioritize hierarchy tree selection if items are selected
        auto selectedTreeItems = m_hierarchyWidget->selectedItemsInOrder();
        if (!selectedTreeItems.isEmpty() && m_manager) {
            QStringList addrStrings;
            for (auto* item : selectedTreeItems) {
                uint32_t addr = m_hierarchyWidget->resolveNodeAddress(item, m_currentBaseAddress, m_snapshotMemory, rootArr);
                if (addr != 0) {
                    addrStrings.append("0x" + m_manager->formatDisplayAddress(addr));
                }
            }
            if (!addrStrings.isEmpty()) {
                QApplication::clipboard()->setText(addrStrings.join("\n"));
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            QString addrStr = m_manager->formatDisplayAddress(m_currentViewerSelectedAddress);
            QApplication::clipboard()->setText("0x" + addrStr);
        }
    }

    void StructureWorkspaceDialog::onCopySelectedValue() {
        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        // Prioritize hierarchy tree selection if items are selected
        auto selectedTreeItems = m_hierarchyWidget->selectedItemsInOrder();
        if (!selectedTreeItems.isEmpty() && m_manager) {
            QStringList valueStrings;
            for (auto* item : selectedTreeItems) {
                uint32_t addr = m_hierarchyWidget->resolveNodeAddress(item, m_currentBaseAddress, m_snapshotMemory, rootArr);
                if (addr != 0) {
                    uint32_t dwordVal = m_memoryViewer ? m_memoryViewer->readDwordAt(addr) : 0;
                    if (dwordVal == 0) {
                        auto valOpt = StructureAnalyzer::readMemoryDword(addr, m_manager, m_snapshotMemory);
                        if (valOpt.has_value()) dwordVal = valOpt.value();
                    }
                    valueStrings.append("0x" + QString("%1").arg(dwordVal, 8, 16, QChar('0')).toUpper());
                }
            }
            if (!valueStrings.isEmpty()) {
                QApplication::clipboard()->setText(valueStrings.join("\n"));
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            uint32_t dwordVal = m_memoryViewer ? m_memoryViewer->readDwordAt(m_currentViewerSelectedAddress) : 0;
            if (dwordVal == 0) {
                auto valOpt = StructureAnalyzer::readMemoryDword(m_currentViewerSelectedAddress, m_manager, m_snapshotMemory);
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
                uint32_t rootArrSize = 0;
                bool rootArr = isRootArray(&rootArrSize);
                m_hierarchyWidget->removeDocumentationAtAddress(address, m_currentBaseAddress, m_snapshotMemory, rootArr);
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

        if (m_hierarchyWidget->currentItem()) {
            uint32_t curDeref = m_hierarchyWidget->currentItem()->data(0, Qt::UserRole + 4).toUInt();
            if (curDeref == startAddress) {
                targetNode = m_hierarchyWidget->currentItem();
            }
        }

        if (!targetNode) {
            for (int i = 0; i < m_hierarchyWidget->treeWidget()->topLevelItemCount(); ++i) {
                findTargetNode(m_hierarchyWidget->treeWidget()->topLevelItem(i));
            }
        }

        if (targetNode) {
            // Standardized array formatting: ++0x78=[128 bytes] Pointer to Array
            QString arrayFormatText = QString("[%1 bytes] Pointer to Array").arg(byteCount);

            targetNode->setText(0, arrayFormatText);
            targetNode->setData(0, Qt::UserRole + 5, byteCount); // Array size in bytes
            targetNode->setData(0, Qt::UserRole + 6, true);      // Array scope flag
            m_memoryViewer->clearRangeSelection();

            uint32_t rootArrSize = 0;
            bool rootArr = isRootArray(&rootArrSize);
            m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
            syncTreeToCodeNoteText();
            updateMemoryHighlights();
        }
        else {
            auto match = findBestScopeForAddress(startAddress);
            if (match.isValid) {
                // If defined at the base of the dissected structure itself
                QString arrayFormatText = QString("[%1 bytes] Array").arg(byteCount);
                auto* scopeNode = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
                m_hierarchyWidget->insertNodeInScope(scopeNode, match.offset, arrayFormatText, match.isArrayScope);
                m_memoryViewer->clearRangeSelection();

                uint32_t rootArrSize = 0;
                bool rootArr = isRootArray(&rootArrSize);
                m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
                syncTreeToCodeNoteText();
                updateMemoryHighlights();
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

    // Traverses the pointer directly to its pointed destination, establishing the new structure scope
    void StructureWorkspaceDialog::onFollowPointerClicked() {
        // Priority 1: If an item in the Structure Hierarchy is active, navigate to its memory location in RAM
        auto* current = m_hierarchyWidget->currentItem();
        if (current) {
            uint32_t rootArrSize = 0;
            bool rootArr = isRootArray(&rootArrSize);
            uint32_t resolved = m_hierarchyWidget->resolveNodeAddress(current, m_currentBaseAddress, m_snapshotMemory, rootArr);
            if (resolved != 0) {
                navigateToAddress(resolved);
                return;
            }
        }

        // Priority 2: If an address is selected in Memory Viewer and holds a valid pointer, follow it directly
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            uint32_t dwordVal = m_memoryViewer->readDwordAt(m_currentViewerSelectedAddress);
            if (dwordVal != 0 && m_manager && m_manager->isValidPointerTarget(dwordVal)) {
                uint32_t nativeTarget = m_manager->unnormalizeAddress(QString::number(dwordVal, 16));
                navigateToAddress(nativeTarget);
                return;
            }
        }
    }

    void StructureWorkspaceDialog::onTreeSelectionChanged() {
        updateOffsetPreview();
        updateMemoryHighlights();
        // Selection change only updates highlights and preview without moving the viewport
    }

    void StructureWorkspaceDialog::updateMemoryHighlights() {
        if (!m_manager) return;

        std::unordered_map<uint32_t, Widgets::MemoryViewerWidget::HighlightInfo> highlights;
        std::vector<Widgets::MemoryViewerWidget::DefinedArrayInfo> definedArrays;

        // Add root array highlight to Memory Viewer if the dissected note declares an array
        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        if (rootArr && rootArrSize > 0 && m_currentBaseAddress != 0) {
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

        // If this note has no sub-offsets, document the root base address itself as a single variable
        if (m_hierarchyWidget->treeWidget()->topLevelItemCount() == 0 && m_currentBaseAddress != 0 && !rootArr) {
            auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
                return n.address == m_currentBaseAddress;
            });
            if (it != m_gameData.notes.end()) {
                uint32_t base = m_manager->unnormalizeAddress(QString::number(m_currentBaseAddress, 16));
                if (base != 0) {
                    Widgets::MemoryViewerWidget::HighlightInfo info;
                    QString rawTitle = it->title;
                    static const QRegularExpression sizeTagRegex("^\\s*\\[([^\\]]+)\\]\\s*");
                    auto match = sizeTagRegex.match(rawTitle);

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

                    QString cleanName = rawTitle;
                    cleanName.remove(sizeTagRegex);
                    cleanName = cleanName.trimmed();
                    if (cleanName.isEmpty()) cleanName = rawTitle;

                    info.name = cleanName;
                    info.offset = "+0x00";
                    info.label = cleanName;
                    info.startAddress = base;
                    info.size = varSize;
                    info.isPointer = false;
                    info.isChildOfActive = true;

                    for (uint32_t b = 0; b < varSize; ++b) {
                        highlights[base + b] = info;
                    }
                }
            }
        }

        m_hierarchyWidget->gatherHighlights(highlights, definedArrays, m_currentBaseAddress, rootArr, m_snapshotMemory);

        m_memoryViewer->setHighlightedAddresses(highlights);
        m_memoryViewer->setDefinedArrays(definedArrays);
    }

    bool StructureWorkspaceDialog::isRootArray(uint32_t* outSize) const {
        QString header = m_headerComment.trimmed();
        if (header.isEmpty() && m_codeNoteEditor) {
            QString firstLine = m_codeNoteEditor->toPlainText().split('\n').value(0).trimmed();
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

    ScopeMatch StructureWorkspaceDialog::findBestScopeForAddress(uint32_t selectedAddr) {
        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        std::vector<ScopeCandidate> candidateScopes;
        m_hierarchyWidget->collectScopes(candidateScopes, m_currentBaseAddress, m_snapshotMemory, rootArr);

        return StructureAnalyzer::findBestScopeForAddress(
            selectedAddr,
            m_currentBaseAddress,
            candidateScopes,
            m_manager,
            m_snapshotMemory,
            rootArr,
            rootArrSize
        );
    }

    void StructureWorkspaceDialog::updateOffsetPreview() {
        auto match = findBestScopeForAddress(m_currentViewerSelectedAddress);
        m_hierarchyWidget->updateOffsetPreview(m_currentViewerSelectedAddress, match, m_headerComment);
    }

    QString StructureWorkspaceDialog::suggestDefaultType(uint32_t address) {
        return StructureAnalyzer::suggestDefaultType(address, m_manager, m_snapshotMemory);
    }

    void StructureWorkspaceDialog::onAddVariableClicked() {
        if (m_currentViewerSelectedAddress == 0) return;

        // Align address based on current active view mode
        uint32_t alignedAddr = m_currentViewerSelectedAddress;
        auto mode = m_memoryViewer->getViewMode();
        if (mode == Widgets::MemoryViewerWidget::ViewMode::Mode32Bit ||
            mode == Widgets::MemoryViewerWidget::ViewMode::Mode32BitBE ||
            mode == Widgets::MemoryViewerWidget::ViewMode::ModeFloat ||
            mode == Widgets::MemoryViewerWidget::ViewMode::ModeFloatBE) {
            alignedAddr &= ~3U;
        }
        else if (mode == Widgets::MemoryViewerWidget::ViewMode::Mode16Bit) {
            alignedAddr &= ~1U;
        }

        auto match = findBestScopeForAddress(alignedAddr);
        if (!match.isValid) {
            QMessageBox::warning(this, "Structure Placement", "Selected address does not fall within any active pointer scope.");
            return;
        }

        bool ok = false;
        QString defaultDesc = (mode == Widgets::MemoryViewerWidget::ViewMode::ModeFloat || mode == Widgets::MemoryViewerWidget::ViewMode::ModeFloatBE)
            ? "[Float] Variable"
            : suggestDefaultType(alignedAddr);
        QString desc = QInputDialog::getText(this, "Add Variable", "Variable description:", QLineEdit::Normal, defaultDesc, &ok);
        if (!ok || desc.trimmed().isEmpty()) return;

        auto* scopeNode = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
        m_hierarchyWidget->insertNodeInScope(scopeNode, match.offset, desc.trimmed(), match.isArrayScope);

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        syncTreeToCodeNoteText();
        updateMemoryHighlights();
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

        auto* scopeNode = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
        m_hierarchyWidget->insertNodeInScope(scopeNode, match.offset, ptrDesc.trimmed(), match.isArrayScope);

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        syncTreeToCodeNoteText();
        updateMemoryHighlights();

        // Directly navigate into the new pointer's target in RAM
        auto valOpt = StructureAnalyzer::readMemoryDword(alignedAddr, m_manager, m_snapshotMemory);
        if (valOpt.has_value() && m_manager && m_manager->isValidPointerTarget(valOpt.value())) {
            uint32_t nativeTarget = m_manager->unnormalizeAddress(QString::number(valOpt.value(), 16));
            navigateToAddress(nativeTarget);
        }
    }

    void StructureWorkspaceDialog::onRemoveNodeClicked() {
        // Collect all selected items in hierarchy order
        auto selected = m_hierarchyWidget->selectedItemsInOrder();
        if (selected.isEmpty()) {
            auto* current = m_hierarchyWidget->currentItem();
            if (current) {
                selected.append(current);
            }
        }
        if (selected.isEmpty()) return;

        // Filter out items whose ancestor is also selected to avoid double-deletion
        QList<QTreeWidgetItem*> itemsToDelete;
        for (auto* item : selected) {
            bool hasSelectedAncestor = false;
            for (auto* p = item->parent(); p != nullptr; p = p->parent()) {
                if (selected.contains(p)) {
                    hasSelectedAncestor = true;
                    break;
                }
            }
            if (!hasSelectedAncestor) {
                itemsToDelete.append(item);
            }
        }

        for (auto* item : itemsToDelete) {
            delete item;
        }

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        updateOffsetPreview();
        syncTreeToCodeNoteText();
        updateMemoryHighlights();
    }

    void StructureWorkspaceDialog::onTreeDataChanged() {
        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        updateOffsetPreview();
        syncTreeToCodeNoteText();
        updateMemoryHighlights();
    }

    void StructureWorkspaceDialog::onToggleCodeNotePanelClicked() {
        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() < 3) return;

        if (sizes[2] > 0) {
            m_lastCodeNotePanelWidth = sizes[2];
            sizes[0] += sizes[2]; // Reallocate space to Structure Hierarchy
            sizes[2] = 0;
            m_mainSplitter->setSizes(sizes);
            m_addressBar->setCodeNotePanelButtonChecked(false);
        }
        else {
            int restoreWidth = (m_lastCodeNotePanelWidth > 60) ? m_lastCodeNotePanelWidth : 330;
            sizes[0] = std::max(100, sizes[0] - restoreWidth);
            sizes[2] = restoreWidth;
            m_mainSplitter->setSizes(sizes);
            m_addressBar->setCodeNotePanelButtonChecked(true);
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
                m_addressBar->setCodeNotePanelButtonChecked(visible);
                saveWorkspaceSettings();
            }
        }
    }

    void StructureWorkspaceDialog::onHierarchyStyleChanged() {
        saveWorkspaceSettings();
    }

    void StructureWorkspaceDialog::onFixHierarchyClicked() {
        if (m_isSyncing || !m_codeNoteEditor) return;
        m_isSyncing = true;

        QString text = m_codeNoteEditor->toPlainText().trimmed();
        if (text.isEmpty()) {
            m_hierarchyWidget->clearTree();
            m_headerComment.clear();
            m_isSyncing = false;
            return;
        }

        m_headerComment.clear();
        QStringList lines = text.split(QRegularExpression("[\r\n]+"));
        QString style = m_codeNoteEditor->getStyle();
        bool indentDesc = m_codeNoteEditor->isIndentDescriptions();

        auto parsedTree = CodeNoteHierarchyEngine::parseLinesToTree(lines, m_headerComment);
        auto mergedTree = CodeNoteHierarchyEngine::mergeTree(parsedTree);
        QString finalResult = CodeNoteHierarchyEngine::rebuildTextFromTree(mergedTree, 0, style, indentDesc, m_headerComment);

        m_codeNoteEditor->setPlainText(finalResult.trimmed());

        m_hierarchyWidget->populateFromNodes(mergedTree);

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        updateOffsetPreview();
        updateMemoryHighlights();

        saveWorkspaceSettings();
        m_isSyncing = false;
    }

    void StructureWorkspaceDialog::syncTreeToCodeNoteText() {
        if (m_isSyncing || !m_codeNoteEditor) return;
        m_isSyncing = true;

        QString style = m_codeNoteEditor->getStyle();
        bool indentDesc = m_codeNoteEditor->isIndentDescriptions();
        QString newText = m_hierarchyWidget->generateCodeNoteFromTree(style, indentDesc, m_headerComment);

        if (m_codeNoteEditor->toPlainText() != newText) {
            int cursorPos = m_codeNoteEditor->getCursorPosition();
            m_codeNoteEditor->setPlainText(newText);
            m_codeNoteEditor->setCursorPosition(cursorPos);
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
        if (m_viewModeCombo && viewModeId >= 0 && viewModeId < m_viewModeCombo->count()) {
            m_viewModeCombo->blockSignals(true);
            m_viewModeCombo->setCurrentIndex(viewModeId);
            m_viewModeCombo->blockSignals(false);
        }
        onViewModeChanged();

        QString style = settings.value("hierarchyStyle", "Plusses (+)").toString();
        m_codeNoteEditor->setStyle(style);
        m_codeNoteEditor->setIndentDescriptions(settings.value("indentDescriptions", true).toBool());

        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() > 2) {
            m_addressBar->setCodeNotePanelButtonChecked(sizes[2] > 0);
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

        settings.setValue("viewMode", m_viewModeCombo ? m_viewModeCombo->currentIndex() : 2);
        settings.setValue("hierarchyStyle", m_codeNoteEditor->getStyle());
        settings.setValue("indentDescriptions", m_codeNoteEditor->isIndentDescriptions());

        settings.endGroup();
    }

}
