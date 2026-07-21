#include "StateBasedScannerStrategyBase.h"

#include "../../Common/EndianUtils.h"
#include "../IEmulatorManager.h"

#include <cmath>
#include <cstring>
#include <QtConcurrent>

namespace PointerFinder2::Emulators::StateBased {

    using namespace PointerFinder2::DataModels;

    StateBasedScannerStrategyBase::StateBasedScannerStrategyBase() = default;

    StateBasedScannerStrategyBase::~StateBasedScannerStrategyBase() {
        for (size_t i = 0; i < VISITED_SHARDS; ++i) {
            m_visitedNodes[i].clear();
        }
        for (size_t i = 0; i < ALLOCATOR_SHARDS; ++i) {
            m_nodePools[i].clear();
        }
    }

    // Thread-safe allocation structure that manages candidate nodes in sharded pools.
    // Designed to minimize mutex contention under high-throughput parallel execution.
    StateBasedScannerStrategyBase::PathNode* StateBasedScannerStrategyBase::allocateNode(uint32_t address, int32_t offset, PathNode* child) {
        size_t shard = (static_cast<size_t>(address) ^ static_cast<size_t>(offset)) % ALLOCATOR_SHARDS;
        std::lock_guard<std::mutex> lock(m_poolMutexes[shard]);
        m_nodePools[shard].push_back({ address, offset, child, false });
        return &m_nodePools[shard].back();
    }

    bool StateBasedScannerStrategyBase::shouldUpdateProgress(int64_t currentCount) {
        std::lock_guard<std::mutex> lock(m_reportingMutex);
        if (currentCount >= m_nextUpdateThreshold) {
            if (currentCount < 10000)
                m_nextUpdateThreshold = currentCount + 100;
            else if (currentCount < 100000)
                m_nextUpdateThreshold = currentCount + 1000;
            else
                m_nextUpdateThreshold = currentCount + 10000;
            return true;
        }
        return false;
    }

