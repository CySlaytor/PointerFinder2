#define NOMINMAX 
#include "MainWindow.h"

#include "../Common/EndianUtils.h"
#include "../Core/ArrayDetector.h"
#include "../Core/CodeNoteHelper.h"
#include "../Core/ProcessScanner.h"
#include "../Core/SessionManager.h"
#include "../Core/SettingsManager.h"
#include "../Core/SoundManager.h"
#include "../Emulators/EmulatorProfileRegistry.h"
#include "../Models/CodeNoteSettings.h"
#include "../Models/SessionData.h"
#include "Dialogs/AboutDialog.h"
#include "Dialogs/AddressSearchDialog.h"
#include "Dialogs/CodeNoteConverterDialog.h"
#include "Dialogs/CodeNoteHierarchyFixerDialog.h"
#include "Dialogs/SettingsDialog.h"
#include "Dialogs/StateCaptureDialog.h"
#include "Dialogs/UpdateDialog.h"
#include "StaticRangeFinders/DolphinFileRangeFinderDialog.h"
#include "StaticRangeFinders/NdsStaticRangeFinderDialog.h"
#include "StaticRangeFinders/Pcsx2RamScanRangeFinderDialog.h"
#include "StaticRangeFinders/PpssppRamScanRangeFinderDialog.h"
#include "Styles/ThemeManager.h"
#include "Widgets/PointerResultsHeaderView.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStatusBar>
#include <QtConcurrent>
#include <QUrl>
#include <QVBoxLayout>
#include <unordered_set>

#ifndef VERSION_STRING
#define VERSION_STRING "0.0.0"
#endif

namespace PointerFinder2::UI {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::Emulators;
    using namespace PointerFinder2::DataModels;

    const QString CURRENT_VERSION = VERSION_STRING;

    MainWindow::MainWindow(const QStringList& args, QWidget* parent)
        : QMainWindow(parent) {
        setWindowTitle("Pointer Finder 2.0");
        resize(980, 600);
        setMinimumSize(640, 480);

        setupUI();
        setUIStateDetached();

        connect(&m_resultsManager, &ResultsManager::resultsChanged, this, &MainWindow::onResultsChanged);
        connect(&m_resultsManager, &ResultsManager::sortChanged, this, &MainWindow::onSortChanged);

        connect(&m_scanCoordinator, &ScanCoordinator::operationStarted, this, &MainWindow::onOperationStarted);
        connect(&m_scanCoordinator, &ScanCoordinator::operationFinished, this, &MainWindow::onOperationFinished);
        connect(&m_scanCoordinator, &ScanCoordinator::progressUpdated, this, &MainWindow::onProgressUpdated);
        connect(&m_scanCoordinator, &ScanCoordinator::foundCountUpdated, this, &MainWindow::onFoundCountUpdated);
        connect(&m_scanCoordinator, &ScanCoordinator::scanCompleted, this, &MainWindow::onScanCompleted);
        connect(&m_scanCoordinator, &ScanCoordinator::filterCompleted, this, &MainWindow::onFilterCompleted);
        connect(&m_arrayWatcher, &QFutureWatcher<Controls::ArrayDetectionResult>::finished, this, &MainWindow::onArrayDetectionFinished);

        connect(&m_updateChecker, &UpdateChecker::updateCheckFinished, this, &MainWindow::onUpdateCheckFinished);
        connect(&m_updateChecker, &UpdateChecker::updateCheckFailed, this, &MainWindow::onUpdateCheckFailed);

        m_scanTimer.setInterval(100);
        connect(&m_scanTimer, &QTimer::timeout, this, &MainWindow::onScanTimerTick);

        m_processMonitorTimer.setInterval(2000);
        connect(&m_processMonitorTimer, &QTimer::timeout, this, &MainWindow::onProcessMonitorTick);
        m_processMonitorTimer.start();

        QSettings settings;
        settings.beginGroup("MainWindow");
        if (settings.contains("geometry")) {
            restoreGeometry(settings.value("geometry").toByteArray());
        }
        settings.endGroup();

        parseCommandLineArgs(args);

        m_updateChecker.checkForUpdates(true);
    }

    MainWindow::~MainWindow() {
        m_arrayWatcher.cancel();
        m_arrayWatcher.waitForFinished();
    }

    QIcon MainWindow::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void MainWindow::updateThemeIcons() {
        const auto actionsList = findChildren<QAction*>();
        for (QAction* action : actionsList) {
            QVariant iconPathVar = action->property("iconPath");
            if (iconPathVar.isValid()) {
                action->setIcon(getThemeIcon(iconPathVar.toString()));
            }
        }

        if (m_emptyStateIconLabel) {
            m_emptyStateIconLabel->setPixmap(getThemeIcon(":/icons/find.svg").pixmap(QSize(48, 48)));
        }

        if (m_bookmarksWidget) {
            m_bookmarksWidget->updateThemeIcons();
        }

        if (m_arrayDetectionWidget) {
            m_arrayDetectionWidget->updateThemeIcons();
        }
    }

    void MainWindow::setupUI() {
        auto* centralWidget = new QWidget(this);
        auto* mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        setupMenuBar();

        m_mainTabWidget = new QTabWidget(centralWidget);
        m_viewStack = new QStackedWidget(m_mainTabWidget);

        setupEmptyStateView();
        setupResultsTableView();

        m_mainTabWidget->addTab(m_viewStack, "Search Results");

        m_bookmarksWidget = new Controls::BookmarksWidget(m_mainTabWidget);
        connect(m_bookmarksWidget, &Controls::BookmarksWidget::statusMessageRequested, this, &MainWindow::updateStatus);
        m_mainTabWidget->addTab(m_bookmarksWidget, "Bookmarks");

        m_arrayDetectionWidget = new Controls::ArrayDetectionWidget(m_mainTabWidget);
        connect(m_arrayDetectionWidget, &Controls::ArrayDetectionWidget::statusMessageRequested, this, &MainWindow::updateStatus);
        connect(m_arrayDetectionWidget, &Controls::ArrayDetectionWidget::bookmarkRequested, this, [this](const PointerPath& path) {
            Bookmark bm;
            bm.path = path;
            QString baseStr = m_currentManager ? m_currentManager->formatDisplayAddress(bm.path.baseAddress) : QString::number(bm.path.baseAddress, 16).toUpper();
            bm.name = QString("Watchpoint [0x%1]").arg(baseStr);
            m_bookmarksWidget->addBookmark(bm);
            });
        m_mainTabWidget->addTab(m_arrayDetectionWidget, "Array Detection");

        mainLayout->addWidget(m_mainTabWidget, 1);

        setupBottomPanel();
        mainLayout->addWidget(m_bottomPanel);

        setCentralWidget(centralWidget);
        setupStatusBar();
        setWindowIcon(QIcon(":/appicon.ico"));
    }

