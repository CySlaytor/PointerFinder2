using PointerFinder2.DataModels;
using PointerFinder2.Emulators.EmulatorManager;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.StateBased.PCSX2
{
    public class Pcsx2StateBasedScannerStrategy : StateBasedScannerStrategyBase
    {
        // Scans the PCSX2 memory dump to build the pointer map.
        // NOTE: The memory dump captured by StateCaptureForm is ONLY the 32MB EE RAM,
        // so this method inherently only scans that region.
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

                    // We only care about values that point to valid game regions (EE RAM or Game Code).
                    if (_manager.IsValidPointerTarget(value))
                    {
                        // The address of the pointer we just read is its offset in the dump
                        // plus the start of EE RAM.
                        uint currentAddress = _manager.MainMemoryStart + (uint)addr;

                        // The old code had a redundant check for the Game Code region.
                        // Since the memory dump is only EE RAM, that check was unnecessary and confusing.
                        // This simplified logic correctly builds the map from the EE RAM dump.
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

        // Handles PCSX2's specific address mirroring (0x0... vs 0x20...).
        protected override IEnumerable<uint> FindSourcesForValue(uint value)
        {
            var sourcesToAdd = new HashSet<uint>();
            if (_pointerMap.TryGetValue(value, out var fullSources))
            {
                foreach (var s in fullSources) sourcesToAdd.Add(s);
            }

            if (value >= Pcsx2Manager.PS2_EEMEM_START)
            {
                uint shortValue = value - Pcsx2Manager.PS2_EEMEM_START;
                if (_pointerMap.TryGetValue(shortValue, out var shortSources))
                {
                    foreach (var s in shortSources) sourcesToAdd.Add(s);
                }
            }
            return sourcesToAdd;
        }
    }
}