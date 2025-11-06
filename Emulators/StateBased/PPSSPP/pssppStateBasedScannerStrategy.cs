using PointerFinder2.DataModels;
using PointerFinder2.Emulators.StateBased;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.StateBased.PPSSPP
{
    // Fix: Added new state-based scan strategy for PPSSPP.
    public class PpssppStateBasedScannerStrategy : StateBasedScannerStrategyBase
    {
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

        protected override IEnumerable<uint> FindSourcesForValue(uint value)
        {
            return _pointerMap.TryGetValue(value, out var sources) ? sources : Enumerable.Empty<uint>();
        }
    }
}