    void MainWindow::setupEmptyStateView() {
        m_emptyStateWidget = new QWidget(m_viewStack);
        auto* emptyLayout = new QVBoxLayout(m_emptyStateWidget);
        emptyLayout->setAlignment(Qt::AlignCenter);
        emptyLayout->setContentsMargins(40, 40, 40, 40);
        emptyLayout->setSpacing(12);

        m_emptyStateIconLabel = new QLabel(m_emptyStateWidget);
        m_emptyStateIconLabel->setPixmap(getThemeIcon(":/icons/find.svg").pixmap(QSize(48, 48)));
        m_emptyStateIconLabel->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(m_emptyStateIconLabel);

        auto* titleLabel = new QLabel("No Scan Results Loaded", m_emptyStateWidget);
        titleLabel->setObjectName("emptyStateTitle");
        titleLabel->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(titleLabel);

        auto* descLabel = new QLabel(
            "To begin scanning or locating memory pointers:\n\n"
            "1. Click File -> Attach to Emulator... to bind to a running emulator process.\n"
            "2. Click the State-Based Scan button to capture memory states and search for paths.\n"
            "3. Once attached, you can also click File -> Load Session... to resume a saved session.",
            m_emptyStateWidget
        );
        descLabel->setObjectName("emptyStateDesc");
        descLabel->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(descLabel);

        m_viewStack->addWidget(m_emptyStateWidget);
    }

    void MainWindow::setupResultsTableView() {
        m_tableView = new QTableView(m_viewStack);

        auto* customHeader = new Controls::PointerResultsHeaderView(Qt::Horizontal, m_tableView);
        m_tableView->setHorizontalHeader(customHeader);

        m_tableModel = new Controls::PointerResultsModel(&m_resultsManager, this);
        m_tableView->setModel(m_tableModel);

        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_tableView->setAlternatingRowColors(true);

        m_tableView->setWordWrap(false);
        m_tableView->setCornerButtonEnabled(false);
        m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        m_tableView->setTextElideMode(Qt::ElideRight);
        m_tableView->viewport()->setAttribute(Qt::WA_StaticContents);

        m_tableView->verticalHeader()->setVisible(false);
        m_tableView->verticalHeader()->setMinimumSectionSize(24);
        m_tableView->verticalHeader()->setDefaultSectionSize(24);
        m_tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        m_tableView->horizontalHeader()->setFixedHeight(28);
        m_tableView->horizontalHeader()->setMinimumSectionSize(40);
        m_tableView->horizontalHeader()->setDefaultSectionSize(60);
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        m_tableView->horizontalHeader()->setSortIndicatorShown(true);

        connect(m_tableModel, &QAbstractItemModel::modelReset, this, [this]() {
            int colCount = m_tableModel->columnCount();
            if (colCount > 0) {
                m_tableView->setColumnWidth(0, 100);
                for (int i = 1; i < colCount - 1; ++i) {
                    m_tableView->setColumnWidth(i, 60);
                }
                if (colCount > 1) {
                    m_tableView->setColumnWidth(colCount - 1, 90);
                }
            }
            });

        m_copyBaseAddressAction = new QAction(getThemeIcon(":/icons/copy.svg"), "Copy Base Address", this);
        m_copyBaseAddressAction->setProperty("iconPath", ":/icons/copy.svg");
        m_copyBaseAddressAction->setShortcut(QKeySequence::Copy);
        connect(m_copyBaseAddressAction, &QAction::triggered, this, &MainWindow::onCopyBaseAddress);

        m_copyAsRAAction = new QAction(getThemeIcon(":/icons/copy_ra.svg"), "Copy as RetroAchievements Format", this);
        m_copyAsRAAction->setProperty("iconPath", ":/icons/copy_ra.svg");
        m_copyAsRAAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        connect(m_copyAsRAAction, &QAction::triggered, this, &MainWindow::onCopyAsRAFormat);

        m_copyAsCodeNoteAction = new QAction(getThemeIcon(":/icons/copy_note.svg"), "Copy as Code Note...", this);
        m_copyAsCodeNoteAction->setProperty("iconPath", ":/icons/copy_note.svg");
        connect(m_copyAsCodeNoteAction, &QAction::triggered, this, &MainWindow::onCopyAsCodeNote);

        m_deleteSelectedAction = new QAction(getThemeIcon(":/icons/delete.svg"), "Delete Selected", this);
        m_deleteSelectedAction->setProperty("iconPath", ":/icons/delete.svg");
        m_deleteSelectedAction->setShortcut(QKeySequence::Delete);
        connect(m_deleteSelectedAction, &QAction::triggered, this, &MainWindow::onDeleteSelected);

        m_tableView->addAction(m_copyBaseAddressAction);
        m_tableView->addAction(m_copyAsRAAction);
        m_tableView->addAction(m_deleteSelectedAction);

        m_tableView->installEventFilter(this);

        connect(m_tableView, &QWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
        connect(m_tableView->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
            QString colName = m_tableModel->headerData(logicalIndex, Qt::Horizontal).toString();
            if (colName == "Base Address") colName = "colBase";
            else if (colName == "Final Address") colName = "colFinal";
            else colName = QString("colOffset%1").arg(logicalIndex);

            m_resultsManager.sort(colName, m_resultsManager.getSortOrder());
            });

        m_viewStack->addWidget(m_tableView);
        m_viewStack->setCurrentWidget(m_emptyStateWidget);
    }

