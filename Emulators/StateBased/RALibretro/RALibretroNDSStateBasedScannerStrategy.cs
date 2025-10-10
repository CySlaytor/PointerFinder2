using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.StateBased.RALibretro
{
    public class RALibretroNDSStateBasedScannerStrategy : StateBasedScannerStrategyBase
    {
        // Scans the 4MB NDS RAM dump to build the pointer map.
        protected override Task BuildPointerMapAsync(ScanState state, CancellationToken token)
        {
            _pointerMap = new Dictionary<uint, List<uint>>();
            ReportProgress("Building map for State 1...", 0, 1, 0);

            return Task.Run(() =>
            {
                byte[] memory = state.MemoryDump;
                for (int addr = 0; addr < memory.Length; addr += 4)
                {
                    if (token.IsCancellationRequested) break;
                    uint value = BitConverter.ToUInt32(memory, addr);
                    if (_manager.IsValidPointerTarget(value))
                    {
                        if (!_pointerMap.TryGetValue(value, out var list))
                        {
                            list = new List<uint>();
                            _pointerMap[value] = list;
                        }
                        list.Add(_manager.MainMemoryStart + (uint)addr);
                    }
                }
                ReportProgress("Building map for State 1... Complete", 1, 1, 0);
            }, token);
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