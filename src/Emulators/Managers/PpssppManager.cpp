#include "PpssppManager.h"

#include <windows.h>

#include <cstring>

namespace PointerFinder2::Emulators::EmulatorManager {

PpssppManager::PpssppManager() = default;

QString PpssppManager::getEmulatorName() const { return "PPSSPP"; }

uint32_t PpssppManager::getMainMemoryStart() const { return PSP_MEM_START; }

uint32_t PpssppManager::getMainMemorySize() const { return PSP_MEM_SIZE; }

QString PpssppManager::getRetroAchievementsPrefix() const { return "X"; }

bool PpssppManager::attach(uint32_t processId) {
    detach();
    m_processId = processId;
    m_processHandle = Core::Memory::openProcessHandle(m_processId);

    if (!m_processHandle) {
        return false;
    }

    m_memoryBasePC = findPspRamBaseBySize();
    if (m_memoryBasePC == 0) {
        detach();
        return false;
    }

    return true;
}

// Verifies if the PPSSPP emulator's active RAM footprint is still mapped and accessible.
bool PpssppManager::verifyAttachment() {
    if (!isAttached())
        return false;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(static_cast<HANDLE>(m_processHandle), reinterpret_cast<LPCVOID>(m_memoryBasePC), &mbi, sizeof(mbi)) == 0) {
        return false;
    }
    return (mbi.State == MEM_COMMIT) && (static_cast<int64_t>(mbi.RegionSize) >= PSP_SCAN_SIZE);
}

// Reads raw data directly from the emulator's active PSP memory block.
std::vector<uint8_t> PpssppManager::readMemory(uint32_t address, size_t count) {
    if (!isAttached() || !isValidPointerTarget(address))
        return {};
    uintptr_t targetPC = m_memoryBasePC + (address - PSP_MEM_START);
    return Core::Memory::readBytes(m_processHandle, targetPC, count);
}

// Checks if a given value is a valid, 4-byte aligned PSP memory address.
bool PpssppManager::isValidPointerTarget(uint32_t value) const {
    return (value & 3) == 0 && (value >= PSP_MEM_START && value < PSP_MEM_END);
}

bool PpssppManager::isPotentialPointer(uint32_t value) const { return (value >= PSP_MEM_START && value < PSP_MEM_END); }

bool PpssppManager::isValidPlatformPointer(uint32_t value) const {
    return (value >= PSP_MEM_START && value < PSP_MEM_END) && ((value & 3) == 0);
}

// Formats a raw address into a clean string relative to standard PSP RAM.
QString PpssppManager::formatDisplayAddress(uint32_t address) const {
    if (address >= 0x09000000 && address < PSP_MEM_END) {
        return QString::number(0x01000000 + (address - 0x09000000), 16).toUpper();
    }
    if (address >= PSP_MEM_START && address < 0x09000000) {
        return QString::number(address - PSP_MEM_START, 16).toUpper();
    }
    return QString::number(address, 16).toUpper().rightJustified(8, '0');
}

// Translates plain user hex text back into a full PSP memory address.
uint32_t PpssppManager::unnormalizeAddress(const QString& address) const {
    bool ok;
    uint32_t parsed = address.toUInt(&ok, 16);
    if (!ok)
        return 0;

    if (isValidPointerTarget(parsed)) {
        return parsed;
    }

    if (parsed >= 0x01000000 && parsed < 0x02000000) {
        return (parsed - 0x01000000) + 0x09000000;
    }

    return parsed + PSP_MEM_START;
}

std::pair<uint32_t, bool> PpssppManager::normalizeAddressForRead(uint32_t address) const { return {address, false}; }

bool PpssppManager::areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const { return addr1 == addr2; }

DataModels::AppSettings PpssppManager::getDefaultSettings() const {
    DataModels::AppSettings settings;
    settings.staticAddressStart = "800000";
    settings.staticAddressEnd = "CFFFFF";
    settings.maxOffset = 4095;
    settings.maxLevel = 7;
    settings.maxCandidates = 10000000;
    settings.fastScanMode = true;
    return settings;
}

// Translates an active system address into a physical offset index in the captured snapshot file.
int64_t PpssppManager::getIndexForStateDump(uint32_t address) const {
    if (address >= PSP_MEM_START && address < PSP_MEM_END) {
        return address - PSP_MEM_START;
    }
    return -1;
}

QString PpssppManager::getPlatformPointerTypeDescription(uint32_t) const { return "[32-bit & 0x1FFFFFF] Pointer"; }

uintptr_t PpssppManager::findPspRamBaseBySize() {
    auto regions = Core::Memory::queryMemoryRegions(m_processHandle);

    // Primary pass: Match the 31MB MAX_MMAP_SIZE region and verify the active PSP kernel header
    for (const auto& region : regions) {
        if (region.isCommitted && static_cast<int64_t>(region.size) == PSP_SCAN_SIZE) {
            auto testBytes = Core::Memory::readBytes(m_processHandle, region.baseAddress, 8);
            if (testBytes.size() == 8) {
                uint32_t w0 = 0, w1 = 0;
                std::memcpy(&w0, &testBytes[0], 4);
                std::memcpy(&w1, &testBytes[4], 4);
                if (w0 != 0 || w1 != 0) {
                    return region.baseAddress;
                }
            }
        }
    }

    // Fallback pass: If the memory was just reserved/mapped during boot before vectors were written
    for (const auto& region : regions) {
        if (region.isCommitted && static_cast<int64_t>(region.size) == PSP_SCAN_SIZE) {
            return region.baseAddress;
        }
    }

    return 0;
}

} // namespace PointerFinder2::Emulators::EmulatorManager
