#pragma once

#include "BaseEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    // This connector class hooks into DuckStation to read active PS1 RAM.
    class DuckStationManager : public BaseEmulatorManager {
    public:
        static constexpr uint32_t PS1_RAM_START = 0x80000000;
        static constexpr uint32_t PS1_RAM_SIZE = 0x200000; // 2MB Main RAM
        static constexpr uint32_t PS1_RAM_END = PS1_RAM_START + PS1_RAM_SIZE;

        DuckStationManager();

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
    };

}