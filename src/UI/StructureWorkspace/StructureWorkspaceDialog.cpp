#define NOMINMAX
#include "StructureWorkspaceDialog.h"

#include "Common/EndianUtils.h"
#include "Core/CodeNotes/CodeNoteHelper.h"
#include "Core/CodeNotes/ExportService.h"
#include "Models/CodeNoteSettings.h"
#include "Models/GlobalSettings.h"
#include "UI/Common/ThemeManager.h"
#include "UI/Dialogs/AddPointerDialog.h"
#include "UI/Dialogs/AddVariableDialog.h"
#include "UI/Dialogs/CloneStrideDialog.h"

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
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
        resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
        setMinimumSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

        setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

        setupLayout();
        updateViewModeComboForEmulator();
        loadWorkspaceSettings();

        if (m_manager && m_manager->isAttached()) {
            m_hierarchyWidget->setEmulatorManager(m_manager);
            refreshMemorySnapshot();
            loadActiveGameData();
            populateAddressDropdown();
            updateStatusPermanentInfo();
            showStatusMessage("Ready");
        }
        else {
            m_ramStatusLabel->setText("Emulator not attached.");
            m_refreshRamBtn->setEnabled(false);
            updateStatusPermanentInfo();
            showStatusMessage("Not Attached");
        }
    }

    StructureWorkspaceDialog::~StructureWorkspaceDialog() {
        saveWorkspaceSettings();
    }

    QIcon StructureWorkspaceDialog::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void StructureWorkspaceDialog::showStatusMessage(const QString& message, int /*timeout*/) {
        if (m_statusLabel) {
            if (message.startsWith("Status:", Qt::CaseInsensitive)) {
                m_statusLabel->setText(message);
            }
            else {
                m_statusLabel->setText(QString("Status: %1").arg(message));
            }
        }
    }

    void StructureWorkspaceDialog::updateStatusPermanentInfo() {
        if (!m_statusPermanentLabel) return;
        if (m_manager && m_manager->isAttached()) {
            QString emuName = m_manager->getEmulatorName();
            QString gameIdStr = m_gameData.isValid ? QString(" | Game ID: %1").arg(m_gameData.gameId) : "";
            m_statusPermanentLabel->setText(QString("Attached: %1%2").arg(emuName, gameIdStr));
        }
        else {
            m_statusPermanentLabel->setText("Not attached");
        }
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

    void StructureWorkspaceDialog::resizeEvent(QResizeEvent* event) {
        QDialog::resizeEvent(event);
    }

    void StructureWorkspaceDialog::setupLayout() {
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        auto* contentWidget = new QWidget(this);
        auto* contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(8, 8, 8, 4);
        contentLayout->setSpacing(6);

        // Top Toolbar
        m_addressBar = new DissectAddressBar(contentWidget);
        contentLayout->addWidget(m_addressBar);

        m_mainSplitter = new QSplitter(Qt::Horizontal, contentWidget);

        // --- Left Panel: Structure Editor ---
        m_hierarchyWidget = new StructureHierarchyWidget(m_mainSplitter);
        m_hierarchyWidget->setMinimumWidth(280);
        m_mainSplitter->addWidget(m_hierarchyWidget);

        // --- Right Panel: Memory Viewer ---
        m_middleContainer = new QWidget(m_mainSplitter);
        auto* middleLayout = new QVBoxLayout(m_middleContainer);
        middleLayout->setContentsMargins(0, 0, 0, 0);
        middleLayout->setSpacing(6);

        // Top Header Row for Memory Viewer: Re-dump / Refresh RAM, Snapshot Info, and Memory View Selector
        auto* viewerTopRow = new QHBoxLayout();
        viewerTopRow->setContentsMargins(0, 0, 0, 0);
        viewerTopRow->setSpacing(5);

        m_refreshRamBtn = new QPushButton("Refresh RAM", m_middleContainer);
        m_refreshRamBtn->setIcon(getThemeIcon(":/icons/refresh.svg"));
        m_refreshRamBtn->setToolTip("Re-dump / Refresh RAM from active emulator");
        m_refreshRamBtn->setFixedHeight(26);
        m_refreshRamBtn->setAutoDefault(false);
        m_refreshRamBtn->setDefault(false);
        viewerTopRow->addWidget(m_refreshRamBtn);

        m_ramStatusLabel = new QLabel("No dump loaded.", m_middleContainer);
        m_ramStatusLabel->setStyleSheet("color: #8B949E; font-size: 11px;");
        m_ramStatusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_ramStatusLabel->setMinimumWidth(0);
        viewerTopRow->addWidget(m_ramStatusLabel);

        viewerTopRow->addStretch(1);

        // Synchronized combo box container to ensure identical widths and dynamic adaptation
        m_selectorsContainer = new QWidget(m_middleContainer);
        auto* selectorsLayout = new QHBoxLayout(m_selectorsContainer);
        selectorsLayout->setContentsMargins(0, 0, 0, 0);
        selectorsLayout->setSpacing(4);

        // Display Mode Selector (always visible, enabled only when Float mode is active)
        m_floatDisplayLabel = new QLabel("Display:", m_selectorsContainer);
        m_floatDisplayCombo = new QComboBox(m_selectorsContainer);
        m_floatDisplayCombo->addItems({
            "Raw Float",
            "Time (Seconds)",
            "Time (Frames 60Hz)",
            "Time (Frames 50Hz)",
            "Time (Centiseconds)",
            "Time (Milliseconds)"
        });
        m_floatDisplayCombo->setFixedHeight(26);
        m_floatDisplayCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_floatDisplayCombo->setMinimumContentsLength(6);
        m_floatDisplayCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_floatDisplayCombo->setMinimumWidth(95);
        m_floatDisplayLabel->setEnabled(false);
        m_floatDisplayCombo->setEnabled(false);
        selectorsLayout->addWidget(m_floatDisplayLabel);
        selectorsLayout->addWidget(m_floatDisplayCombo, 1);

        selectorsLayout->addSpacing(4);

        // Memory View Mode Selector (consistently right-aligned at the far right)
        m_viewModeLabel = new QLabel("Memory View:", m_selectorsContainer);
        m_viewModeCombo = new QComboBox(m_selectorsContainer);
        m_viewModeCombo->setFixedHeight(26);
        m_viewModeCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_viewModeCombo->setMinimumContentsLength(6);
        m_viewModeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_viewModeCombo->setMinimumWidth(95);
        selectorsLayout->addWidget(m_viewModeLabel);
        selectorsLayout->addWidget(m_viewModeCombo, 1);

        viewerTopRow->addWidget(m_selectorsContainer, 2);

        middleLayout->addLayout(viewerTopRow);

        m_memoryViewer = new Widgets::MemoryViewerWidget(m_middleContainer);
        middleLayout->addWidget(m_memoryViewer, 1);

        // Enforce the 32-bit content width floor so the splitter cannot shrink Panel 2 past its grid
        int contentWidth32 = m_memoryViewer->getContentWidth32Bit();
        m_middleContainer->setMinimumWidth(contentWidth32);

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
        m_codeNoteEditor->setMinimumWidth(320);
        m_mainSplitter->addWidget(m_codeNoteEditor);

        m_mainSplitter->setCollapsible(0, false);
        m_mainSplitter->setCollapsible(1, false);
        m_mainSplitter->setCollapsible(2, true);

        // Keep Panel 2 locked to its 32-bit content width (stretch factor 0) while letting Panels 0 & 2 absorb window resizing
        m_mainSplitter->setStretchFactor(0, 1);
        m_mainSplitter->setStretchFactor(1, 0);
        m_mainSplitter->setStretchFactor(2, 1);
        contentLayout->addWidget(m_mainSplitter, 1);

        rootLayout->addWidget(contentWidget, 1);

        // Dedicated Structure Dissect Status Bar matching main PF2 window
        m_statusBar = new QStatusBar(this);
        m_statusBar->setSizeGripEnabled(false);

        m_statusLabel = new QLabel("Status: Ready", this);
        m_statusPermanentLabel = new QLabel(this);
        m_statusPermanentLabel->setStyleSheet("color: #8B949E; font-size: 11px;");

        m_statusBar->addWidget(m_statusLabel, 1);
        m_statusBar->addPermanentWidget(m_statusPermanentLabel);
        rootLayout->addWidget(m_statusBar);

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
                    showStatusMessage(QString("Navigated history: step %1 of %2").arg(currentIndex).arg(totalCount));
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
            QString addrStr = m_manager ? m_manager->formatDisplayAddress(address) : QString::number(address, 16).toUpper();
            showStatusMessage(QString("Removed documentation at address 0x%1").arg(addrStr));
        });

        // Connect clipboard copy signals from Memory Viewer directly to dedicated viewer copy slots
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyAddressTriggered, this, &StructureWorkspaceDialog::onCopyViewerAddress);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyValueTriggered, this, &StructureWorkspaceDialog::onCopyViewerValue);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyRaTriggered, this, &StructureWorkspaceDialog::onCopyViewerAsRaFormat);
        connect(m_memoryViewer, &Widgets::MemoryViewerWidget::copyCodeNoteTriggered, this, &StructureWorkspaceDialog::onCopyViewerAsCodeNote);

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
        connect(m_floatDisplayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StructureWorkspaceDialog::onFloatDisplayModeChanged);

        // Address Bar Connections
        connect(m_addressBar, &DissectAddressBar::newAddressClicked, this, &StructureWorkspaceDialog::onNewAddressClicked);
        connect(m_addressBar, &DissectAddressBar::cloneStrideClicked, this, &StructureWorkspaceDialog::onCloneStrideClicked);
        connect(m_addressBar, &DissectAddressBar::deleteAddressClicked, this, &StructureWorkspaceDialog::onDeleteAddressClicked);
        connect(m_addressBar, &DissectAddressBar::addressSelected, this, &StructureWorkspaceDialog::onAddressSelectedFromBar);
        connect(m_addressBar, &DissectAddressBar::saveUserTxtClicked, this, &StructureWorkspaceDialog::onSaveUserTxtClicked);
        connect(m_addressBar, &DissectAddressBar::revertServerClicked, this, &StructureWorkspaceDialog::onRevertServerClicked);
        connect(m_addressBar, &DissectAddressBar::resetClicked, this, &StructureWorkspaceDialog::onResetClicked);
        connect(m_addressBar, &DissectAddressBar::toggleCodeNotePanelClicked, this, &StructureWorkspaceDialog::onToggleCodeNotePanelClicked);

        // Code Note Panel Connections
        connect(m_codeNoteEditor, &CodeNoteEditorWidget::fixHierarchyClicked, this, &StructureWorkspaceDialog::onFixHierarchyClicked);
        connect(m_codeNoteEditor, &CodeNoteEditorWidget::hierarchyStyleChanged, this, &StructureWorkspaceDialog::onHierarchyStyleChanged);
        connect(m_codeNoteEditor, &CodeNoteEditorWidget::statusMessageRequested, this, [this](const QString& message) {
            showStatusMessage(message);
        });

        connect(m_mainSplitter, &QSplitter::splitterMoved, this, &StructureWorkspaceDialog::onSplitterMoved);
    }

    void StructureWorkspaceDialog::updateViewModeComboForEmulator() {
        if (!m_viewModeCombo) return;

        bool isBE = (m_manager && m_manager->getRetroAchievementsPrefix() == "G");
        QString currentText = m_viewModeCombo->currentText();

        m_viewModeCombo->blockSignals(true);
        m_viewModeCombo->clear();

        if (isBE) {
            m_viewModeCombo->addItems({ "8-bit", "16-bit", "32-bit BE", "Float BE" });
        }
        else {
            m_viewModeCombo->addItems({ "8-bit", "16-bit", "32-bit", "Float" });
        }

        int targetIdx = m_viewModeCombo->findText(currentText);
        if (targetIdx != -1) {
            m_viewModeCombo->setCurrentIndex(targetIdx);
        }
        else {
            if (currentText == "Float" && isBE) {
                targetIdx = m_viewModeCombo->findText("Float BE");
            }
            else if (currentText == "Float BE" && !isBE) {
                targetIdx = m_viewModeCombo->findText("Float");
            }
            else if (currentText == "32-bit" && isBE) {
                targetIdx = m_viewModeCombo->findText("32-bit BE");
            }
            else if (currentText == "32-bit BE" && !isBE) {
                targetIdx = m_viewModeCombo->findText("32-bit");
            }

            if (targetIdx != -1) {
                m_viewModeCombo->setCurrentIndex(targetIdx);
            }
            else {
                m_viewModeCombo->setCurrentIndex(2); // Default to 32-bit / 32-bit BE
            }
        }

        m_viewModeCombo->blockSignals(false);
        onViewModeChanged();
    }

    bool StructureWorkspaceDialog::confirmSaveUserTxt() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0) return false;

        QString addrStr = m_manager ? m_manager->formatDisplayAddress(activeAddr) : QString::number(activeAddr, 16).toUpper();
        QString noteTitle = "Untitled Note";

        auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
            return n.address == activeAddr;
        });
        if (it != m_gameData.notes.end() && !it->title.isEmpty()) {
            noteTitle = it->title;
        }

        QFileInfo fi(m_gameData.userTxtPath);
        QString fileName = fi.fileName().isEmpty() ? "User.txt" : fi.fileName();

        int modifiedOrLocalCount = 0;
        for (const auto& n : m_gameData.notes) {
            if (n.isModified || n.isLocalOnly) modifiedOrLocalCount++;
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Confirm Save to User.txt");
        if (modifiedOrLocalCount > 1) {
            msgBox.setText(QString("Save all %1 pending local/modified code notes to %2?").arg(modifiedOrLocalCount).arg(fileName));
        } else {
            msgBox.setText(QString("Save changes for address 0x%1 to %2?").arg(addrStr, fileName));
        }

        msgBox.setInformativeText(
            QString("Structure: %1\nTarget File: %2\n\n"
                    "This will write your local developer note override to disk. "
                    "The modified note will take precedence over the server baseline in future sessions.")
            .arg(noteTitle, fileName)
        );
        msgBox.setIcon(QMessageBox::Question);

        QPushButton* saveBtn = msgBox.addButton("Save", QMessageBox::AcceptRole);
        msgBox.addButton("Cancel", QMessageBox::RejectRole);
        msgBox.setDefaultButton(saveBtn);
        msgBox.exec();

        return msgBox.clickedButton() == saveBtn;
    }

    bool StructureWorkspaceDialog::confirmRevertServer() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0) return false;

        QString addrStr = m_manager ? m_manager->formatDisplayAddress(activeAddr) : QString::number(activeAddr, 16).toUpper();
        QString noteTitle = "Untitled Note";

        auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
            return n.address == activeAddr;
        });
        if (it != m_gameData.notes.end() && !it->title.isEmpty()) {
            noteTitle = it->title;
        }

        QFileInfo fi(m_gameData.userTxtPath);
        QString fileName = fi.fileName().isEmpty() ? "User.txt" : fi.fileName();

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Confirm Revert to Server");
        msgBox.setText(QString("Discard local changes and revert address 0x%1 to server note?").arg(addrStr));
        msgBox.setInformativeText(
            QString("Structure: %1\nTarget File: %2\n\n"
                    "Warning: All local overrides for this address will be permanently removed from %2. "
                    "The structure hierarchy and note text will revert to the server baseline.")
            .arg(noteTitle, fileName)
        );
        msgBox.setIcon(QMessageBox::Warning);

        QPushButton* revertBtn = msgBox.addButton("Revert", QMessageBox::DestructiveRole);
        QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
        msgBox.setDefaultButton(cancelBtn);
        msgBox.exec();

        return msgBox.clickedButton() == revertBtn;
    }

    bool StructureWorkspaceDialog::confirmReset() {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Confirm Workspace Reset");
        msgBox.setText("Reload all game code notes and structures from disk?");
        msgBox.setInformativeText(
            "Warning: Any unsaved modifications in the Structure Hierarchy or Code Note Editor "
            "that have not been saved to User.txt will be discarded.\n\n"
            "Both Notes.json and User.txt will be reloaded from disk."
        );
        msgBox.setIcon(QMessageBox::Warning);

        QPushButton* resetBtn = msgBox.addButton("Reset", QMessageBox::DestructiveRole);
        QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
        msgBox.setDefaultButton(cancelBtn);
        msgBox.exec();

        return msgBox.clickedButton() == resetBtn;
    }

    bool StructureWorkspaceDialog::loadActiveGameData() {
        if (!m_manager || !m_manager->isAttached()) return false;
        bool loaded = RaStorageService::loadActiveGameData(m_manager->getProcessHandle(), m_gameData);
        if (loaded) {
            updateStatusPermanentInfo();
        }
        return loaded;
    }

    void StructureWorkspaceDialog::populateAddressDropdown(bool autoNavigate) {
        m_addressBar->populateAddressDropdown(m_gameData, m_currentBaseAddress, autoNavigate);
        updateStatusPermanentInfo();
    }

    void StructureWorkspaceDialog::onAddressSelectedFromBar(uint32_t address, bool autoNavigate) {
        selectAddressInUI(address, autoNavigate);
        QString addrStr = m_manager ? m_manager->formatDisplayAddress(address) : QString::number(address, 16).toUpper();
        showStatusMessage(QString("Loaded structure for address 0x%1").arg(addrStr));
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

            if (it != m_gameData.notes.begin() && it != m_gameData.notes.end()) {
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

            QString addrStr = m_manager ? m_manager->formatDisplayAddress(address) : QString::number(address, 16).toUpper();
            showStatusMessage(QString("Created new address structure at 0x%1").arg(addrStr));
        }
    }

    void StructureWorkspaceDialog::onCloneStrideClicked() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0 || !m_gameData.isValid) return;

        QString currentNote = m_codeNoteEditor ? m_codeNoteEditor->toPlainText() : "";
        if (currentNote.trimmed().isEmpty()) {
            auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
                return n.address == activeAddr;
            });
            if (it != m_gameData.notes.end()) {
                currentNote = it->noteText;
            }
        }

        CloneStrideDialog dialog(m_manager, activeAddr, currentNote, this);
        if (dialog.exec() != QDialog::Accepted) return;

        auto generated = dialog.getGeneratedNotes();
        if (generated.empty()) return;

        int addedCount = 0;
        int updatedCount = 0;

        for (const auto& g : generated) {
            auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
                return n.address == g.address;
            });

            if (it != m_gameData.notes.end()) {
                it->noteText = g.noteText;
                it->title = g.title;
                it->isModified = !it->isLocalOnly;
                updatedCount++;
            }
            else {
                RaCodeNote newNote;
                newNote.address = g.address;
                newNote.addressHex = QString("0x%1").arg(g.address, 8, 16, QChar('0')).toUpper();
                newNote.noteText = g.noteText;
                newNote.title = g.title;
                newNote.author = "Local";
                newNote.isModified = false;
                newNote.isLocalOnly = true;
                m_gameData.notes.push_back(newNote);
                addedCount++;
            }
        }

        populateAddressDropdown(false);
        updateToolbarButtonStates();

        // Switch to the next generated note in sequence so the developer can immediately inspect it in Memory Viewer
        uint32_t nextInspectAddr = (generated.size() > 1) ? generated[1].address : generated[0].address;
        selectAddressInUI(nextInspectAddr, true);

        showStatusMessage(QString("Batch cloned %1 sequential note(s) (%2 new, %3 updated). Click 'Reset' to discard or 'Save to User.txt' to commit.")
            .arg(generated.size())
            .arg(addedCount)
            .arg(updatedCount));
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

        showStatusMessage(QString("Removed address 0x%1 from address list and User.txt").arg(addrStr));
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
        updateStatusPermanentInfo();
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

        // 1. Sync active editor text into active address first
        QString currentNote = m_codeNoteEditor->toPlainText();
        auto it = std::find_if(m_gameData.notes.begin(), m_gameData.notes.end(), [&](const RaCodeNote& n) {
            return n.address == activeAddr;
        });
        if (it != m_gameData.notes.end()) {
            it->noteText = currentNote;
            it->title = RaStorageService::extractTitleFromNote(currentNote);
            it->isModified = !it->isLocalOnly;
        }

        // 2. Persist all local overrides and newly created/cloned notes into User.txt
        bool anyFailed = false;
        for (const auto& note : m_gameData.notes) {
            if (note.isLocalOnly || note.isModified) {
                if (!RaStorageService::saveActiveNoteToUserTxt(m_gameData, note.address, note.noteText)) {
                    anyFailed = true;
                }
            }
        }

        if (!anyFailed) {
            populateAddressDropdown(false);
            updateToolbarButtonStates();

            // Restore exact cursor and scroll state in Memory Viewer
            if (savedCursor != 0) {
                m_memoryViewer->setSelectedAddress(savedCursor, false, false);
                m_memoryViewer->verticalScrollBar()->setValue(savedScroll);
            }
            return true;
        }
        return false;
    }

    void StructureWorkspaceDialog::onSaveUserTxtClicked() {
        if (!confirmSaveUserTxt()) {
            return;
        }

        if (saveActiveNoteToUserTxt()) {
            m_addressBar->showSaveFeedback();
            QString addrStr = m_manager ? m_manager->formatDisplayAddress(m_currentBaseAddress) : QString::number(m_currentBaseAddress, 16).toUpper();
            showStatusMessage(QString("Structure note for 0x%1 saved to User.txt").arg(addrStr));
        }
        else {
            showStatusMessage("Failed to save structure note to User.txt");
        }
    }

    void StructureWorkspaceDialog::onRevertServerClicked() {
        uint32_t activeAddr = m_currentBaseAddress;
        if (activeAddr == 0) return;

        if (!confirmRevertServer()) {
            return;
        }

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

            QString addrStr = m_manager ? m_manager->formatDisplayAddress(activeAddr) : QString::number(activeAddr, 16).toUpper();
            showStatusMessage(QString("Changes reverted: restored server note for address 0x%1").arg(addrStr));
        }
        else {
            showStatusMessage("Failed to revert note to server baseline.");
        }
    }

    void StructureWorkspaceDialog::onResetClicked() {
        if (!confirmReset()) {
            return;
        }

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
        showStatusMessage("Reset completed: reloaded all game code notes from disk.");
    }

    void StructureWorkspaceDialog::updateToolbarButtonStates() {
        m_addressBar->updateToolbarButtonStates(m_gameData, m_currentBaseAddress);
    }

    void StructureWorkspaceDialog::onViewModeChanged() {
        QString text = m_viewModeCombo ? m_viewModeCombo->currentText() : "32-bit";

        if (text == "8-bit") {
            m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode8Bit);
        }
        else if (text == "16-bit") {
            m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode16Bit);
        }
        else if (text == "32-bit") {
            m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode32Bit);
        }
        else if (text == "32-bit BE") {
            m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::Mode32BitBE);
        }
        else if (text == "Float") {
            m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::ModeFloat);
        }
        else if (text == "Float BE") {
            m_memoryViewer->setViewMode(Widgets::MemoryViewerWidget::ViewMode::ModeFloatBE);
        }

        bool isFloat = (text == "Float" || text == "Float BE");
        if (m_floatDisplayLabel) m_floatDisplayLabel->setEnabled(isFloat);
        if (m_floatDisplayCombo) m_floatDisplayCombo->setEnabled(isFloat);

        showStatusMessage(QString("Memory View switched to %1").arg(text));
    }

    void StructureWorkspaceDialog::onFloatDisplayModeChanged() {
        if (!m_floatDisplayCombo || !m_memoryViewer) return;

        int idx = m_floatDisplayCombo->currentIndex();
        Widgets::MemoryViewerWidget::FloatDisplayMode mode = Widgets::MemoryViewerWidget::FloatDisplayMode::Raw;

        switch (idx) {
        case 0: mode = Widgets::MemoryViewerWidget::FloatDisplayMode::Raw; break;
        case 1: mode = Widgets::MemoryViewerWidget::FloatDisplayMode::TimeSeconds; break;
        case 2: mode = Widgets::MemoryViewerWidget::FloatDisplayMode::TimeFrames60; break;
        case 3: mode = Widgets::MemoryViewerWidget::FloatDisplayMode::TimeFrames50; break;
        case 4: mode = Widgets::MemoryViewerWidget::FloatDisplayMode::TimeCentiseconds; break;
        case 5: mode = Widgets::MemoryViewerWidget::FloatDisplayMode::TimeMilliseconds; break;
        default: break;
        }

        m_memoryViewer->setFloatDisplayMode(mode);

        showStatusMessage(QString("Float display format set to %1").arg(m_floatDisplayCombo->currentText()));
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

            showStatusMessage(QString("Memory dump refreshed successfully (%1 KB)").arg(size / 1024));
        }
        else {
            m_ramStatusLabel->setText("Failed to read memory from emulator.");
            showStatusMessage("Failed to read memory dump from emulator.");
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
            showStatusMessage(QString("Jumped to address 0x%1 in Memory Viewer").arg(text.toUpper()));
        }
        else {
            showStatusMessage("Invalid address entered for navigation.");
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
            QString targetStr = m_manager ? m_manager->formatDisplayAddress(nativeTarget) : QString::number(nativeTarget, 16).toUpper();
            showStatusMessage(QString("Followed pointer to target address 0x%1").arg(targetStr));
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
                QApplication::clipboard()->setText(raStrings.join("_"));
                if (raStrings.size() == 1) {
                    showStatusMessage("Copied RetroAchievements trigger to clipboard.");
                }
                else {
                    showStatusMessage(QString("Copied %1 RetroAchievements trigger conditions to clipboard.").arg(raStrings.size()));
                }
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            onCopyViewerAsRaFormat();
        }
    }

    void StructureWorkspaceDialog::onCopyViewerAsRaFormat() {
        if (m_currentViewerSelectedAddress == 0 || !m_manager) return;

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        // 1. Check if the address directly corresponds to a documented node in the Structure Hierarchy
        QTreeWidgetItem* matchedItem = nullptr;
        std::function<void(QTreeWidgetItem*)> findItem = [&](QTreeWidgetItem* item) {
            if (matchedItem) return;
            uint32_t addr = m_hierarchyWidget->resolveNodeAddress(item, m_currentBaseAddress, m_snapshotMemory, rootArr);
            if (addr == m_currentViewerSelectedAddress) {
                matchedItem = item;
                return;
            }
            for (int i = 0; i < item->childCount(); ++i) {
                findItem(item->child(i));
            }
        };

        for (int i = 0; i < m_hierarchyWidget->treeWidget()->topLevelItemCount(); ++i) {
            findItem(m_hierarchyWidget->treeWidget()->topLevelItem(i));
            if (matchedItem) break;
        }

        if (matchedItem) {
            QString trigger = m_hierarchyWidget->formatRetroAchievementsTrigger(matchedItem, m_currentBaseAddress, rootArr, m_headerComment);
            if (!trigger.isEmpty()) {
                QApplication::clipboard()->setText(trigger);
                showStatusMessage("Copied RetroAchievements trigger to clipboard.");
                return;
            }
        }

        // 2. Fallback: resolve path through active structure/array scopes
        auto pathOpt = getPathForViewerAddress(m_currentViewerSelectedAddress);
        if (pathOpt.has_value()) {
            ExportService::copyAsRetroAchievementsFormat(pathOpt.value(), m_manager);
            showStatusMessage("Copied RetroAchievements trigger to clipboard.");
        }
        else {
            showStatusMessage("Cannot generate RA trigger: address is outside active structure scopes.");
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
                if (noteStrings.size() == 1) {
                    showStatusMessage("Copied code note to clipboard.");
                }
                else {
                    showStatusMessage(QString("Copied %1 code notes to clipboard.").arg(noteStrings.size()));
                }
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            onCopyViewerAsCodeNote();
        }
    }

    void StructureWorkspaceDialog::onCopyViewerAsCodeNote() {
        if (m_currentViewerSelectedAddress == 0 || !m_manager) return;

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);

        QTreeWidgetItem* matchedItem = nullptr;
        std::function<void(QTreeWidgetItem*)> findItem = [&](QTreeWidgetItem* item) {
            if (matchedItem) return;
            uint32_t addr = m_hierarchyWidget->resolveNodeAddress(item, m_currentBaseAddress, m_snapshotMemory, rootArr);
            if (addr == m_currentViewerSelectedAddress) {
                matchedItem = item;
                return;
            }
            for (int i = 0; i < item->childCount(); ++i) {
                findItem(item->child(i));
            }
        };

        for (int i = 0; i < m_hierarchyWidget->treeWidget()->topLevelItemCount(); ++i) {
            findItem(m_hierarchyWidget->treeWidget()->topLevelItem(i));
            if (matchedItem) break;
        }

        if (matchedItem) {
            auto pathOpt = m_hierarchyWidget->getPathForTreeItem(matchedItem, m_currentBaseAddress, m_snapshotMemory, rootArr);
            if (pathOpt.has_value()) {
                CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();
                QString ptrTemplate = m_manager->getPlatformPointerTypeDescription();
                QString note = CodeNoteHelper::buildCodeNote(pathOpt->offsets, settings, "", matchedItem->text(0), ptrTemplate);
                QApplication::clipboard()->setText(note);
                showStatusMessage("Copied code note to clipboard.");
                return;
            }
        }

        auto pathOpt = getPathForViewerAddress(m_currentViewerSelectedAddress);
        if (pathOpt.has_value()) {
            ExportService::copyAsCodeNote(pathOpt.value(), m_manager);
            showStatusMessage("Copied code note to clipboard.");
        }
        else {
            showStatusMessage("Cannot generate code note: address is outside active structure scopes.");
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
                if (addrStrings.size() == 1) {
                    showStatusMessage(QString("Copied address %1 to clipboard.").arg(addrStrings.first()));
                }
                else {
                    showStatusMessage(QString("Copied %1 addresses to clipboard.").arg(addrStrings.size()));
                }
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            onCopyViewerAddress();
        }
    }

    void StructureWorkspaceDialog::onCopyViewerAddress() {
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            QString addrStr = m_manager->formatDisplayAddress(m_currentViewerSelectedAddress);
            QApplication::clipboard()->setText("0x" + addrStr);
            showStatusMessage(QString("Copied address 0x%1 to clipboard.").arg(addrStr));
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
                if (valueStrings.size() == 1) {
                    showStatusMessage(QString("Copied value %1 to clipboard.").arg(valueStrings.first()));
                }
                else {
                    showStatusMessage(QString("Copied %1 values to clipboard.").arg(valueStrings.size()));
                }
            }
            return;
        }

        // Fallback for Memory Viewer selection
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            onCopyViewerValue();
        }
    }

    void StructureWorkspaceDialog::onCopyViewerValue() {
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            uint32_t dwordVal = m_memoryViewer ? m_memoryViewer->readDwordAt(m_currentViewerSelectedAddress) : 0;
            if (dwordVal == 0) {
                auto valOpt = StructureAnalyzer::readMemoryDword(m_currentViewerSelectedAddress, m_manager, m_snapshotMemory);
                if (valOpt.has_value()) dwordVal = valOpt.value();
            }
            QString valHex = "0x" + QString("%1").arg(dwordVal, 8, 16, QChar('0')).toUpper();
            QApplication::clipboard()->setText(valHex);
            showStatusMessage(QString("Copied value %1 to clipboard.").arg(valHex));
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
                QString targetStr = m_manager ? m_manager->formatDisplayAddress(nativeTarget) : QString::number(nativeTarget, 16).toUpper();
                showStatusMessage(QString("Followed pointer: navigated to 0x%1 in Memory Viewer").arg(targetStr));
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
                QString addrStr = m_manager ? m_manager->formatDisplayAddress(address) : QString::number(address, 16).toUpper();
                showStatusMessage(QString("Removed documentation at address 0x%1").arg(addrStr));
            });
            removeDocAct->setProperty("iconPath", ":/icons/delete.svg");
        }

        menu.addSeparator();

        // Standardized clipboard copy choices consistent with main results view targeting the active memory viewer address
        auto* copyRaAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this, &StructureWorkspaceDialog::onCopyViewerAsRaFormat);
        copyRaAct->setProperty("iconPath", ":/icons/copy_ra.svg");
        copyRaAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        copyRaAct->setEnabled(getPathForViewerAddress(address).has_value());

        auto* copyNoteAct = menu.addAction(getThemeIcon(":/icons/copy_note.svg"), "Copy Code Note", this, &StructureWorkspaceDialog::onCopyViewerAsCodeNote);
        copyNoteAct->setProperty("iconPath", ":/icons/copy_note.svg");
        copyNoteAct->setEnabled(getPathForViewerAddress(address).has_value());

        auto* copyAddrAct = menu.addAction(getThemeIcon(":/icons/copy.svg"), "Copy Address", this, &StructureWorkspaceDialog::onCopyViewerAddress);
        copyAddrAct->setProperty("iconPath", ":/icons/copy.svg");
        copyAddrAct->setShortcut(QKeySequence::Copy);

        auto* copyValAct = menu.addAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy Value", this, &StructureWorkspaceDialog::onCopyViewerValue);
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

                QString newAddrStr = m_manager ? m_manager->formatDisplayAddress(newAddr) : QString::number(newAddr, 16).toUpper();
                showStatusMessage(QString("Created new address structure at 0x%1").arg(newAddrStr));
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

        QString startStr = m_manager ? m_manager->formatDisplayAddress(startAddress) : QString::number(startAddress, 16).toUpper();
        QString endStr = m_manager ? m_manager->formatDisplayAddress(startAddress + byteCount - 1) : QString::number(startAddress + byteCount - 1, 16).toUpper();

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

            showStatusMessage(QString("Defined array: [0x%1 - 0x%2] (%3 bytes)").arg(startStr, endStr, QString::number(byteCount)));
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

                showStatusMessage(QString("Defined array: [0x%1 - 0x%2] (%3 bytes)").arg(startStr, endStr, QString::number(byteCount)));
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

                    showStatusMessage(QString("Created Root Array Structure at 0x%1 (%2 bytes)").arg(displayAddrStr).arg(byteCount));
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
                QString targetStr = m_manager ? m_manager->formatDisplayAddress(resolved) : QString::number(resolved, 16).toUpper();
                showStatusMessage(QString("Navigated to 0x%1 in Memory Viewer").arg(targetStr));
                return;
            }
        }

        // Priority 2: If an address is selected in Memory Viewer and holds a valid pointer, follow it directly
        if (m_currentViewerSelectedAddress != 0 && m_manager) {
            uint32_t dwordVal = m_memoryViewer->readDwordAt(m_currentViewerSelectedAddress);
            if (dwordVal != 0 && m_manager && m_manager->isValidPointerTarget(dwordVal)) {
                uint32_t nativeTarget = m_manager->unnormalizeAddress(QString::number(dwordVal, 16));
                navigateToAddress(nativeTarget);
                QString targetStr = m_manager ? m_manager->formatDisplayAddress(nativeTarget) : QString::number(nativeTarget, 16).toUpper();
                showStatusMessage(QString("Followed pointer: navigated to 0x%1 in Memory Viewer").arg(targetStr));
                return;
            }
        }

        showStatusMessage("No pointer target could be resolved.");
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

        // Determine suggested data type based on active memory view mode and memory heuristics
        QString defaultType = "32-bit";
        if (mode == Widgets::MemoryViewerWidget::ViewMode::ModeFloat) {
            defaultType = "Float";
        }
        else if (mode == Widgets::MemoryViewerWidget::ViewMode::ModeFloatBE) {
            defaultType = "Float BE";
        }
        else if (mode == Widgets::MemoryViewerWidget::ViewMode::Mode16Bit) {
            defaultType = "16-bit";
        }
        else if (mode == Widgets::MemoryViewerWidget::ViewMode::Mode8Bit) {
            defaultType = "8-bit";
        }
        else if (mode == Widgets::MemoryViewerWidget::ViewMode::Mode32BitBE) {
            defaultType = "32-bit BE";
        }
        else {
            QString suggested = suggestDefaultType(alignedAddr);
            if (suggested.contains("Float", Qt::CaseInsensitive)) {
                defaultType = "Float";
            }
            else if (suggested.contains("16-bit", Qt::CaseInsensitive)) {
                defaultType = "16-bit";
            }
            else if (suggested.contains("8-bit", Qt::CaseInsensitive)) {
                defaultType = "8-bit";
            }
            else if (m_manager && m_manager->getRetroAchievementsPrefix() == "G") {
                defaultType = "32-bit BE";
            }
        }

        AddVariableDialog dialog(defaultType, "Variable", this);
        if (dialog.exec() != QDialog::Accepted) return;

        QString desc = dialog.getFullDescription();
        if (desc.trimmed().isEmpty()) return;

        auto* scopeNode = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
        m_hierarchyWidget->insertNodeInScope(scopeNode, match.offset, desc.trimmed(), match.isArrayScope);

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        syncTreeToCodeNoteText();
        updateMemoryHighlights();

        QString offsetHex = QString("%1").arg(std::abs(match.offset), 2, 16, QChar('0')).toUpper();
        QString prefix = match.isArrayScope ? "|" : ((match.offset < 0) ? "-" : "+");
        showStatusMessage(QString("Variable added: %1 at offset %2%3").arg(desc.trimmed(), prefix, "0x" + offsetHex));
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

        uint32_t dwordVal = 0;
        auto valOpt = StructureAnalyzer::readMemoryDword(alignedAddr, m_manager, m_snapshotMemory);
        if (valOpt.has_value()) {
            dwordVal = valOpt.value();
        }

        QString defaultPtrDesc = m_manager ? m_manager->getPlatformPointerTypeDescription(dwordVal) : "[32-bit] Pointer";
        
        AddPointerDialog dialog("Pointer", this);
        if (dialog.exec() != QDialog::Accepted) return;

        QString ptrDesc = dialog.getFullDescription(defaultPtrDesc);
        if (ptrDesc.trimmed().isEmpty()) return;

        auto* scopeNode = static_cast<QTreeWidgetItem*>(match.scopeIdentifier);
        m_hierarchyWidget->insertNodeInScope(scopeNode, match.offset, ptrDesc.trimmed(), match.isArrayScope);

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        syncTreeToCodeNoteText();
        updateMemoryHighlights();

        QString offsetHex = QString("%1").arg(std::abs(match.offset), 2, 16, QChar('0')).toUpper();
        QString prefix = match.isArrayScope ? "|" : ((match.offset < 0) ? "-" : "+");

        // Directly navigate into the new pointer's target in RAM
        if (valOpt.has_value() && m_manager && m_manager->isValidPointerTarget(valOpt.value())) {
            uint32_t nativeTarget = m_manager->unnormalizeAddress(QString::number(valOpt.value(), 16));
            navigateToAddress(nativeTarget);
            QString targetStr = m_manager ? m_manager->formatDisplayAddress(nativeTarget) : QString::number(nativeTarget, 16).toUpper();
            showStatusMessage(QString("Pointer added: %1 at offset %2%3 -> Target: 0x%4").arg(ptrDesc.trimmed(), prefix, "0x" + offsetHex, targetStr));
        }
        else {
            showStatusMessage(QString("Pointer added: %1 at offset %2%3").arg(ptrDesc.trimmed(), prefix, "0x" + offsetHex));
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

        int countDeleted = itemsToDelete.size();
        for (auto* item : itemsToDelete) {
            delete item;
        }

        uint32_t rootArrSize = 0;
        bool rootArr = isRootArray(&rootArrSize);
        m_hierarchyWidget->recalculateTreeAddresses(m_currentBaseAddress, m_snapshotMemory, rootArr);
        updateOffsetPreview();
        syncTreeToCodeNoteText();
        updateMemoryHighlights();

        showStatusMessage(QString("Removed %1 node(s) from structure hierarchy").arg(countDeleted));
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
            m_codeNoteEditor->setMinimumWidth(0);
            sizes[2] = 0;
            // Distribute available space evenly (50/50) between Panel 0 and Panel 1 in compact mode
            int totalAvailable = m_mainSplitter->width() - m_mainSplitter->handleWidth() * 2;
            int half = totalAvailable / 2;
            sizes[0] = half;
            sizes[1] = totalAvailable - half;
            m_mainSplitter->setSizes(sizes);
            m_addressBar->setCodeNotePanelButtonChecked(false);
            showStatusMessage("Code note panel collapsed.");
        }
        else {
            int restoreWidth = (m_lastCodeNotePanelWidth >= 320) ? m_lastCodeNotePanelWidth : 350;
            int contentW32 = m_memoryViewer ? m_memoryViewer->getContentWidth32Bit() : 485;

            m_codeNoteEditor->setMinimumWidth(320);

            int totalAvailable = m_mainSplitter->width() - m_mainSplitter->handleWidth() * 2;
            int remainingForPanel0 = std::max(280, totalAvailable - contentW32 - restoreWidth);
            sizes[0] = remainingForPanel0;
            sizes[1] = contentW32;
            sizes[2] = restoreWidth;
            m_mainSplitter->setSizes(sizes);
            m_addressBar->setCodeNotePanelButtonChecked(true);
            showStatusMessage("Code note panel expanded.");
        }
    }

    void StructureWorkspaceDialog::onSplitterMoved(int, int index) {
        if (index == 2) {
            QList<int> sizes = m_mainSplitter->sizes();
            if (sizes.size() > 2) {
                bool visible = (sizes[2] > 0);
                if (visible) {
                    m_lastCodeNotePanelWidth = sizes[2];
                    m_codeNoteEditor->setMinimumWidth(320);
                }
                else {
                    m_codeNoteEditor->setMinimumWidth(0);
                }
                m_addressBar->setCodeNotePanelButtonChecked(visible);
            }
        }
    }

    void StructureWorkspaceDialog::onHierarchyStyleChanged() {
    }

    void StructureWorkspaceDialog::onFixHierarchyClicked() {
        if (m_isSyncing || !m_codeNoteEditor) return;
        m_isSyncing = true;

        QString text = m_codeNoteEditor->toPlainText().trimmed();
        if (text.isEmpty()) {
            m_hierarchyWidget->clearTree();
            m_headerComment.clear();
            m_isSyncing = false;
            showStatusMessage("Cleared structure hierarchy.");
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

        m_isSyncing = false;

        showStatusMessage(QString("Hierarchy fixed and reformatted (%1 top-level node(s)).").arg(mergedTree.size()));
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

        m_isSyncing = false;
    }

    void StructureWorkspaceDialog::loadWorkspaceSettings() {
        m_isLoadingSettings = true;

        QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.beginGroup("DissectStructure");

        int savedWidth = settings.value("windowWidth", DEFAULT_WINDOW_WIDTH).toInt();
        int savedHeight = settings.value("windowHeight", DEFAULT_WINDOW_HEIGHT).toInt();
        bool isMax = settings.value("isMaximized", false).toBool();

        int contentW32 = m_memoryViewer ? m_memoryViewer->getContentWidth32Bit() : 485;

        m_lastCodeNotePanelWidth = settings.value("lastCodeNotePanelWidth", 350).toInt();
        if (m_lastCodeNotePanelWidth < 320) {
            m_lastCodeNotePanelWidth = 350;
        }

        bool panelVisible = settings.value("codeNotePanelVisible", true).toBool();

        int hWidth = settings.value("panelHierarchyWidth", 380).toInt();
        int mWidth = settings.value("panelMemoryViewWidth", contentW32).toInt();
        int cWidth = settings.value("panelCodeNoteWidth", 380).toInt();

        // Enforce healthy minimums
        hWidth = std::max(280, hWidth);
        mWidth = std::max(contentW32, mWidth);
        if (panelVisible) {
            cWidth = std::max(320, cWidth);
            m_codeNoteEditor->setMinimumWidth(320);
        }
        else {
            cWidth = 0;
            m_codeNoteEditor->setMinimumWidth(0);
        }

        // Apply clean window size bounded by default window size
        savedWidth = std::max(savedWidth, DEFAULT_WINDOW_WIDTH);
        savedHeight = std::max(savedHeight, DEFAULT_WINDOW_HEIGHT);
        resize(savedWidth, savedHeight);

        m_mainSplitter->setSizes({ hWidth, mWidth, cWidth });

        if (isMax) {
            showMaximized();
        }

        QString savedModeText = settings.value("viewModeText", "").toString();
        if (!savedModeText.isEmpty() && m_viewModeCombo) {
            int idx = m_viewModeCombo->findText(savedModeText);
            if (idx != -1) {
                m_viewModeCombo->blockSignals(true);
                m_viewModeCombo->setCurrentIndex(idx);
                m_viewModeCombo->blockSignals(false);
            }
        }
        else {
            int viewModeId = settings.value("viewMode", 2).toInt();
            if (m_viewModeCombo && viewModeId >= 0 && viewModeId < m_viewModeCombo->count()) {
                m_viewModeCombo->blockSignals(true);
                m_viewModeCombo->setCurrentIndex(viewModeId);
                m_viewModeCombo->blockSignals(false);
            }
        }
        onViewModeChanged();

        int floatModeId = settings.value("floatDisplayMode", 0).toInt();
        if (m_floatDisplayCombo && floatModeId >= 0 && floatModeId < m_floatDisplayCombo->count()) {
            m_floatDisplayCombo->blockSignals(true);
            m_floatDisplayCombo->setCurrentIndex(floatModeId);
            m_floatDisplayCombo->blockSignals(false);
            onFloatDisplayModeChanged();
        }

        QString style = settings.value("hierarchyStyle", "Plusses (+)").toString();
        m_codeNoteEditor->setStyle(style);
        m_codeNoteEditor->setIndentDescriptions(settings.value("indentDescriptions", true).toBool());

        m_addressBar->setCodeNotePanelButtonChecked(panelVisible);

        settings.endGroup();

        m_isLoadingSettings = false;
    }

    void StructureWorkspaceDialog::saveWorkspaceSettings() {
        if (m_isLoadingSettings) return;

        QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.beginGroup("DissectStructure");

        // Save window geometry as clean integers
        settings.setValue("isMaximized", isMaximized());
        if (!isMaximized()) {
            settings.setValue("windowWidth", std::max(width(), DEFAULT_WINDOW_WIDTH));
            settings.setValue("windowHeight", std::max(height(), DEFAULT_WINDOW_HEIGHT));
        }

        // Save splitter panel sizes as clean integers
        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() >= 3) {
            settings.setValue("panelHierarchyWidth", sizes[0]);
            settings.setValue("panelMemoryViewWidth", sizes[1]);
            settings.setValue("panelCodeNoteWidth", sizes[2]);
            settings.setValue("codeNotePanelVisible", sizes[2] > 0);
        }

        settings.setValue("lastCodeNotePanelWidth", m_lastCodeNotePanelWidth);

        settings.setValue("viewMode", m_viewModeCombo ? m_viewModeCombo->currentIndex() : 2);
        settings.setValue("viewModeText", m_viewModeCombo ? m_viewModeCombo->currentText() : "32-bit");
        settings.setValue("floatDisplayMode", m_floatDisplayCombo ? m_floatDisplayCombo->currentIndex() : 0);

        settings.setValue("hierarchyStyle", m_codeNoteEditor->getStyle());
        settings.setValue("indentDescriptions", m_codeNoteEditor->isIndentDescriptions());

        settings.endGroup();
    }

}
