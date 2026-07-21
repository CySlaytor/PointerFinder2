#pragma once

#include "BaseEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    // This connector class hooks into PCSX2 to read active PS2 Emotion Engine RAM.
    class Pcsx2Manager : public BaseEmulatorManager {
    public:
        static constexpr uint32_t PS2_EEMEM_START = 0x20000000;
        static constexpr uint32_t PS2_EEMEM_END = 0x22000000;
        static constexpr uint32_t PS2_GAME_CODE_START = 0x00100000;
        static constexpr uint32_t PS2_GAME_CODE_END = 0x02000000;

        Pcsx2Manager();

        QString getEmulatorName() const override;
        uint32_t getMainMemoryStart() const override;
        uint32_t getMainMemorySize() const override;
        QString getRetroAchievementsPrefix() const override;

        bool attach(uint32_t processId) override;
        bool verifyAttachment() override;
        std::vector<uint8_t> readMemory(uint32_t address, size_t count) override;
        bool isValidPointerTarget(uint32_t value) const override;

        bool isPotentialPointer(uint32_t value) const override;
        bool isValidPlatformPointer(uint32_t value) const override;

        QString formatDisplayAddress(uint32_t address) const override;
        uint32_t unnormalizeAddress(const QString& address) const override;
        std::pair<uint32_t, bool> normalizeAddressForRead(uint32_t address) const override;
        bool areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const override;
        DataModels::AppSettings getDefaultSettings() const override;
        int64_t getIndexForStateDump(uint32_t address) const override;

    private:
        uintptr_t m_ps2MemoryBaseInPC = 0;
    };

}