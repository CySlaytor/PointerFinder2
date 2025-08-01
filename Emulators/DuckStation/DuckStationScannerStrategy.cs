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
    // This strategy scans the single 2MB PS1 RAM region using a high-performance parallel algorithm.
    public class DuckStationScannerStrategy : IPointerScannerStrategy
    {
        private ScanParameters _params;
        private IEmulatorManager _manager;
        private ConcurrentBag<PointerPath> _foundPaths;
        private IProgress<ScanProgressReport> _progress;
        private CancellationToken _cancellationToken;
        private ConcurrentDictionary<uint, List<uint>> _pointerMap;
        private readonly DebugLogForm logger = DebugLogForm.Instance;
        private long _foundPathsCounter = 0;

        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new ConcurrentBag<PointerPath>();
            _pointerMap = new ConcurrentDictionary<uint, List<uint>>(Environment.ProcessorCount, 50000);
            _foundPathsCounter = 0;

            if (_params == null) return new List<PointerPath>();

            try
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW SCAN ---");

                // Phase 1: Scan all of RAM to build a map of what points to where.
                await BuildPointerMapAsync();
                if (cancellationToken.IsCancellationRequested) return _foundPaths.ToList();
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] Pointer map built with {_pointerMap.Count} unique pointers. Resolving paths...");

                // Phase 2: Work backwards from the target address to find valid chains.
                ReportProgress("Resolving paths...", 0, 1, 0);
                await Task.Run(() => ResolvePathsBackward(_params.TargetAddress, new List<int>(), 1), cancellationToken);

                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN COMPLETE: Found {_foundPaths.Count} paths. ---");
            }
            catch (OperationCanceledException)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN CANCELLED. Found {_foundPaths.Count} paths. ---");
            }
            finally
            {
                _pointerMap?.Clear();
                _pointerMap = null;
            }

            return _foundPaths.ToList();
        }

        // Recursively searches backwards for pointers that could lead to the target address.
        private void ResolvePathsBackward(uint addressToFind, List<int> previousOffsets, int level)
        {
            if (_cancellationToken.IsCancellationRequested || _params == null || previousOffsets.Count >= _params.MaxLevel || _foundPathsCounter >= _params.MaxResults)
            {
                return;
            }

            // Create a list of all potential pointer values we need to look for.
            // This includes the address itself (for an offset of 0) and all addresses
            // that could point to it with a positive offset.
            var searchItems = new List<(uint valueToSearch, int offset)>();
            searchItems.Add((addressToFind, 0));

            for (int offset = 4; offset <= _params.MaxOffset; offset += 4)
            {
                searchItems.Add((addressToFind - (uint)offset, offset));
            }

            // On the very first level, also search for pointers with negative offsets.
            // This is key to finding the *base* of a structure, not just a member inside it.
            if (previousOffsets.Count == 0 && _params.ScanForStructureBase)
            {
                for (int offset = -4; offset >= -_params.MaxNegativeOffset; offset -= 4)
                {
                    searchItems.Add((addressToFind - (uint)offset, offset));
                }
            }

            // This loop is intentionally serial. The work items are too small and numerous
            // for parallelization to be efficient here; it would cause CPU thrashing.
            // The entire ResolvePathsBackward method is already running on a background thread via Task.Run.
            foreach (var item in searchItems)
            {
                if (_cancellationToken.IsCancellationRequested) return;
                CheckPointerAndRecurse(item.valueToSearch, item.offset, previousOffsets, level);
            }
        }

        // Checks if a given pointer value exists in our map and, if so, continues the search.
        private void CheckPointerAndRecurse(uint pointerValueToSearch, int offset, List<int> previousOffsets, int level)
        {
            if (!_pointerMap.TryGetValue(pointerValueToSearch, out var sources)) return;

            var newOffsets = new List<int>(previousOffsets) { offset };

            foreach (uint sourceAddress in sources)
            {
                if (_cancellationToken.IsCancellationRequested || _foundPathsCounter >= _params.MaxResults) break;

                // Check if we've hit a static base address. If so, we found a complete path.
                if (sourceAddress >= _params.StaticBaseStart && sourceAddress <= _params.StaticBaseEnd)
                {
                    var finalOffsets = new List<int>(newOffsets);
                    finalOffsets.Reverse();
                    var newPath = new PointerPath { BaseAddress = sourceAddress, Offsets = finalOffsets, FinalAddress = _params.TargetAddress };

                    _foundPaths.Add(newPath);
                    long currentCount = Interlocked.Increment(ref _foundPathsCounter);

                    // Throttle UI updates to prevent flooding the message queue.
                    if (currentCount % 100 == 0 || currentCount == _params.MaxResults)
                    {
                        ReportProgress(null, 0, 0, (int)currentCount);
                    }
                }
                else
                {
                    // If it's not a static address, continue searching backwards from this new address.
                    ResolvePathsBackward(sourceAddress, newOffsets, level + 1);
                }
            }
        }

        // Scans the entire PS1 RAM in parallel chunks to build a map of {Value -> [AddressOfValue]}.
        // This pre-computation makes the path resolution phase much faster.
        private async Task BuildPointerMapAsync()
        {
            long totalSize = DuckStationManager.PS1_RAM_SIZE;
            long processedSize = 0;
            int chunkSize = 65536; // 64K chunks.

            // Prepare a list of all memory chunks we need to read.
            var allChunks = new List<(uint, int)>();
            for (uint addr = DuckStationManager.PS1_RAM_START; addr < DuckStationManager.PS1_RAM_END; addr += (uint)chunkSize)
            {
                allChunks.Add((addr, (int)Math.Min(chunkSize, DuckStationManager.PS1_RAM_END - addr)));
            }

            // Process the chunks in parallel for maximum speed.
            await Task.Run(() =>
            {
                Parallel.ForEach(allChunks,
                    new ParallelOptions { CancellationToken = _cancellationToken },
                    // Each thread gets its own private, local map to avoid locking.
                    () => new Dictionary<uint, List<uint>>(),
                    (chunkInfo, loopState, localMap) =>
                    {
                        var (addr, size) = chunkInfo;
                        byte[] chunk = _manager.ReadMemory(addr, size);
                        if (chunk != null)
                        {
                            for (int i = 0; i + 3 < chunk.Length; i += 4)
                            {
                                uint value = BitConverter.ToUInt32(chunk, i);
                                if (_manager.IsValidPointerTarget(value))
                                {
                                    if (!localMap.TryGetValue(value, out var list))
                                    {
                                        list = new List<uint>();
                                        localMap[value] = list;
                                    }
                                    list.Add(addr + (uint)i);
                                }
                            }
                        }
                        long currentProcessed = Interlocked.Add(ref processedSize, size);
                        // Report progress periodically to the UI.
                        if (currentProcessed % (chunkSize * 16) == 0)
                        {
                            ReportProgress($"Building pointer map... {((double)currentProcessed / totalSize):P0}", currentProcessed, totalSize, 0);
                        }
                        return localMap;
                    },
                    // After a thread finishes its chunks, merge its local map into the main concurrent dictionary.
                    (finalLocalMap) =>
                    {
                        foreach (var kvp in finalLocalMap)
                        {
                            var list = _pointerMap.GetOrAdd(kvp.Key, _ => new List<uint>());
                            lock (list) // Lock only the small list, not the whole dictionary.
                            {
                                list.AddRange(kvp.Value);
                            }
                        }
                    }
                );
            }, _cancellationToken);
            ReportProgress($"Building pointer map... 100%", totalSize, totalSize, 0);
        }

        // Helper to send progress updates to the UI thread.
        private void ReportProgress(string message, long current, long max, int found)
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