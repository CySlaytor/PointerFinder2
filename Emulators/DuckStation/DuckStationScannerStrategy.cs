using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.DuckStation
{
    public class DuckStationScannerStrategy : IPointerScannerStrategy
    {
        private ScanParameters _params;
        private IEmulatorManager _manager;
        private List<PointerPath> _foundPaths;
        private IProgress<ScanProgressReport> _progress;
        private CancellationToken _cancellationToken;
        private Dictionary<uint, List<uint>> _pointerMap;
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new List<PointerPath>();
            _pointerMap = new Dictionary<uint, List<uint>>();

            if (_params == null) return new List<PointerPath>();

            logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW SCAN (SIMPLE MAP ALGORITHM) ---");

            await BuildPointerMapAsync();
            if (_cancellationToken.IsCancellationRequested) return new List<PointerPath>();

            logger.Log($"[{_manager.EmulatorName}] Pointer map built. Contains {_pointerMap.Count} unique pointer values. Starting path resolution for target 0x{_params.TargetAddress:X8}.");
            ReportProgress("Resolving paths...", 0, 1, 0);

            await Task.Run(() =>
            {
                ResolvePathsBackward(_params.TargetAddress, new List<int>(), 1);
            }, cancellationToken);

            logger.Log($"[{_manager.EmulatorName}] --- SCAN COMPLETE: Found {_foundPaths.Count} paths. ---");

            var resultsToReturn = new List<PointerPath>(_foundPaths);

            _pointerMap?.Clear();
            _pointerMap = null;
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

            // PS1 always uses a 4-byte step. The 16-byte alignment option is ignored.
            for (int offset = 4; offset <= _params.MaxOffset; offset += 4)
            {
                if (_cancellationToken.IsCancellationRequested || _foundPaths.Count >= _params.MaxResults) break;
                CheckPointerAndRecurse(addressToFind - (uint)offset, offset, currentOffsets, level + 1);
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
            if (_params == null || _manager == null || _pointerMap == null) return;

            if (!_pointerMap.TryGetValue(pointerValueToSearch, out var sources)) return;

            var newOffsets = new List<int>(currentOffsets);
            newOffsets.Insert(0, offset);

            var staticPointers = sources.Where(s => s >= _params.StaticBaseStart && s <= _params.StaticBaseEnd).ToList();
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

            foreach (uint dynamicPointer in sources.Where(s => s < _params.StaticBaseStart || s > _params.StaticBaseEnd))
            {
                ResolvePathsBackward(dynamicPointer, newOffsets, nextLevel);
            }
        }

        private async Task BuildPointerMapAsync()
        {
            long totalSize = DuckStationManager.PS1_RAM_SIZE;
            long processedSize = 0;
            int chunkSize = 65536; // 64KB

            logger.Log($"[{_manager.EmulatorName}] Building pointer map by scanning {totalSize / 1024}KB of PS1 RAM.");

            for (uint addr = DuckStationManager.PS1_RAM_START; addr < DuckStationManager.PS1_RAM_END; addr += (uint)chunkSize)
            {
                if (_cancellationToken.IsCancellationRequested) return;

                int readSize = (int)Math.Min(chunkSize, DuckStationManager.PS1_RAM_END - addr);
                byte[] chunk = _manager.ReadMemory(addr, readSize);
                if (chunk == null) continue;

                for (int i = 0; i + 3 < chunk.Length; i += 4)
                {
                    uint value = BitConverter.ToUInt32(chunk, i);
                    if (_manager.IsValidPointerTarget(value))
                    {
                        uint pointerAddress = addr + (uint)i;
                        if (!_pointerMap.ContainsKey(value))
                        {
                            _pointerMap[value] = new List<uint>();
                        }
                        _pointerMap[value].Add(pointerAddress);
                    }
                }
                processedSize += chunkSize;
                ReportProgress($"Building pointer map... {((double)processedSize / totalSize):P0}", processedSize, totalSize, _foundPaths.Count);
                await Task.Yield();
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