    // Initiates the backward Breadth-First Search (BFS) scanner across captured memory dumps.
    // Performs path discovery and validates candidates concurrently against all comparison snapshots.
    std::vector<PointerPath> StateBasedScannerStrategyBase::scan(
        IEmulatorManager* manager,
        const ScanParameters& parameters,
        std::function<void(const DataModels::ScanProgressReport&)> progressCallback,
        std::atomic<bool>& cancellationToken
    ) {
        m_manager = manager;
        m_params = parameters;
        m_progressCallback = progressCallback;
        m_cancelToken = &cancellationToken;

        m_foundPaths.clear();

        for (size_t i = 0; i < VISITED_SHARDS; ++i) {
            m_visitedNodes[i].clear();
        }
        for (size_t i = 0; i < ALLOCATOR_SHARDS; ++i) {
            m_nodePools[i].clear();
        }
        m_pointerMap.clear();

        m_candidatesGenerated = 0;
        m_candidatesValidated = 0;
        m_staticPathsFoundCounter = 0;
        m_foundPathsCounter = 0;
        m_nextUpdateThreshold = 100;
        m_lastReportTime = std::chrono::steady_clock::now();

        if (m_params.capturedStates.empty() || m_params.capturedStates.size() < 2) {
            return {};
        }

        reportProgress("Building map for State 1...", 0, 1);
        buildPointerMap(m_params.capturedStates[0].memoryDump, cancellationToken);

        if (cancellationToken.load()) {
            return {};
        }
        reportProgress("Building map for State 1... Complete", 1, 1);

        std::vector<PathNode*> currentLevelCandidates;
        uint32_t targetAddress = m_params.capturedStates[0].targetAddress;

        if (m_params.fastScanMode) {
            size_t shard = static_cast<size_t>(targetAddress) % VISITED_SHARDS;
            std::lock_guard<std::mutex> lock(m_visitedMutexes[shard]);
            m_visitedNodes[shard][targetAddress] = 0;
        }

        reportProgress("Searching Level 1...", 0, m_params.maxOffset / 4);
        int candidatesFound = 0;

        int32_t startOffset = 0;
        int32_t endOffset = m_params.maxOffset;
        if (m_params.lastOffsetHint.has_value()) {
            startOffset = m_params.lastOffsetHint.value();
            endOffset = m_params.lastOffsetHint.value();
        }

        for (int32_t offset = startOffset; offset <= endOffset; offset += 4) {
            if (cancellationToken.load()) break;
            if (candidatesFound >= m_params.candidatesPerLevel) break;

            uint32_t valueToFind = targetAddress - static_cast<uint32_t>(offset);
            SourceResults sources = findSourcesForValue(valueToFind);

            auto processLevel1Source = [&](uint32_t source) {
                bool shouldProcess = true;

                if (m_params.fastScanMode) {
                    size_t shard = static_cast<size_t>(source) % VISITED_SHARDS;
                    std::lock_guard<std::mutex> lock(m_visitedMutexes[shard]);
                    auto& map = m_visitedNodes[shard];
                    auto it = map.find(source);
                    if (it == map.end()) {
                        map[source] = 1;
                    }
                    else {
                        if (1 < it->second) {
                            it->second = 1;
                        }
                        else {
                            shouldProcess = false;
                        }
                    }
                }

                if (shouldProcess) {
                    PathNode* node = allocateNode(source, offset, nullptr);
                    currentLevelCandidates.push_back(node);
                    m_candidatesGenerated++;
                    candidatesFound++;
                }
                };

            if (sources.first) {
                for (uint32_t source : *sources.first) {
                    processLevel1Source(source);
                }
            }
            if (sources.second) {
                for (uint32_t source : *sources.second) {
                    processLevel1Source(source);
                }
            }
        }

        for (PathNode* candidate : currentLevelCandidates) {
            if (cancellationToken.load()) break;

            bool isStatic = false;
            if (m_params.dynamicStaticDetection) {
                PointerPath path;
                path.baseAddress = candidate->address;
                path.offsets = candidate->getForwardOffsets();
                path.finalAddress = m_params.finalAddressTarget;
                if (isValidInAllStates(path)) {
                    isStatic = true;
                    candidate->isStableEndpoint = true;
                }
            } else {
                isStatic = (candidate->address >= m_params.staticBaseStart && candidate->address <= m_params.staticBaseEnd);
            }

            if (isStatic) {
                PointerPath path;
                path.baseAddress = candidate->address;
                path.offsets = candidate->getForwardOffsets();
                path.finalAddress = m_params.finalAddressTarget;

                if (m_params.dynamicStaticDetection || isValidInAllStates(path)) {
                    std::lock_guard<std::mutex> lock(m_resultsMutex);
                    m_foundPaths.push_back(path);
                    m_staticPathsFoundCounter++;
                    m_foundPathsCounter++;
                    if (m_params.stopOnFirstPathFound) {
                        cancellationToken.store(true);
                    }
                }
            }
        }

        if (!m_params.findAllPathLevels && m_staticPathsFoundCounter.load() > 0) {
            return m_foundPaths;
        }

        QThreadPool customPool;
        if (m_params.limitCpuUsage) {
            customPool.setMaxThreadCount(std::max(1, QThread::idealThreadCount() / 2));
        }
        else {
            customPool.setMaxThreadCount(QThread::idealThreadCount());
        }

        for (int level = 2; level <= m_params.maxLevel; ++level) {
            if (cancellationToken.load() || currentLevelCandidates.empty() || m_candidatesGenerated.load() >= m_params.maxCandidates) {
                break;
            }

            reportProgress(QString("Searching Level %1...").arg(level), 0, currentLevelCandidates.size());

            std::vector<PathNode*> nextLevelCandidates;
            std::mutex nextLevelMutex;
            std::atomic<int64_t> processed{ 0 };

            QtConcurrent::blockingMap(&customPool, currentLevelCandidates, [&](PathNode* candidate) {
                if (cancellationToken.load() || m_candidatesGenerated.load() >= m_params.maxCandidates) {
                    return;
                }

                bool isPruned = false;
                if (m_params.dynamicStaticDetection) {
                    isPruned = candidate->isStableEndpoint;
                } else {
                    isPruned = (candidate->address >= m_params.staticBaseStart && candidate->address <= m_params.staticBaseEnd);
                }

                if (isPruned) {
                    // Evaluated inside base level logic.
                }
                else {
                    int candidatesFoundThisLevel = 0;
                    for (int32_t offset = 0; offset <= m_params.maxOffset; offset += 4) {
                        if (cancellationToken.load() || m_candidatesGenerated.load() >= m_params.maxCandidates) {
                            return;
                        }
                        if (candidatesFoundThisLevel >= m_params.candidatesPerLevel) break;

                        uint32_t valueToFind = candidate->address - static_cast<uint32_t>(offset);
                        SourceResults sources = findSourcesForValue(valueToFind);

                        auto processSource = [&](uint32_t source) {
                            bool shouldProcess = true;

                            if (m_params.fastScanMode) {
                                size_t shard = static_cast<size_t>(source) % VISITED_SHARDS;
                                std::lock_guard<std::mutex> lock(m_visitedMutexes[shard]);
                                auto& map = m_visitedNodes[shard];
                                auto it = map.find(source);
                                if (it == map.end()) {
                                    map[source] = level;
                                }
                                else {
                                    if (level < it->second) {
                                        it->second = level;
                                    }
                                    else {
                                        shouldProcess = false;
                                    }
                                }
                            }

                            if (shouldProcess) {
                                PathNode* node = allocateNode(source, offset, candidate);
                                {
                                    std::lock_guard<std::mutex> lock(nextLevelMutex);
                                    nextLevelCandidates.push_back(node);
                                }
                                m_candidatesGenerated++;
                                candidatesFoundThisLevel++;
                            }
                            };

                        if (sources.first) {
                            for (uint32_t source : *sources.first) {
                                processSource(source);
                            }
                        }
                        if (sources.second) {
                            for (uint32_t source : *sources.second) {
                                processSource(source);
                            }
                        }
                    }

                    if (candidatesFoundThisLevel == 0 && m_params.printPartialPaths) {
                        PointerPath path;
                        path.baseAddress = candidate->address;
                        path.offsets = candidate->getForwardOffsets();
                        path.finalAddress = m_params.finalAddressTarget;
                        path.isPartial = true;

                        for (size_t i = 0; i < m_params.capturedStates.size(); ++i) {
                            uint32_t lastValid = path.baseAddress;
                            auto currentAddress = readValueFromState(path.baseAddress, m_params.capturedStates[i].memoryDump);
                            if (currentAddress.has_value()) {
                                bool broke = false;
                                for (size_t j = 0; j < path.offsets.size() - 1; ++j) {
                                    uint32_t next = currentAddress.value() + static_cast<uint32_t>(path.offsets[j]);
                                    lastValid = next;
                                    currentAddress = readValueFromState(next, m_params.capturedStates[i].memoryDump);
                                    if (!currentAddress.has_value()) {
                                        broke = true;
                                        break;
                                    }
                                }
                                if (!broke && !path.offsets.empty()) {
                                    lastValid = currentAddress.value() + static_cast<uint32_t>(path.offsets.back());
                                }
                            }
                            path.brokenStateAddresses.push_back({ static_cast<int>(i), lastValid });
                        }

                        {
                            std::lock_guard<std::mutex> lock(m_resultsMutex);
                            m_foundPaths.push_back(path);
                            m_foundPathsCounter++;
                        }
                    }
                }

                int64_t curr = ++processed;
                if (shouldUpdateProgress(curr)) {
                    reportProgress(QString("Searching Level %1...").arg(level), curr, currentLevelCandidates.size());
                }
                });

            if (m_params.fastScanMode && nextLevelCandidates.size() > 150000) {
                std::sort(nextLevelCandidates.begin(), nextLevelCandidates.end(), [](const PathNode* a, const PathNode* b) {
                    if (std::abs(a->offset) != std::abs(b->offset)) {
                        return std::abs(a->offset) < std::abs(b->offset);
                    }
                    return a->address < b->address;
                    });
                nextLevelCandidates.resize(150000);
            }

            currentLevelCandidates = std::move(nextLevelCandidates);

            for (PathNode* candidate : currentLevelCandidates) {
                if (cancellationToken.load()) break;

                bool isStatic = false;
                if (m_params.dynamicStaticDetection) {
                    PointerPath path;
                    path.baseAddress = candidate->address;
                    path.offsets = candidate->getForwardOffsets();
                    path.finalAddress = m_params.finalAddressTarget;
                    if (isValidInAllStates(path)) {
                        isStatic = true;
                        candidate->isStableEndpoint = true;
                    }
                } else {
                    isStatic = (candidate->address >= m_params.staticBaseStart && candidate->address <= m_params.staticBaseEnd);
                }

                if (isStatic) {
                    PointerPath path;
                    path.baseAddress = candidate->address;
                    path.offsets = candidate->getForwardOffsets();
                    path.finalAddress = m_params.finalAddressTarget;

                    if (m_params.dynamicStaticDetection || isValidInAllStates(path)) {
                        std::lock_guard<std::mutex> lock(m_resultsMutex);
                        m_foundPaths.push_back(path);
                        m_staticPathsFoundCounter++;
                        m_foundPathsCounter++;
                        if (m_params.stopOnFirstPathFound) {
                            cancellationToken.store(true);
                        }
                    }
                }
            }

            if (!m_params.findAllPathLevels && m_staticPathsFoundCounter.load() > 0) {
                break;
            }
        }

        reportProgress("Search phase complete.", m_params.maxCandidates, m_params.maxCandidates);
        return m_foundPaths;
    }

