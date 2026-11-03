#include "ResultsManager.h"

#include "../Models/GlobalSettings.h"

#include <algorithm>
#include <QtConcurrent>
#include <unordered_map>

namespace PointerFinder2::Core {

    using namespace PointerFinder2::DataModels;

    ResultsManager::ResultsManager(QObject* parent) : QObject(parent) {}

    ResultsManager::~ResultsManager() {
        m_sortWatcher.cancel();
        m_sortWatcher.waitForFinished();
    }

    const std::vector<PointerPath>& ResultsManager::getCurrentResults() const {
        return m_currentResults;
    }

    int ResultsManager::getResultsCount() const {
        return static_cast<int>(m_currentResults.size());
    }

    bool ResultsManager::hasResults() const {
        return !m_currentResults.empty();
    }

    bool ResultsManager::canUndo() const {
        return !m_undoStack.empty();
    }

    bool ResultsManager::canRedo() const {
        return !m_redoStack.empty();
    }

    QString ResultsManager::getSortedColumnName() const {
        return m_sortedColumnName;
    }

    Qt::SortOrder ResultsManager::getSortOrder() const {
        return m_sortOrder;
    }

    void ResultsManager::setNewResults(std::vector<PointerPath> newResults, bool isNewDataSet) {
        if (isNewDataSet) {
            pushUndoState();
        }
        m_currentResults = std::move(newResults);
        if (isNewDataSet) {
            clearSorting();
        }
        emit resultsChanged(isNewDataSet);
    }

    // Wipes out the active list of found pointer paths, saving the previous state to the undo history.
    void ResultsManager::clearResults() {
        pushUndoState();
        m_currentResults.clear();
        clearSorting();
        emit resultsChanged(true);
    }

    void ResultsManager::sort(const QString& columnName, Qt::SortOrder currentSortOrder) {
        if (!hasResults()) return;

        Qt::SortOrder newOrder = (m_sortedColumnName == columnName)
            ? ((currentSortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder)
            : Qt::AscendingOrder;

        int offsetIndex = -1;
        bool isBase = false;
        bool isFinal = false;

        if (columnName == "colBase") {
            isBase = true;
        }
        else if (columnName == "colFinal") {
            isFinal = true;
        }
        else if (columnName.startsWith("colOffset")) {
            offsetIndex = columnName.mid(9).toInt() - 1;
        }

        std::sort(m_currentResults.begin(), m_currentResults.end(), [isBase, isFinal, offsetIndex, newOrder](const PointerPath& p1, const PointerPath& p2) {
            if (isBase) {
                return (newOrder == Qt::DescendingOrder) ? (p1.baseAddress > p2.baseAddress) : (p1.baseAddress < p2.baseAddress);
            }
            else if (isFinal) {
                return (newOrder == Qt::DescendingOrder) ? (p1.finalAddress > p2.finalAddress) : (p1.finalAddress < p2.finalAddress);
            }
            else if (offsetIndex >= 0) {
                int offset1 = (offsetIndex < static_cast<int>(p1.offsets.size())) ? p1.offsets[offsetIndex] : std::numeric_limits<int>::min();
                int offset2 = (offsetIndex < static_cast<int>(p2.offsets.size())) ? p2.offsets[offsetIndex] : std::numeric_limits<int>::min();
                return (newOrder == Qt::DescendingOrder) ? (offset1 > offset2) : (offset1 < offset2);
            }
            return false;
            });

        m_sortedColumnName = columnName;
        m_sortOrder = newOrder;

        emit sortChanged(columnName, newOrder);
        emit resultsChanged(false);
    }

    void ResultsManager::sortByLowestOffsets() {
        if (!hasResults()) return;

        m_sortWatcher.cancel();
        m_sortWatcher.waitForFinished();

        pushUndoState();
        clearSorting();

        std::vector<PointerPath> resultsCopy = m_currentResults;
        bool sortByLevelFirst = GlobalSettings::sortByLevelFirst;

        QFuture<void> future = QtConcurrent::run([this, resultsCopy = std::move(resultsCopy), sortByLevelFirst]() mutable {
            std::vector<std::vector<int32_t>> offsetCache(resultsCopy.size());
            std::vector<size_t> indices(resultsCopy.size());

            for (size_t i = 0; i < resultsCopy.size(); ++i) {
                indices[i] = i;
                auto& absOffsets = offsetCache[i];
                absOffsets.reserve(resultsCopy[i].offsets.size());
                for (int32_t o : resultsCopy[i].offsets) {
                    absOffsets.push_back(std::abs(o));
                }
                std::sort(absOffsets.begin(), absOffsets.end());
            }

            std::sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
                const auto& p1 = resultsCopy[i1];
                const auto& p2 = resultsCopy[i2];

                if (sortByLevelFirst) {
                    if (p1.offsets.size() != p2.offsets.size()) {
                        return p1.offsets.size() < p2.offsets.size();
                    }
                }

                const auto& p2Offsets = offsetCache[i2];
                const auto& p1Offsets = offsetCache[i1];

                size_t minCount = std::min(p1Offsets.size(), p2Offsets.size());
                for (size_t i = 0; i < minCount; ++i) {
                    if (p1Offsets[i] != p2Offsets[i]) {
                        return p1Offsets[i] < p2Offsets[i];
                    }
                }

                return p1.offsets.size() < p2.offsets.size();
                });

            std::vector<PointerPath> sortedResults;
            sortedResults.reserve(resultsCopy.size());
            for (size_t idx : indices) {
                sortedResults.push_back(std::move(resultsCopy[idx]));
            }

            QMetaObject::invokeMethod(this, [this, sortedResults = std::move(sortedResults)]() mutable {
                m_currentResults = std::move(sortedResults);
                emit resultsChanged(false);
                emit asyncSortCompleted();
                }, Qt::QueuedConnection);
            });

