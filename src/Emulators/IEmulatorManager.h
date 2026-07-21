#pragma once

#include "../Common/Memory.h"
#include "../Models/AppSettings.h"

#include <memory>
#include <optional>
#include <QString>
#include <vector>

namespace PointerFinder2::DataModels {
    class PointerPath;
}

namespace PointerFinder2::Emulators {

    // This interface acts as the master blueprint for connecting to other programs.
    // It defines actions like attaching, detaching, reading RAM, and formatting addresses.
    class IEmulatorManager {
    public:
        virtual ~IEmulatorManager() = default;

        virtual QString getEmulatorName() const = 0;
        virtual uint32_t getMainMemoryStart() const = 0;
        virtual uint32_t getMainMemorySize() const = 0;
        virtual QString getRetroAchievementsPrefix() const = 0;
        virtual bool isAttached() const = 0;
        virtual Core::PlatformHandle getProcessHandle() const = 0;
        virtual uintptr_t getMemoryBasePC() const = 0;
        virtual uint32_t getProcessId() const = 0;

        virtual bool attach(uint32_t processId) = 0;
        virtual void detach() = 0;
        virtual bool verifyAttachment() = 0;
        virtual bool isProcessRunning() const = 0;

        virtual std::vector<uint8_t> readMemory(uint32_t address, size_t count) = 0;
        virtual std::optional<uint32_t> readUInt32(uint32_t address) = 0;
        virtual std::optional<uint32_t> recalculateFinalAddress(const DataModels::PointerPath& path, uint32_t expectedFinalAddress) = 0;

        virtual bool isValidPointerTarget(uint32_t value) const = 0;
        virtual bool isPotentialPointer(uint32_t value) const = 0;
        virtual bool isValidPlatformPointer(uint32_t value) const = 0;

        virtual QString formatDisplayAddress(uint32_t address) const = 0;
        virtual uint32_t unnormalizeAddress(const QString& address) const = 0;
        virtual std::pair<uint32_t, bool> normalizeAddressForRead(uint32_t address) const = 0;
        virtual bool areAddressesEquivalent(uint32_t addr1, uint32_t addr2) const = 0;

        virtual DataModels::AppSettings getDefaultSettings() const = 0;
        virtual int64_t getIndexForStateDump(uint32_t address) const = 0;
    };

}