#include "BaseEmulatorManager.h"

#include "../../Models/PointerPath.h"

#include <windows.h>
#include <cstring>

namespace PointerFinder2::Emulators::EmulatorManager {

    BaseEmulatorManager::BaseEmulatorManager() = default;

    BaseEmulatorManager::~BaseEmulatorManager() {
        detach();
    }

    // Returns true if the program has successfully attached to a running emulator process.
    bool BaseEmulatorManager::isAttached() const {
        return m_processHandle != nullptr && m_memoryBasePC != 0;
    }

    Core::PlatformHandle BaseEmulatorManager::getProcessHandle() const {
        return m_processHandle;
    }

    uintptr_t BaseEmulatorManager::getMemoryBasePC() const {
        return m_memoryBasePC;
    }

    uint32_t BaseEmulatorManager::getProcessId() const {
        return m_processId;
    }

    // Closes communication handles and resets tracking variables to safely disconnect from the emulator.
    void BaseEmulatorManager::detach() {
        if (m_processHandle) {
            Core::Memory::closeHandle(m_processHandle);
            m_processHandle = nullptr;
        }
        m_processId = 0;
        m_memoryBasePC = 0;
    }

    // Verifies if the connected emulator process is still active and its RAM is still accessible.
    bool BaseEmulatorManager::verifyAttachment() {
        if (!isAttached()) return false;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(static_cast<HANDLE>(m_processHandle), reinterpret_cast<LPCVOID>(m_memoryBasePC), &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        return mbi.State == MEM_COMMIT;
    }

    bool BaseEmulatorManager::isProcessRunning() const {
        if (!m_processHandle || m_processId == 0) return false;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_processId);
        bool running = false;
        if (hProcess) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(hProcess, &exitCode)) {
                running = (exitCode == STILL_ACTIVE);
            }
            CloseHandle(hProcess);
        }
        return running;
    }

    // Reads a block of raw bytes directly from the emulator's active system memory.
    std::vector<uint8_t> BaseEmulatorManager::readMemory(uint32_t address, size_t count) {
        if (!isAttached()) return {};

        uintptr_t targetPC = m_memoryBasePC + (address - getMainMemoryStart());
        return Core::Memory::readBytes(m_processHandle, targetPC, count);
    }

    // Reads a single 4-byte number (an unsigned integer) from a specific emulator address.
    std::optional<uint32_t> BaseEmulatorManager::readUInt32(uint32_t address) {
        auto bytes = readMemory(address, 4);
        if (bytes.size() < 4) return std::nullopt;

        uint32_t val;
        std::memcpy(&val, bytes.data(), 4);
        return val;
    }

    std::optional<uint32_t> BaseEmulatorManager::recalculateFinalAddress(const DataModels::PointerPath& path, uint32_t expectedFinalAddress) {
        uint32_t normalizedExpected = expectedFinalAddress;
        auto [normed, wasNorm] = normalizeAddressForRead(expectedFinalAddress);
        if (wasNorm) {
            normalizedExpected = normed;
        }

        if (path.offsets.empty()) return std::nullopt;

        auto currentAddress = readUInt32(path.baseAddress);
        if (!currentAddress.has_value() || !isValidPointerTarget(currentAddress.value())) return std::nullopt;

        // Perform sequential multi-level pointer dereferencing with on-the-fly address normalization.
        for (size_t i = 0; i < path.offsets.size() - 1; ++i) {
            uint32_t nextToRead = currentAddress.value() + static_cast<uint32_t>(path.offsets[i]);
            auto [normedNext, wasNormedNext] = normalizeAddressForRead(nextToRead);
            if (wasNormedNext) {
                nextToRead = normedNext;
            }

            currentAddress = readUInt32(nextToRead);
            if (!currentAddress.has_value() || !isValidPointerTarget(currentAddress.value())) return std::nullopt;
        }

        uint32_t finalAddress = currentAddress.value() + static_cast<uint32_t>(path.offsets.back());
        auto [normedFinal, wasNormedFinal] = normalizeAddressForRead(finalAddress);
        if (wasNormedFinal) {
            finalAddress = normedFinal;
        }

        return finalAddress;
    }

    // Evaluates if a given memory value falls within standard system boundaries to be a valid pointer.
    bool BaseEmulatorManager::isPotentialPointer(uint32_t value) const {
        return isValidPointerTarget(value);
    }

    bool BaseEmulatorManager::isValidPlatformPointer(uint32_t value) const {
        return isPotentialPointer(value);
    }

}