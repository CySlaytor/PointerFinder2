#include "RALibretroDCStateBasedScanner.h"

#include "../IEmulatorManager.h"
#include "../Managers/RALibretroDCManager.h"

#include <cstring>

namespace PointerFinder2::Emulators::StateBased {

// Reads the captured 16MB Dreamcast memory snapshot and indexes all active SH4 Little-Endian pointers.
void RALibretroDCStateBasedScanner::buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) {
    m_pointerMap.clear();
    for (size_t addr = 0; addr + 3 < memory.size(); addr += 4) {
        if (token.load())
            return;

        uint32_t value;
        std::memcpy(&value, &memory[addr], 4);

        if (m_manager->isValidPointerTarget(value)) {
            // Canonicalize target address to 0x8C cached space so both 0x8C and 0xAC pointers map seamlessly
            uint32_t canonicalTarget = 0x8C000000 | (value & 0x00FFFFFF);
            uint32_t currentAddress = EmulatorManager::RALibretroDCManager::DC_RAM_START + static_cast<uint32_t>(addr);
            m_pointerMap[canonicalTarget].push_back(currentAddress);
        }
    }
}

// Searches the index to find all pointers that point to the target address, normalized across mirrors.
SourceResults RALibretroDCStateBasedScanner::findSourcesForValue(uint32_t value) const {
    SourceResults res;
    uint32_t canonicalValue = 0x8C000000 | (value & 0x00FFFFFF);
    auto it = m_pointerMap.find(canonicalValue);
    if (it != m_pointerMap.end()) {
        res.first = &it->second;
    }
    return res;
}

} // namespace PointerFinder2::Emulators::StateBased
