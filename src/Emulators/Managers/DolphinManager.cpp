#include "DolphinManager.h"

#include "../../Common/EndianUtils.h"
#include "../../Models/PointerPath.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace PointerFinder2::Emulators::EmulatorManager {

DolphinManager::DolphinManager() = default;

QString DolphinManager::getEmulatorName() const {
    switch (m_detectedSystem) {
    case SystemType::GameCube:
        return "Dolphin (GameCube)";
    case SystemType::Wii:
        return "Dolphin (Wii)";
    default:
        return "Dolphin (Unknown)";
    }
}

uint32_t DolphinManager::getMainMemoryStart() const { return MEM1_INGAME_BASE; }

uint32_t DolphinManager::getMainMemorySize() const { return m_detectedSystem == SystemType::Wii ? MEM1_SIZE + MEM2_SIZE : MEM1_SIZE; }

QString DolphinManager::getRetroAchievementsPrefix() const { return "G"; }

bool DolphinManager::isValidGameHeader(uintptr_t address, bool* outIsWii) {
    if (!m_processHandle || address == 0)
        return false;

    // Read first 6 bytes for Game ID header (ASCII)
    auto headerBytes = Core::Memory::readBytes(m_processHandle, address, 6);
    if (headerBytes.size() < 6)
        return false;

    bool allPrintable = true;
    for (uint8_t b : headerBytes) {
        if (b < 32 || b > 126) {
            allPrintable = false;
            break;
        }
    }

    if (!allPrintable)
        return false;

    char firstChar = static_cast<char>(std::toupper(headerBytes[0]));
    if (outIsWii) {
        // Standard Wii disc identifiers start with R, S, W, or H
        *outIsWii = (firstChar == 'R' || firstChar == 'S' || firstChar == 'W' || firstChar == 'H');
    }
    return true;
}

bool DolphinManager::attach(uint32_t processId) {
    detach();
    m_processId = processId;
    m_processHandle = Core::Memory::openProcessHandle(m_processId);

    if (!m_processHandle) {
        return false;
    }

    auto mem1Candidates = findCandidateAddresses(MEM1_SIZE);
    if (mem1Candidates.empty()) {
        mem1Candidates = findCandidateAddresses(32 * 1024 * 1024);
    }

    auto mem2Candidates = findCandidateAddresses(MEM2_SIZE);
    if (mem2Candidates.empty()) {
        mem2Candidates = findCandidateAddresses(64 * 1024 * 1024);
    }

    // Locate verified MEM1 candidate by checking the game header
    uintptr_t verifiedMem1 = 0;
    bool headerIndicatesWii = false;

    for (uintptr_t cand : mem1Candidates) {
        bool isWii = false;
        if (isValidGameHeader(cand, &isWii)) {
            verifiedMem1 = cand;
            headerIndicatesWii = isWii;
            break;
        }
    }

    if (verifiedMem1 == 0 && !mem1Candidates.empty()) {
        verifiedMem1 = mem1Candidates.back();
    }

    if (verifiedMem1 == 0) {
        detach();
        return false;
    }

    m_memoryBasePC = verifiedMem1;

    // Detect Wii mode by calculating the distance between discovered MEM1 and MEM2 pages.
    // 1. First check Dolphin's Fastmem Arena where MEM2 is mapped at exactly MEM1 + 0x10000000 (256 MB)
    uintptr_t fastmemMem2 = m_memoryBasePC + 0x10000000ULL;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(static_cast<HANDLE>(m_processHandle), reinterpret_cast<LPCVOID>(fastmemMem2), &mbi, sizeof(mbi)) != 0 &&
        mbi.State == MEM_COMMIT && static_cast<uint32_t>(mbi.RegionSize) >= MEM2_SIZE) {
        m_mem2BasePC = fastmemMem2;
        m_detectedSystem = SystemType::Wii;
        return true;
    }

    // 2. Fallback: check discrete MEM2 candidate allocations
    if (!mem2Candidates.empty()) {
        bool wiiDetected = false;
        for (uintptr_t m2 : mem2Candidates) {
            int64_t diff = static_cast<int64_t>(m2) - static_cast<int64_t>(m_memoryBasePC);
            if (diff >= static_cast<int64_t>(MEM1_SIZE) && diff <= 0x10000000LL) {
                m_mem2BasePC = m2;
                m_detectedSystem = SystemType::Wii;
                wiiDetected = true;
                break;
            }
        }

        if (!wiiDetected) {
            m_detectedSystem = headerIndicatesWii ? SystemType::Wii : SystemType::GameCube;
        }
    } else {
        m_detectedSystem = headerIndicatesWii ? SystemType::Wii : SystemType::GameCube;
    }

    return true;
}

