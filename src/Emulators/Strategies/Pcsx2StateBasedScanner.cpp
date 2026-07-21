#include "Pcsx2StateBasedScanner.h"

#include "../IEmulatorManager.h"
#include "../Managers/Pcsx2Manager.h"

#include <cstring>

namespace PointerFinder2::Emulators::StateBased {

    void Pcsx2StateBasedScanner::buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) {
        m_pointerMap.clear();
        for (size_t addr = 0; addr + 3 < memory.size(); addr += 4) {
            if (token.load()) return;

            uint32_t value;
            std::memcpy(&value, &memory[addr], 4);

            if (m_manager->isValidPointerTarget(value)) {
                uint32_t currentAddress = m_manager->getMainMemoryStart() + static_cast<uint32_t>(addr);
                m_pointerMap[value].push_back(currentAddress);
            }
        }
    }

    SourceResults Pcsx2StateBasedScanner::findSourcesForValue(uint32_t value) const {
        SourceResults res;

        auto it = m_pointerMap.find(value);
        if (it != m_pointerMap.end()) {
            res.first = &it->second;
        }

        // Resolves mapping inconsistencies where pointers omit the PS2 EE memory prefix (0x20000000).
        if (value >= EmulatorManager::Pcsx2Manager::PS2_EEMEM_START) {
            uint32_t shortValue = value - EmulatorManager::Pcsx2Manager::PS2_EEMEM_START;
            auto shortIt = m_pointerMap.find(shortValue);
            if (shortIt != m_pointerMap.end()) {
                res.second = &shortIt->second;
            }
        }
        return res;
    }

}
