#include "RALibretroDCManager.h"

#include <windows.h>

namespace PointerFinder2::Emulators::EmulatorManager {

RALibretroDCManager::RALibretroDCManager() = default;

QString RALibretroDCManager::getEmulatorName() const { return "RALibretro (Dreamcast - Flycast)"; }

uint32_t RALibretroDCManager::getMainMemoryStart() const { return DC_RAM_START; }

uint32_t RALibretroDCManager::getMainMemorySize() const { return DC_RAM_SIZE; }

// Returns 'W' (24-bit read) so RetroAchievements indirect pointer triggers automatically truncate
// the upper 8 bits (0x8C), mapping directly to RA's 16MB Dreamcast address space.
QString RALibretroDCManager::getRetroAchievementsPrefix() const { return "W"; }

bool RALibretroDCManager::attach(uint32_t processId) {
    detach();
    m_processId = processId;
    m_processHandle = Core::Memory::openProcessHandle(m_processId);

    if (!m_processHandle) {
        return false;
    }

    m_memoryBasePC = findDreamcastRamBase();
    if (m_memoryBasePC == 0) {
        detach();
        return false;
    }

    return true;
}

bool RALibretroDCManager::verifyAttachment() {
    if (!isAttached())
        return false;
    uintptr_t currentBase = findDreamcastRamBase();
    return currentBase != 0 && m_memoryBasePC == currentBase;
}

std::vector<uint8_t> RALibretroDCManager::readMemory(uint32_t address, size_t count) {
    if (!isAttached())
        return {};

    uint32_t offset = address & 0x00FFFFFF;
    if (offset + count > DC_RAM_SIZE)
        return {};

    uintptr_t targetPC = m_memoryBasePC + offset;
    return Core::Memory::readBytes(m_processHandle, targetPC, count);
}

// Evaluates whether a value is a valid 4-byte aligned Dreamcast RAM pointer in Area 3 (0x8C, 0xAC, or 0x0C).
bool RALibretroDCManager::isValidPointerTarget(uint32_t value) const {
    if ((value & 3) != 0)
        return false;
    // Area 3 check: ((value >> 26) & 7) == 3, not in P4 space: ((value >> 29) & 7) != 7
    bool isArea3 = ((value >> 26) & 7) == 3 && ((value >> 29) & 7) != 7;
    return isArea3 && ((value & 0x00FFFFFF) < DC_RAM_SIZE);
}

bool RALibretroDCManager::isPotentialPointer(uint32_t value) const {
    bool isArea3 = ((value >> 26) & 7) == 3 && ((value >> 29) & 7) != 7;
    return isArea3 && ((value & 0x00FFFFFF) < DC_RAM_SIZE);
}

bool RALibretroDCManager::isValidPlatformPointer(uint32_t value) const {
    return isValidPointerTarget(value);
}

// Strips the 0x8C prefix to present the clean 24-bit relative offset used by RetroAchievements.
QString RALibretroDCManager::formatDisplayAddress(uint32_t address) const {
    uint32_t offset = address & 0x00FFFFFF;
    return QString::number(offset, 16).toUpper().rightJustified(6, '0');
}

// Restores the canonical 0x8C000000 base to user-entered RA addresses.
uint32_t RALibretroDCManager::unnormalizeAddress(const QString& address) const {
    bool ok;
    uint32_t parsed = address.toUInt(&ok, 16);
    if (!ok)
        return 0;

    if (isValidPointerTarget(parsed)) {
        return 0x8C000000 | (parsed & 0x00FFFFFF);
    }

    if (parsed < DC_RAM_SIZE) {
        return DC_RAM_START + parsed;
    }

    return 0x8C000000 | (parsed & 0x00FFFFFF);
}

std::pair<uint32_t, bool> RALibretroDCManager::normalizeAddressForRead(uint32_t address) const {
    return {0x8C000000 | (address & 0x00FFFFFF), false};
}

bool RALibretroDCManager::areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const {
    return (addr1 & 0x00FFFFFF) == (addr2 & 0x00FFFFFF);
}

DataModels::AppSettings RALibretroDCManager::getDefaultSettings() const {
    DataModels::AppSettings settings;
    settings.staticAddressStart = "010000"; // 0x8C010000 (standard 1st_read.bin entry point)
    settings.staticAddressEnd = "7FFFFF";   // First 8MB static data boundary
    settings.maxOffset = 4095;
    settings.maxLevel = 7;
    settings.maxCandidates = 10000000;
    settings.stopOnFirstPathFound = false;
    settings.candidatesPerLevel = 10;
    settings.fastScanMode = true;
    return settings;
}

int64_t RALibretroDCManager::getIndexForStateDump(uint32_t address) const {
    uint32_t offset = address & 0x00FFFFFF;
    if (offset < DC_RAM_SIZE) {
        return static_cast<int64_t>(offset);
    }
    return -1;
}

QString RALibretroDCManager::getPlatformPointerTypeDescription(uint32_t) const {
    return "[24-bit] Pointer";
}

// Locates the committed 16MB (0x01000000 bytes) System RAM allocation in the target process.
uintptr_t RALibretroDCManager::findDreamcastRamBase() {
    auto regions = Core::Memory::queryMemoryRegions(m_processHandle);
    for (const auto& region : regions) {
        if (region.isCommitted && region.size == DC_RAM_SIZE) {
            // Verify heuristic: check for typical SH4 boot pointers or non-zero initialization
            auto testBytes = Core::Memory::readBytes(m_processHandle, region.baseAddress, 256);
            if (!testBytes.empty()) {
                return region.baseAddress;
            }
        }
    }
    return 0;
}

} // namespace PointerFinder2::Emulators::EmulatorManager
