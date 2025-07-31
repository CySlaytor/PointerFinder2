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
    // This strategy scans the single PS1 RAM region using a high-performance parallel algorithm.
    public class DuckStationScannerStrategy : IPointerScannerStrategy
    {
        private ScanParameters _params;
        private IEmulatorManager _manager;
        private ConcurrentBag<PointerPath> _foundPaths;
        private IProgress<ScanProgressReport> _progress;
        private CancellationToken _cancellationToken;
        // The pointer map uses a List<uint> for its value, populated by merging thread-local results.
        private ConcurrentDictionary<uint, List<uint>> _pointerMap;
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new ConcurrentBag<PointerPath>();
            _pointerMap = new ConcurrentDictionary<uint, List<uint>>(Environment.ProcessorCount, 50000);

            if (_params == null) return new List<PointerPath>();

            try
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW SCAN (STABLE MULTI-CORE ALGORITHM) ---");

                await BuildPointerMapAsync();
                if (cancellationToken.IsCancellationRequested) return _foundPaths.ToList();

                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] Pointer map built. Contains {_pointerMap.Count} unique pointer values. Starting path resolution for target 0x{_params.TargetAddress:X8}.");
                ReportProgress("Resolving paths...", 0, 1, 0);

                await Task.Run(() => ResolvePathsBackward(_params.TargetAddress, new List<int>(), 1), cancellationToken);

                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN COMPLETE: Found {_foundPaths.Count} paths. ---");
            }
            catch (OperationCanceledException)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN CANCELLED BY USER. Found {_foundPaths.Count} paths. ---");
            }
            finally
            {
                _pointerMap?.Clear();
                _pointerMap = null;
            }

            return _foundPaths.ToList();
        }

        private void ResolvePathsBackward(uint addressToFind, List<int> previousOffsets, int level)
        {
            if (_cancellationToken.IsCancellationRequested || _params == null || previousOffsets.Count >= _params.MaxLevel || _foundPaths.Count >= _params.MaxResults)
            {
                return;
            }

            var searchItems = new List<(uint valueToSearch, int offset)>();
            searchItems.Add((addressToFind, 0));

            for (int offset = 4; offset <= _params.MaxOffset; offset += 4)
            {
                searchItems.Add((addressToFind - (uint)offset, offset));
            }

            if (previousOffsets.Count == 0 && _params.ScanForStructureBase)
            {
                for (int offset = -4; offset >= -_params.MaxNegativeOffset; offset -= 4)
                {
                    searchItems.Add((addressToFind - (uint)offset, offset));
                }
            }

            Parallel.ForEach(searchItems, new ParallelOptions { CancellationToken = _cancellationToken }, item =>
            {
                CheckPointerAndRecurse(item.valueToSearch, item.offset, previousOffsets, level);
            });
        }

        private void CheckPointerAndRecurse(uint pointerValueToSearch, int offset, List<int> previousOffsets, int level)
        {
            if (!_pointerMap.TryGetValue(pointerValueToSearch, out var sources)) return;

            var newOffsets = new List<int>(previousOffsets) { offset };

            foreach (uint sourceAddress in sources)
            {
                if (_cancellationToken.IsCancellationRequested || _foundPaths.Count >= _params.MaxResults) break;

                if (sourceAddress >= _params.StaticBaseStart && sourceAddress <= _params.StaticBaseEnd)
                {
                    var finalOffsets = new List<int>(newOffsets);
                    finalOffsets.Reverse();
                    var newPath = new PointerPath { BaseAddress = sourceAddress, Offsets = finalOffsets, FinalAddress = _params.TargetAddress };

                    _foundPaths.Add(newPath);
                    ReportProgress(null, 0, 0, _foundPaths.Count);
                }
                else
                {
                    ResolvePathsBackward(sourceAddress, newOffsets, level + 1);
                }
            }
        }

        private async Task BuildPointerMapAsync()
        {
            long totalSize = DuckStationManager.PS1_RAM_SIZE;
            long processedSize = 0;
            int chunkSize = 65536;

            var allChunks = new List<(uint, int)>();
            for (uint addr = DuckStationManager.PS1_RAM_START; addr < DuckStationManager.PS1_RAM_END; addr += (uint)chunkSize)
            {
                allChunks.Add((addr, (int)Math.Min(chunkSize, DuckStationManager.PS1_RAM_END - addr)));
            }

            await Task.Run(() =>
            {
                Parallel.ForEach(allChunks,
                    new ParallelOptions { CancellationToken = _cancellationToken },
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
                        if (currentProcessed % (chunkSize * 16) == 0)
                        {
                            ReportProgress($"Building pointer map... {((double)currentProcessed / totalSize):P0}", currentProcessed, totalSize, 0);
                        }
                        return localMap;
                    },
                    (finalLocalMap) =>
                    {
                        foreach (var kvp in finalLocalMap)
                        {
                            var list = _pointerMap.GetOrAdd(kvp.Key, _ => new List<uint>());
                            lock (list)
                            {
                                list.AddRange(kvp.Value);
                            }
                        }
                    }
                );
            }, _cancellationToken);
            ReportProgress($"Building pointer map... 100%", totalSize, totalSize, 0);
        }

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