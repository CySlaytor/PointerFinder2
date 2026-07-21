#include "Pcsx2Manager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    Pcsx2Manager::Pcsx2Manager() = default;

    QString Pcsx2Manager::getEmulatorName() const {
        return "PCSX2";
    }

    uint32_t Pcsx2Manager::getMainMemoryStart() const {
        return PS2_EEMEM_START;
    }

    uint32_t Pcsx2Manager::getMainMemorySize() const {
        return 32 * 1024 * 1024; // 32MB Emotion Engine RAM
    }

    QString Pcsx2Manager::getRetroAchievementsPrefix() const {
        return "X";
    }

    bool Pcsx2Manager::attach(uint32_t processId) {
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

        // Resolves primary EE segment references exported by the core execution module.
        uintptr_t eeMemBasePC = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "EEMem", true);
        if (eeMemBasePC == 0) {
            eeMemBasePC = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "EEMem", false);
        }

        if (eeMemBasePC == 0) {
            detach();
            return false;
        }

        auto eeMemPtr = Core::Memory::readInt64(m_processHandle, eeMemBasePC);
        if (!eeMemPtr.has_value()) {
            detach();
            return false;
        }

        m_memoryBasePC = static_cast<uintptr_t>(eeMemPtr.value());
        m_ps2MemoryBaseInPC = m_memoryBasePC - PS2_EEMEM_START;
        return true;
    }

    // Verifies if the PCSX2 'EEMem' connector handle is still pointing to active PS2 memory.
    bool Pcsx2Manager::verifyAttachment() {
        if (!isAttached()) return false;

        uintptr_t baseAddress = Core::Memory::getModuleBaseAddress(m_processHandle);
        if (baseAddress == 0) return false;

        uintptr_t eeMemBasePC = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "EEMem", true);
        if (eeMemBasePC == 0) {
            eeMemBasePC = Core::Memory::findExportedAddress(m_processHandle, baseAddress, "EEMem", false);
        }
        if (eeMemBasePC == 0) return false;

        auto eeMemPtr = Core::Memory::readInt64(m_processHandle, eeMemBasePC);
        if (!eeMemPtr.has_value()) return false;

        return m_memoryBasePC == static_cast<uintptr_t>(eeMemPtr.value());
    }

    // Reads raw data directly from the emulator's active Emotion Engine memory bank.
    std::vector<uint8_t> Pcsx2Manager::readMemory(uint32_t address, size_t count) {
        if (!isAttached()) return {};
        uintptr_t targetPC = m_ps2MemoryBaseInPC + address;
        return Core::Memory::readBytes(m_processHandle, targetPC, count);
    }

    // Checks if a given value is a valid address inside standard PS2 memory blocks.
    bool Pcsx2Manager::isValidPointerTarget(uint32_t value) const {
        if ((value & 3) != 0) return false;
        bool isGameCode = value >= PS2_GAME_CODE_START && value < PS2_GAME_CODE_END;
        bool isEeMem = value >= PS2_EEMEM_START && value < PS2_EEMEM_END;
        return isGameCode || isEeMem;
    }

    bool Pcsx2Manager::isPotentialPointer(uint32_t value) const {
        bool isEemem = (value >= PS2_EEMEM_START && value < PS2_EEMEM_END);
        bool isGameCode = (value >= PS2_GAME_CODE_START && value < PS2_GAME_CODE_END) && ((value & 0xF) == 0);
        return isEemem || isGameCode;
    }

    bool Pcsx2Manager::isValidPlatformPointer(uint32_t value) const {
        return isPotentialPointer(value) && ((value & 0xF) == 0);
    }

    // Formats raw addresses into clean strings, omitting standard PS2 system prefixes.
    QString Pcsx2Manager::formatDisplayAddress(uint32_t address) const {
        if (address >= PS2_EEMEM_START && address < PS2_EEMEM_END) {
            return QString::number(address - PS2_EEMEM_START, 16).toUpper();
        }
        return QString::number(address, 16).toUpper().rightJustified(8, '0');
    }

    // Translates a plain user hex string back into a system-specific PS2 memory address.
    uint32_t Pcsx2Manager::unnormalizeAddress(const QString& address) const {
        bool ok;
        uint32_t parsed = address.toUInt(&ok, 16);
        if (!ok) return 0;

        if (parsed < 0x2000000) {
            return parsed + PS2_EEMEM_START;
        }
        return parsed;
    }

    // Restores missing PS2 prefixes (0x20000000) to addresses so they can be read correctly.
    std::pair<uint32_t, bool> Pcsx2Manager::normalizeAddressForRead(uint32_t address) const {
        if (address < PS2_EEMEM_START) {
            return { address + PS2_EEMEM_START, true };
        }
        return { address, false };
    }

    bool Pcsx2Manager::areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const {
        uint32_t norm1 = (addr1 < PS2_EEMEM_START) ? addr1 + PS2_EEMEM_START : addr1;
        uint32_t norm2 = (addr2 < PS2_EEMEM_START) ? addr2 + PS2_EEMEM_START : addr2;
        return norm1 == norm2;
    }

    DataModels::AppSettings Pcsx2Manager::getDefaultSettings() const {
        DataModels::AppSettings settings;
        settings.staticAddressStart = "100000";
        settings.staticAddressEnd = "7FFFFF";
        settings.maxOffset = 4095;
        settings.maxLevel = 7;
        settings.maxCandidates = 10000000;
        settings.stopOnFirstPathFound = false;
        settings.candidatesPerLevel = 10;
        settings.fastScanMode = true;
        return settings;
    }

    // Translates an active system address into a physical offset index in the captured snapshot file.
    int64_t Pcsx2Manager::getIndexForStateDump(uint32_t address) const {
        auto [normed, wasNorm] = normalizeAddressForRead(address);
        if (normed >= PS2_EEMEM_START && normed < PS2_EEMEM_END) {
            return normed - PS2_EEMEM_START;
        }
        return -1;
    }

}