    // Performs verification checks on a discovered path.
    // Resolves and evaluates offsets across every loaded state dump to confirm link stability.
    bool StateBasedScannerStrategyBase::isValidInAllStates(const PointerPath& path) {
        for (const auto& state : m_params.capturedStates) {
            auto finalAddress = recalculatePathInState(path, state);
            if (!finalAddress.has_value() || !m_manager->areAddressesEquivalent(finalAddress.value(), state.targetAddress)) {
                return false;
            }
        }
        return true;
    }

    std::optional<uint32_t> StateBasedScannerStrategyBase::recalculatePathInState(const PointerPath& path, const ScanState& state) {
        auto currentAddress = readValueFromState(path.baseAddress, state.memoryDump);
        if (!currentAddress.has_value()) return std::nullopt;

        for (size_t i = 0; i < path.offsets.size() - 1; ++i) {
            uint32_t nextAddress = currentAddress.value() + static_cast<uint32_t>(path.offsets[i]);
            currentAddress = readValueFromState(nextAddress, state.memoryDump);
            if (!currentAddress.has_value()) return std::nullopt;
        }

        int32_t lastOffset = path.offsets.back();
        uint32_t finalAddress = currentAddress.value() + static_cast<uint32_t>(lastOffset);
        return finalAddress;
    }

