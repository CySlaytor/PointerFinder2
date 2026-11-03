#include "ArrayDetector.h"

#include "../Common/EndianUtils.h"

#include <algorithm>
#include <cstring>
#include <unordered_set>

namespace PointerFinder2::Core {

    std::vector<DataModels::ArrayGroup> ArrayDetector::detectArraysInDump(
        const std::vector<uint8_t>& memory,
        const std::vector<DataModels::PointerPath>& paths,
        uint32_t range,
        Emulators::IEmulatorManager* manager
    ) {
        std::vector<DataModels::ArrayGroup> groups;
        if (memory.empty() || paths.empty() || !manager) return groups;

        std::unordered_set<uint32_t> processedBases;

        for (const auto& path : paths) {
            std::vector<uint32_t> candidateBases;
            candidateBases.push_back(path.baseAddress);

            uint32_t currentAddress = path.baseAddress;
            for (size_t i = 0; i < path.offsets.size(); ++i) {
                auto [normalizedReadAddress, isShort] = manager->normalizeAddressForRead(currentAddress);
                int64_t indexInDump = manager->getIndexForStateDump(normalizedReadAddress);
                if (indexInDump < 0 || indexInDump + 3 >= static_cast<int64_t>(memory.size())) {
                    break;
                }
                uint32_t value;
                std::memcpy(&value, &memory[indexInDump], 4);
                if (manager->getRetroAchievementsPrefix() == "G") {
                    value = swapEndian32(value);
                }
                candidateBases.push_back(value);

                if (i < path.offsets.size() - 1) {
                    currentAddress = value + static_cast<uint32_t>(path.offsets[i]);
                }
            }

            for (uint32_t B : candidateBases) {
                if (processedBases.count(B)) continue;
                processedBases.insert(B);

                uint32_t halfRange = range / 2;
                uint32_t startAddr = (B >= halfRange) ? B - halfRange : B;
                uint32_t endAddr = B + halfRange;

                startAddr = startAddr & 0xFFFFFFFC;
                endAddr = endAddr & 0xFFFFFFFC;

                struct MemoryWord {
                    uint32_t address;
                    uint32_t value;
                    bool isValid;
                };

                std::vector<MemoryWord> words;
                for (uint32_t addr = startAddr; addr <= endAddr; addr += 4) {
                    int64_t idx = manager->getIndexForStateDump(addr);
                    if (idx < 0 || idx + 3 >= static_cast<int64_t>(memory.size())) continue;

                    uint32_t value;
                    std::memcpy(&value, &memory[idx], 4);

                    if (manager->getRetroAchievementsPrefix() == "G") {
                        value = swapEndian32(value);
                    }

                    words.push_back({ addr, value, manager->isValidPlatformPointer(value) });
                }

                // Identify continuous sequences of four or more matching pointers
                std::vector<MemoryWord> arrayElements;
                size_t i = 0;
                while (i < words.size()) {
                    if (words[i].isValid) {
                        size_t startRun = i;
                        while (i < words.size() && words[i].isValid) {
                            i++;
                        }
                        size_t runLength = i - startRun;
                        if (runLength >= 4) {
                            for (size_t k = startRun; k < i; ++k) {
                                arrayElements.push_back(words[k]);
                            }
                        }
                    }
                    else {
                        i++;
                    }
                }

                if (!arrayElements.empty()) {
                    DataModels::ArrayGroup group;
                    group.baseAddress = arrayElements.front().address;
                    group.elementCount = static_cast<uint32_t>(arrayElements.size());
                    for (const auto& w : arrayElements) {
                        group.elementAddresses.push_back(w.address);
                        group.targets.push_back(w.value);
                        processedBases.insert(w.address);
                    }
                    groups.push_back(group);
                }
            }
        }

        return groups;
    }

    UI::Controls::ArrayDetectionResult ArrayDetector::performArrayDetectionAndMatching(
        const std::vector<uint8_t>& memory,
        const std::vector<DataModels::PointerPath>& paths,
        uint32_t range,
        Emulators::IEmulatorManager* manager,
        const DataModels::ScanState& state0
    ) {
        UI::Controls::ArrayDetectionResult result;
        if (memory.empty() || paths.empty() || !manager) return result;

        result.groups = detectArraysInDump(memory, paths, range, manager);
        if (result.groups.empty()) return result;

        std::vector<UI::Controls::PathArrayMatch> allMatches;
        for (const auto& path : paths) {
            std::vector<uint32_t> stepAddresses;
            stepAddresses.push_back(path.baseAddress);

            uint32_t currentAddress = path.baseAddress;
            for (size_t i = 0; i < path.offsets.size() - 1; ++i) {
                auto [normalizedReadAddress, isShort] = manager->normalizeAddressForRead(currentAddress);
                int64_t indexInDump = manager->getIndexForStateDump(normalizedReadAddress);
                if (indexInDump < 0 || indexInDump + 3 >= static_cast<int64_t>(state0.memoryDump.size())) {
                    break;
                }
                uint32_t value;
                std::memcpy(&value, &state0.memoryDump[indexInDump], 4);
                if (manager->getRetroAchievementsPrefix() == "G") {
                    value = swapEndian32(value);
                }

                uint32_t nextAddress = value + static_cast<uint32_t>(path.offsets[i]);
                stepAddresses.push_back(nextAddress);
                currentAddress = nextAddress;
            }

            for (const auto& group : result.groups) {
                for (size_t i = 0; i < stepAddresses.size(); ++i) {
                    uint32_t stepAddr = stepAddresses[i];
                    auto it = std::find(group.elementAddresses.begin(), group.elementAddresses.end(), stepAddr);
                    if (it != group.elementAddresses.end()) {
                        UI::Controls::PathArrayMatch match;
                        match.path = path;
                        match.stepIndex = static_cast<int>(i);
                        match.group = group;
                        allMatches.push_back(match);
                        break;
                    }
                }
            }
        }

        // Prioritize short paths resolving closest to the target object.
        std::sort(allMatches.begin(), allMatches.end(), [](const UI::Controls::PathArrayMatch& a, const UI::Controls::PathArrayMatch& b) {
            if (a.path.offsets.size() != b.path.offsets.size()) {
                return a.path.offsets.size() < b.path.offsets.size();
            }
            return a.stepIndex > b.stepIndex;
            });

        // Deduplicate using unique starting base addresses.
        std::unordered_set<uint32_t> uniqueArrayBases;
        result.matches.clear();

        for (const auto& match : allMatches) {
            uint32_t key = match.group.baseAddress;
            if (uniqueArrayBases.count(key) == 0) {
                uniqueArrayBases.insert(key);
                result.matches.push_back(match);
            }
        }

        return result;
    }

}