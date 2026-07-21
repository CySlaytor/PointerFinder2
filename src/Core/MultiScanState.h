#pragma once

#include "../Models/ScanState.h"

#include <memory>
#include <vector>

namespace PointerFinder2::Core {

    // Manages a revolving set of up to four memory dump states captured from the target process.
    // This class manages your captured memory snapshots, offering slots 
    // to save or release active snapshots.
    class MultiScanState {
    public:
        MultiScanState();
        ~MultiScanState();

        DataModels::ScanState* getState(int index);
        bool hasState(int index) const;

        int getLastCapturedSlotIndex() const;
        int getCapturedCount() const;

        void captureState(int slotIndex, const DataModels::ScanState& state);
        void releaseState(int slotIndex);
        void clearAll();

        std::vector<DataModels::ScanState> getCapturedStates() const;

    private:
        std::vector<std::unique_ptr<DataModels::ScanState>> m_capturedStates;
        int m_lastCapturedSlotIndex = -1;
        void updateLastCapturedIndex();
    };

}