    std::optional<uint32_t> StateBasedScannerStrategyBase::readValueFromState(uint32_t address, const std::vector<uint8_t>& memory) {
        auto [normalizedReadAddress, isShort] = m_manager->normalizeAddressForRead(address);
        int64_t indexInDump = m_manager->getIndexForStateDump(normalizedReadAddress);

        if (indexInDump < 0 || indexInDump + 3 >= static_cast<int64_t>(memory.size())) {
            return std::nullopt;
        }

        uint32_t value;
        std::memcpy(&value, &memory[indexInDump], 4);

        if (m_manager->getRetroAchievementsPrefix() == "G") {
            value = Core::swapEndian32(value);
        }

        return value;
    }

    void StateBasedScannerStrategyBase::reportProgress(const QString& message, int64_t current, int64_t max) {
        if (!m_progressCallback) return;

        {
            std::lock_guard<std::mutex> lock(m_reportingMutex);
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastReportTime).count();
            if (elapsed < 100 && current < max && current > 0) {
                return;
            }
            m_lastReportTime = now;
        }

        ScanProgressReport report;
        report.statusMessage = message;
        report.currentValue = current;
        report.maxValue = max;
        report.foundCount = static_cast<int>(m_staticPathsFoundCounter.load());
        report.partialCount = static_cast<int>(m_foundPathsCounter.load() - report.foundCount);

        m_progressCallback(report);
    }

}
