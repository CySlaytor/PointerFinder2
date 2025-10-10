using PointerFinder2.DataModels;
using PointerFinder2.Emulators.EmulatorManager;
// Added using statement for the new LiveScannerStrategyBase.
using PointerFinder2.Emulators.LiveScan;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.LiveScan.RALibretro
{
    public class RALibretroNDSScannerStrategy : LiveScannerStrategyBase
    {
        // Scans both static memory and main RAM in parallel to build the pointer map.
        protected override Task BuildPointerMapAsync()
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

        // NDS has no address mirroring, so this is a simple lookup.
        protected override IEnumerable<uint> FindSourcesForValue(uint value)
        {
            if (_pointerMap.TryGetValue(value, out var sources))
            {
                return sources;
            }
            return Enumerable.Empty<uint>();
        }
    }
}