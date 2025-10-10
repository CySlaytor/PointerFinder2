using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators.EmulatorManager;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

// Changed namespace to align with project structure and resolve compilation errors.
namespace PointerFinder2.Emulators.LiveScan.PCSX2
{
    // Implements the pointer scanning strategy specifically for the PCSX2 emulator.
    // This strategy is "intelligent" as it considers multiple memory regions (EE RAM, Game Code)
    // and handles PCSX2's specific memory address formats using a high-performance parallel algorithm.
    public class Pcsx2ScannerStrategy : IPointerScannerStrategy
    {
        private ScanParameters _params;
        private IEmulatorManager _manager;
        private ConcurrentBag<PointerPath> _foundPaths;
        private IProgress<ScanProgressReport> _progress;
        private CancellationToken _cancellationToken;
        private ConcurrentDictionary<uint, List<uint>> _intelligentPointerMap;
        private readonly DebugLogForm logger = DebugLogForm.Instance;
        private long _foundPathsCounter = 0;

        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new ConcurrentBag<PointerPath>();
            _intelligentPointerMap = new ConcurrentDictionary<uint, List<uint>>(Environment.ProcessorCount, 100000);
            _foundPathsCounter = 0;

            if (_params == null) return new List<PointerPath>();

            try
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW SCAN ---");

                // Phase 1: Scan all relevant memory regions to build a map of what points to where.
                await BuildIntelligentPointerMapAsync();
                if (cancellationToken.IsCancellationRequested) return _foundPaths.ToList();
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] Pointer map built with {_intelligentPointerMap.Count:N0} unique pointers. Resolving paths...");

                // Phase 2: Work backwards from the target address using a hybrid parallel DFS.
                ReportProgress("Resolving paths...", 0, 1, 0);
                await Task.Run(() => ParallelResolveLauncher(), cancellationToken);

                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN COMPLETE: Found {_foundPaths.Count:N0} paths. ---");
            }
            catch (OperationCanceledException)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN CANCELLED. Found {_foundPaths.Count:N0} paths. ---");
            }
            finally
            {
                _intelligentPointerMap?.Clear();
                _intelligentPointerMap = null;
            }

            return _foundPaths.ToList();
        }

        // HYBRID PARALLEL DFS LAUNCHER: This method performs the first level of the search
        // and then launches the memory-efficient recursive DFS for each sub-problem in parallel.
        private void ParallelResolveLauncher()
        {
            var workItems = new List<(uint startAddress, List<int> initialOffsets)>();

            // --- Step 1: Find all Level 1 pointers ---
            var searchItems = new List<(uint valueToSearch, int offset)>();
            searchItems.Add((_params.TargetAddress, 0));
            int step = _params.Use16ByteAlignment ? 16 : 4;
            uint startAddress = _params.Use16ByteAlignment ? _params.TargetAddress & 0xFFFFFFF0 : _params.TargetAddress;
            for (uint currentAddress = startAddress; currentAddress > 4096 && currentAddress >= _params.TargetAddress - _params.MaxOffset; currentAddress -= (uint)step)
            {
                int offset = (int)(_params.TargetAddress - currentAddress);
                if (offset == 0) continue;
                searchItems.Add((currentAddress, offset));
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
                if (!_intelligentPointerMap.TryGetValue(item.valueToSearch, out var sources) &&
                    !(item.valueToSearch >= Pcsx2Manager.PS2_EEMEM_START && _intelligentPointerMap.TryGetValue(item.valueToSearch - Pcsx2Manager.PS2_EEMEM_START, out sources)))
                {
                    continue;
                }
                foreach (var source in sources)
                {
                    workItems.Add((source, new List<int> { item.offset }));
                }
            }

            // --- Step 2: Process work items in parallel ---
            var parallelOptions = new ParallelOptions { CancellationToken = _cancellationToken };
            if (_params.LimitCpuUsage)
            {
                int coreCount = Math.Max(1, Environment.ProcessorCount / 2);
                parallelOptions.MaxDegreeOfParallelism = coreCount;
            }

            // Wrap the parallel loop in a try-catch block to gracefully handle cancellation.
            // This prevents the debugger from breaking on an "unhandled" OperationCanceledException
            // when the user stops the scan.
            try
            {
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
            catch (OperationCanceledException)
            {
                // This is expected when the scan is cancelled by the user. Absorb it.
            }
        }

        // Memory-efficient recursive Depth-First Search. Called in parallel by the launcher.
        private void ResolvePathsBackward(uint addressToFind, List<int> currentOffsets, int level)
        {
            if (_cancellationToken.IsCancellationRequested || currentOffsets.Count >= _params.MaxLevel || _foundPathsCounter >= _params.MaxResults)
            {
                return;
            }

            var searchItems = new List<(uint valueToSearch, int offset)>();
            searchItems.Add((addressToFind, 0));
            int step = _params.Use16ByteAlignment ? 16 : 4;
            uint startAddress = _params.Use16ByteAlignment ? addressToFind & 0xFFFFFFF0 : addressToFind;
            for (uint currentAddress = startAddress; currentAddress > 4096 && currentAddress >= addressToFind - _params.MaxOffset; currentAddress -= (uint)step)
            {
                int offset = (int)(addressToFind - currentAddress);
                if (offset == 0) continue;
                searchItems.Add((currentAddress, offset));
            }

            foreach (var item in searchItems)
            {
                if (_cancellationToken.IsCancellationRequested) return;

                if (!_intelligentPointerMap.TryGetValue(item.valueToSearch, out var sources) &&
                    !(item.valueToSearch >= Pcsx2Manager.PS2_EEMEM_START && _intelligentPointerMap.TryGetValue(item.valueToSearch - Pcsx2Manager.PS2_EEMEM_START, out sources)))
                {
                    continue;
                }

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

        // Scans PS2 EE RAM to build the pointer map. The Game Code region is intentionally
        // excluded to maintain fast scan performance.
        private async Task BuildIntelligentPointerMapAsync()
        {
            // Reverted to only scan the EE RAM region to ensure fast, focused scans.
            // The more thorough "Game Code" scan was removed based on user feedback.
            var regionsToScan = new List<(uint Start, uint End)>
            {
                (Pcsx2Manager.PS2_EEMEM_START, Pcsx2Manager.PS2_EEMEM_END)
            };

            long totalSize = regionsToScan.Sum(r => (long)r.End - r.Start);
            long processedSize = 0;
            int chunkSize = 131072; // 128K chunks.

            var allChunks = new List<(uint, int)>();
            foreach (var region in regionsToScan)
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

                try
                {
                    Parallel.ForEach(
                        allChunks,
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
                                var list = _intelligentPointerMap.GetOrAdd(kvp.Key, _ => new List<uint>());
                                lock (list)
                                {
                                    list.AddRange(kvp.Value);
                                }
                            }
                        }
                    );
                }
                catch (OperationCanceledException)
                {
                    // This is expected when the scan is cancelled by the user. Absorb it.
                }
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