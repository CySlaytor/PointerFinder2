#include "RALibretroGBAStateBasedScanner.h"

#include "../IEmulatorManager.h"

#include <cstring>

namespace PointerFinder2::Emulators::StateBased {

    void RALibretroGBAStateBasedScanner::buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) {
        m_pointerMap.clear();
        for (size_t addr = 0; addr + 3 < memory.size(); addr += 4) {
            if (token.load()) return;

            uint32_t value;
            std::memcpy(&value, &memory[addr], 4);

            if (m_manager->isValidPointerTarget(value)) {
                uint32_t currentAddress;
                if (addr < 0x40000) {
                    currentAddress = 0x02000000 + static_cast<uint32_t>(addr);
                }
                else {
                    currentAddress = 0x03000000 + static_cast<uint32_t>(addr - 0x40000);
                }
                m_pointerMap[value].push_back(currentAddress);
            }
        }
    }

    SourceResults RALibretroGBAStateBasedScanner::findSourcesForValue(uint32_t value) const {
        SourceResults res;
        auto it = m_pointerMap.find(value);
        if (it != m_pointerMap.end()) {
            res.first = &it->second;
        }
        return res;
    }

}