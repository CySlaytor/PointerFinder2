#include "RALibretroGBAManager.h"

#include <windows.h>

namespace PointerFinder2::Emulators::EmulatorManager {

    RALibretroGBAManager::RALibretroGBAManager() = default;

    QString RALibretroGBAManager::getEmulatorName() const {
        return "RALibretro (GBA - mGBA)";
    }

    uint32_t RALibretroGBAManager::getMainMemoryStart() const {
        return GBA_EWRAM_START;
    }

    uint32_t RALibretroGBAManager::getMainMemorySize() const {
        return GBA_TOTAL_SIZE;
    }

    QString RALibretroGBAManager::getRetroAchievementsPrefix() const {
        return "O";
    }

    bool RALibretroGBAManager::attach(uint32_t processId) {
        detach();
        m_processId = processId;
        m_processHandle = Core::Memory::openProcessHandle(m_processId);

        if (!m_processHandle) {
            return false;
        }

        m_memoryBasePC = findGbaRamBase();
        if (m_memoryBasePC == 0) {
            detach();
            return false;
        }

        return true;
    }

    bool RALibretroGBAManager::verifyAttachment() {
        if (!isAttached()) return false;
        uintptr_t currentBase = findGbaRamBase();
        return currentBase != 0 && m_memoryBasePC == currentBase;
    }

    std::vector<uint8_t> RALibretroGBAManager::readMemory(uint32_t address, size_t count) {
        if (!isAttached()) return {};

        // Handle reading the entire state dump (both EWRAM & IWRAM combined)
        if (address == GBA_EWRAM_START && count == GBA_TOTAL_SIZE) {
            return Core::Memory::readBytes(m_processHandle, m_memoryBasePC, count);
        }

        if (address >= GBA_EWRAM_START && address < GBA_EWRAM_END) {
            uintptr_t targetPC = m_memoryBasePC + (address - GBA_EWRAM_START);
            return Core::Memory::readBytes(m_processHandle, targetPC, count);
        }

        if (address >= GBA_IWRAM_START && address < GBA_IWRAM_END) {
            // Since IWRAM is physically mapped after EWRAM inside mGBA's memory region:
            uintptr_t targetPC = m_memoryBasePC + GBA_EWRAM_SIZE + (address - GBA_IWRAM_START);
            return Core::Memory::readBytes(m_processHandle, targetPC, count);
        }

        return {};
    }

    bool RALibretroGBAManager::isValidPointerTarget(uint32_t value) const {
        if ((value & 3) != 0) return false;
        return (value >= GBA_EWRAM_START && value < GBA_EWRAM_END) ||
            (value >= GBA_IWRAM_START && value < GBA_IWRAM_END);
    }

    bool RALibretroGBAManager::isPotentialPointer(uint32_t value) const {
        return (value >= GBA_EWRAM_START && value < GBA_EWRAM_END) ||
            (value >= GBA_IWRAM_START && value < GBA_IWRAM_END);
    }

    bool RALibretroGBAManager::isValidPlatformPointer(uint32_t value) const {
        return (value & 3) == 0 && (
            (value >= GBA_EWRAM_START && value < GBA_EWRAM_END) ||
            (value >= GBA_IWRAM_START && value < GBA_IWRAM_END)
            );
    }

    // Translates a raw GBA CPU address into a display address matched to RA's layout
    QString RALibretroGBAManager::formatDisplayAddress(uint32_t address) const {
        if (address >= GBA_EWRAM_START && address < GBA_EWRAM_END) {
            // EWRAM: Maps to RA offset [0x8000 - 0x47FFF]
            return QString::number((address - GBA_EWRAM_START) + GBA_IWRAM_SIZE, 16).toUpper();
        }
        if (address >= GBA_IWRAM_START && address < GBA_IWRAM_END) {
            // IWRAM: Maps to RA offset [0x0000 - 0x007FFF]
            return QString::number(address - GBA_IWRAM_START, 16).toUpper();
        }
        return QString::number(address, 16).toUpper().rightJustified(8, '0');
    }

    // Translates a user-entered RA display address into a raw GBA CPU address
    uint32_t RALibretroGBAManager::unnormalizeAddress(const QString& address) const {
        bool ok;
        uint32_t parsed = address.toUInt(&ok, 16);
        if (!ok) return 0;

        if (isValidPointerTarget(parsed)) {
            return parsed;
        }

        if (parsed < GBA_IWRAM_SIZE) {
            // Address is in the IWRAM range (starts at GBA 0x03000000)
            return GBA_IWRAM_START + parsed;
        }
        if (parsed >= GBA_IWRAM_SIZE && parsed < GBA_TOTAL_SIZE) {
            // Address is in the EWRAM range (starts at GBA 0x02000000)
            return GBA_EWRAM_START + (parsed - GBA_IWRAM_SIZE);
        }

        return parsed;
    }

    std::pair<uint32_t, bool> RALibretroGBAManager::normalizeAddressForRead(uint32_t address) const {
        return { address, false };
    }

    bool RALibretroGBAManager::areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const {
        return addr1 == addr2;
    }

    DataModels::AppSettings RALibretroGBAManager::getDefaultSettings() const {
        DataModels::AppSettings settings;
        settings.staticAddressStart = "10000";
        settings.staticAddressEnd = "1FFFF";
        settings.maxOffset = 4095;
        settings.maxLevel = 7;
        settings.maxCandidates = 10000000;
        settings.stopOnFirstPathFound = false;
        settings.candidatesPerLevel = 10;
        settings.fastScanMode = true;
        settings.dynamicStaticDetection = true;
        return settings;
    }

    int64_t RALibretroGBAManager::getIndexForStateDump(uint32_t address) const {
        if (address >= GBA_EWRAM_START && address < GBA_EWRAM_END) {
            return address - GBA_EWRAM_START;
        }
        if (address >= GBA_IWRAM_START && address < GBA_IWRAM_END) {
            return (address - GBA_IWRAM_START) + GBA_EWRAM_SIZE;
        }
        return -1;
    }

    uintptr_t RALibretroGBAManager::findGbaRamBase() {
        std::vector<uintptr_t> foundAddresses;
        auto regions = Core::Memory::queryMemoryRegions(m_processHandle);
        for (const auto& region : regions) {
            if (region.isCommitted && static_cast<int64_t>(region.size) == GBA_TOTAL_SIZE) {
                foundAddresses.push_back(region.baseAddress);
            }
        }
        // Return the first matched region (lowest virtual address) to align with active GBA RAM
        return !foundAddresses.empty() ? foundAddresses.front() : 0;
    }

}