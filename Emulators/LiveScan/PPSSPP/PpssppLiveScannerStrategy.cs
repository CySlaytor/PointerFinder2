using PointerFinder2.DataModels;
using PointerFinder2.Emulators.LiveScan;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.LiveScan.PPSSPP
{
    // Fix: Added new live scan strategy for PPSSPP.
    public class PpssppLiveScannerStrategy : LiveScannerStrategyBase
    {
        protected override Task BuildPointerMapAsync()
        {
            long totalSize = _manager.MainMemorySize;
            long processedSize = 0;
            int chunkSize = 65536; // 64K chunks.

            var allChunks = new List<(uint, int)>();
            for (uint addr = _manager.MainMemoryStart; addr < (_manager.MainMemoryStart + _manager.MainMemorySize); addr += (uint)chunkSize)
            {
                allChunks.Add((addr, (int)Math.Min(chunkSize, (_manager.MainMemoryStart + _manager.MainMemorySize) - addr)));
            }

            return Task.Run(() =>
            {
                var parallelOptions = new ParallelOptions { CancellationToken = _cancellationToken };
                if (_params.LimitCpuUsage)
                {
                    parallelOptions.MaxDegreeOfParallelism = Math.Max(1, Environment.ProcessorCount / 2);
                }

                try
                {
                    Parallel.ForEach(allChunks,
                        parallelOptions,
                        () => new Dictionary<uint, List<uint>>(),
                        (chunkInfo, loopState, localMap) =>
                        {
                            if (loopState.ShouldExitCurrentIteration || _cancellationToken.IsCancellationRequested)
                            {
                                loopState.Stop();
                                return localMap;
                            }

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
                }
                catch (OperationCanceledException) { /* Absorb */ }
            }, _cancellationToken);
        }

        protected override IEnumerable<uint> FindSourcesForValue(uint value)
        {
            return _pointerMap.TryGetValue(value, out var sources) ? sources : Enumerable.Empty<uint>();
        }
    }
}