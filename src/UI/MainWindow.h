#pragma once
#include "../Core/MultiScanState.h"
#include "../Core/ResultsManager.h"
#include "../Core/ScanCoordinator.h"
#include "../Core/UpdateChecker.h"
#include "../Emulators/EmulatorProfile.h"
#include "../Emulators/IEmulatorManager.h"
#include "../Models/PointerPath.h"
#include "Dialogs/CodeNoteConverterDialog.h"
#include "Dialogs/CodeNoteHierarchyFixerDialog.h"
#include "Dialogs/EmulatorSelectionDialog.h"
#include "Widgets/ArrayDetectionWidget.h"
#include "Widgets/BookmarksWidget.h"
#include "Widgets/PointerResultsModel.h"

#include <QAction>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableView>
#include <QTabWidget>
#include <QTimer>

namespace PointerFinder2::UI {

	// The MainWindow class represents the main application window. It manages menus, 
	// processes user clicks, tracks active scans, and switches between visual tabs.
	class MainWindow : public QMainWindow {
		Q_OBJECT
	public:
		explicit MainWindow(const QStringList& args, QWidget* parent = nullptr);
		~MainWindow() override;

		void restartApplication();
		void updateThemeIcons();

	protected:
		void closeEvent(QCloseEvent* event) override;
		bool eventFilter(QObject* obj, QEvent* event) override;

	private slots:
		void onAttachClicked();
		bool onSaveSessionClicked();
		void onLoadSessionClicked();
		void onUndoClicked();
		void onRedoClicked();
		void onFindClicked();
		void onSettingsClicked();
		void onStaticRangeFinderClicked();
		void onCodeNoteConverterClicked();
		void onCodeNoteHierarchyFixerClicked();
		void onTutorialClicked();
		void onAboutClicked();
		void onCheckForUpdatesClicked();

		void onUpdateCheckFinished(bool updateAvailable, const QString& latestVersion, const QString& releaseNotes, const QString& downloadUrl, bool silentOnLatest);
		void onUpdateCheckFailed(const QString& errorMessage, bool silentOnLatest);

		void onCopyBaseAddress();
		void onCopyAsRAFormat();
		void onCopyAsCodeNote();
		void onDeleteSelected();
		void onSortByLowestOffsets();
		void showContextMenu(const QPoint& pos);

		void onStateBasedScanClicked();
		void onFilterClicked();
		void onStopScanClicked();

		void onOperationStarted(const QString& message);
		void onOperationFinished(const QString& finalStatusMessage);
		void onProgressUpdated(const DataModels::ScanProgressReport& report);
		void onScanCompleted(const std::vector<DataModels::PointerPath>& results, qint64 durationMs, bool wasCancelled);
		void onFilterCompleted(const std::vector<DataModels::PointerPath>& filteredResults);
		void onFoundCountUpdated(int count);
		void onResultsChanged(bool isNewDataSet);
		void onSortChanged(const QString& columnName, Qt::SortOrder order);

		void onScanTimerTick();
		void onProcessMonitorTick();
		void onAddBookmarkTriggered();
		void onArrayDetectionFinished();

	private:
		Core::ResultsManager m_resultsManager;
		Core::ScanCoordinator m_scanCoordinator;
		Core::MultiScanState m_multiScanState;
		Core::UpdateChecker m_updateChecker;

		std::unique_ptr<Emulators::IEmulatorManager> m_currentManager = nullptr;
		DataModels::AppSettings m_currentSettings;
		std::optional<Emulators::EmulatorProfile> m_activeProfile = std::nullopt;

		QTabWidget* m_mainTabWidget = nullptr;
		QStackedWidget* m_viewStack = nullptr;
		QWidget* m_emptyStateWidget = nullptr;
		QTableView* m_tableView = nullptr;
		Controls::PointerResultsModel* m_tableModel = nullptr;

		Controls::BookmarksWidget* m_bookmarksWidget = nullptr;
		Controls::ArrayDetectionWidget* m_arrayDetectionWidget = nullptr;

		QWidget* m_bottomPanel = nullptr;
		QProgressBar* m_progressBar = nullptr;
		QPushButton* m_stopScanButton = nullptr;
		QLabel* m_progressLabel = nullptr;
		QLabel* m_timeLabel = nullptr;

		QWidget* m_scanButtonsWidget = nullptr;
		QPushButton* m_stateScanButton = nullptr;
		QPushButton* m_filterButton = nullptr;

		QLabel* m_statusLabel = nullptr;
		QLabel* m_baseAddressLabel = nullptr;
		QLabel* m_resultCountLabel = nullptr;

		QAction* m_copyBaseAddressAction = nullptr;
		QAction* m_copyAsRAAction = nullptr;
		QAction* m_copyAsCodeNoteAction = nullptr;
		QAction* m_deleteSelectedAction = nullptr;

		QTimer m_scanTimer;
		QElapsedTimer m_scanStopwatch;
		QTimer m_processMonitorTimer;

		QString m_currentSearchTerm = "";

		QPointer<CodeNoteConverterDialog> m_codeNoteConverterDialog;
		QPointer<CodeNoteHierarchyFixerDialog> m_codeNoteHierarchyFixerDialog;
		QPointer<QDialog> m_staticRangeFinderDialog;
		QPointer<QProgressDialog> m_updateProgressDialog;

		QPropertyAnimation* m_progressAnimation = nullptr;
		QFutureWatcher<Controls::ArrayDetectionResult> m_arrayWatcher;

		qint64 m_lastScanDurationMs = 0;
		bool m_lastScanWasCancelled = false;

		void setupUI();
		void setupMenuBar();
		void setupStatusBar();

		void setupEmptyStateView();
		void setupResultsTableView();
		void setupBottomPanel();

		bool performAttachment(const Emulators::EmulatorProfile& profile, uint32_t processId);
		void detachAndReset();
		void setUIStateDetached();
		void switchToScanUI(bool scanningActive, const QString& customMessage = "");
		void updateStatus(const QString& status);
		void parseCommandLineArgs(const QStringList& args);
		void autoAttachOnRestart(uint32_t pid, const QString& targetName);

		void animateProgressBar(int targetValue);
		void fadeTransitionToWidget(QWidget* targetWidget);
		void finalizeScanStatus(int resultsCount, qint64 durationMs, bool wasCancelled);

		QIcon getThemeIcon(const QString& resourcePath) const;
		QLabel* m_emptyStateIconLabel = nullptr;
	};

}