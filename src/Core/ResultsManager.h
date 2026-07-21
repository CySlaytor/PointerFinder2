#pragma once

#include "../Models/PointerPath.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <stack>
#include <vector>

namespace PointerFinder2::Core {

    // Manages, filters, sorts, and maintains history of discovered pointer chains.
    // This class manages the list of found pointer paths, keeping track of your 
    // search history to support Undo and Redo operations.
    class ResultsManager : public QObject {
        Q_OBJECT
    public:
        explicit ResultsManager(QObject* parent = nullptr);
        ~ResultsManager() override;

        const std::vector<DataModels::PointerPath>& getCurrentResults() const;
        int getResultsCount() const;
        bool hasResults() const;
        bool canUndo() const;
        bool canRedo() const;

        QString getSortedColumnName() const;
        Qt::SortOrder getSortOrder() const;

        void setNewResults(std::vector<DataModels::PointerPath> newResults, bool isNewDataSet);
        void clearResults();
        void sort(const QString& columnName, Qt::SortOrder currentSortOrder);

        // Sorts results asynchronously to minimize absolute offset values.
        void sortByLowestOffsets();

        void applyFilterResults(const std::vector<DataModels::PointerPath>& filteredPaths);
        void deleteSelected(const std::vector<int>& indicesToRemove);

        void undo();
        void redo();
        void clearHistory();
        void clearSorting();

    signals:
        void resultsChanged(bool isNewDataSet);
        void sortChanged(const QString& columnName, Qt::SortOrder sortOrder);
        void asyncSortCompleted();

    private:
        std::vector<DataModels::PointerPath> m_currentResults;
        std::stack<std::vector<DataModels::PointerPath>> m_undoStack;
        std::stack<std::vector<DataModels::PointerPath>> m_redoStack;

        QString m_sortedColumnName = "";
        Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

        QFutureWatcher<void> m_sortWatcher;

        void pushUndoState();
    };

}
