#include "DuckStationManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    DuckStationManager::DuckStationManager() = default;

    QString DuckStationManager::getEmulatorName() const {
        return "DuckStation";
    }

    uint32_t DuckStationManager::getMainMemoryStart() const {
        return PS1_RAM_START;
    }

    uint32_t DuckStationManager::getMainMemorySize() const {
        return PS1_RAM_SIZE;
    }

    QString DuckStationManager::getRetroAchievementsPrefix() const {
        return "W";
    }

    bool DuckStationManager::attach(uint32_t processId) {
        detach();
        m_processId = processId;
        m_processHandle = Core::Memory::openProcessHandle(m_processId);

        if (!m_processHandle) {
            return false;
        }

        uintptr_t baseAddress = Core::Memory::getModuleBaseAddress(m_processHandle);
        if (baseAddress == 0) {
            detach();
            return false;
        }

        // Resolves the pointer referencing dynamic RAM segment bounds by scanning PE module exports.
        uintptr_t ramExportAddress = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "RAM", true);
        if (ramExportAddress == 0) {
            ramExportAddress = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "RAM", false);
        }

        if (ramExportAddress == 0) {
            detach();
            return false;
        }

        auto ramPtr = Core::Memory::readInt64(m_processHandle, ramExportAddress);
        if (!ramPtr.has_value()) {
            detach();
            return false;
        }

        m_memoryBasePC = static_cast<uintptr_t>(ramPtr.value());
        return true;
    }

    // Verifies if the DuckStation RAM connector handle is still pointing to active PS1 memory.
    bool DuckStationManager::verifyAttachment() {
        if (!isAttached()) return false;

        uintptr_t baseAddress = Core::Memory::getModuleBaseAddress(m_processHandle);
        if (baseAddress == 0) return false;

        uintptr_t ramExportAddress = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "RAM", true);
        if (ramExportAddress == 0) {
            ramExportAddress = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "RAM", false);
        }
        if (ramExportAddress == 0) return false;

        auto ramPtr = Core::Memory::readInt64(m_processHandle, ramExportAddress);
        if (!ramPtr.has_value()) return false;

        return m_memoryBasePC == static_cast<uintptr_t>(ramPtr.value());
    }

    // Reads raw data directly from the emulator's active PS1 RAM block.
    std::vector<uint8_t> DuckStationManager::readMemory(uint32_t address, size_t count) {
        if (!isAttached() || address < PS1_RAM_START || address >= PS1_RAM_END) return {};
        uintptr_t targetPC = m_memoryBasePC + (address - PS1_RAM_START);
        return Core::Memory::readBytes(m_processHandle, targetPC, count);
    }

    // Checks if a given value is a valid, 4-byte aligned PS1 memory address.
    bool DuckStationManager::isValidPointerTarget(uint32_t value) const {
        return (value & 3) == 0 && (value >= PS1_RAM_START && value < PS1_RAM_END);
    }

    bool DuckStationManager::isPotentialPointer(uint32_t value) const {
        return (value >= PS1_RAM_START && value < PS1_RAM_END);
    }

    bool DuckStationManager::isValidPlatformPointer(uint32_t value) const {
        return (value >= PS1_RAM_START && value < PS1_RAM_END) && ((value & 3) == 0);
    }

    // Formats raw addresses into a clean string, omitting generic PS1 system prefixes.
    QString DuckStationManager::formatDisplayAddress(uint32_t address) const {
        if (address >= PS1_RAM_START && address < PS1_RAM_END) {
            return QString::number(address - PS1_RAM_START, 16).toUpper();
        }
        return QString::number(address, 16).toUpper().rightJustified(8, '0');
    }

    // Translates a plain user-entered hex string back into a system-specific PS1 memory address.
    uint32_t DuckStationManager::unnormalizeAddress(const QString& address) const {
        bool ok;
        uint32_t parsed = address.toUInt(&ok, 16);
        if (!ok) return 0;

        if (parsed < PS1_RAM_SIZE) {
            return parsed + PS1_RAM_START;
        }
        return parsed;
    }

    std::pair<uint32_t, bool> DuckStationManager::normalizeAddressForRead(uint32_t address) const {
        return { address, false };
    }

    bool DuckStationManager::areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const {
        return addr1 == addr2;
    }

    DataModels::AppSettings DuckStationManager::getDefaultSettings() const {
        DataModels::AppSettings settings;
        settings.staticAddressStart = "1000";
        settings.staticAddressEnd = "7FFFF";
        settings.maxOffset = 4095;
        settings.maxLevel = 7;
        settings.maxCandidates = 10000000;
        settings.stopOnFirstPathFound = false;
        settings.candidatesPerLevel = 10;
        settings.fastScanMode = true;
        return settings;
    }

    // Translates an active system address into a physical offset index in the captured snapshot file.
    int64_t DuckStationManager::getIndexForStateDump(uint32_t address) const {
        if (address >= PS1_RAM_START && address < PS1_RAM_END) {
            return address - PS1_RAM_START;
        }
        return -1;
    }

}