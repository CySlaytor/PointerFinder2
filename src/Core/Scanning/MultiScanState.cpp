#include "MultiScanState.h"

#include <algorithm>

namespace PointerFinder2::Core {

    using namespace PointerFinder2::DataModels;

    MultiScanState::MultiScanState() {
        m_capturedStates.resize(4);
    }

    MultiScanState::~MultiScanState() {
        clearAll();
    }

    ScanState* MultiScanState::getState(int index) {
        if (index < 0 || index >= 4) return nullptr;
        return m_capturedStates[index].get();
    }

    // Returns true if a valid memory snapshot has been captured in the specified slot.
    bool MultiScanState::hasState(int index) const {
        if (index < 0 || index >= 4) return false;
        return m_capturedStates[index] != nullptr;
    }

    int MultiScanState::getLastCapturedSlotIndex() const {
        return m_lastCapturedSlotIndex;
    }

    int MultiScanState::getCapturedCount() const {
        int count = 0;
        for (const auto& state : m_capturedStates) {
            if (state != nullptr) count++;
        }
        return count;
    }

    // Wipes the selected slot and saves a new emulator memory snapshot inside it.
    void MultiScanState::captureState(int slotIndex, const ScanState& state) {
        if (slotIndex < 0 || slotIndex >= 4) return;

        // Release memory buffers assigned to the target slot prior to allocation.
        if (m_capturedStates[slotIndex] != nullptr) {
            m_capturedStates[slotIndex]->memoryDump.clear();
        }

        m_capturedStates[slotIndex] = std::make_unique<ScanState>(state);
        updateLastCapturedIndex();
    }

    // Clears a captured memory snapshot from the slot to free up system memory.
    void MultiScanState::releaseState(int slotIndex) {
        if (slotIndex < 0 || slotIndex >= 4) return;
        if (m_capturedStates[slotIndex] != nullptr) {
            m_capturedStates[slotIndex]->memoryDump.clear();
            m_capturedStates[slotIndex].reset(nullptr);
        }
        updateLastCapturedIndex();
    }

    void MultiScanState::clearAll() {
        for (int i = 0; i < 4; ++i) {
            if (m_capturedStates[i] != nullptr) {
                m_capturedStates[i]->memoryDump.clear();
                m_capturedStates[i].reset(nullptr);
            }
        }
        m_lastCapturedSlotIndex = -1;
    }

    std::vector<ScanState> MultiScanState::getCapturedStates() const {
        std::vector<ScanState> active;
        for (const auto& state : m_capturedStates) {
            if (state != nullptr) {
                active.push_back(*state);
            }
        }
        return active;
    }

    void MultiScanState::updateLastCapturedIndex() {
        m_lastCapturedSlotIndex = -1;
        for (int i = 3; i >= 0; --i) {
            if (m_capturedStates[i] != nullptr) {
                m_lastCapturedSlotIndex = i;
                break;
            }
        }
    }

}
