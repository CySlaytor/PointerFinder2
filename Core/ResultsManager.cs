using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    // Manages the state of the scan results, including data, sorting, and undo/redo history.
    public class ResultsManager
    {
        public IReadOnlyList<PointerPath> CurrentResults => _currentResults;
        public int ResultsCount => _currentResults.Count;
        public bool HasResults => _currentResults.Any();
        public bool CanUndo => _undoStack.Any();
        public bool CanRedo => _redoStack.Any();

        public string SortedColumnName { get; private set; }
        public SortOrder SortOrder { get; private set; } = SortOrder.None;

        private List<PointerPath> _currentResults = new List<PointerPath>();
        private readonly Stack<List<PointerPath>> _undoStack = new Stack<List<PointerPath>>();
        private readonly Stack<List<PointerPath>> _redoStack = new Stack<List<PointerPath>>();

        public event EventHandler<ResultsChangedEventArgs> ResultsChanged;
        public event EventHandler<SortChangedEventArgs> SortChanged;

        public void SetNewResults(List<PointerPath> newResults, bool isNewDataSet)
        {
            if (isNewDataSet)
            {
                PushUndoState();
            }
            _currentResults = newResults ?? new List<PointerPath>();
            if (isNewDataSet)
            {
                ClearSorting();
            }
            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(isNewDataSet));
        }

        public void ClearResults()
        {
            PushUndoState();
            _currentResults.Clear();
            ClearSorting();
            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(true));
        }

        public void Sort(string columnName, SortOrder currentSortOrder)
        {
            if (!HasResults) return;

            SortOrder newOrder;
            if (SortedColumnName == columnName)
            {
                newOrder = (currentSortOrder == SortOrder.Ascending) ? SortOrder.Descending : SortOrder.Ascending;
            }
            else
            {
                newOrder = SortOrder.Ascending;
            }

            _currentResults.Sort((p1, p2) =>
            {
                int compareResult = 0;
                if (columnName == "colBase") compareResult = p1.BaseAddress.CompareTo(p2.BaseAddress);
                else if (columnName == "colFinal") compareResult = p1.FinalAddress.CompareTo(p2.FinalAddress);
                else if (columnName.StartsWith("colOffset"))
                {
                    int offsetIndex = int.Parse(columnName.Replace("colOffset", "")) - 1;
                    int offset1 = (offsetIndex < p1.Offsets.Count) ? p1.Offsets[offsetIndex] : int.MinValue;
                    int offset2 = (offsetIndex < p2.Offsets.Count) ? p2.Offsets[offsetIndex] : int.MinValue;
                    compareResult = offset1.CompareTo(offset2);
                }
                return (newOrder == SortOrder.Descending) ? -compareResult : compareResult;
            });

            SortedColumnName = columnName;
            SortOrder = newOrder;

            SortChanged?.Invoke(this, new SortChangedEventArgs(columnName, newOrder));
            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(false));
        }

        public async Task SortByLowestOffsetsAsync()
        {
            if (!HasResults) return;

            PushUndoState();
            ClearSorting();

            await Task.Run(() =>
            {
                var offsetCache = _currentResults.ToDictionary(
                    p => p,
                    p => p.Offsets.Select(o => Math.Abs(o)).OrderBy(o => o).ToList()
                );

                _currentResults.Sort((p1, p2) =>
                {
                    if (GlobalSettings.SortByLevelFirst)
                    {
                        int levelComparison = p1.Offsets.Count.CompareTo(p2.Offsets.Count);
                        if (levelComparison != 0) return levelComparison;
                    }

                    var p1Offsets = offsetCache[p1];
                    var p2Offsets = offsetCache[p2];

                    int minCount = Math.Min(p1Offsets.Count, p2Offsets.Count);
                    for (int i = 0; i < minCount; i++)
                    {
                        int comparison = p1Offsets[i].CompareTo(p2Offsets[i]);
                        if (comparison != 0) return comparison;
                    }

                    return p1.Offsets.Count.CompareTo(p2.Offsets.Count);
                });
            });

            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(false));
        }

        // Added a new method to handle the result of a filtering operation, ensuring an undo state is created.
        public void ApplyFilterResults(List<PointerPath> filteredPaths)
        {
            PushUndoState();
            _currentResults = filteredPaths ?? new List<PointerPath>();
            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(isNewDataSet: false));
        }

        public void DeleteSelected(IEnumerable<int> indicesToRemove)
        {
            PushUndoState();
            var indicesSet = new HashSet<int>(indicesToRemove);
            _currentResults = _currentResults.Where((path, index) => !indicesSet.Contains(index)).ToList();
            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(false));
        }

        public void Undo()
        {
            if (!CanUndo) return;
            _redoStack.Push(new List<PointerPath>(_currentResults));
            _currentResults = _undoStack.Pop();
            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(false));
        }

        public void Redo()
        {
            if (!CanRedo) return;
            _undoStack.Push(new List<PointerPath>(_currentResults));
            _currentResults = _redoStack.Pop();
            ResultsChanged?.Invoke(this, new ResultsChangedEventArgs(false));
        }

        public void ClearHistory()
        {
            _undoStack.Clear();
            _redoStack.Clear();
        }

        public void ClearSorting()
        {
            SortedColumnName = null;
            SortOrder = SortOrder.None;
            SortChanged?.Invoke(this, new SortChangedEventArgs(null, SortOrder.None));
        }

        private void PushUndoState()
        {
            _undoStack.Push(new List<PointerPath>(_currentResults));
            _redoStack.Clear();
        }
    }

    public class ResultsChangedEventArgs : EventArgs
    {
        public bool IsNewDataSet { get; }
        public ResultsChangedEventArgs(bool isNewDataSet)
        {
            IsNewDataSet = isNewDataSet;
        }
    }

    public class SortChangedEventArgs : EventArgs
    {
        public string ColumnName { get; }
        public SortOrder SortOrder { get; }
        public SortChangedEventArgs(string columnName, SortOrder sortOrder)
        {
            ColumnName = columnName;
            SortOrder = sortOrder;
        }
    }
}