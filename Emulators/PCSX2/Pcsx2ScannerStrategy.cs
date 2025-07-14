using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators.PCSX2; // Corrected using
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.PCSX2
{
    public class Pcsx2ScannerStrategy : IPointerScannerStrategy
    {
        private ScanParameters _params;
        private IEmulatorManager _manager;
        private List<PointerPath> _foundPaths;
        private IProgress<ScanProgressReport> _progress;
        private CancellationToken _cancellationToken;
        private Dictionary<uint, List<uint>> _intelligentPointerMap;
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new List<PointerPath>();
            _intelligentPointerMap = new Dictionary<uint, List<uint>>();

            if (_params == null) return new List<PointerPath>();

            logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW LIVE SCAN (INTELLIGENT MAP ALGORITHM) ---");

            await BuildIntelligentPointerMapAsync();
            if (_cancellationToken.IsCancellationRequested) return new List<PointerPath>();

            logger.Log($"[{_manager.EmulatorName}] Pointer map built. Contains {_intelligentPointerMap.Count} unique pointer values. Starting path resolution for target 0x{_params.TargetAddress:X8}.");
            ReportProgress("Resolving paths...", 0, 1, 0);

            await Task.Run(() =>
            {
                ResolvePathsBackward(_params.TargetAddress, new List<int>(), 1);
            }, cancellationToken);

            logger.Log($"[{_manager.EmulatorName}] --- LIVE SCAN COMPLETE: Found {_foundPaths.Count} paths. ---");

            var resultsToReturn = new List<PointerPath>(_foundPaths);

            _intelligentPointerMap?.Clear();
            _intelligentPointerMap = null;
            _foundPaths?.Clear();
            _foundPaths = null;

            return resultsToReturn;
        }

        private void ResolvePathsBackward(uint addressToFind, List<int> currentOffsets, int level)
        {
            if (_cancellationToken.IsCancellationRequested || _params == null || currentOffsets.Count >= _params.MaxLevel || _foundPaths.Count >= _params.MaxResults)
            {
                return;
            }

            CheckPointerAndRecurse(addressToFind, 0, currentOffsets, level + 1);
            if (_cancellationToken.IsCancellationRequested || _foundPaths.Count >= _params.MaxResults) return;

            uint startAddress = addressToFind;
            int step = _params.Use16ByteAlignment ? 16 : 4;

            if (_params.Use16ByteAlignment)
            {
                startAddress &= 0xFFFFFFF0;
            }

            for (uint currentAddress = startAddress; currentAddress > 4096 && currentAddress >= addressToFind - _params.MaxOffset; currentAddress -= (uint)step)
            {
                if (_cancellationToken.IsCancellationRequested || _foundPaths.Count >= _params.MaxResults) break;
                int offset = (int)(addressToFind - currentAddress);
                if (offset == 0) continue;
                CheckPointerAndRecurse(currentAddress, offset, currentOffsets, level + 1);
            }

            if (currentOffsets.Count == 0 && _params.ScanForStructureBase)
            {
                for (int offset = -4; offset >= -_params.MaxNegativeOffset; offset -= 4)
                {
                    if (_cancellationToken.IsCancellationRequested || _foundPaths.Count >= _params.MaxResults) break;
                    CheckPointerAndRecurse(addressToFind - (uint)offset, offset, currentOffsets, level + 1);
                }
            }
        }

        private void CheckPointerAndRecurse(uint pointerValueToSearch, int offset, List<int> currentOffsets, int nextLevel)
        {
            if (_params == null || _manager == null || _intelligentPointerMap == null) return;

            var pointerSources = new List<uint>();
            if (_intelligentPointerMap.TryGetValue(pointerValueToSearch, out var sources))
            {
                pointerSources.AddRange(sources);
            }
            // Also check for the "kernel" address format (without the 0x20000000 base)
            if (pointerValueToSearch >= Pcsx2Manager.PS2_EEMEM_START && _intelligentPointerMap.TryGetValue(pointerValueToSearch - Pcsx2Manager.PS2_EEMEM_START, out var kernelSources))
            {
                pointerSources.AddRange(kernelSources);
            }

            if (!pointerSources.Any()) return;

            var newOffsets = new List<int>(currentOffsets);
            newOffsets.Insert(0, offset);

            var staticPointers = pointerSources.Where(s => s >= _params.StaticBaseStart && s <= _params.StaticBaseEnd).ToList();
            if (staticPointers.Any())
            {
                lock (_foundPaths)
                {
                    if (_foundPaths == null) return;
                    foreach (uint staticBase in staticPointers)
                    {
                        var newPath = new PointerPath { BaseAddress = staticBase, Offsets = newOffsets, FinalAddress = _params.TargetAddress };
                        if (!_foundPaths.Any(p => p.BaseAddress == newPath.BaseAddress && p.GetOffsetsString() == newPath.GetOffsetsString()))
                        {
                            _foundPaths.Add(newPath);
                            if (_foundPaths.Count >= _params.MaxResults) break;
                        }
                    }
                }
                ReportProgress(null, 0, 0, _foundPaths.Count);
            }

            if (_foundPaths.Count >= _params.MaxResults) return;

            foreach (uint dynamicPointer in pointerSources.Where(s => s < _params.StaticBaseStart || s > _params.StaticBaseEnd))
            {
                ResolvePathsBackward(dynamicPointer, newOffsets, nextLevel);
            }
        }

        private async Task BuildIntelligentPointerMapAsync()
        {
            long eeMemSize = Pcsx2Manager.PS2_EEMEM_END - Pcsx2Manager.PS2_EEMEM_START;
            long gameCodeSize = Pcsx2Manager.PS2_GAME_CODE_END - Pcsx2Manager.PS2_GAME_CODE_START;
            long totalSize = eeMemSize + gameCodeSize;
            long processedSize = 0;
            int chunkSize = 65536;

            var memoryRegionsToScan = new[]
            {
                new { Start = Pcsx2Manager.PS2_EEMEM_START, End = Pcsx2Manager.PS2_EEMEM_END },
                new { Start = Pcsx2Manager.PS2_GAME_CODE_START, End = Pcsx2Manager.PS2_GAME_CODE_END }
            };

            foreach (var region in memoryRegionsToScan)
            {
                for (uint addr = region.Start; addr < region.End; addr += (uint)chunkSize)
                {
                    if (_cancellationToken.IsCancellationRequested) return;
                    byte[] chunk = _manager.ReadMemory(addr, chunkSize);
                    if (chunk == null) continue;

                    for (int i = 0; i + 3 < chunk.Length; i += 4)
                    {
                        uint value = BitConverter.ToUInt32(chunk, i);
                        if (_manager.IsValidPointerTarget(value))
                        {
                            uint pointerAddress = addr + (uint)i;
                            if (!_intelligentPointerMap.ContainsKey(value))
                            {
                                _intelligentPointerMap[value] = new List<uint>();
                            }
                            _intelligentPointerMap[value].Add(pointerAddress);
                        }
                    }
                    processedSize += chunkSize;
                    ReportProgress($"Building pointer map... {((double)processedSize / totalSize):P0}", processedSize, totalSize, _foundPaths.Count);
                    await Task.Yield();
                }
            }
        }

        private void ReportProgress(string message, long current, long max, int found)
        {
            var report = new ScanProgressReport
            {
                FoundCount = found
            };
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