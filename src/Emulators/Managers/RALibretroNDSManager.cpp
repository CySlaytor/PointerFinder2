#include "RALibretroNDSManager.h"

#include <windows.h>
#include <psapi.h>

namespace PointerFinder2::Emulators::EmulatorManager {

    static const RALibretroNDSManager::CoreProfile CoreProfiles[] = {
        { "melonDS",          17678336, 0x0 },       // Updated melonDS core (0x10DC000)
        { "melonDS (Legacy)", 17760256, 0x0 }        // Legacy melonDS core (0x10F0000)
    };

    // Offsets from desmume_libretro.dll module base (AllocationBase) to NDS RAM (0x02000000)
    static constexpr uintptr_t DESMUME_NEW_OFFSET    = 0x1120F40; // Git 8f6b32c+
    static constexpr uintptr_t DESMUME_LEGACY_OFFSET = 0x112BF40; // Older libretro builds

    RALibretroNDSManager::RALibretroNDSManager() = default;

    QString RALibretroNDSManager::getEmulatorName() const {
        return QString("RALibretro (NDS - %1)").arg(m_foundCoreName);
    }

    uint32_t RALibretroNDSManager::getMainMemoryStart() const {
        return NDS_RAM_START;
    }

    uint32_t RALibretroNDSManager::getMainMemorySize() const {
        return NDS_RAM_SIZE;
    }

    QString RALibretroNDSManager::getRetroAchievementsPrefix() const {
        return "D";
    }

    bool RALibretroNDSManager::attach(uint32_t processId) {
        detach();
        m_processId = processId;
        m_processHandle = Core::Memory::openProcessHandle(m_processId);

        if (!m_processHandle) {
            return false;
        }

        // 1. First attempt: Direct module resolution for DeSmuME core (desmume_libretro.dll)
        uintptr_t desmumeDllBase = findModuleBase(L"desmume_libretro.dll");
        if (desmumeDllBase != 0) {
            // Check new core offset first, then fall back to legacy offset
            uintptr_t candidatePC = desmumeDllBase + DESMUME_NEW_OFFSET;
            auto testRead = Core::Memory::readBytes(m_processHandle, candidatePC, 4);

            if (!testRead.empty()) {
                m_foundCoreName = "DeSmuME";
                m_memoryBasePC = candidatePC;
            }
            else {
                candidatePC = desmumeDllBase + DESMUME_LEGACY_OFFSET;
                testRead = Core::Memory::readBytes(m_processHandle, candidatePC, 4);
                if (!testRead.empty()) {
                    m_foundCoreName = "DeSmuME (Legacy)";
                    m_memoryBasePC = candidatePC;
                }
            }

            if (m_memoryBasePC != 0) {
                m_ndsMemoryBaseInPC = m_memoryBasePC - NDS_RAM_START;
                return true;
            }
        }

        // 2. Second attempt: Region size scanning for melonDS (and other standalone cores)
        for (const auto& profile : CoreProfiles) {
            uintptr_t parentBlockBase = findBaseAddressBySize(profile.searchSize);
            if (parentBlockBase != 0) {
                m_foundCoreName = profile.name;
                m_memoryBasePC = parentBlockBase + profile.offset;
                m_ndsMemoryBaseInPC = m_memoryBasePC - NDS_RAM_START;
                return true;
            }
        }

        detach();
        return false;
    }

