#include "DolphinStateBasedScanner.h"

#include "../../Common/EndianUtils.h"
#include "../IEmulatorManager.h"
#include "../Managers/DolphinManager.h"

#include <cstring>

namespace PointerFinder2::Emulators::StateBased {

    void DolphinStateBasedScanner::buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) {
        m_pointerMap.clear();
        for (size_t addr = 0; addr + 3 < memory.size(); addr += 4) {
            if (token.load()) return;

            uint32_t value;
            std::memcpy(&value, &memory[addr], 4);
            value = Core::swapEndian32(value);

            if (m_manager->isValidPointerTarget(value)) {
                uint32_t currentAddress;
                // Reconstruct GameCube/Wii Big-Endian logical pointers into flat offset lookups matching physical segments.
                if (addr < EmulatorManager::DolphinManager::MEM1_SIZE) {
                    currentAddress = EmulatorManager::DolphinManager::MEM1_INGAME_BASE + static_cast<uint32_t>(addr);
                }
                else {
                    currentAddress = EmulatorManager::DolphinManager::MEM2_INGAME_BASE + static_cast<uint32_t>(addr - EmulatorManager::DolphinManager::MEM1_SIZE);
                }
                m_pointerMap[value].push_back(currentAddress);
            }
        }
    }

    SourceResults DolphinStateBasedScanner::findSourcesForValue(uint32_t value) const {
        SourceResults res;
        auto it = m_pointerMap.find(value);
        if (it != m_pointerMap.end()) {
            res.first = &it->second;
        }
        return res;
    }

}
