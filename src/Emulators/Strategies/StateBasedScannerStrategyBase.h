#pragma once
#include "../IPointerScannerStrategy.h"

#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>

namespace PointerFinder2::Emulators::StateBased {

    struct SourceResults {
        const std::vector<uint32_t>* first = nullptr;
        const std::vector<uint32_t>* second = nullptr;
    };

    // This is the core background engine that implements state-based searching, following 
    // memory links backward from your target address to find stable pointer paths.
    class StateBasedScannerStrategyBase : public IPointerScannerStrategy {
    public:
        StateBasedScannerStrategyBase();
        virtual ~StateBasedScannerStrategyBase() override;

        std::vector<DataModels::PointerPath> scan(
            IEmulatorManager* manager,
            const DataModels::ScanParameters& parameters,
            std::function<void(const DataModels::ScanProgressReport&)> progressCallback,
            std::atomic<bool>& cancellationToken
        ) override;

    protected:
        struct PathNode {
            uint32_t address;
            int32_t offset;
            PathNode* child = nullptr;
            bool isStableEndpoint = false;

            std::vector<int32_t> getForwardOffsets() const {
                std::vector<int32_t> outOffsets;
                const PathNode* current = this;
                while (current != nullptr) {
                    outOffsets.push_back(current->offset);
                    current = current->child;
                }
                return outOffsets;
            }
        };

        virtual void buildPointerMap(const std::vector<uint8_t>& memory, std::atomic<bool>& token) = 0;
        virtual SourceResults findSourcesForValue(uint32_t value) const = 0;

        void reportProgress(const QString& message, int64_t current, int64_t max);
        bool isValidInAllStates(const DataModels::PointerPath& path);
        std::optional<uint32_t> recalculatePathInState(const DataModels::PointerPath& path, const DataModels::ScanState& state);
        std::optional<uint32_t> readValueFromState(uint32_t address, const std::vector<uint8_t>& memory);

        IEmulatorManager* m_manager = nullptr;
        DataModels::ScanParameters m_params;
        std::function<void(const DataModels::ScanProgressReport&)> m_progressCallback;
        std::atomic<bool>* m_cancelToken = nullptr;

        std::unordered_map<uint32_t, std::vector<uint32_t>> m_pointerMap;

        static constexpr size_t VISITED_SHARDS = 67;
        // True sharded container array matching the lock array
        std::unordered_map<uint32_t, int> m_visitedNodes[VISITED_SHARDS];
        std::mutex m_visitedMutexes[VISITED_SHARDS];

        std::vector<DataModels::PointerPath> m_foundPaths;
        std::mutex m_resultsMutex;

        static constexpr size_t ALLOCATOR_SHARDS = 64;
        std::deque<PathNode> m_nodePools[ALLOCATOR_SHARDS];
        std::mutex m_poolMutexes[ALLOCATOR_SHARDS];

        std::atomic<int64_t> m_candidatesGenerated{ 0 };
        std::atomic<int64_t> m_candidatesValidated{ 0 };
        std::atomic<int64_t> m_staticPathsFoundCounter{ 0 };
        std::atomic<int64_t> m_foundPathsCounter{ 0 };

        int64_t m_nextUpdateThreshold = 100;
        std::mutex m_reportingMutex;
        std::chrono::steady_clock::time_point m_lastReportTime;

        bool shouldUpdateProgress(int64_t currentCount);
        PathNode* allocateNode(uint32_t address, int32_t offset, PathNode* child);
    };

}
