#include "StructureAnalyzer.h"

#include "../Common/EndianUtils.h"

#include <cmath>
#include <cstring>

namespace PointerFinder2::Core {

    std::optional<uint32_t> StructureAnalyzer::readMemoryDword(
        uint32_t address,
        Emulators::IEmulatorManager* manager,
        const std::vector<uint8_t>& snapshotMemory
    ) {
        if (!manager) return std::nullopt;

        auto [normAddr, wasNorm] = manager->normalizeAddressForRead(address);

        if (!snapshotMemory.empty()) {
            int64_t indexInDump = manager->getIndexForStateDump(normAddr);
            if (indexInDump >= 0 && indexInDump + 3 < static_cast<int64_t>(snapshotMemory.size())) {
                uint32_t val = 0;
                std::memcpy(&val, &snapshotMemory[indexInDump], 4);
                if (manager->getRetroAchievementsPrefix() == "G") {
                    val = swapEndian32(val);
                }
                return val;
            }
        }

        return manager->readUInt32(normAddr);
    }

    ScopeMatch StructureAnalyzer::findBestScopeForAddress(
        uint32_t selectedAddr,
        uint32_t rootBaseAddress,
        const std::vector<ScopeCandidate>& scopes,
        Emulators::IEmulatorManager* manager,
        const std::vector<uint8_t>& snapshotMemory,
        bool rootIsArray,
        uint32_t rootArraySize
    ) {
        ScopeMatch best;
        if (!manager || selectedAddr == 0 || rootBaseAddress == 0) return best;

        uint32_t base = manager->unnormalizeAddress(QString::number(rootBaseAddress, 16));
        if (base == 0) return best;

        uint32_t initialParentBase = base;

        // When the root address itself is declared as a static array, do not dereference
        // the address as a pointer. The array elements reside directly starting at 'base'.
        if (!rootIsArray) {
            auto basePtr = readMemoryDword(base, manager, snapshotMemory);
            if (basePtr.has_value() && manager->isValidPointerTarget(basePtr.value())) {
                initialParentBase = manager->unnormalizeAddress(QString::number(basePtr.value(), 16));
            }
        }

        std::vector<ScopeCandidate> allScopes;
        allScopes.push_back({ nullptr, initialParentBase, 0, rootIsArray, rootArraySize });
        allScopes.insert(allScopes.end(), scopes.begin(), scopes.end());

        constexpr uint32_t MAX_STRUCT_SPAN = 0x10000; // 64 KB struct span
        int maxDepthMatched = -1;
        uint32_t smallestOffset = 0xFFFFFFFF;

        for (const auto& scope : allScopes) {
            if (scope.isArrayScope && scope.arraySize > 0) {
                // Address falls strictly within defined array boundaries
                if (selectedAddr >= scope.targetBase && selectedAddr < scope.targetBase + scope.arraySize) {
                    uint32_t delta = selectedAddr - scope.targetBase;
                    if (scope.depth >= maxDepthMatched) {
                        maxDepthMatched = scope.depth;
                        smallestOffset = delta;
                        best.scopeIdentifier = scope.identifier;
                        best.baseAddress = scope.targetBase;
                        best.offset = static_cast<int32_t>(delta);
                        best.depth = scope.depth;
                        best.isValid = true;
                        best.isArrayScope = true;
                    }
                }
            }
            else if (selectedAddr >= scope.targetBase) {
                uint32_t delta = selectedAddr - scope.targetBase;
                if (delta < MAX_STRUCT_SPAN) {
                    if (scope.depth > maxDepthMatched || (scope.depth == maxDepthMatched && delta < smallestOffset)) {
                        maxDepthMatched = scope.depth;
                        smallestOffset = delta;
                        best.scopeIdentifier = scope.identifier;
                        best.baseAddress = scope.targetBase;
                        best.offset = static_cast<int32_t>(delta);
                        best.depth = scope.depth;
                        best.isValid = true;
                        best.isArrayScope = false;
                    }
                }
            }
        }

        return best;
    }

    QString StructureAnalyzer::suggestDefaultType(
        uint32_t address,
        Emulators::IEmulatorManager* manager,
        const std::vector<uint8_t>& snapshotMemory
    ) {
        auto valOpt = readMemoryDword(address, manager, snapshotMemory);
        if (!valOpt.has_value()) return "[32-bit] Variable";

        uint32_t rawVal = valOpt.value();
        if (rawVal != 0 && manager && manager->isValidPointerTarget(rawVal)) {
            return manager->getPlatformPointerTypeDescription(rawVal);
        }

        float fVal;
        std::memcpy(&fVal, &rawVal, 4);
        if (!std::isnan(fVal) && !std::isinf(fVal) && std::abs(fVal) >= 0.0001f && std::abs(fVal) <= 1000000.0f) {
            return "[Float] Variable";
        }

        return "[32-bit] Variable";
    }

}