    void MainWindow::setupBottomPanel() {
        m_bottomPanel = new QWidget(this);
        auto* bottomLayout = new QVBoxLayout(m_bottomPanel);
        bottomLayout->setContentsMargins(5, 5, 5, 5);

        auto* progressRow = new QHBoxLayout();
        m_timeLabel = new QLabel("Time: 00:00.0", m_bottomPanel);
        m_progressBar = new QProgressBar(m_bottomPanel);
        m_progressBar->setValue(0);
        m_progressLabel = new QLabel("0 / 0", m_bottomPanel);
        m_stopScanButton = new QPushButton("Stop", m_bottomPanel);

        progressRow->addWidget(m_timeLabel);
        progressRow->addWidget(m_progressBar, 1);
        progressRow->addWidget(m_progressLabel);
        progressRow->addWidget(m_stopScanButton);
        bottomLayout->addLayout(progressRow);

        m_scanButtonsWidget = new QWidget(m_bottomPanel);
        auto* scanButtonsLayout = new QHBoxLayout(m_scanButtonsWidget);
        scanButtonsLayout->setContentsMargins(0, 0, 0, 0);

        m_stateScanButton = new QPushButton("State-Based Scan", m_scanButtonsWidget);
        m_filterButton = new QPushButton("Filter Dynamic Paths", m_scanButtonsWidget);
        m_stateScanButton->setFixedHeight(35);
        m_filterButton->setFixedHeight(35);

        scanButtonsLayout->addWidget(m_stateScanButton);
        scanButtonsLayout->addWidget(m_filterButton);
        bottomLayout->addWidget(m_scanButtonsWidget);

        connect(m_stateScanButton, &QPushButton::clicked, this, &MainWindow::onStateBasedScanClicked);
        connect(m_filterButton, &QPushButton::clicked, this, &MainWindow::onFilterClicked);
        connect(m_stopScanButton, &QPushButton::clicked, this, &MainWindow::onStopScanClicked);
    }

    void MainWindow::setupMenuBar() {
        auto* bar = menuBar();

        auto* fileMenu = bar->addMenu("&File");
        auto* attachAct = fileMenu->addAction(getThemeIcon(":/icons/attach.svg"), "Attach to Emulator...", this, &MainWindow::onAttachClicked);
        attachAct->setObjectName("menuAttach");
        attachAct->setProperty("iconPath", ":/icons/attach.svg");

        fileMenu->addSeparator();

        auto* saveAct = fileMenu->addAction(getThemeIcon(":/icons/save.svg"), "Save Session...", this, &MainWindow::onSaveSessionClicked);
        saveAct->setShortcut(QKeySequence::Save);
        saveAct->setProperty("iconPath", ":/icons/save.svg");
        saveAct->setObjectName("menuSave");

        auto* loadAct = fileMenu->addAction(getThemeIcon(":/icons/load.svg"), "Load Session...", this, &MainWindow::onLoadSessionClicked);
        loadAct->setShortcut(QKeySequence::Open);
        loadAct->setProperty("iconPath", ":/icons/load.svg");
        loadAct->setObjectName("menuLoad");

        fileMenu->addSeparator();
        fileMenu->addAction("E&xit", qApp, &QCoreApplication::quit);

        auto* editMenu = bar->addMenu("&Edit");
        auto* undoAct = editMenu->addAction(getThemeIcon(":/icons/undo.svg"), "Undo", this, &MainWindow::onUndoClicked);
        undoAct->setShortcut(QKeySequence::Undo);
        undoAct->setProperty("iconPath", ":/icons/undo.svg");

        auto* redoAct = editMenu->addAction(getThemeIcon(":/icons/redo.svg"), "Redo", this, &MainWindow::onRedoClicked);
        redoAct->setShortcut(QKeySequence::Redo);
        redoAct->setProperty("iconPath", ":/icons/redo.svg");

        editMenu->addSeparator();
        auto* findAct = editMenu->addAction(getThemeIcon(":/icons/find.svg"), "Find...", this, &MainWindow::onFindClicked);
        findAct->setShortcut(QKeySequence::Find);
        findAct->setProperty("iconPath", ":/icons/find.svg");

        editMenu->addSeparator();
        auto* settingsAct = editMenu->addAction(getThemeIcon(":/icons/settings.svg"), "Settings...", this, &MainWindow::onSettingsClicked);
        settingsAct->setProperty("iconPath", ":/icons/settings.svg");

        auto* toolsMenu = bar->addMenu("&Tools");
        auto* rangeAct = toolsMenu->addAction(getThemeIcon(":/icons/range_finder.svg"), "Static Range Finder...", this, &MainWindow::onStaticRangeFinderClicked);
        rangeAct->setProperty("iconPath", ":/icons/range_finder.svg");
        rangeAct->setEnabled(false);

        auto* convAct = toolsMenu->addAction(getThemeIcon(":/icons/converter.svg"), "Code Note Converter...", this, &MainWindow::onCodeNoteConverterClicked);
        convAct->setProperty("iconPath", ":/icons/converter.svg");

        auto* hierAct = toolsMenu->addAction(getThemeIcon(":/icons/hierarchy_fixer.svg"), "Code Note Hierarchy Fixer...", this, &MainWindow::onCodeNoteHierarchyFixerClicked);
        hierAct->setProperty("iconPath", ":/icons/hierarchy_fixer.svg");

        auto* helpMenu = bar->addMenu("&Help");
        auto* tutAct = helpMenu->addAction(getThemeIcon(":/icons/tutorial.svg"), "Video Tutorial...", this, &MainWindow::onTutorialClicked);
        tutAct->setProperty("iconPath", ":/icons/tutorial.svg");

        helpMenu->addSeparator();

        auto* checkUpdatesAct = helpMenu->addAction(getThemeIcon(":/icons/update.svg"), "Check for Updates...", this, &MainWindow::onCheckForUpdatesClicked);
        checkUpdatesAct->setProperty("iconPath", ":/icons/update.svg");

        auto* aboutAct = helpMenu->addAction(getThemeIcon(":/icons/about.svg"), "About Pointer Finder 2.0", this, &MainWindow::onAboutClicked);
        aboutAct->setProperty("iconPath", ":/icons/about.svg");

        connect(fileMenu, &QMenu::aboutToShow, this, [=]() {
            bool active = m_currentManager && m_currentManager->isAttached();
            saveAct->setEnabled(active && m_resultsManager.hasResults());
            loadAct->setEnabled(active);
            });

        connect(editMenu, &QMenu::aboutToShow, this, [=]() {
            undoAct->setEnabled(m_resultsManager.canUndo());
            redoAct->setEnabled(m_resultsManager.canRedo());
            findAct->setEnabled(m_resultsManager.hasResults());
            });

        connect(toolsMenu, &QMenu::aboutToShow, this, [=]() {
            bool attached = m_currentManager && m_currentManager->isAttached();
            bool supportsStatic = m_activeProfile.has_value() && m_activeProfile->supportsStaticRangeFinder;
            rangeAct->setEnabled(attached && supportsStatic);
            });
    }

    void MainWindow::setupStatusBar() {
        auto* bar = statusBar();

        m_statusLabel = new QLabel("Status: Not Attached", this);
        m_baseAddressLabel = new QLabel("", this);
        m_resultCountLabel = new QLabel("Results: 0", this);

        bar->addWidget(m_statusLabel, 1);
        bar->addPermanentWidget(m_baseAddressLabel);
        bar->addPermanentWidget(m_resultCountLabel);
    }

    bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
        if (obj == m_tableView && event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->matches(QKeySequence::Paste)) {
                QString clipboardText = QApplication::clipboard()->text().trimmed();
                if (!clipboardText.isEmpty() && (clipboardText.contains("I:0x") || clipboardText.contains("0xH"))) {
                    onCodeNoteConverterClicked();
                    if (m_codeNoteConverterDialog) {
                        m_codeNoteConverterDialog->processTrigger(clipboardText);
                        m_codeNoteConverterDialog->raise();
                        m_codeNoteConverterDialog->activateWindow();
                    }
                    return true;
                }
            }
        }
        return QMainWindow::eventFilter(obj, event);
    }

    void MainWindow::onAttachClicked() {
        if (m_currentManager && m_currentManager->isAttached()) {
            detachAndReset();
            return;
        }

        auto runningInstances = ProcessScanner::scanForRunningEmulators();
        if (runningInstances.empty()) {
            QMessageBox::information(this, "Attach Failed", "No supported emulator process found.");
            return;
        }

        if (runningInstances.size() == 1) {
            performAttachment(runningInstances[0].profile, runningInstances[0].pid);
        }
        else {
            switchToScanUI(false);
            EmulatorSelectionDialog dialog(runningInstances, this);
            if (dialog.exec() == QDialog::Accepted) {
                auto sel = dialog.getSelectedInstance();
                performAttachment(sel.profile, sel.pid);
            }
        }
    }

    // Establishes a system connection with the selected emulator process.
    // Configures manager strategies, loads system bounds, and updates central window properties.
    bool MainWindow::performAttachment(const EmulatorProfile& profile, uint32_t processId) {
        m_currentManager = profile.managerFactory();
        m_activeProfile = profile;

        auto defaultSettings = m_currentManager->getDefaultSettings();
        m_currentSettings = SettingsManager::load(profile.target, defaultSettings);

        if (m_currentManager->attach(processId)) {
            m_tableModel->setEmulatorManager(m_currentManager.get());
            m_bookmarksWidget->setEmulatorManager(m_currentManager.get());
            m_arrayDetectionWidget->setEmulatorManager(m_currentManager.get());

            QString emuName = m_currentManager->getEmulatorName();
            QString windowTitle = m_currentManager->getEmulatorName();

            setWindowTitle(QString("Pointer Finder 2.0 - [%1]").arg(windowTitle));
            updateStatus(QString("Attached to %1 (PID: %2)").arg(emuName).arg(processId));

            m_baseAddressLabel->setText(QString("%1 Base (PC): %2")
                .arg(emuName.split(' ').first())
                .arg(QString::number(m_currentManager->getMemoryBasePC(), 16).toUpper()));

            auto* attachAct = findChild<QAction*>("menuAttach");
            if (attachAct) {
                attachAct->setText(QString("Detach from %1").arg(emuName));
            }

            auto* loadAct = findChild<QAction*>("menuLoad");
            if (loadAct) {
                loadAct->setEnabled(true);
            }

            switchToScanUI(false);
            return true;
        }
        else {
            QMessageBox::critical(this, "Attach Failed", QString("Could not find required memory exports for %1.").arg(profile.name));
            detachAndReset();
            return false;
        }
    }

    void MainWindow::detachAndReset() {
        m_scanCoordinator.stopCurrentOperation();
        m_scanCoordinator.waitForDecoupling();

        if (m_currentManager) {
            m_currentManager->detach();
        }
        m_currentManager.reset();
        m_activeProfile = std::nullopt;
        m_resultsManager.clearHistory();
        m_resultsManager.clearResults();
        m_multiScanState.clearAll();

        m_bookmarksWidget->clearBookmarks();
        m_bookmarksWidget->setEmulatorManager(nullptr);
        m_arrayDetectionWidget->clear();
        m_arrayDetectionWidget->setEmulatorManager(nullptr);

        setUIStateDetached();
    }

    void MainWindow::setUIStateDetached() {
        setWindowTitle("Pointer Finder 2.0");
        updateStatus("Not Attached");
        m_baseAddressLabel->clear();
        m_resultCountLabel->setVisible(false);

        auto* attachAct = findChild<QAction*>("menuAttach");
        if (attachAct) {
            attachAct->setText("Attach to Emulator...");
        }

        auto* loadAct = findChild<QAction*>("menuLoad");
        if (loadAct) {
            loadAct->setEnabled(false);
        }

        m_stateScanButton->setEnabled(false);
        m_filterButton->setEnabled(false);
        switchToScanUI(false);
    }

    void MainWindow::switchToScanUI(bool scanningActive, const QString& customMessage) {
        m_scanButtonsWidget->setVisible(!scanningActive);
        m_progressBar->setVisible(scanningActive);
        m_stopScanButton->setVisible(scanningActive);
        m_progressLabel->setVisible(scanningActive);
        m_timeLabel->setVisible(scanningActive);

        bool attached = m_currentManager && m_currentManager->isAttached();
        m_resultCountLabel->setVisible(!scanningActive && m_resultsManager.hasResults());

        if (scanningActive) {
            if (!customMessage.isEmpty()) {
                updateStatus(customMessage);
            }
            menuBar()->setEnabled(false);
        }
        else {
            menuBar()->setEnabled(true);
            m_stateScanButton->setEnabled(attached);
            m_filterButton->setEnabled(attached && m_resultsManager.hasResults());
        }
    }

    void MainWindow::updateStatus(const QString& status) {
        m_statusLabel->setText(QString("Status: %1").arg(status));
    }

    void MainWindow::onProcessMonitorTick() {
        if (m_currentManager && m_currentManager->isAttached()) {
            if (!m_currentManager->isProcessRunning()) {
                detachAndReset();
            }
        }
    }

    void MainWindow::parseCommandLineArgs(const QStringList& args) {
        uint32_t pidToAutoAttach = 0;
        QString targetToAutoAttach = "";
        bool isRestart = false;

        for (const auto& arg : args) {
            if (arg.compare("/restart", Qt::CaseInsensitive) == 0) {
                isRestart = true;
            }
            else if (arg.startsWith("/pid:", Qt::CaseInsensitive)) {
                pidToAutoAttach = arg.mid(5).toUInt();
            }
            else if (arg.startsWith("/target:", Qt::CaseInsensitive)) {
                targetToAutoAttach = arg.mid(8);
            }
        }

        if (isRestart && QFile::exists("restart.tmp")) {
            QFile file("restart.tmp");
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                int x = in.readLine().toInt();
                int y = in.readLine().toInt();
                int w = in.readLine().toInt();
                int h = in.readLine().toInt();
                setGeometry(x, y, w, h);
                file.close();
                QFile::remove("restart.tmp");
            }
        }

        if (pidToAutoAttach != 0 && !targetToAutoAttach.isEmpty()) {
            autoAttachOnRestart(pidToAutoAttach, targetToAutoAttach);
        }
    }

    void MainWindow::autoAttachOnRestart(uint32_t pid, const QString& targetName) {
        const auto& profiles = EmulatorProfileRegistry::getProfiles();
        for (const auto& prof : profiles) {
            if (QString::compare(QString::number(static_cast<int>(prof.target)), targetName, Qt::CaseInsensitive) == 0 ||
                prof.name.compare(targetName, Qt::CaseInsensitive) == 0) {
                performAttachment(prof, pid);
                break;
            }
        }
    }

    void MainWindow::restartApplication() {
        QStringList args = { "/restart" };
        if (m_activeProfile.has_value() && m_currentManager && m_currentManager->isAttached()) {
            args << QString("/target:%1").arg(m_activeProfile->name);
            args << QString("/pid:%1").arg(m_currentManager->getProcessId());
        }

        QFile file("restart.tmp");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << geometry().x() << "\n"
                << geometry().y() << "\n"
                << geometry().width() << "\n"
                << geometry().height() << "\n";
            file.close();
        }

        QProcess::startDetached(qApp->arguments()[0], args);
        qApp->quit();
        std::exit(0);
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        if (m_bookmarksWidget && !m_bookmarksWidget->getBookmarks().empty()) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Save Session",
                "You have unsaved bookmarks. Would you like to save your current session before exiting?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::Yes
            );

            if (reply == QMessageBox::Yes) {
                if (onSaveSessionClicked()) {
                    QSettings settings;
                    settings.beginGroup("MainWindow");
                    settings.setValue("geometry", saveGeometry());
                    settings.endGroup();
                    event->accept();
                }
                else {
                    event->ignore();
                }
            }
            else if (reply == QMessageBox::No) {
                QSettings settings;
                settings.beginGroup("MainWindow");
                settings.setValue("geometry", saveGeometry());
                settings.endGroup();
                event->accept();
            }
            else {
                event->ignore();
            }
        }
        else {
            QSettings settings;
            settings.beginGroup("MainWindow");
            settings.setValue("geometry", saveGeometry());
            settings.endGroup();
            event->accept();
        }
    }

    bool MainWindow::onSaveSessionClicked() {
        QString defaultFileName = QString("session_%1.pfs")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

        QString filePath = QFileDialog::getSaveFileName(
            this,
            "Save Session",
            defaultFileName,
            "Pointer Finder Session (*.pfs)"
        );

        if (filePath.isEmpty()) {
            return false;
        }

        SessionData sessionData;
        sessionData.emulatorTargetName = m_activeProfile.has_value() ? m_activeProfile->name : "";
        sessionData.processId = m_currentManager ? static_cast<int32_t>(m_currentManager->getProcessId()) : -1;
        sessionData.lastScanParameters = m_scanCoordinator.getLastScanParams();
        sessionData.results = m_resultsManager.getCurrentResults();

        sessionData.bookmarks = m_bookmarksWidget->getBookmarks();
        sessionData.arrayGroups = m_arrayDetectionWidget->getArrayGroups();

        sessionData.sortedColumnName = m_resultsManager.getSortedColumnName();
        sessionData.sortDirection = static_cast<int>(m_resultsManager.getSortOrder());

        SessionManager mgr;
        if (mgr.saveSession(sessionData, filePath)) {
            updateStatus("Session saved.");
            return true;
        }
        return false;
    }

    void MainWindow::onLoadSessionClicked() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Load Session",
            "",
            "Pointer Finder Session (*.pfs);;All files (*.*)"
        );

        if (filePath.isEmpty()) {
            return;
        }

        SessionManager mgr;
        auto sessionOpt = mgr.loadSession(filePath);
        if (!sessionOpt.has_value()) return;

        auto session = sessionOpt.value();
        if (m_currentManager && m_currentManager->isAttached()) {
            if (m_activeProfile.has_value() && m_activeProfile->name != session.emulatorTargetName) {
                auto reply = QMessageBox::question(
                    this,
                    "Emulator Target Mismatch",
                    QString("This session was saved for '%1', but you are currently attached to '%2'.\n\n"
                        "Loading this session may lead to unstable address evaluation. Would you like to proceed?")
                    .arg(session.emulatorTargetName)
                    .arg(m_activeProfile->name),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No
                );
                if (reply == QMessageBox::No) {
                    return;
                }
            }
        }

        detachAndReset();

        const auto& profiles = EmulatorProfileRegistry::getProfiles();
        for (const auto& prof : profiles) {
            if (prof.name == session.emulatorTargetName) {
                m_activeProfile = prof;
                m_currentManager = m_activeProfile->managerFactory();
                setUIStateDetached();
                setWindowTitle("Pointer Finder 2.0 - [Loaded Session]");

                auto* attachAct = findChild<QAction*>("menuAttach");
                if (attachAct) {
                    attachAct->setText(QString("Attach to %1").arg(prof.name));
                }

                m_bookmarksWidget->setEmulatorManager(m_currentManager.get());
                m_arrayDetectionWidget->setEmulatorManager(m_currentManager.get());
                break;
            }
        }

        m_resultsManager.setNewResults(session.results, true);
        m_scanCoordinator.setLastScanParams(session.lastScanParameters);

        m_bookmarksWidget->setBookmarks(session.bookmarks);
        m_arrayDetectionWidget->setResults(session.arrayGroups, {});

        if (m_activeProfile.has_value() && session.processId != -1) {
            performAttachment(m_activeProfile.value(), static_cast<uint32_t>(session.processId));
        }

        updateStatus(QString("Session loaded with %1 results, %2 bookmark(s), and %3 array(s).")
            .arg(m_resultsManager.getResultsCount())
            .arg(m_bookmarksWidget->getBookmarks().size())
            .arg(m_arrayDetectionWidget->getArrayGroups().size()));
    }

    void MainWindow::onUndoClicked() { m_resultsManager.undo(); }
    void MainWindow::onRedoClicked() { m_resultsManager.redo(); }

    void MainWindow::onFindClicked() {
        AddressSearchDialog dialog(m_currentSearchTerm, this);
        if (dialog.exec() == QDialog::Accepted) {
            m_currentSearchTerm = dialog.getSearchAddress();

            if (m_currentManager) {
                const auto& results = m_resultsManager.getCurrentResults();
                for (size_t i = 0; i < results.size(); ++i) {
                    QString addrStr = m_currentManager->formatDisplayAddress(results[i].baseAddress);
                    if (addrStr.compare(m_currentSearchTerm, Qt::CaseInsensitive) == 0) {
                        m_tableView->selectRow(static_cast<int>(i));
                        m_tableView->scrollTo(m_tableModel->index(static_cast<int>(i), 0));
                        updateStatus(QString("Found and selected base address '%1'.").arg(m_currentSearchTerm));
                        return;
                    }
                }
                updateStatus(QString("Base address '%1' not found in results.").arg(m_currentSearchTerm));
            }
        }
    }

    void MainWindow::onSettingsClicked() {
        SettingsDialog dialog(this);
        dialog.exec();
    }

    void MainWindow::onStaticRangeFinderClicked() {
        if (m_staticRangeFinderDialog) {
            m_staticRangeFinderDialog->raise();
            m_staticRangeFinderDialog->activateWindow();
            return;
        }

        if (m_activeProfile.has_value() && m_activeProfile->supportsStaticRangeFinder) {
            QWidget* widget = nullptr;
            auto target = m_activeProfile->target;

            if (target == EmulatorTarget::PCSX2) {
                widget = new StaticRangeFinders::Pcsx2RamScanRangeFinderDialog(m_currentManager.get(), this);
            }
            else if (target == EmulatorTarget::RALibretroNDS) {
                widget = new StaticRangeFinders::NdsStaticRangeFinderDialog(m_currentManager.get(), this);
            }
            else if (target == EmulatorTarget::Dolphin) {
                widget = new StaticRangeFinders::DolphinFileRangeFinderDialog(m_currentManager.get(), this);
            }
            else if (target == EmulatorTarget::PPSSPP) {
                widget = new StaticRangeFinders::PpssppRamScanRangeFinderDialog(m_currentManager.get(), this);
            }

            if (widget) {
                m_staticRangeFinderDialog = qobject_cast<QDialog*>(widget);
                if (m_staticRangeFinderDialog) {
                    m_staticRangeFinderDialog->setAttribute(Qt::WA_DeleteOnClose);
                    connect(m_staticRangeFinderDialog, &QDialog::accepted, this, [this]() {
                        m_currentSettings = SettingsManager::load(m_activeProfile->target, m_currentManager->getDefaultSettings());
                        });
                    m_staticRangeFinderDialog->show();
                }
            }
        }
    }

    void MainWindow::onCodeNoteConverterClicked() {
        if (m_codeNoteConverterDialog) {
            m_codeNoteConverterDialog->raise();
            m_codeNoteConverterDialog->activateWindow();
            return;
        }
        m_codeNoteConverterDialog = new CodeNoteConverterDialog(m_currentManager.get(), this);
        m_codeNoteConverterDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_codeNoteConverterDialog->show();
    }

    void MainWindow::onCodeNoteHierarchyFixerClicked() {
        if (m_codeNoteHierarchyFixerDialog) {
            m_codeNoteHierarchyFixerDialog->raise();
            m_codeNoteHierarchyFixerDialog->activateWindow();
            return;
        }
        m_codeNoteHierarchyFixerDialog = new CodeNoteHierarchyFixerDialog(this);
        m_codeNoteHierarchyFixerDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_codeNoteHierarchyFixerDialog->show();
    }

    void MainWindow::onTutorialClicked() {
        QDesktopServices::openUrl(QUrl("https://youtu.be/QwHTML0kRtI"));
    }

    void MainWindow::onAboutClicked() {
        AboutDialog dialog(this);
        dialog.exec();
    }

    void MainWindow::onCheckForUpdatesClicked() {
        if (m_updateProgressDialog) {
            return;
        }

        m_updateProgressDialog = new QProgressDialog("Checking for updates...", "Cancel", 0, 0, this);
        m_updateProgressDialog->setWindowTitle("Software Update");
        m_updateProgressDialog->setWindowModality(Qt::WindowModal);
        m_updateProgressDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_updateProgressDialog->show();

        m_updateChecker.checkForUpdates(false);
    }

    void MainWindow::onUpdateCheckFinished(bool updateAvailable, const QString& latestVersion, const QString& releaseNotes, const QString& downloadUrl, bool silentOnLatest) {
        if (m_updateProgressDialog) {
            m_updateProgressDialog->close();
        }

        if (updateAvailable) {
            auto* dialog = new UpdateDialog(CURRENT_VERSION, latestVersion, releaseNotes, downloadUrl, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
        else {
            if (!silentOnLatest) {
                QMessageBox::information(this, "Software Update",
                    QString("You are up to date!\n\nPointer Finder 2.0 %1 is currently the latest version.").arg(CURRENT_VERSION));
            }
        }
    }

    void MainWindow::onUpdateCheckFailed(const QString& errorMessage, bool silentOnLatest) {
        if (m_updateProgressDialog) {
            m_updateProgressDialog->close();
        }

        if (!silentOnLatest) {
            QMessageBox::warning(this, "Update Check Failed",
                QString("Could not reach update server:\n%1").arg(errorMessage));
        }
    }

    void MainWindow::onStateBasedScanClicked() {
        if (m_scanCoordinator.isBusy() || !m_activeProfile.has_value() || !m_currentManager->isAttached()) return;

        StateCaptureDialog dialog(m_currentManager.get(), m_currentSettings, &m_multiScanState, this);
        if (dialog.exec() == QDialog::Accepted) {
            auto params = dialog.getScanParameters();

            m_resultsManager.setNewResults({}, true);
            updateStatus("Purging application memory...");
            Core::Memory::forceGarbageCollection();

            dialog.updateSettings(m_currentSettings);
            SettingsManager::save(m_activeProfile->target, m_currentSettings);

            auto scanner = m_activeProfile->stateBasedScannerFactory();
            m_scanCoordinator.startScan(m_currentManager.get(), std::move(scanner), params);
        }
    }

    void MainWindow::onFilterClicked() {
        if (m_scanCoordinator.isBusy() || !m_resultsManager.hasResults()) return;

        if (m_currentManager && !m_currentManager->verifyAttachment()) {
            if (m_currentManager->isProcessRunning()) {
                auto reply = QMessageBox::question(
                    this,
                    "Memory Layout Changed",
                    "The emulator's memory layout has changed (possibly due to a game reboot or ASLR shift).\n\n"
                    "Would you like to reattach to the current memory banks before starting filtering?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes
                );

                if (reply == QMessageBox::Yes) {
                    if (m_currentManager->attach(m_currentManager->getProcessId())) {
                        m_baseAddressLabel->setText(QString("%1 Base (PC): %2")
                            .arg(m_currentManager->getEmulatorName().split(' ').first())
                            .arg(QString::number(m_currentManager->getMemoryBasePC(), 16).toUpper()));
                        updateStatus("Reattached successfully.");
                    }
                    else {
                        QMessageBox::critical(this, "Reattach Failed", "Could not locate the emulator's memory banks.");
                        return;
                    }
                }
                else {
                    return;
                }
            }
            else {
                detachAndReset();
                return;
            }
        }

        m_scanCoordinator.startFiltering(m_currentManager.get(), m_resultsManager.getCurrentResults());
    }

    void MainWindow::onStopScanClicked() {
        m_scanCoordinator.stopCurrentOperation();
        if (m_arrayWatcher.isRunning()) {
            m_arrayWatcher.cancel();
            m_arrayDetectionWidget->clear();
            switchToScanUI(false);
            updateStatus("Array analysis stopped by user.");
        }
    }

    void MainWindow::onOperationStarted(const QString& message) {
        switchToScanUI(true, message);
        m_scanStopwatch.restart();
        m_scanTimer.start();
        m_progressBar->setValue(0);
    }

    void MainWindow::onOperationFinished(const QString& finalStatusMessage) {
        m_scanTimer.stop();
        switchToScanUI(false);
        updateStatus(finalStatusMessage);
    }

    void MainWindow::onProgressUpdated(const ScanProgressReport& report) {
        if (!report.statusMessage.isEmpty()) {
            updateStatus(report.statusMessage);
        }

        int max = static_cast<int>(std::min(report.maxValue > 0 ? report.maxValue : static_cast<int64_t>(m_scanCoordinator.getLastScanParams().maxCandidates), static_cast<int64_t>(std::numeric_limits<int>::max())));
        m_progressBar->setMaximum(max);

        int64_t current = report.currentValue > 0 ? report.currentValue : report.foundCount;
        int targetValue = static_cast<int>(std::min(current, static_cast<int64_t>(max)));

        animateProgressBar(targetValue);

        m_progressLabel->setText(report.maxValue > 0
            ? QString("%1 / %2").arg(report.currentValue).arg(report.maxValue)
            : QString::number(report.foundCount + report.partialCount));
    }

    void MainWindow::animateProgressBar(int targetValue) {
        if (!m_progressAnimation) {
            m_progressAnimation = new QPropertyAnimation(m_progressBar, "value", this);
        }
        m_progressAnimation->stop();
        m_progressAnimation->setDuration(120);
        m_progressAnimation->setStartValue(m_progressBar->value());
        m_progressAnimation->setEndValue(targetValue);
        m_progressAnimation->setEasingCurve(QEasingCurve::OutQuad);
        m_progressAnimation->start();
    }

    void MainWindow::onScanCompleted(const std::vector<PointerPath>& results, qint64 durationMs, bool wasCancelled) {
        m_resultsManager.setNewResults(results, true);

        auto lastParams = m_scanCoordinator.getLastScanParams();
        if (!wasCancelled && lastParams.detectArrays && m_multiScanState.hasState(0) && !results.empty()) {
            switchToScanUI(true, "Detecting array lists...");
            m_progressBar->setMaximum(0);
            m_progressBar->setValue(0);
            m_progressLabel->setText("Running array analysis...");

            m_lastScanDurationMs = durationMs;
            m_lastScanWasCancelled = wasCancelled;

            auto future = QtConcurrent::run([this, results, lastParams]() {
                return Core::ArrayDetector::performArrayDetectionAndMatching(
                    m_multiScanState.getState(0)->memoryDump,
                    results,
                    lastParams.arraySearchRange,
                    m_currentManager.get(),
                    *m_multiScanState.getState(0)
                );
                });
            m_arrayWatcher.setFuture(future);
        }
        else {
            m_arrayDetectionWidget->clear();
            switchToScanUI(false);
            finalizeScanStatus(results.size(), durationMs, wasCancelled);
        }
    }

    void MainWindow::onArrayDetectionFinished() {
        Controls::ArrayDetectionResult result = m_arrayWatcher.result();
        m_arrayDetectionWidget->setResults(result.groups, result.matches);

        switchToScanUI(false);
        finalizeScanStatus(m_resultsManager.getCurrentResults().size(), m_lastScanDurationMs, m_lastScanWasCancelled);
    }

    void MainWindow::finalizeScanStatus(int resultsCount, qint64 durationMs, bool wasCancelled) {
        QString durationStr = QString("%1ms").arg(durationMs);
        if (durationMs > 1000) {
            durationStr = QString("%1s").arg(durationMs / 1000.0, 0, 'f', 1);
        }

        if (wasCancelled) {
            updateStatus(QString("Scan stopped. Found %1 paths and %2 array(s) in %3.")
                .arg(resultsCount).arg(m_arrayDetectionWidget->getArrayGroups().size()).arg(durationStr));
            SoundManager::playNotify();
        }
        else {
            if (resultsCount == 0) {
                updateStatus(QString("Scan complete. No paths found in %1.").arg(durationStr));
                SoundManager::playFail();
            }
            else {
                updateStatus(QString("Scan complete. Found %1 paths and %2 array(s) in %3.")
                    .arg(resultsCount).arg(m_arrayDetectionWidget->getArrayGroups().size()).arg(durationStr));
                SoundManager::playSuccess();
            }
        }
    }

    void MainWindow::onFilterCompleted(const std::vector<PointerPath>& filteredResults) {
        m_resultsManager.applyFilterResults(filteredResults);
    }

    void MainWindow::onResultsChanged(bool) {
        bool hasResults = m_resultsManager.hasResults();
        QWidget* targetWidget = hasResults ? m_tableView : m_emptyStateWidget;

        if (m_viewStack->currentWidget() != targetWidget) {
            fadeTransitionToWidget(targetWidget);
        }

        m_resultCountLabel->setText(QString("Results: %1").arg(m_resultsManager.getResultsCount()));
        m_resultCountLabel->setVisible(hasResults);
    }

    void MainWindow::fadeTransitionToWidget(QWidget* targetWidget) {
        if (!targetWidget) return;

        targetWidget->setVisible(true);
        m_viewStack->setCurrentWidget(targetWidget);

        auto* opacityEffect = new QGraphicsOpacityEffect(targetWidget);
        targetWidget->setGraphicsEffect(opacityEffect);

        auto* anim = new QPropertyAnimation(opacityEffect, "opacity");
        anim->setDuration(300);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::InOutQuad);

        connect(anim, &QPropertyAnimation::finished, this, [targetWidget]() {
            targetWidget->setGraphicsEffect(nullptr);
            });

        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void MainWindow::onSortChanged(const QString& columnName, Qt::SortOrder order) {
        if (columnName.isEmpty()) {
            m_tableView->horizontalHeader()->setSortIndicatorShown(false);
            return;
        }

        int colIndex = -1;
        if (columnName == "colBase") {
            colIndex = 0;
        }
        else if (columnName == "colFinal") {
            colIndex = m_tableModel->columnCount() - 1;
        }
        else if (columnName.startsWith("colOffset")) {
            colIndex = columnName.mid(9).toInt();
        }

        if (colIndex != -1) {
            m_tableView->horizontalHeader()->setSortIndicatorShown(true);
            m_tableView->horizontalHeader()->setSortIndicator(colIndex, order);
        }
    }

    void MainWindow::onScanTimerTick() {
        if (m_scanStopwatch.isValid()) {
            qint64 elapsed = m_scanStopwatch.elapsed();
            int minutes = static_cast<int>(elapsed / 60000);
            int seconds = static_cast<int>((elapsed % 60000) / 1000);
            int ms = static_cast<int>((elapsed % 1000) / 100);

            m_timeLabel->setText(QString("Time: %1:%2.%3")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(ms));
        }
    }

    // Renders a context menu containing clipboard choices, bookmark actions,
    // and offset sorting utilities.
    void MainWindow::showContextMenu(const QPoint& pos) {
        QModelIndex index = m_tableView->indexAt(pos);
        if (!index.isValid()) return;

        auto* menu = new QMenu(this);

        menu->addAction(m_copyBaseAddressAction);
        menu->addAction(m_copyAsRAAction);
        menu->addAction(m_copyAsCodeNoteAction);
        menu->addSeparator();

        QAction* addBookmarkAct = menu->addAction(getThemeIcon(":/icons/bookmark_add.svg"), "Add to Bookmarks");
        addBookmarkAct->setProperty("iconPath", ":/icons/bookmark_add.svg");
        connect(addBookmarkAct, &QAction::triggered, this, &MainWindow::onAddBookmarkTriggered);

        menu->addSeparator();
        menu->addAction(m_deleteSelectedAction);
        menu->addSeparator();

        QAction* sortAction = menu->addAction(getThemeIcon(":/icons/sort.svg"), "Sort by Lowest Offsets");
        sortAction->setProperty("iconPath", ":/icons/sort.svg");
        connect(sortAction, &QAction::triggered, this, &MainWindow::onSortByLowestOffsets);

        menu->exec(m_tableView->viewport()->mapToGlobal(pos));
        menu->deleteLater();
    }

    void MainWindow::onCopyBaseAddress() {
        QItemSelectionModel* select = m_tableView->selectionModel();
        QModelIndexList rows = select->selectedRows();
        if (rows.empty() || !m_currentManager) return;

        const auto& results = m_resultsManager.getCurrentResults();
        QStringList addresses;
        for (const auto& idx : rows) {
            int row = idx.row();
            if (row >= 0 && row < static_cast<int>(results.size())) {
                addresses << m_currentManager->formatDisplayAddress(results[row].baseAddress);
            }
        }

        QApplication::clipboard()->setText(addresses.join("\n"));
        updateStatus(QString("Copied %1 base address(es) to clipboard.").arg(static_cast<int>(addresses.size())));
    }

    void MainWindow::onCopyAsRAFormat() {
        QItemSelectionModel* select = m_tableView->selectionModel();
        QModelIndexList rows = select->selectedRows();
        if (static_cast<int>(rows.size()) != 1 || !m_currentManager) return;

        int row = rows.first().row();
        const auto& results = m_resultsManager.getCurrentResults();
        if (row >= 0 && row < static_cast<int>(results.size())) {
            QApplication::clipboard()->setText(results[row].toRetroAchievementsString(m_currentManager.get()));
            updateStatus("Copied path in RetroAchievements format.");
        }
    }

    void MainWindow::onCopyAsCodeNote() {
        QItemSelectionModel* select = m_tableView->selectionModel();
        QModelIndexList rows = select->selectedRows();
        if (static_cast<int>(rows.size()) != 1) return;

        int row = rows.first().row();
        const auto& results = m_resultsManager.getCurrentResults();
        if (row >= 0 && row < static_cast<int>(results.size())) {
            CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();
            QString note = CodeNoteHelper::buildCodeNote(results[row].offsets, settings);

            QApplication::clipboard()->setText(note);
            updateStatus("Copied path as custom code note.");
        }
    }

    void MainWindow::onDeleteSelected() {
        QItemSelectionModel* select = m_tableView->selectionModel();
        QModelIndexList rows = select->selectedRows();
        if (rows.empty()) return;

        std::vector<int> indices;
        indices.reserve(static_cast<size_t>(rows.size()));
        for (const auto& idx : rows) {
            indices.push_back(idx.row());
        }

        m_resultsManager.deleteSelected(indices);
    }

    void MainWindow::onSortByLowestOffsets() {
        m_resultsManager.sortByLowestOffsets();
    }

    void MainWindow::onFoundCountUpdated(int count) {
        m_resultCountLabel->setText(QString("Results: %1").arg(count));
    }

    void MainWindow::onAddBookmarkTriggered() {
        QItemSelectionModel* select = m_tableView->selectionModel();
        QModelIndexList rows = select->selectedRows();
        if (rows.empty()) return;

        const auto& results = m_resultsManager.getCurrentResults();
        int countAdded = 0;

        for (const auto& idx : rows) {
            int row = idx.row();
            if (row >= 0 && row < static_cast<int>(results.size())) {
                const PointerPath& path = results[row];

                Bookmark bm;
                QString baseStr = m_currentManager ? m_currentManager->formatDisplayAddress(path.baseAddress) : QString::number(path.baseAddress, 16).toUpper();
                bm.name = QString("Watchpoint [0x%1]").arg(baseStr);
                bm.path = path;

                m_bookmarksWidget->addBookmark(bm);
                countAdded++;
            }
        }

        if (countAdded > 0) {
            updateStatus(QString("Added %1 pointer(s) to bookmarks.").arg(countAdded));
            m_mainTabWidget->setCurrentIndex(1);
        }
    }

}