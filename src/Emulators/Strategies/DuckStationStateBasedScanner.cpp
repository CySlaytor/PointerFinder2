#include "DuckStationStateBasedScanner.h"

#include "../IEmulatorManager.h"

#include <cstring>

namespace PointerFinder2::Emulators::StateBased {

    // Reads the captured PS1 memory snapshot and builds an index map of all active pointers.
    void DuckStationStateBasedScanner::buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) {
        m_pointerMap.clear();
        for (size_t addr = 0; addr + 3 < memory.size(); addr += 4) {
            if (token.load()) return;

            uint32_t value;
            std::memcpy(&value, &memory[addr], 4);

            if (m_manager->isValidPointerTarget(value)) {
                m_pointerMap[value].push_back(m_manager->getMainMemoryStart() + static_cast<uint32_t>(addr));
            }
        }
    }

    // Searches the loaded index to find all pointers that point directly to a specific target address.
    SourceResults DuckStationStateBasedScanner::findSourcesForValue(uint32_t value) const {
        SourceResults res;
        auto it = m_pointerMap.find(value);
        if (it != m_pointerMap.end()) {
            res.first = &it->second;
        }
        return res;
    }

}
