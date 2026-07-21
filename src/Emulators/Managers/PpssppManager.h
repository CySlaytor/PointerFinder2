#pragma once

#include "BaseEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    // This connector class hooks into PPSSPP to read active PSP RAM.
    class PpssppManager : public BaseEmulatorManager {
    public:
        static constexpr uint32_t PSP_MEM_START = 0x08000000;
        static constexpr uint32_t PSP_MEM_END = 0x0A000000;
        static constexpr uint32_t PSP_MEM_SIZE = 32 * 1024 * 1024; // 32MB Logical Segment space
        static constexpr int64_t PSP_SCAN_SIZE = 31 * 1024 * 1024; // 31MB Physical footprint size allocation

        PpssppManager();

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
        uintptr_t findPspRamBaseBySize();
    };

}