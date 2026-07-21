#include "ScanCoordinator.h"

#include "../Emulators/IEmulatorManager.h"

#include <chrono>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
namespace PointerFinder2::Core {

    using namespace PointerFinder2::DataModels;

    ScanCoordinator::ScanCoordinator(QObject* parent) : QObject(parent) {
        connect(&m_scanWatcher, &QFutureWatcher<std::vector<PointerPath>>::finished, this, &ScanCoordinator::onScanFinished);
        connect(&m_filterWatcher, &QFutureWatcher<std::vector<PointerPath>>::finished, this, &ScanCoordinator::onFilterFinished);
    }

    ScanCoordinator::~ScanCoordinator() {
        stopCurrentOperation();
        m_scanWatcher.waitForFinished();
        m_filterWatcher.waitForFinished();
    }

    bool ScanCoordinator::isBusy() const {
        return m_scanWatcher.isRunning() || m_filterWatcher.isRunning();
    }

    ScanParameters ScanCoordinator::getLastScanParams() const {
        return m_lastScanParams;
    }

    void ScanCoordinator::setLastScanParams(const ScanParameters& params) {
        m_lastScanParams = params;
    }

    // Dispatches a background worker thread to start the backward-search engine.
    void ScanCoordinator::startScan(Emulators::IEmulatorManager* manager,
        std::shared_ptr<Emulators::IPointerScannerStrategy> scanner,
        const ScanParameters& parameters) {
        if (isBusy()) return;

        m_lastScanParams = parameters.cloneWithoutStates();
        m_cancelToken.store(false);
        m_elapsedTimer.start();

        emit operationStarted("Starting scan...");

        auto scanProgressCallback = [this](const ScanProgressReport& report) {
            QMetaObject::invokeMethod(this, [this, report]() {
                emit progressUpdated(report);
                if (report.foundCount > 0) {
                    emit foundCountUpdated(report.foundCount);
                }
                }, Qt::QueuedConnection);
            };

        QFuture<std::vector<PointerPath>> future = QtConcurrent::run(
            [scanner, manager, parameters, scanProgressCallback, this] {
                return scanner->scan(manager, parameters, scanProgressCallback, m_cancelToken);
            }
        );

        m_scanWatcher.setFuture(future);
    }

    void ScanCoordinator::onScanFinished() {
        qint64 durationMs = m_elapsedTimer.elapsed();
        bool cancelled = m_cancelToken.load();

        std::vector<PointerPath> results = m_scanWatcher.result();
        emit scanCompleted(results, durationMs, cancelled);
    }

    // Dispatches a background worker to continuously check if found paths still point to the correct addresses.
    void ScanCoordinator::startFiltering(Emulators::IEmulatorManager* manager,
        const std::vector<PointerPath>& initialPaths) {
        if (isBusy()) return;

        m_cancelToken.store(false);
        m_elapsedTimer.start();

        emit operationStarted("Starting continuous filter...");

        // Uses a standard member function pointer format to ensure compatibility with MSVC template matching.
        QFuture<std::vector<PointerPath>> future = QtConcurrent::run(
            &ScanCoordinator::runFilteringTask, this, manager, initialPaths
        );

        m_filterWatcher.setFuture(future);
    }

    std::vector<PointerPath> ScanCoordinator::runFilteringTask(Emulators::IEmulatorManager* manager,
        std::vector<PointerPath> paths) {
        std::vector<PointerPath> currentPaths = std::move(paths);

        while (!m_cancelToken.load()) {
            if (currentPaths.empty()) break;

            auto nextPaths = QtConcurrent::blockingFiltered(currentPaths, [this, manager](const PointerPath& path) {
                if (m_cancelToken.load()) return false;
                auto calculated = manager->recalculateFinalAddress(path, path.finalAddress);
                return calculated.has_value() && calculated.value() == path.finalAddress;
                });

            if (m_cancelToken.load()) break;

            currentPaths = std::move(nextPaths);

            QMetaObject::invokeMethod(this, [this, count = static_cast<int>(currentPaths.size())]() {
                emit foundCountUpdated(count);
                }, Qt::QueuedConnection);

            QThread::msleep(50);
        }

        return currentPaths;
    }

    void ScanCoordinator::onFilterFinished() {
        std::vector<PointerPath> filtered = m_filterWatcher.result();
        emit filterCompleted(filtered);
        emit operationFinished(QString("Filtering stopped. %1 paths remain.").arg(filtered.size()));
    }

    // Signals all background workers to stop immediately.
    void ScanCoordinator::stopCurrentOperation() {
        m_cancelToken.store(true);
    }

    void ScanCoordinator::waitForDecoupling() {
        m_scanWatcher.waitForFinished();
        m_filterWatcher.waitForFinished();
    }

}
