using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.DuckStation
{
    // Implements the pointer scanning strategy specifically for the DuckStation emulator.
    // This is a simpler strategy as it only needs to scan the main PS1 RAM.
    public class DuckStationScannerStrategy : IPointerScannerStrategy
    {
        // Private members for managing scan state.
        private ScanParameters _params;
        private IEmulatorManager _manager;
        private ConcurrentBag<PointerPath> _foundPaths;
        private IProgress<ScanProgressReport> _progress;
        private CancellationToken _cancellationToken;
        // Using a ConcurrentDictionary for the pointer map is crucial for thread-safe parallel processing.
        private ConcurrentDictionary<uint, List<uint>> _pointerMap;
        // Singleton instance of the logger for debug output.
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        // Main entry point for initiating a pointer scan.
        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            // Initialize state for a new scan.
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new ConcurrentBag<PointerPath>();
            _pointerMap = new ConcurrentDictionary<uint, List<uint>>();

            if (_params == null) return new List<PointerPath>();

            try
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW PARALLEL SCAN (SIMPLE MAP ALGORITHM) ---");

                // Phase 1: Build the pointer map.
                await BuildPointerMapAsync();
                if (cancellationToken.IsCancellationRequested) return _foundPaths.ToList();

                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] Pointer map built. Contains {_pointerMap.Count} unique pointer values. Starting path resolution for target 0x{_params.TargetAddress:X8}.");
                ReportProgress("Resolving paths...", 0, 1, 0);

                // Phase 2: Recursively search backwards from the target.
                await Task.Run(() =>
                {
                    ResolvePathsBackward(_params.TargetAddress, new List<int>(), 1);
                }, cancellationToken);

                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- PARALLEL SCAN COMPLETE: Found {_foundPaths.Count} paths. ---");
            }
            catch (OperationCanceledException)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN CANCELLED BY USER. Found {_foundPaths.Count} paths. ---");
            }
            finally
            {
                // Clean up to free memory.
                _pointerMap?.Clear();
                _pointerMap = null;
            }

            // Return whatever was found.
            return _foundPaths.ToList();
        }

        // The top level of the recursive search. It generates all possible starting points (offsets)
        // and processes them in parallel.
        private void ResolvePathsBackward(uint addressToFind, List<int> previousOffsets, int level)
        {
            _cancellationToken.ThrowIfCancellationRequested();
            if (_params == null || previousOffsets.Count >= _params.MaxLevel || _foundPaths.Count >= _params.MaxResults)
            {
                return;
            }

            var searchItems = new List<(uint valueToSearch, int offset)>();

            // Candidate 1: Direct pointer.
            searchItems.Add((addressToFind, 0));

            // Candidate 2: Positive offsets.
            for (int offset = 4; offset <= _params.MaxOffset; offset += 4)
            {
                searchItems.Add((addressToFind - (uint)offset, offset));
            }

            // Candidate 3: Negative offsets for finding structure bases.
            if (previousOffsets.Count == 0 && _params.ScanForStructureBase)
            {
                for (int offset = -4; offset >= -_params.MaxNegativeOffset; offset -= 4)
                {
                    searchItems.Add((addressToFind - (uint)offset, offset));
                }
            }

            // Process candidates in parallel.
            Parallel.ForEach(searchItems, new ParallelOptions { CancellationToken = _cancellationToken }, item =>
            {
                CheckPointerAndRecurse(item.valueToSearch, item.offset, previousOffsets, level);
            });
        }

        // The core recursive function. Checks the pointer map for a value and continues the search.
        private void CheckPointerAndRecurse(uint pointerValueToSearch, int offset, List<int> previousOffsets, int level)
        {
            if (!_pointerMap.TryGetValue(pointerValueToSearch, out var pointerSources))
            {
                return; // End of this branch.
            }

            // Create a new list for this branch to ensure thread safety.
            var newOffsets = new List<int>(previousOffsets);
            newOffsets.Add(offset);

            foreach (uint sourceAddress in pointerSources)
            {
                _cancellationToken.ThrowIfCancellationRequested();
                if (_foundPaths.Count >= _params.MaxResults) break;

                // If the source is a static address, we've found a full path.
                if (sourceAddress >= _params.StaticBaseStart && sourceAddress <= _params.StaticBaseEnd)
                {
                    var finalOffsets = new List<int>(newOffsets);
                    finalOffsets.Reverse(); // Reverse once at the end.
                    var newPath = new PointerPath { BaseAddress = sourceAddress, Offsets = finalOffsets, FinalAddress = _params.TargetAddress };

                    if (!_foundPaths.Any(p => p.BaseAddress == newPath.BaseAddress && p.GetOffsetsString() == newPath.GetOffsetsString()))
                    {
                        _foundPaths.Add(newPath);
                        ReportProgress(null, 0, 0, _foundPaths.Count);
                    }
                }
                else
                {
                    // It's a dynamic pointer, so recurse deeper.
                    ResolvePathsBackward(sourceAddress, newOffsets, level + 1);
                }
            }
        }

        // Reads through the emulator's memory in parallel to build the pointer map.
        private async Task BuildPointerMapAsync()
        {
            long totalSize = DuckStationManager.PS1_RAM_SIZE;
            long processedSize = 0;
            int chunkSize = 65536;

            if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] Building pointer map by scanning {totalSize / 1024}KB of PS1 RAM.");

            var chunksToProcess = new List<(uint, int)>();
            for (uint addr = DuckStationManager.PS1_RAM_START; addr < DuckStationManager.PS1_RAM_END; addr += (uint)chunkSize)
            {
                chunksToProcess.Add((addr, (int)Math.Min(chunkSize, DuckStationManager.PS1_RAM_END - addr)));
            }

            await Task.Run(() => Parallel.ForEach(chunksToProcess, new ParallelOptions { CancellationToken = _cancellationToken }, (chunkInfo) =>
            {
                _cancellationToken.ThrowIfCancellationRequested();
                var (addr, size) = chunkInfo;
                byte[] chunk = _manager.ReadMemory(addr, size);
                if (chunk == null) return;

                for (int i = 0; i + 3 < chunk.Length; i += 4)
                {
                    uint value = BitConverter.ToUInt32(chunk, i);
                    if (_manager.IsValidPointerTarget(value))
                    {
                        uint pointerAddress = addr + (uint)i;
                        var list = _pointerMap.GetOrAdd(value, (v) => new List<uint>());
                        lock (list)
                        {
                            list.Add(pointerAddress);
                        }
                    }
                }
                Interlocked.Add(ref processedSize, size);
                ReportProgress($"Building pointer map... {((double)processedSize / totalSize):P0}", processedSize, totalSize, 0);
            }), _cancellationToken);
        }

        // Helper method to report progress to the UI.
        private void ReportProgress(string message, long current, long max, int found)
        {
            var report = new ScanProgressReport
            {
                FoundCount = found,
                StatusMessage = message
            };

            if (max > 0)
            {
                report.CurrentValue = current;
                report.MaxValue = max;
            }
            _progress?.Report(report);
        }
    }
}