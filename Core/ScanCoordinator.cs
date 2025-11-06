using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using PointerFinder2.Emulators.StateBased;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    // Orchestrates and manages all scanning and filtering operations in the background.
    public class ScanCoordinator : IDisposable
    {
        public event Action<ScanProgressReport> ProgressUpdated;
        public event Action<ScanCompletedEventArgs> ScanCompleted;
        public event Action<string> OperationStarted;
        public event Action<string> OperationFinished;
        public event Action<int> FoundCountUpdated;
        public event Action<List<PointerPath>> FilterCompleted;

        private CancellationTokenSource _operationCts;
        private readonly Stopwatch _operationStopwatch = new Stopwatch();

        public bool IsBusy => _operationCts != null && !_operationCts.IsCancellationRequested;
        public ScanParameters LastScanParams { get; private set; }

        public async Task StartScan(IEmulatorManager manager, IPointerScannerStrategy scanner, ScanParameters parameters, bool isRefine, HashSet<PointerPath> existingPaths = null)
        {
            if (IsBusy) return;

            LastScanParams = parameters;
            _operationCts = new CancellationTokenSource();
            _operationStopwatch.Restart();

            OperationStarted?.Invoke(isRefine ? "Refining results..." : null);

            var progress = new Progress<ScanProgressReport>(report =>
            {
                ProgressUpdated?.Invoke(report);
                if (report.FoundCount > 0)
                {
                    FoundCountUpdated?.Invoke(report.FoundCount);
                }
            });

            List<PointerPath> results = null;
            bool wasCancelled = false;

            try
            {
                var scanTask = Task.Run(() => scanner.Scan(manager, parameters, progress, _operationCts.Token), _operationCts.Token);
                results = await scanTask;

                if (isRefine && existingPaths != null)
                {
                    ProgressUpdated?.Invoke(new ScanProgressReport { StatusMessage = "Scan phase complete, intersecting results..." });
                    results = results.Where(p => existingPaths.Contains(p)).ToList();
                }
            }
            catch (OperationCanceledException)
            {
                wasCancelled = true;
                results = new List<PointerPath>(); // Return empty list on cancellation
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                results = new List<PointerPath>();
            }
            finally
            {
                _operationStopwatch.Stop();
                var eventArgs = new ScanCompletedEventArgs(
                    results,
                    _operationStopwatch.Elapsed,
                    wasCancelled || (_operationCts?.IsCancellationRequested ?? false),
                    false, // Placeholder for memory monitor feature if re-added
                    scanner is StateBasedScannerStrategyBase,
                    isRefine
                );
                ScanCompleted?.Invoke(eventArgs);
                DisposeCts();
            }
        }

        public async Task StartFiltering(IEmulatorManager manager, IEnumerable<PointerPath> initialPaths)
        {
            if (IsBusy) return;

            _operationCts = new CancellationTokenSource();
            _operationStopwatch.Restart();
            OperationStarted?.Invoke("Starting filter...");

            var validFilteredPaths = new ConcurrentBag<PointerPath>(initialPaths);
            List<PointerPath> finalResults = null;

            try
            {
                await FilterPathsContinuously(manager, validFilteredPaths, _operationCts.Token);
            }
            catch (TaskCanceledException) { }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during filtering: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            finally
            {
                // The final list of results is correctly captured from the state of the ConcurrentBag
                // at the end of the operation. This ensures that if the bag is empty (e.g., after a save state load),
                // an empty list is sent back to the UI, correctly clearing the grid.
                finalResults = validFilteredPaths.ToList();
                FilterCompleted?.Invoke(finalResults);

                _operationStopwatch.Stop();
                OperationFinished?.Invoke($"Filtering stopped. {finalResults.Count:N0} paths remain.");
                DisposeCts();
            }
        }

        private async Task FilterPathsContinuously(IEmulatorManager manager, ConcurrentBag<PointerPath> validPaths, CancellationToken token)
        {
            var pathsToProcess = validPaths;
            var validatedPaths = new ConcurrentBag<PointerPath>();

            while (!token.IsCancellationRequested)
            {
                if (pathsToProcess.IsEmpty) break;

                await Task.Run(() =>
                {
                    Parallel.ForEach(pathsToProcess, path =>
                    {
                        if (token.IsCancellationRequested) return;
                        uint? calculatedAddress = manager.RecalculateFinalAddress(path, path.FinalAddress);
                        if (calculatedAddress.HasValue && calculatedAddress.Value == path.FinalAddress)
                        {
                            validatedPaths.Add(path);
                        }
                    });
                }, token);

                if (token.IsCancellationRequested) break;

                // This is the swap-buffer pattern. The `validPaths` variable, which is scoped to the parent `StartFiltering`
                // method, is updated to point to the newly validated list. The original list is then cleared.
                // This is crucial for the `finally` block to have the correct final list of results.
                validPaths.Clear();
                pathsToProcess = validatedPaths;
                foreach (var path in pathsToProcess)
                {
                    validPaths.Add(path);
                }
                validatedPaths = new ConcurrentBag<PointerPath>();

                FoundCountUpdated?.Invoke(validPaths.Count);

                int pathCount = validPaths.Count;
                int delayMs = (pathCount < 1000) ? 30 : (pathCount / 100);

                try
                {
                    await Task.Delay(delayMs, token);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
            }
        }

        public void StopCurrentOperation()
        {
            _operationCts?.Cancel();
        }

        private void DisposeCts()
        {
            _operationCts?.Dispose();
            _operationCts = null;
        }

        public void Dispose()
        {
            DisposeCts();
        }
    }

    public class ScanCompletedEventArgs : EventArgs
    {
        public List<PointerPath> Results { get; }
        public TimeSpan Duration { get; }
        public bool WasCancelled { get; }
        public bool StoppedByMemoryMonitor { get; }
        public bool IsStateScan { get; }
        public bool IsRefineScan { get; }

        public ScanCompletedEventArgs(List<PointerPath> results, TimeSpan duration, bool wasCancelled, bool stoppedByMemoryMonitor, bool isStateScan, bool isRefineScan)
        {
            Results = results;
            Duration = duration;
            WasCancelled = wasCancelled;
            StoppedByMemoryMonitor = stoppedByMemoryMonitor;
            IsStateScan = isStateScan;
            IsRefineScan = isRefineScan;
        }
    }
}