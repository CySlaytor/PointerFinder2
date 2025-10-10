using PointerFinder2.DataModels;
using PointerFinder2.Emulators.EmulatorManager;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.StateBased.Dolphin
{
    // Added new state-based scan strategy for Dolphin.
    public class DolphinStateBasedScannerStrategy : StateBasedScannerStrategyBase
    {
        protected override Task BuildPointerMapAsync(ScanState state, CancellationToken token)
        {
            _pointerMap = new Dictionary<uint, List<uint>>();
            ReportProgress("Building map for State 1...", 0, 1, 0);

            return Task.Run(() =>
            {
                byte[] memory = state.MemoryDump;
                bool isWii = memory.Length > DolphinManager.MEM1_SIZE;

                for (int addr = 0; addr < memory.Length; addr += 4)
                {
                    if (token.IsCancellationRequested) break;

                    byte[] valueBytes = new byte[4];
                    Buffer.BlockCopy(memory, addr, valueBytes, 0, 4);
                    Array.Reverse(valueBytes); // Handle Big-Endian
                    uint value = BitConverter.ToUInt32(valueBytes, 0);

                    if (_manager.IsValidPointerTarget(value))
                    {
                        uint currentAddress;
                        if (addr < DolphinManager.MEM1_SIZE)
                        {
                            currentAddress = DolphinManager.MEM1_INGAME_BASE + (uint)addr;
                        }
                        else // This code path only runs for Wii dumps
                        {
                            currentAddress = DolphinManager.MEM2_INGAME_BASE + ((uint)addr - DolphinManager.MEM1_SIZE);
                        }

                        if (!_pointerMap.TryGetValue(value, out var list))
                        {
                            list = new List<uint>();
                            _pointerMap[value] = list;
                        }
                        list.Add(currentAddress);
                    }
                }
                ReportProgress("Building map for State 1... Complete", 1, 1, 0);
            }, token);
        }

        // Dolphin has no address mirroring, so this is a simple lookup.
        protected override IEnumerable<uint> FindSourcesForValue(uint value)
        {
            return _pointerMap.TryGetValue(value, out var sources) ? sources : Enumerable.Empty<uint>();
        }
    }
}