        m_sortWatcher.setFuture(future);
    }

    void ResultsManager::applyFilterResults(const std::vector<PointerPath>& filteredPaths) {
        pushUndoState();
        m_currentResults = filteredPaths;
        emit resultsChanged(false);
    }

    void ResultsManager::deleteSelected(const std::vector<int>& indicesToRemove) {
        if (indicesToRemove.empty()) return;
        pushUndoState();

        std::vector<bool> removeMap(m_currentResults.size(), false);
        for (int idx : indicesToRemove) {
            if (idx >= 0 && idx < static_cast<int>(m_currentResults.size())) {
                removeMap[idx] = true;
            }
        }

        std::vector<PointerPath> filtered;
        filtered.reserve(m_currentResults.size() - indicesToRemove.size());
        for (size_t i = 0; i < m_currentResults.size(); ++i) {
            if (!removeMap[i]) {
                filtered.push_back(m_currentResults[i]);
            }
        }

        m_currentResults = std::move(filtered);
        emit resultsChanged(false);
    }

    // Reverts your last filter or deletion step, restoring the previous list of found paths.
    void ResultsManager::undo() {
        if (!canUndo()) return;

        // Only push non-empty states to the redo history
        if (!m_currentResults.empty()) {
            m_redoStack.push(m_currentResults);
        }

        m_currentResults = m_undoStack.top();
        m_undoStack.pop();
        emit resultsChanged(false);
    }

    // Re-applies a filter or deletion step that you previously reverted.
    void ResultsManager::redo() {
        if (!canRedo()) return;

        // Only push non-empty states to the undo history
        if (!m_currentResults.empty()) {
            m_undoStack.push(m_currentResults);
        }

        m_currentResults = m_redoStack.top();
        m_redoStack.pop();
        emit resultsChanged(false);
    }

    void ResultsManager::clearHistory() {
        while (!m_undoStack.empty()) m_undoStack.pop();
        while (!m_redoStack.empty()) m_redoStack.pop();
    }

    void ResultsManager::clearSorting() {
        m_sortedColumnName = "";
        m_sortOrder = Qt::AscendingOrder;
        emit sortChanged("", Qt::AscendingOrder);
    }

    void ResultsManager::pushUndoState() {
        // Prevent empty results (the Dashboard state) from entering the Undo stack
        if (!m_currentResults.empty()) {
            m_undoStack.push(m_currentResults);
        }
        while (!m_redoStack.empty()) m_redoStack.pop();
    }

}