// Verifies if Dolphin's virtual RAM pages are still locked and accessible in the system.
bool DolphinManager::verifyAttachment() {
    if (!isAttached())
        return false;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(static_cast<HANDLE>(m_processHandle), reinterpret_cast<LPCVOID>(m_memoryBasePC), &mbi, sizeof(mbi)) == 0) {
        return false;
    }
    if (mbi.State != MEM_COMMIT) {
        return false;
    }

    if (m_detectedSystem == SystemType::Wii && m_mem2BasePC != 0) {
        if (VirtualQueryEx(static_cast<HANDLE>(m_processHandle), reinterpret_cast<LPCVOID>(m_mem2BasePC), &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        if (mbi.State != MEM_COMMIT) {
            return false;
        }
    }

    return true;
}

// Reads active RAM blocks, stitching Wii MEM1 and MEM2 segments together if running a full dump.
std::vector<uint8_t> DolphinManager::readMemory(uint32_t address, size_t count) {
    if (!isAttached())
        return {};

    if (m_detectedSystem == SystemType::Wii && count == (MEM1_SIZE + MEM2_SIZE) && address == MEM1_INGAME_BASE) {
        std::vector<uint8_t> combined(count);
        auto m1Bytes = Core::Memory::readBytes(m_processHandle, m_memoryBasePC, MEM1_SIZE);
        auto m2Bytes = Core::Memory::readBytes(m_processHandle, m_mem2BasePC, MEM2_SIZE);

        if (m1Bytes.size() == MEM1_SIZE && m2Bytes.size() == MEM2_SIZE) {
            std::memcpy(combined.data(), m1Bytes.data(), MEM1_SIZE);
            std::memcpy(combined.data() + MEM1_SIZE, m2Bytes.data(), MEM2_SIZE);
            return combined;
        }
        return {};
    }

    if (address >= MEM1_INGAME_BASE && address < MEM1_INGAME_END) {
        uintptr_t targetPC = m_memoryBasePC + (address - MEM1_INGAME_BASE);
        return Core::Memory::readBytes(m_processHandle, targetPC, count);
    }

    if (m_detectedSystem == SystemType::Wii && address >= MEM2_INGAME_BASE && address < MEM2_INGAME_END) {
        uintptr_t targetPC = m_mem2BasePC + (address - MEM2_INGAME_BASE);
        return Core::Memory::readBytes(m_processHandle, targetPC, count);
    }

    return {};
}

std::optional<uint32_t> DolphinManager::readUInt32(uint32_t address) {
    auto bytes = readMemory(address, 4);
    if (bytes.size() < 4)
        return std::nullopt;

    uint32_t val;
    std::memcpy(&val, bytes.data(), 4);
    val = Core::swapEndian32(val);
    return val;
}

std::optional<uint32_t> DolphinManager::recalculateFinalAddress(const DataModels::PointerPath& path, uint32_t) {
    if (path.offsets.empty())
        return std::nullopt;

    auto currentAddress = readUInt32(path.baseAddress);
    if (!currentAddress.has_value() || !isValidPointerTarget(currentAddress.value()))
        return std::nullopt;

    for (size_t i = 0; i < path.offsets.size() - 1; ++i) {
        uint32_t nextToRead = currentAddress.value() + static_cast<uint32_t>(path.offsets[i]);
        currentAddress = readUInt32(nextToRead);
        if (!currentAddress.has_value() || !isValidPointerTarget(currentAddress.value()))
            return std::nullopt;
    }

    uint32_t finalAddress = currentAddress.value() + static_cast<uint32_t>(path.offsets.back());
    return finalAddress;
}

// Verifies if a given value is a valid memory address inside GameCube or Wii RAM boundaries.
bool DolphinManager::isValidPointerTarget(uint32_t value) const {
    if ((value & 3) != 0)
        return false;
    bool inMem1 = value >= MEM1_INGAME_BASE && value < MEM1_INGAME_END;
    if (inMem1)
        return true;
    if (m_detectedSystem == SystemType::Wii) {
        return value >= MEM2_INGAME_BASE && value < MEM2_INGAME_END;
    }
    return false;
}

bool DolphinManager::isPotentialPointer(uint32_t value) const {
    bool inMem1 = value >= MEM1_INGAME_BASE && value < MEM1_INGAME_END;
    if (inMem1)
        return true;
    if (m_detectedSystem == SystemType::Wii) {
        return value >= MEM2_INGAME_BASE && value < MEM2_INGAME_END;
    }
    return false;
}

bool DolphinManager::isValidPlatformPointer(uint32_t value) const {
    return (value & 3) == 0 &&
           ((value >= MEM1_INGAME_BASE && value < MEM1_INGAME_END) || (value >= MEM2_INGAME_BASE && value < MEM2_INGAME_END));
}

QString DolphinManager::formatDisplayAddress(uint32_t address) const {
    if (address >= MEM1_INGAME_BASE && address < MEM1_INGAME_END) {
        return QString::number(address - MEM1_INGAME_BASE, 16).toUpper();
    }
    if (m_detectedSystem == SystemType::Wii && address >= MEM2_INGAME_BASE && address < MEM2_INGAME_END) {
        return QString::number(0x10000000 + (address - MEM2_INGAME_BASE), 16).toUpper();
    }
    return QString::number(address, 16).toUpper().rightJustified(8, '0');
}

uint32_t DolphinManager::unnormalizeAddress(const QString& address) const {
    bool ok;
    uint32_t parsed = address.toUInt(&ok, 16);
    if (!ok)
        return 0;

    // Check if already in native CPU address range (aligned or unaligned)
    if ((parsed >= MEM1_INGAME_BASE && parsed < MEM1_INGAME_END) ||
        (m_detectedSystem == SystemType::Wii && parsed >= MEM2_INGAME_BASE && parsed < MEM2_INGAME_END)) {
        return parsed;
    }

    // Check if in RetroAchievements MEM2 range [0x10000000 - 0x137FFFFF]
    if (m_detectedSystem == SystemType::Wii && (parsed >= 0x10000000 && parsed < 0x20000000)) {
        return (parsed - 0x10000000) + MEM2_INGAME_BASE;
    }

    return parsed + MEM1_INGAME_BASE;
}

std::pair<uint32_t, bool> DolphinManager::normalizeAddressForRead(uint32_t address) const { return {address, false}; }

bool DolphinManager::areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const { return addr1 == addr2; }

DataModels::AppSettings DolphinManager::getDefaultSettings() const {
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
int64_t DolphinManager::getIndexForStateDump(uint32_t address) const {
    if (address >= MEM1_INGAME_BASE && address < MEM1_INGAME_END) {
        return address - MEM1_INGAME_BASE;
    }
    if (m_detectedSystem == SystemType::Wii && address >= MEM2_INGAME_BASE && address < MEM2_INGAME_END) {
        return (address - MEM2_INGAME_BASE) + MEM1_SIZE;
    }
    return -1;
}

QString DolphinManager::getPlatformPointerTypeDescription(uint32_t) const { return "[32-bit BE & 0x1FFFFFFF] Pointer"; }

std::vector<uintptr_t> DolphinManager::findCandidateAddresses(uintptr_t exactRegionSize) {
    std::vector<uintptr_t> foundAddresses;
    auto regions = Core::Memory::queryMemoryRegions(m_processHandle);
    for (const auto& region : regions) {
        if (region.isCommitted && region.size == exactRegionSize) {
            foundAddresses.push_back(region.baseAddress);
        }
    }
    return foundAddresses;
}

} // namespace PointerFinder2::Emulators::EmulatorManager
