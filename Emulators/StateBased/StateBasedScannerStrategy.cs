using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace PointerFinder2.Emulators.StateBased
{
    // This abstract base class contains the core logic for the "state-based scan" (algo2).
    // It works by building a map of a static memory dump and then performing a parallel,
    // breadth-first search backwards from the target addresses, validating candidates
    // against all provided states. Console-specific details are handled by inheriting classes.
    public abstract class StateBasedScannerStrategyBase : IPointerScannerStrategy
    {
        // Replaced PathCandidate with a memory-efficient linked-list Node structure.
        // This avoids instantiating millions of List<int> objects, saving massive amounts of RAM
        // and garbage collection pauses during deep, explosive scans.
        protected class PathNode
        {
            public uint Address { get; }
            public int Offset { get; }
            public PathNode Child { get; }

            public PathNode(uint address, int offset, PathNode child)
            {
                Address = address;
                Offset = offset;
                Child = child;
            }

            public List<int> GetForwardOffsets()
            {
                var offsets = new List<int>();
                PathNode current = this;
                while (current != null)
                {
                    offsets.Add(current.Offset);
                    current = current.Child;
                }
                return offsets;
            }
        }

        protected ScanParameters _params;
        protected IEmulatorManager _manager;
        protected IProgress<ScanProgressReport> _progress;
        protected readonly DebugLogForm logger = DebugLogForm.Instance;

        protected Dictionary<uint, List<uint>> _pointerMap;
        // Global deduplication dictionary. Maps a dynamic memory address to the shortest Level it was reached at.
        // This prevents Re-convergence (duplicating subtrees) and completely kills Cycles/Linked List loops.
        protected ConcurrentDictionary<uint, int> _visitedNodes;

        private ConcurrentBag<PointerPath> _foundPaths;
        private long _foundPathsCounter;
        private long _staticPathsFoundCounter;
        private long _candidatesValidated;
        private long _candidatesGenerated;
        private CancellationTokenSource _stopOnFirstCts;
        private ProgressThresholdManager _progressThresholdManager;

        // Hard limit to prevent RAM/CPU death if a specific level fans out astronomically.
        private const int MAX_NODES_PER_LEVEL = 150000;

        #region Abstract Methods
        // Builds the pointer map by scanning the relevant memory regions from a captured memory dump.
        protected abstract Task BuildPointerMapAsync(ScanState state, CancellationToken token);

        // Finds all addresses that point to a given value, handling console-specific logic like address mirroring.
        protected abstract IEnumerable<uint> FindSourcesForValue(uint value);
        #endregion

        public async Task<List<PointerPath>> Scan(IEmulatorManager manager, ScanParameters parameters, IProgress<ScanProgressReport> progress, CancellationToken cancellationToken)
        {
            _manager = manager;
            _params = parameters;
            _progress = progress;
            _foundPaths = new ConcurrentBag<PointerPath>();
            _visitedNodes = new ConcurrentDictionary<uint, int>();
            _candidatesValidated = 0;
            _candidatesGenerated = 0;
            _foundPathsCounter = 0;
            _staticPathsFoundCounter = 0;
            _progressThresholdManager = new ProgressThresholdManager(1);
            _stopOnFirstCts = new CancellationTokenSource();

            var combinedToken = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, _stopOnFirstCts.Token).Token;

            if (_params?.CapturedStates == null || _params.CapturedStates.Count < 2)
            {
                logger.Log("[StateScan] ERROR: At least two states must be captured to perform a scan.");
                return new List<PointerPath>();
            }

            logger.Log("=====================================================================");
            logger.Log($"[{_manager.EmulatorName}] --- STARTING STATE-BASED SCAN ({_params.CapturedStates.Count} states) ---");
            logger.Log("This algorithm uses a Breadth-First Search (BFS) to find the shortest pointer paths first.");
            if (_params.FastScanMode)
            {
                logger.Log("Fast Mode (Aggressive Pruning) is ENABLED. Redundant paths and loops will be skipped.");
            }

            try
            {
                await BuildPointerMapAsync(_params.CapturedStates[0], combinedToken);
                if (combinedToken.IsCancellationRequested) return new List<PointerPath>();
                logger.Log($"[Phase 1] Complete: Built pointer map for State 1 with {_pointerMap.Count:N0} unique values.");

                await SearchAndValidateAsync(combinedToken);
                logger.Log($"[Phase 2] Complete: Generated {_candidatesGenerated:N0} total candidate paths. Validated {_candidatesValidated:N0} static paths.");

                logger.Log("=====================================================================");
                logger.Log($"[{_manager.EmulatorName}] --- SCAN COMPLETE: Found {_foundPaths.Count:N0} paths. ---");
            }
            catch (OperationCanceledException)
            {
                logger.Log($"[{_manager.EmulatorName}] --- SCAN CANCELLED. ---");
            }
            finally
            {
                _stopOnFirstCts.Dispose();
                _visitedNodes.Clear(); // Free memory immediately
            }

            var finalList = _foundPaths.ToList();

            if (!_params.FindAllPathLevels && finalList.Any(p => !p.IsPartial))
            {
                int minLevel = finalList.Where(p => !p.IsPartial).Min(p => p.Offsets.Count);
                return finalList.Where(p => p.IsPartial || p.Offsets.Count == minLevel)
                                .OrderBy(p => p.BaseAddress)
                                .ToList();
            }
            else
            {
                return finalList.OrderBy(p => p.Offsets.Count)
                                .ThenBy(p => p.BaseAddress)
                                .ToList();
            }
        }

        private Task SearchAndValidateAsync(CancellationToken token)
        {
            var currentLevelCandidates = new List<PathNode>();
            uint targetAddress = _params.CapturedStates[0].TargetAddress;

            // Seed the deduplication dictionary only if Fast Mode is active
            if (_params.FastScanMode)
            {
                _visitedNodes[targetAddress] = 0;
            }

            logger.Log("[Phase 2] Finding Level 1 candidates...");
            ReportProgress("Searching Level 1...", 0, _params.MaxOffset / 4);
            int candidatesFound = 0;

            for (int offset = 0; offset <= _params.MaxOffset; offset += 4)
            {
                if (token.IsCancellationRequested) break;
                if (candidatesFound >= _params.CandidatesPerLevel) break;

                uint valueToFind = targetAddress - (uint)offset;
                var sources = FindSourcesForValue(valueToFind).ToList();

                foreach (var source in sources)
                {
                    bool shouldProcess = true;

                    if (_params.FastScanMode)
                    {
                        shouldProcess = false;
                        // Thread-safe deduplication check: Have we reached this address via a shorter or equal path?
                        _visitedNodes.AddOrUpdate(source,
                            (k) => { shouldProcess = true; return 1; },
                            (k, existingLevel) =>
                            {
                                if (1 < existingLevel) { shouldProcess = true; return 1; }
                                return existingLevel;
                            });
                    }

                    if (shouldProcess)
                    {
                        currentLevelCandidates.Add(new PathNode(source, offset, null));
                        Interlocked.Increment(ref _candidatesGenerated);
                        candidatesFound++;
                    }
                }

                if (offset % 1024 == 0) ReportProgress("Searching Level 1...", offset / 4, _params.MaxOffset / 4);
            }
            logger.Log($"[Phase 2] Found {currentLevelCandidates.Count:N0} Level 1 candidates.");

            foreach (var candidate in currentLevelCandidates)
            {
                if (token.IsCancellationRequested) break;
                if (candidate.Address >= _params.StaticBaseStart && candidate.Address <= _params.StaticBaseEnd)
                {
                    ValidateAndAdd(candidate);
                }
            }

            // Stop early if valid paths were found at Level 1 and we don't want to dig deeper
            if (!_params.FindAllPathLevels && Interlocked.Read(ref _staticPathsFoundCounter) > 0)
            {
                logger.Log($"[Phase 2] Found {Interlocked.Read(ref _staticPathsFoundCounter):N0} static paths at Level 1. Stopping deeper search because 'Find all path levels' is unchecked.");
                ReportProgress("Search phase complete.", _params.MaxCandidates, _params.MaxCandidates);
                return Task.CompletedTask;
            }

            for (int level = 2; level <= _params.MaxLevel; level++)
            {
                if (token.IsCancellationRequested || !currentLevelCandidates.Any() || _candidatesGenerated >= _params.MaxCandidates) break;

                logger.Log($"[Phase 2] Finding Level {level} candidates from {currentLevelCandidates.Count:N0} previous level nodes...");
                var nextLevelCandidates = new ConcurrentBag<PathNode>();
                var parallelOptions = new ParallelOptions { CancellationToken = token };
                if (_params.LimitCpuUsage) parallelOptions.MaxDegreeOfParallelism = Math.Max(1, Environment.ProcessorCount / 2);

                long processedCandidates = 0;
                ReportProgress($"Searching Level {level}...", 0, currentLevelCandidates.Count);

                Parallel.ForEach(currentLevelCandidates, parallelOptions, (candidate, loopState) =>
                {
                    if (_candidatesGenerated >= _params.MaxCandidates)
                    {
                        loopState.Stop();
                        return;
                    }

                    if (candidate.Address >= _params.StaticBaseStart && candidate.Address <= _params.StaticBaseEnd)
                    {
                        // No need to search deeper from a static address.
                    }
                    else
                    {
                        int candidatesFoundThisLevel = 0;
                        for (int offset = 0; offset <= _params.MaxOffset; offset += 4)
                        {
                            if (token.IsCancellationRequested || _candidatesGenerated >= _params.MaxCandidates)
                            {
                                loopState.Stop();
                                return;
                            }
                            if (candidatesFoundThisLevel >= _params.CandidatesPerLevel) break;

                            uint valueToFind = candidate.Address - (uint)offset;
                            var sources = FindSourcesForValue(valueToFind).ToList();

                            foreach (var source in sources)
                            {
                                bool shouldProcess = true;

                                if (_params.FastScanMode)
                                {
                                    shouldProcess = false;
                                    // Deduplication & Cycle Prevention
                                    _visitedNodes.AddOrUpdate(source,
                                        (k) => { shouldProcess = true; return level; },
                                        (k, existingLevel) =>
                                        {
                                            if (level < existingLevel) { shouldProcess = true; return level; }
                                            return existingLevel;
                                        });
                                }

                                if (shouldProcess)
                                {
                                    nextLevelCandidates.Add(new PathNode(source, offset, candidate));
                                    Interlocked.Increment(ref _candidatesGenerated);
                                    candidatesFoundThisLevel++;
                                }
                            }
                        }

                        // --- DEAD END CHECK ---
                        if (candidatesFoundThisLevel == 0 && _params.PrintPartialPaths)
                        {
                            AddPartialPath(candidate);
                        }
                    }

                    long currentProcessed = Interlocked.Increment(ref processedCandidates);
                    if (currentProcessed % 256 == 0)
                    {
                        ReportProgress($"Searching Level {level}...", currentProcessed, currentLevelCandidates.Count);
                    }
                });

                // Fan-out Limiter: Prevents memory exhaustion if the level explodes massively.
                if (_params.FastScanMode && nextLevelCandidates.Count > MAX_NODES_PER_LEVEL)
                {
                    logger.Log($"[Phase 2] Level {level} produced {nextLevelCandidates.Count:N0} nodes. Truncating to top {MAX_NODES_PER_LEVEL:N0} (ranked by lowest immediate offset) to prevent combinatorial explosion...");

                    currentLevelCandidates = nextLevelCandidates
                        .OrderBy(n => Math.Abs(n.Offset))
                        .Take(MAX_NODES_PER_LEVEL)
                        .ToList();
                }
                else
                {
                    currentLevelCandidates = nextLevelCandidates.ToList();
                }

                logger.Log($"[Phase 2] Moving forward with {currentLevelCandidates.Count:N0} Level {level} candidates.");

                foreach (var candidate in currentLevelCandidates)
                {
                    if (token.IsCancellationRequested) break;
                    if (candidate.Address >= _params.StaticBaseStart && candidate.Address <= _params.StaticBaseEnd)
                    {
                        ValidateAndAdd(candidate);
                    }
                }

                if (!_params.FindAllPathLevels && Interlocked.Read(ref _staticPathsFoundCounter) > 0)
                {
                    logger.Log($"[Phase 2] Found {Interlocked.Read(ref _staticPathsFoundCounter):N0} static paths at Level {level}. Stopping deeper search because 'Find all path levels' is unchecked.");
                    break;
                }
            }

            // --- CATCH MAX LEVEL DEAD ENDS ---
            if (_params.PrintPartialPaths && !token.IsCancellationRequested && currentLevelCandidates.Any())
            {
                foreach (var candidate in currentLevelCandidates)
                {
                    if (candidate.Address < _params.StaticBaseStart || candidate.Address > _params.StaticBaseEnd)
                    {
                        AddPartialPath(candidate);
                    }
                }
            }

            ReportProgress("Search phase complete.", _params.MaxCandidates, _params.MaxCandidates);
            return Task.CompletedTask;
        }

        private void ValidateAndAdd(PathNode candidate)
        {
            if (_stopOnFirstCts.IsCancellationRequested) return;
            Interlocked.Increment(ref _candidatesValidated);

            var finalPath = new PointerPath
            {
                BaseAddress = candidate.Address,
                Offsets = candidate.GetForwardOffsets(), // Returns offsets accurately from Base -> Target
                FinalAddress = _params.FinalAddressTarget
            };

            if (IsValidInAllStates(finalPath))
            {
                _foundPaths.Add(finalPath);
                long currentCount = Interlocked.Increment(ref _foundPathsCounter);
                Interlocked.Increment(ref _staticPathsFoundCounter);

                if (_progressThresholdManager.ShouldUpdate(currentCount))
                {
                    ReportProgress(null, -1, -1);
                }

                if (_params.StopOnFirstPathFound)
                {
                    _stopOnFirstCts.Cancel();
                }
            }
        }

        private void AddPartialPath(PathNode candidate)
        {
            if (_stopOnFirstCts.IsCancellationRequested) return;

            var finalPath = new PointerPath
            {
                BaseAddress = candidate.Address,
                Offsets = candidate.GetForwardOffsets(),
                FinalAddress = _params.FinalAddressTarget,
                IsPartial = true
            };

            var brokenAddresses = new Dictionary<int, uint>();
            for (int i = 0; i < _params.CapturedStates.Count; i++)
            {
                // Trace the path forward to see where it breaks
                uint lastValidAddress = finalPath.BaseAddress;
                uint? currentAddress = ReadValueFromState(finalPath.BaseAddress, _params.CapturedStates[i].MemoryDump, null, "");

                if (currentAddress.HasValue)
                {
                    bool broke = false;
                    for (int j = 0; j < finalPath.Offsets.Count - 1; j++)
                    {
                        uint nextAddressToRead = currentAddress.Value + (uint)finalPath.Offsets[j];
                        lastValidAddress = nextAddressToRead; // Note where we are trying to read
                        currentAddress = ReadValueFromState(nextAddressToRead, _params.CapturedStates[i].MemoryDump, null, "");

                        if (!currentAddress.HasValue)
                        {
                            broke = true;
                            break;
                        }
                    }
                    if (!broke && finalPath.Offsets.Count > 0)
                    {
                        lastValidAddress = currentAddress.Value + (uint)finalPath.Offsets.Last();
                    }
                }
                brokenAddresses[i] = lastValidAddress;
            }
            finalPath.BrokenStateAddresses = brokenAddresses;

            _foundPaths.Add(finalPath);
            long currentCount = Interlocked.Increment(ref _foundPathsCounter);

            if (_progressThresholdManager.ShouldUpdate(currentCount))
            {
                ReportProgress(null, -1, -1);
            }
        }

        #region Generic Validation Logic
        private bool IsValidInAllStates(PointerPath path)
        {
            bool detailedLog = DebugSettings.LogStateBasedScanDetails;
            StringBuilder logBuilder = detailedLog ? new StringBuilder() : null;

            if (detailedLog)
            {
                logBuilder.AppendLine($"[Validate] Path: {_manager.FormatDisplayAddress(path.BaseAddress)} -> {path.GetOffsetsString()}");
            }

            for (int i = 0; i < _params.CapturedStates.Count; i++)
            {
                var state = _params.CapturedStates[i];
                if (detailedLog)
                {
                    logBuilder.AppendLine($"--- Checking in State {i + 1} (Target: {state.TargetAddress:X8}) ---");
                }

                uint? finalAddress = RecalculatePathInState(path, state, logBuilder);

                if (!finalAddress.HasValue || !_manager.AreAddressesEquivalent(finalAddress.Value, state.TargetAddress))
                {
                    if (detailedLog)
                    {
                        logBuilder.AppendLine($"  -> RESULT: FAILED! Path is invalid in this state.");
                        logger.Log(logBuilder.ToString());
                    }
                    return false;
                }
            }

            if (detailedLog)
            {
                logBuilder.AppendLine("  -> RESULT: SUCCESS! Path is valid in all states.");
                logger.Log(logBuilder.ToString());
            }
            return true;
        }

        private uint? RecalculatePathInState(PointerPath path, ScanState state, StringBuilder logBuilder)
        {
            uint? currentAddress = ReadValueFromState(path.BaseAddress, state.MemoryDump, logBuilder, "Base");
            if (!currentAddress.HasValue) return null;

            for (int i = 0; i < path.Offsets.Count - 1; i++)
            {
                uint nextAddressToRead = currentAddress.Value + (uint)path.Offsets[i];
                currentAddress = ReadValueFromState(nextAddressToRead, state.MemoryDump, logBuilder, $"L{i + 1}");
                if (!currentAddress.HasValue) return null;
            }

            int lastOffset = path.Offsets.Last();
            uint finalAddress = currentAddress.Value + (uint)lastOffset;

            if (logBuilder != null)
            {
                string offsetStr = lastOffset < 0 ? $"- 0x{Math.Abs(lastOffset):X}" : $"+ 0x{lastOffset:X}";
                logBuilder.AppendLine($"  -> Final: 0x{currentAddress.Value:X8} {offsetStr} = 0x{finalAddress:X8}");
            }

            return finalAddress;
        }

        private uint? ReadValueFromState(uint address, byte[] memory, StringBuilder logBuilder, string levelId)
        {
            (uint normalizedReadAddress, bool isShort) = _manager.NormalizeAddressForRead(address);

            long indexInDump = _manager.GetIndexForStateDump(normalizedReadAddress);

            if (indexInDump < 0 || indexInDump + 3 >= memory.Length)
            {
                logBuilder?.AppendLine($"  -> {levelId} FAIL: Address 0x{address:X8} is out of memory dump bounds.");
                return null;
            }

            byte[] valueBytes = new byte[4];
            Buffer.BlockCopy(memory, (int)indexInDump, valueBytes, 0, 4);
            if (_manager.RetroAchievementsPrefix == "G") // "G" is our flag for Big-Endian systems like Dolphin.
            {
                Array.Reverse(valueBytes);
            }
            uint value = BitConverter.ToUInt32(valueBytes, 0);

            if (logBuilder != null)
            {
                string logMsg = isShort ? $"  -> {levelId}: Read[0x{address:X8} (norm: 0x{normalizedReadAddress:X8})] = 0x{value:X8}"
                                        : $"  -> {levelId}: Read[0x{address:X8}] = 0x{value:X8}";
                logBuilder.AppendLine(logMsg);

                if (!_manager.IsValidPointerTarget(value))
                {
                    logBuilder.AppendLine($"     (WARN: Value 0x{value:X8} is not a valid pointer target)");
                }
            }
            return value;
        }

        protected void ReportProgress(string message, long current, long max)
        {
            long currentStatic = Interlocked.Read(ref _staticPathsFoundCounter);
            long currentTotal = Interlocked.Read(ref _foundPathsCounter);

            var report = new ScanProgressReport
            {
                StatusMessage = message,
                CurrentValue = current,
                MaxValue = max,
                FoundCount = (int)currentStatic,
                PartialCount = (int)(currentTotal - currentStatic)
            };
            _progress?.Report(report);
        }
        #endregion
    }
}