    // Verifies if the core DS memory blocks are still mapped and readable from RALibretro.
    bool RALibretroNDSManager::verifyAttachment() {
        if (!isAttached()) return false;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(static_cast<HANDLE>(m_processHandle), reinterpret_cast<LPCVOID>(m_memoryBasePC), &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        return mbi.State == MEM_COMMIT;
    }

    // Reads raw data directly from the emulator's active DS memory blocks.
    std::vector<uint8_t> RALibretroNDSManager::readMemory(uint32_t address, size_t count) {
        if (!isAttached()) return {};
        uintptr_t targetPC = m_ndsMemoryBaseInPC + address;
        return Core::Memory::readBytes(m_processHandle, targetPC, count);
    }

    // Checks if a given value is a valid, 4-byte aligned DS memory address.
    bool RALibretroNDSManager::isValidPointerTarget(uint32_t value) const {
        return (value & 3) == 0 && (value >= NDS_RAM_START && value < NDS_RAM_END);
    }

    bool RALibretroNDSManager::isPotentialPointer(uint32_t value) const {
        return (value >= NDS_RAM_START && value < NDS_RAM_END);
    }

    bool RALibretroNDSManager::isValidPlatformPointer(uint32_t value) const {
        return (value >= NDS_RAM_START && value < NDS_RAM_END);
    }

    // Formats raw addresses into clean strings relative to DS RAM.
    QString RALibretroNDSManager::formatDisplayAddress(uint32_t address) const {
        if (address >= NDS_RAM_START && address < NDS_RAM_END) {
            return QString::number(address - NDS_RAM_START, 16).toUpper();
        }
        return QString::number(address, 16).toUpper().rightJustified(8, '0');
    }

    // Translates plain user hex text back into a system-specific DS memory address.
    uint32_t RALibretroNDSManager::unnormalizeAddress(const QString& address) const {
        bool ok;
        uint32_t parsed = address.toUInt(&ok, 16);
        if (!ok) return 0;

        if (parsed < NDS_RAM_SIZE) {
            return parsed + NDS_RAM_START;
        }
        return parsed;
    }

    std::pair<uint32_t, bool> RALibretroNDSManager::normalizeAddressForRead(uint32_t address) const {
        return { address, false };
    }

    bool RALibretroNDSManager::areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const {
        return addr1 == addr2;
    }

    DataModels::AppSettings RALibretroNDSManager::getDefaultSettings() const {
        DataModels::AppSettings settings;
        settings.staticAddressStart = "100000";
        settings.staticAddressEnd = "1FFFFF";
        settings.maxOffset = 4095;
        settings.maxLevel = 7;
        settings.maxCandidates = 10000000;
        settings.stopOnFirstPathFound = false;
        settings.candidatesPerLevel = 10;
        settings.fastScanMode = true;
        return settings;
    }

    // Translates an active system address into a physical offset index in the captured snapshot file.
    int64_t RALibretroNDSManager::getIndexForStateDump(uint32_t address) const {
        if (address >= NDS_RAM_START && address < NDS_RAM_END) {
            return address - NDS_RAM_START;
        }
        return -1;
    }

    uintptr_t RALibretroNDSManager::findBaseAddressBySize(int64_t exactRegionSizeBytes) {
        std::vector<uintptr_t> foundAddresses;
        auto regions = Core::Memory::queryMemoryRegions(m_processHandle);
        for (const auto& region : regions) {
            if (region.isCommitted && static_cast<int64_t>(region.size) == exactRegionSizeBytes) {
                foundAddresses.push_back(region.baseAddress);
            }
        }
        return !foundAddresses.empty() ? foundAddresses.back() : 0;
    }

    uintptr_t RALibretroNDSManager::findModuleBase(const std::wstring& moduleName) {
        if (!m_processHandle) return 0;

        HMODULE hMods[1024];
        DWORD cbNeeded = 0;
        HANDLE hProcess = static_cast<HANDLE>(m_processHandle);

        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
            int count = static_cast<int>(cbNeeded / sizeof(HMODULE));
            for (int i = 0; i < count; ++i) {
                wchar_t szModName[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, hMods[i], szModName, MAX_PATH)) {
                    std::wstring path(szModName);
                    if (path.find(moduleName) != std::wstring::npos) {
                        return reinterpret_cast<uintptr_t>(hMods[i]);
                    }
                }
            }
        }
        return 0;
    }

}
