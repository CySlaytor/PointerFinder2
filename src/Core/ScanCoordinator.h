#pragma once

#include "../Emulators/IPointerScannerStrategy.h"
#include "../Models/PointerPath.h"
#include "../Models/ScanParameters.h"
#include "../Models/ScanProgressReport.h"

#include <atomic>
#include <memory>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QObject>

namespace PointerFinder2::Core {

    // Coordinates background thread dispatching for search operations and real-time path filtering.
    // This class manages background workers, coordinating starting and stopping 
    // actions for active searches and real-time filters.
    class ScanCoordinator : public QObject {
        Q_OBJECT
    public:
        explicit ScanCoordinator(QObject* parent = nullptr);
        ~ScanCoordinator() override;

        bool isBusy() const;
        DataModels::ScanParameters getLastScanParams() const;
        void setLastScanParams(const DataModels::ScanParameters& params);

        void startScan(Emulators::IEmulatorManager* manager,
            std::shared_ptr<Emulators::IPointerScannerStrategy> scanner,
            const DataModels::ScanParameters& parameters);

        void startFiltering(Emulators::IEmulatorManager* manager,
            const std::vector<DataModels::PointerPath>& initialPaths);

        void stopCurrentOperation();
        void waitForDecoupling();

    signals:
        void operationStarted(const QString& message);
        void operationFinished(const QString& finalStatusMessage);
        void progressUpdated(const DataModels::ScanProgressReport& report);
        void foundCountUpdated(int count);

        void scanCompleted(const std::vector<DataModels::PointerPath>& results, qint64 durationMs, bool wasCancelled);
        void filterCompleted(const std::vector<DataModels::PointerPath>& filteredResults);

    private:
        std::atomic<bool> m_cancelToken{ false };
        DataModels::ScanParameters m_lastScanParams;
        QElapsedTimer m_elapsedTimer;

        QFutureWatcher<std::vector<DataModels::PointerPath>> m_scanWatcher;
        QFutureWatcher<std::vector<DataModels::PointerPath>> m_filterWatcher;

        void onScanFinished();
        void onFilterFinished();

        std::vector<DataModels::PointerPath> runFilteringTask(Emulators::IEmulatorManager* manager,
            std::vector<DataModels::PointerPath> paths);
    };

}
