#pragma once

#include "../Models/PointerPath.h"
#include "../Models/ScanParameters.h"
#include "../Models/ScanProgressReport.h"

#include <atomic>
#include <functional>
#include <vector>

namespace PointerFinder2::Emulators {

    class IEmulatorManager;

    // This interface acts as the master blueprint for pointer-searching algorithms,
    // allowing different scanning styles to hook into the program's backend.
    class IPointerScannerStrategy {
    public:
        virtual ~IPointerScannerStrategy() = default;

        virtual std::vector<DataModels::PointerPath> scan(
            IEmulatorManager* manager,
            const DataModels::ScanParameters& parameters,
            std::function<void(const DataModels::ScanProgressReport&)> progressCallback,
            std::atomic<bool>& cancellationToken
        ) = 0;
    };

}
