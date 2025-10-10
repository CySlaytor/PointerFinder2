using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.LiveScan
{
    // This abstract base class contains the core logic for a "live scan" using a parallel
    // Breadth-First Search (BFS) algorithm. It works by building a map of all pointers
    // in memory and then working backwards from the target address level by level.
    // Console-specific details are handled by inheriting classes.
    public abstract class LiveScannerStrategyBase : IPointerScannerStrategy
    {
        protected class PathCandidate
        {
            public uint HeadAddress { get; set; }
            public List<int> ReverseOffsets { get; set; }
        }

        protected ScanParameters _params;
        protected IEmulatorManager _manager;
        protected IProgress<ScanProgressReport> _progress;
        protected CancellationToken _cancellationToken;
        protected ConcurrentDictionary<uint, List<uint>> _pointerMap;
        private ConcurrentBag<PointerPath> _foundPaths;
        private long _foundPathsCounter = 0;
        private readonly DebugLogForm logger = DebugLogForm.Instance;
        private bool _shouldLogDetails;

        #region Abstract Methods
        // Builds the pointer map by scanning the relevant memory regions.
        protected abstract Task BuildPointerMapAsync();

        // Finds all addresses that point to a given value, handling console-specific logic like address mirroring.
        protected abstract IEnumerable<uint> FindSourcesForValue(uint value);
        #endregion

        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new ConcurrentBag<PointerPath>();
            _pointerMap = new ConcurrentDictionary<uint, List<uint>>(Environment.ProcessorCount, 100000);
            _foundPathsCounter = 0;
            _shouldLogDetails = DebugSettings.LogLiveScan;

            if (_params == null) return new List<PointerPath>();

            try
            {
                if (_shouldLogDetails) logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW SCAN ---");

                // Phase 1: Scan all relevant memory regions to build a map of what points to where.
                await BuildPointerMapAsync();
                if (cancellationToken.IsCancellationRequested) return _foundPaths.ToList();
                if (_shouldLogDetails) logger.Log($"[{_manager.EmulatorName}] Pointer map built with {_pointerMap.Count:N0} unique pointers. Resolving paths...");

                // Phase 2: Work backwards from the target address using a parallel Breadth-First Search (BFS).
                await ResolvePathsBackwardAsync();

                if (_shouldLogDetails) logger.Log($"[{_manager.EmulatorName}] --- SCAN COMPLETE: Found {_foundPaths.Count:N0} paths. ---");
            }
            catch (OperationCanceledException)
            {
                if (_shouldLogDetails) logger.Log($"[{_manager.EmulatorName}] --- SCAN CANCELLED. Found {_foundPaths.Count:N0} paths. ---");
            }
            finally
            {
                _pointerMap?.Clear();
                _pointerMap = null;
            }

            return _foundPaths.ToList();
        }

        // Parallel Breadth-First Search (BFS) to resolve paths.
        private async Task ResolvePathsBackwardAsync()
        {
            var currentLevelCandidates = new ConcurrentBag<PathCandidate>();
            uint unnormalizedTarget = _manager.UnnormalizeAddress(_params.TargetAddress.ToString("X"));
            if (_shouldLogDetails) logger.Log($"[Path Resolution] Starting search for Target Address: 0x{unnormalizedTarget:X}");


            // --- Find Level 1 candidates ---
            int step = _params.Use16ByteAlignment ? 16 : 4;
            uint startAddress = _params.Use16ByteAlignment ? unnormalizedTarget & 0xFFFFFFF0 : unnormalizedTarget;

            var searchItems = new List<(uint valueToSearch, int offset)>();
            searchItems.Add((unnormalizedTarget, 0));

            for (uint currentAddress = startAddress; currentAddress > 4096 && currentAddress >= unnormalizedTarget - _params.MaxOffset; currentAddress -= (uint)step)
            {
                int offset = (int)(unnormalizedTarget - currentAddress);
                if (offset != 0) searchItems.Add((currentAddress, offset));
            }

            if (_params.ScanForStructureBase)
            {
                for (int offset = -4; offset >= -_params.MaxNegativeOffset; offset -= 4)
                {
                    searchItems.Add((unnormalizedTarget - (uint)offset, offset));
                }
            }

            foreach (var item in searchItems)
            {
                if (_cancellationToken.IsCancellationRequested) break;
                // Added detailed address-by-address logging.
                if (_shouldLogDetails) logger.Log($"  [L1] Searching for value 0x{item.valueToSearch:X} (Offset: {item.offset:+X;-X})");
                foreach (var source in FindSourcesForValue(item.valueToSearch))
                {
                    if (_shouldLogDetails) logger.Log($"    -> Found source at 0x{source:X}");
                    currentLevelCandidates.Add(new PathCandidate { HeadAddress = source, ReverseOffsets = new List<int> { item.offset } });
                }
            }

            ReportProgress($"Found {currentLevelCandidates.Count:N0} Level 1 candidate paths...", 0, 0, (int)_foundPathsCounter);

            // Check if any Level 1 candidates are already static bases.
            await ProcessCandidates(currentLevelCandidates);

            // --- Iterate through deeper levels ---
            for (int level = 2; level <= _params.MaxLevel; level++)
            {
                if (_cancellationToken.IsCancellationRequested || !currentLevelCandidates.Any() || _foundPathsCounter >= _params.MaxResults) break;

                var candidatesToProcess = currentLevelCandidates.Where(c => c.HeadAddress < _params.StaticBaseStart || c.HeadAddress > _params.StaticBaseEnd).ToList();
                if (!candidatesToProcess.Any()) break;

                ReportProgress($"Searching Level {level} from {candidatesToProcess.Count:N0} paths...", 0, 0, (int)_foundPathsCounter);
                if (_shouldLogDetails) logger.Log($"[Path Resolution] Starting Level {level} search with {candidatesToProcess.Count:N0} candidates...");


                var nextLevelCandidates = new ConcurrentBag<PathCandidate>();
                var parallelOptions = new ParallelOptions { CancellationToken = _cancellationToken };
                if (_params.LimitCpuUsage) parallelOptions.MaxDegreeOfParallelism = Math.Max(1, Environment.ProcessorCount / 2);

                Parallel.ForEach(candidatesToProcess, parallelOptions, (candidate, loopState) =>
                {
                    if (_shouldLogDetails) logger.Log($"  [L{level}] Processing candidate with head at 0x{candidate.HeadAddress:X}");
                    for (int offset = 0; offset <= _params.MaxOffset; offset += step)
                    {
                        if (loopState.ShouldExitCurrentIteration || _foundPathsCounter >= _params.MaxResults)
                        {
                            loopState.Stop();
                            return;
                        }

                        uint valueToFind = candidate.HeadAddress - (uint)offset;
                        foreach (var source in FindSourcesForValue(valueToFind))
                        {
                            if (_shouldLogDetails) logger.Log($"    -> Found source for value 0x{valueToFind:X} at address 0x{source:X}");
                            var newOffsets = new List<int>(candidate.ReverseOffsets);
                            newOffsets.Add(offset);
                            nextLevelCandidates.Add(new PathCandidate { HeadAddress = source, ReverseOffsets = newOffsets });
                        }
                    }
                });

                currentLevelCandidates = nextLevelCandidates;
                await ProcessCandidates(currentLevelCandidates);
            }
        }

        // Checks a collection of candidates, adds valid static paths to results, and reports progress.
        private Task ProcessCandidates(ConcurrentBag<PathCandidate> candidates)
        {
            return Task.Run(() =>
            {
                var parallelOptions = new ParallelOptions { CancellationToken = _cancellationToken };
                if (_params.LimitCpuUsage) parallelOptions.MaxDegreeOfParallelism = Math.Max(1, Environment.ProcessorCount / 2);

                Parallel.ForEach(candidates, parallelOptions, (candidate, loopState) =>
                {
                    if (_foundPathsCounter >= _params.MaxResults)
                    {
                        loopState.Stop();
                        return;
                    }
                    if (candidate.HeadAddress >= _params.StaticBaseStart && candidate.HeadAddress <= _params.StaticBaseEnd)
                    {
                        var finalOffsets = new List<int>(candidate.ReverseOffsets);
                        finalOffsets.Reverse();
                        var newPath = new PointerPath { BaseAddress = candidate.HeadAddress, Offsets = finalOffsets, FinalAddress = _params.TargetAddress };
                        _foundPaths.Add(newPath);
                        long currentCount = Interlocked.Increment(ref _foundPathsCounter);

                        if (_shouldLogDetails) logger.Log($"    >>>>>> VALID STATIC PATH FOUND: {newPath.BaseAddress:X} -> {newPath.GetOffsetsString()}");

                        if (currentCount % 1000 == 0)
                        {
                            ReportProgress(null, 0, 0, (int)currentCount);
                        }
                    }
                });
            }, _cancellationToken);
        }

        // Helper to send progress updates to the UI thread.
        protected void ReportProgress(string message, long current, long max, int found)
        {
            var report = new ScanProgressReport { FoundCount = found };
            if (message != null) report.StatusMessage = message;
            if (max > 0)
            {
                report.CurrentValue = current;
                report.MaxValue = max;
            }
            _progress?.Report(report);
        }
    }
}