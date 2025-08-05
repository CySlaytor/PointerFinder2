using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.RALibretro
{
    // Implements the pointer scanning strategy specifically for NDS cores on RALibretro.
    // This strategy scans the main RAM and static memory regions using a high-performance parallel algorithm.
    public class RALibretroNDSScannerStrategy : IPointerScannerStrategy
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

                // Phase 1: Scan all relevant memory regions to build a map of what points to where.
                await BuildPointerMapAsync();
                if (cancellationToken.IsCancellationRequested) return _foundPaths.ToList();
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] Pointer map built with {_pointerMap.Count} unique pointers. Resolving paths...");

                // Phase 2: Work backwards from the target address using a hybrid parallel DFS.
                ReportProgress("Resolving paths...", 0, 1, 0);
                await Task.Run(() => ParallelResolveLauncher(), cancellationToken);

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

        private void ParallelResolveLauncher()
        {
            var workItems = new List<(uint startAddress, List<int> initialOffsets)>();

            var searchItems = new List<(uint valueToSearch, int offset)>();
            searchItems.Add((_params.TargetAddress, 0));
            for (int offset = 4; offset <= _params.MaxOffset; offset += 4)
            {
                searchItems.Add((_params.TargetAddress - (uint)offset, offset));
            }
            if (_params.ScanForStructureBase)
            {
                for (int offset = -4; offset >= -_params.MaxNegativeOffset; offset -= 4)
                {
                    searchItems.Add((_params.TargetAddress - (uint)offset, offset));
                }
            }

            foreach (var item in searchItems)
            {
                if (_pointerMap.TryGetValue(item.valueToSearch, out var sources))
                {
                    foreach (var source in sources)
                    {
                        workItems.Add((source, new List<int> { item.offset }));
                    }
                }
            }

            var parallelOptions = new ParallelOptions { CancellationToken = _cancellationToken };
            if (_params.LimitCpuUsage)
            {
                int coreCount = Math.Max(1, Environment.ProcessorCount / 2);
                parallelOptions.MaxDegreeOfParallelism = coreCount;
            }

            Parallel.ForEach(workItems, parallelOptions, item =>
            {
                if (_cancellationToken.IsCancellationRequested || _foundPathsCounter >= _params.MaxResults) return;

                if (item.startAddress >= _params.StaticBaseStart && item.startAddress <= _params.StaticBaseEnd)
                {
                    var finalOffsets = new List<int>(item.initialOffsets);
                    finalOffsets.Reverse();
                    var newPath = new PointerPath { BaseAddress = item.startAddress, Offsets = finalOffsets, FinalAddress = _params.TargetAddress };
                    _foundPaths.Add(newPath);
                    Interlocked.Increment(ref _foundPathsCounter);
                }
                else
                {
                    ResolvePathsBackward(item.startAddress, item.initialOffsets, 2);
                }
            });
        }

        private void ResolvePathsBackward(uint addressToFind, List<int> currentOffsets, int level)
        {
            if (_cancellationToken.IsCancellationRequested || currentOffsets.Count >= _params.MaxLevel || _foundPathsCounter >= _params.MaxResults)
            {
                return;
            }

            var searchItems = new List<(uint valueToSearch, int offset)>();
            searchItems.Add((addressToFind, 0));
            for (int offset = 4; offset <= _params.MaxOffset; offset += 4)
            {
                searchItems.Add((addressToFind - (uint)offset, offset));
            }

            foreach (var item in searchItems)
            {
                if (_cancellationToken.IsCancellationRequested) return;

                if (_pointerMap.TryGetValue(item.valueToSearch, out var sources))
                {
                    currentOffsets.Add(item.offset);
                    foreach (uint sourceAddress in sources)
                    {
                        if (_foundPathsCounter >= _params.MaxResults) break;

                        if (sourceAddress >= _params.StaticBaseStart && sourceAddress <= _params.StaticBaseEnd)
                        {
                            var finalOffsets = new List<int>(currentOffsets);
                            finalOffsets.Reverse();
                            var newPath = new PointerPath { BaseAddress = sourceAddress, Offsets = finalOffsets, FinalAddress = _params.TargetAddress };
                            _foundPaths.Add(newPath);
                            long currentCount = Interlocked.Increment(ref _foundPathsCounter);
                            if (currentCount % 1000 == 0)
                            {
                                ReportProgress(null, 0, 0, (int)currentCount);
                            }
                        }
                        else
                        {
                            ResolvePathsBackward(sourceAddress, currentOffsets, level + 1);
                        }
                    }
                    currentOffsets.RemoveAt(currentOffsets.Count - 1);
                }
            }
        }

        // Scans both static memory and main RAM in parallel to build the pointer map.
        private async Task BuildPointerMapAsync()
        {
            long totalSize = (RALibretroNDSManager.NDS_RAM_END - RALibretroNDSManager.NDS_RAM_START) +
                             (RALibretroNDSManager.NDS_STATIC_END - RALibretroNDSManager.NDS_STATIC_START);
            long processedSize = 0;
            int chunkSize = 65536; // 64K chunks.

            var allChunks = new List<(uint, int)>();
            var regions = new[]
            {
                new { Start = RALibretroNDSManager.NDS_RAM_START, End = RALibretroNDSManager.NDS_RAM_END },
                new { Start = RALibretroNDSManager.NDS_STATIC_START, End = RALibretroNDSManager.NDS_STATIC_END }
            };

            foreach (var region in regions)
            {
                for (uint addr = region.Start; addr < region.End; addr += (uint)chunkSize)
                {
                    allChunks.Add((addr, (int)Math.Min(chunkSize, region.End - addr)));
                }
            }

            await Task.Run(() =>
            {
                var parallelOptions = new ParallelOptions { CancellationToken = _cancellationToken };
                if (_params.LimitCpuUsage)
                {
                    int coreCount = Math.Max(1, Environment.ProcessorCount / 2);
                    parallelOptions.MaxDegreeOfParallelism = coreCount;
                }

                Parallel.ForEach(allChunks,
                    parallelOptions,
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