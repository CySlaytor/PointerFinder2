#pragma once

#include "BaseEmulatorManager.h"

namespace PointerFinder2::Emulators::EmulatorManager {

// This connector class hooks into RALibretro (Flycast core) and standalone Flycast
// to read active Dreamcast System RAM (16MB).
class RALibretroDCManager : public BaseEmulatorManager {
  public:
    static constexpr uint32_t DC_RAM_START = 0x8C000000;       // P1 cached RAM area
    static constexpr uint32_t DC_RAM_SIZE = 0x01000000;        // 16MB Main RAM
    static constexpr uint32_t DC_RAM_END = DC_RAM_START + DC_RAM_SIZE;

    RALibretroDCManager();

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
    QString getPlatformPointerTypeDescription(uint32_t pointerValue = 0) const override;

  private:
    uintptr_t findDreamcastRamBase();
};

} // namespace PointerFinder2::Emulators::EmulatorManager
