#pragma once

#include "../IEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    // This base class implements common connector tasks like closing connection handles,
    // verifying target status, and reading raw bytes from active emulator RAM.
    class BaseEmulatorManager : public IEmulatorManager {
    public:
        BaseEmulatorManager();
        virtual ~BaseEmulatorManager() override;

        bool isAttached() const override;
        Core::PlatformHandle getProcessHandle() const override;
        uintptr_t getMemoryBasePC() const override;
        uint32_t getProcessId() const override;

        void detach() override;
        bool verifyAttachment() override;
        bool isProcessRunning() const override;
        std::vector<uint8_t> readMemory(uint32_t address, size_t count) override;
        std::optional<uint32_t> readUInt32(uint32_t address) override;
        std::optional<uint32_t> recalculateFinalAddress(const DataModels::PointerPath& path, uint32_t expectedFinalAddress) override;

        bool isPotentialPointer(uint32_t value) const override;
        bool isValidPlatformPointer(uint32_t value) const override;

    protected:
        uint32_t m_processId = 0;
        Core::PlatformHandle m_processHandle = nullptr;
        uintptr_t m_memoryBasePC = 0;
    };

}