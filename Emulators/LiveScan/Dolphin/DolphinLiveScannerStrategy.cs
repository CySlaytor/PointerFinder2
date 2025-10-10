using PointerFinder2.DataModels;
using PointerFinder2.Emulators.EmulatorManager;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.LiveScan.Dolphin
{
    // Added new live scan strategy for Dolphin.
    public class DolphinLiveScannerStrategy : LiveScannerStrategyBase
    {
        protected override Task BuildPointerMapAsync()
        {
            var regionsToScan = new List<(uint start, uint end, string name)>();

            // Always scan MEM1
            regionsToScan.Add((DolphinManager.MEM1_INGAME_BASE, DolphinManager.MEM1_INGAME_END, "MEM1"));

            // If the manager detected Wii, also scan MEM2
            if (_manager.EmulatorName.Contains("Wii"))
            {
                regionsToScan.Add((DolphinManager.MEM2_INGAME_BASE, DolphinManager.MEM2_INGAME_END, "MEM2"));
            }

            long totalSize = regionsToScan.Sum(r => (long)r.end - r.start);
            long processedSize = 0;
            int chunkSize = 65536; // 64K chunks.

            var allChunks = new List<(uint, int)>();
            foreach (var region in regionsToScan)
            {
                for (uint addr = region.start; addr < region.end; addr += (uint)chunkSize)
                {
                    allChunks.Add((addr, (int)Math.Min(chunkSize, region.end - addr)));
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
                    Parallel.ForEach(allChunks, parallelOptions,
                        () => new Dictionary<uint, List<uint>>(),
                        (chunkInfo, loopState, localMap) =>
                        {
                            var (addr, size) = chunkInfo;
                            byte[] chunk = _manager.ReadMemory(addr, size);
                            if (chunk != null)
                            {
                                for (int i = 0; i + 3 < chunk.Length; i += 4)
                                {
                                    byte[] valueBytes = new byte[4];
                                    Buffer.BlockCopy(chunk, i, valueBytes, 0, 4);
                                    Array.Reverse(valueBytes); // Handle Big-Endian
                                    uint value = BitConverter.ToUInt32(valueBytes, 0);

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
                        });
                }
                catch (OperationCanceledException) { /* Absorb */ }
            }, _cancellationToken);
        }

        // Dolphin has no address mirroring, so this is a simple lookup.
        protected override IEnumerable<uint> FindSourcesForValue(uint value)
        {
            return _pointerMap.TryGetValue(value, out var sources) ? sources : Enumerable.Empty<uint>();
        }
    }
}