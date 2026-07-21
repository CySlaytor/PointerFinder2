#pragma once

#include "BaseEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    // This connector class hooks into RALibretro to read active Nintendo DS RAM, detecting cores dynamically.
    class RALibretroNDSManager : public BaseEmulatorManager {
    public:
        static constexpr uint32_t NDS_RAM_START = 0x02000000;
        static constexpr uint32_t NDS_RAM_SIZE = 0x400000; // 4MB RAM segment
        static constexpr uint32_t NDS_RAM_END = NDS_RAM_START + NDS_RAM_SIZE;

        struct CoreProfile {
            QString name;
            int64_t searchSize;
            int32_t offset;
        };

        RALibretroNDSManager();

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
        uintptr_t m_ndsMemoryBaseInPC = 0;
        QString m_foundCoreName = "Unknown";
        uintptr_t findBaseAddressBySize(int64_t exactRegionSizeBytes);
    };

}