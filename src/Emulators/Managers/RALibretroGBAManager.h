#pragma once

#include "BaseEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    class RALibretroGBAManager : public BaseEmulatorManager {
    public:
        static constexpr uint32_t GBA_EWRAM_START = 0x02000000;
        static constexpr uint32_t GBA_EWRAM_SIZE = 0x40000; // 256KB EWRAM
        static constexpr uint32_t GBA_EWRAM_END = GBA_EWRAM_START + GBA_EWRAM_SIZE;

        static constexpr uint32_t GBA_IWRAM_START = 0x03000000;
        static constexpr uint32_t GBA_IWRAM_SIZE = 0x08000; // 32KB IWRAM
        static constexpr uint32_t GBA_IWRAM_END = GBA_IWRAM_START + GBA_IWRAM_SIZE;

        static constexpr int64_t GBA_TOTAL_SIZE = 0x48000; // 288KB total GBA memory block in mGBA core

        RALibretroGBAManager();

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
        uintptr_t findGbaRamBase();
    };

}