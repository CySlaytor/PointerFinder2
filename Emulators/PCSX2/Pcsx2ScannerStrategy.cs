using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators.PCSX2;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.PCSX2
{
    // Implements the pointer scanning strategy specifically for the PCSX2 emulator.
    // This strategy is "intelligent" as it considers multiple memory regions (EE RAM, Game Code)
    // and handles PCSX2's specific memory address formats.
    public class Pcsx2ScannerStrategy : IPointerScannerStrategy
    {
        // Private members for managing scan state.
        private ScanParameters _params;
        private IEmulatorManager _manager;
        private ConcurrentBag<PointerPath> _foundPaths;
        private IProgress<ScanProgressReport> _progress;
        private CancellationToken _cancellationToken;
        // Using a ConcurrentDictionary for the pointer map is crucial for thread-safe parallel processing.
        private ConcurrentDictionary<uint, List<uint>> _intelligentPointerMap;
        // Singleton instance of the logger for debug output.
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        // Main entry point for initiating a pointer scan.
        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            // Initialize state for a new scan.
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _cancellationToken = cancellationToken;
            _foundPaths = new ConcurrentBag<PointerPath>();
            _intelligentPointerMap = new ConcurrentDictionary<uint, List<uint>>();

            if (_params == null) return new List<PointerPath>();

            try
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- STARTING NEW PARALLEL SCAN (INTELLIGENT MAP ALGORITHM) ---");

                // Phase 1: Build a map of all potential pointers in memory. This is the most time-consuming part.
                await BuildIntelligentPointerMapAsync();
                if (cancellationToken.IsCancellationRequested) return _foundPaths.ToList();

                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] Pointer map built. Contains {_intelligentPointerMap.Count} unique pointer values. Starting path resolution for target 0x{_params.TargetAddress:X8}.");
                ReportProgress("Resolving paths...", 0, 1, 0);

                // Phase 2: Recursively search backwards from the target address to find paths to static bases.
                await Task.Run(() =>
                {
                    ResolvePathsBackward(_params.TargetAddress, new List<int>(), 1);
                }, cancellationToken);


                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- PARALLEL SCAN COMPLETE: Found {_foundPaths.Count} paths. ---");
            }
            catch (OperationCanceledException)
            {
                if (DebugSettings.LogLiveScan) logger.Log($"[{_manager.EmulatorName}] --- SCAN CANCELLED BY USER. Found {_foundPaths.Count} paths. ---");
            }
            finally
            {
                // Clean up large data structures to free memory immediately.
                _intelligentPointerMap?.Clear();
                _intelligentPointerMap = null;
            }

            // Return whatever was found, complete or partial.
            return _foundPaths.ToList();
        }

        // The top level of the recursive search. It generates all possible starting points (offsets)
        // for a given address and processes them in parallel.
        private void ResolvePathsBackward(uint addressToFind, List<int> previousOffsets, int level)
        {
            _cancellationToken.ThrowIfCancellationRequested();
            if (_params == null || previousOffsets.Count >= _params.MaxLevel || _foundPaths.Count >= _params.MaxResults)
            {
                return;
            }

            var searchItems = new List<(uint valueToSearch, int offset)>();

            // Candidate 1: A direct pointer (offset 0).
            searchItems.Add((addressToFind, 0));

            // Candidate 2: Pointers with positive offsets.
            int step = _params.Use16ByteAlignment ? 16 : 4;
            uint startAddress = addressToFind;
            if (_params.Use16ByteAlignment)
            {
                startAddress &= 0xFFFFFFF0; // Align the starting address for efficiency.
            }
            for (uint currentAddress = startAddress; currentAddress > 4096 && currentAddress >= addressToFind - _params.MaxOffset; currentAddress -= (uint)step)
            {
                int offset = (int)(addressToFind - currentAddress);
                if (offset == 0) continue;
                searchItems.Add((currentAddress, offset));
            }

            // Candidate 3: Pointers with negative offsets (only on the first level, to find the base of a structure).
            if (previousOffsets.Count == 0 && _params.ScanForStructureBase)
            {
                for (int offset = -4; offset >= -_params.MaxNegativeOffset; offset -= 4)
                {
                    searchItems.Add((addressToFind - (uint)offset, offset));
                }
            }

            // Process all generated candidates in parallel to leverage multiple CPU cores.
            Parallel.ForEach(searchItems, new ParallelOptions { CancellationToken = _cancellationToken }, item =>
            {
                CheckPointerAndRecurse(item.valueToSearch, item.offset, previousOffsets, level);
            });
        }

        // The core recursive function. Takes a single search candidate, checks the pointer map,
        // and either completes a path or continues the recursion.
        private void CheckPointerAndRecurse(uint pointerValueToSearch, int offset, List<int> previousOffsets, int level)
        {
            List<uint> pointerSources;
            // Check the main memory map for the value we're looking for.
            if (!_intelligentPointerMap.TryGetValue(pointerValueToSearch, out var sources))
            {
                // PCSX2-specific: Also check for the "kernel" address format (without the 0x20000000 base).
                if (!(pointerValueToSearch >= Pcsx2Manager.PS2_EEMEM_START && _intelligentPointerMap.TryGetValue(pointerValueToSearch - Pcsx2Manager.PS2_EEMEM_START, out var kernelSources)))
                {
                    return; // No memory addresses point to the value we need. End of this branch.
                }
                pointerSources = kernelSources;
            }
            else
            {
                pointerSources = sources;
            }

            // Create a new offset list for this specific branch to ensure thread safety.
            var newOffsets = new List<int>(previousOffsets);
            newOffsets.Add(offset); // We use Add for performance and reverse the list once at the end.

            foreach (uint sourceAddress in pointerSources)
            {
                _cancellationToken.ThrowIfCancellationRequested();
                if (_foundPaths.Count >= _params.MaxResults) break;

                // Check if the source address is within the user-defined static address range.
                if (sourceAddress >= _params.StaticBaseStart && sourceAddress <= _params.StaticBaseEnd)
                {
                    // Success! A complete path has been found.
                    var finalOffsets = new List<int>(newOffsets);
                    finalOffsets.Reverse(); // Reverse once to get the correct [Base]->[Offset1]->[Offset2] order.
                    var newPath = new PointerPath { BaseAddress = sourceAddress, Offsets = finalOffsets, FinalAddress = _params.TargetAddress };

                    // Heuristic check to prevent adding the same path if multiple threads find it simultaneously.
                    if (!_foundPaths.Any(p => p.BaseAddress == newPath.BaseAddress && p.GetOffsetsString() == newPath.GetOffsetsString()))
                    {
                        _foundPaths.Add(newPath);
                        ReportProgress(null, 0, 0, _foundPaths.Count);
                    }
                }
                else
                {
                    // This is a dynamic pointer, so continue the search one level deeper.
                    // The next address to find is the location of the current pointer (`sourceAddress`).
                    ResolvePathsBackward(sourceAddress, newOffsets, level + 1);
                }
            }
        }

        // Reads through the emulator's memory in parallel to build a map of potential pointers.
        // The key is the value at an address (a potential target), and the value is a list of addresses containing that key.
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
                // Prepare a list of all memory chunks to be processed.
                var chunksToProcess = new List<(uint, int)>();
                for (uint addr = region.Start; addr < region.End; addr += (uint)chunkSize)
                {
                    chunksToProcess.Add((addr, (int)Math.Min(chunkSize, region.End - addr)));
                }

                // Process all chunks in parallel for maximum speed.
                await Task.Run(() => Parallel.ForEach(chunksToProcess, new ParallelOptions { CancellationToken = _cancellationToken }, (chunkInfo) =>
                {
                    _cancellationToken.ThrowIfCancellationRequested();
                    var (addr, size) = chunkInfo;
                    byte[] chunk = _manager.ReadMemory(addr, size);
                    if (chunk == null) return;

                    for (int i = 0; i + 3 < chunk.Length; i += 4)
                    {
                        uint value = BitConverter.ToUInt32(chunk, i);
                        if (_manager.IsValidPointerTarget(value))
                        {
                            uint pointerAddress = addr + (uint)i;
                            // Thread-safe way to get or add the list for a given pointer value.
                            var list = _intelligentPointerMap.GetOrAdd(value, (v) => new List<uint>());
                            // Lock the list itself to prevent race conditions when multiple threads add to it.
                            lock (list)
                            {
                                list.Add(pointerAddress);
                            }
                        }
                    }
                    // Use Interlocked for thread-safe progress reporting.
                    Interlocked.Add(ref processedSize, size);
                    ReportProgress($"Building pointer map... {((double)processedSize / totalSize):P0}", processedSize, totalSize, _foundPaths.Count);
                }), _cancellationToken);

                if (_cancellationToken.IsCancellationRequested) return;
            }
        }

        // Helper method to report progress back to the UI thread.
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