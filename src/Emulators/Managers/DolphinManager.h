#pragma once

#include "BaseEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

    // This connector class hooks into Dolphin, reading GameCube (MEM1) and Wii (MEM2) memory banks.
    class DolphinManager : public BaseEmulatorManager {
    public:
        static constexpr uint32_t MEM1_INGAME_BASE = 0x80000000;
        static constexpr uint32_t MEM1_SIZE = 24 * 1024 * 1024; // 24MB GameCube Main Memory
        static constexpr uint32_t MEM1_INGAME_END = MEM1_INGAME_BASE + MEM1_SIZE;

        static constexpr uint32_t MEM2_INGAME_BASE = 0x90000000;
        static constexpr uint32_t MEM2_SIZE = 56 * 1024 * 1024; // 56MB Wii Extended Memory Range
        static constexpr uint32_t MEM2_INGAME_END = MEM2_INGAME_BASE + MEM2_SIZE;

        enum class SystemType { Unknown, GameCube, Wii };

        DolphinManager();

        QString getEmulatorName() const override;
        uint32_t getMainMemoryStart() const override;
        uint32_t getMainMemorySize() const override;
        QString getRetroAchievementsPrefix() const override;

        bool attach(uint32_t processId) override;
        bool verifyAttachment() override;
        std::vector<uint8_t> readMemory(uint32_t address, size_t count) override;
        std::optional<uint32_t> readUInt32(uint32_t address) override;
        std::optional<uint32_t> recalculateFinalAddress(const DataModels::PointerPath& path, uint32_t expectedFinalAddress) override;
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
        SystemType m_detectedSystem = SystemType::Unknown;
        uintptr_t m_mem2BasePC = 0;

        std::vector<uintptr_t> findCandidateAddresses(uintptr_t exactRegionSize);
    };

}