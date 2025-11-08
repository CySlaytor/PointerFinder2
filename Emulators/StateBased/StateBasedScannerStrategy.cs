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
        protected class PathCandidate
        {
            public uint HeadAddress { get; set; }
            public List<int> ReverseOffsets { get; set; }
        }

        protected ScanParameters _params;
        protected IEmulatorManager _manager;
        protected IProgress<ScanProgressReport> _progress;
        protected readonly DebugLogForm logger = DebugLogForm.Instance;

        protected Dictionary<uint, List<uint>> _pointerMap;
        private ConcurrentBag<PointerPath> _foundPaths;
        // Added a dedicated counter for found paths to avoid performance issues with ConcurrentBag.Count.
        private long _foundPathsCounter;
        private long _candidatesValidated;
        private long _candidatesGenerated;
        private CancellationTokenSource _stopOnFirstCts;
        // Replaced the threshold field with a dedicated manager class to remove duplicated logic.
        private ProgressThresholdManager _progressThresholdManager;

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
            _candidatesValidated = 0;
            _candidatesGenerated = 0;
            _foundPathsCounter = 0;
            // Initialize the progress threshold manager with a starting threshold of 1 for immediate feedback.
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
            }

            var finalList = _foundPaths.ToList();

            // Added an option to either return all found paths or only the shortest ones.
            if (!_params.FindAllPathLevels && finalList.Any())
            {
                // Old behavior: find the minimum level and return only paths of that level.
                int minLevel = finalList.Min(p => p.Offsets.Count);
                return finalList.Where(p => p.Offsets.Count == minLevel)
                                .OrderBy(p => p.BaseAddress)
                                .ToList();
            }
            else
            {
                // New behavior: return all paths, sorted by level.
                return finalList.OrderBy(p => p.Offsets.Count)
                                .ThenBy(p => p.BaseAddress)
                                .ToList();
            }
        }

        private Task SearchAndValidateAsync(CancellationToken token)
        {
            var currentLevelCandidates = new List<PathCandidate>();

            uint targetAddress = _params.CapturedStates[0].TargetAddress;
            logger.Log("[Phase 2] Finding Level 1 candidates...");
            ReportProgress("Searching Level 1...", 0, _params.MaxOffset / 4, 0);
            int candidatesFound = 0;

            for (int offset = 0; offset <= _params.MaxOffset; offset += 4)
            {
                if (token.IsCancellationRequested) break;
                if (candidatesFound >= _params.CandidatesPerLevel) break;

                uint valueToFind = targetAddress - (uint)offset;
                var sources = FindSourcesForValue(valueToFind).ToList();
                if (sources.Any())
                {
                    foreach (var source in sources)
                    {
                        currentLevelCandidates.Add(new PathCandidate { HeadAddress = source, ReverseOffsets = new List<int> { offset } });
                        Interlocked.Increment(ref _candidatesGenerated);
                    }
                    candidatesFound++;
                }
                if (offset % 1024 == 0) ReportProgress("Searching Level 1...", offset / 4, _params.MaxOffset / 4, 0);
            }
            logger.Log($"[Phase 2] Found {currentLevelCandidates.Count:N0} Level 1 candidates.");

            foreach (var candidate in currentLevelCandidates)
            {
                if (token.IsCancellationRequested) break;
                if (candidate.HeadAddress >= _params.StaticBaseStart && candidate.HeadAddress <= _params.StaticBaseEnd)
                {
                    ValidateAndAdd(candidate);
                }
            }

            for (int level = 2; level <= _params.MaxLevel; level++)
            {
                // Use the new MaxCandidates parameter instead of MaxResults.
                if (token.IsCancellationRequested || !currentLevelCandidates.Any() || _candidatesGenerated >= _params.MaxCandidates) break;

                logger.Log($"[Phase 2] Finding Level {level} candidates from {currentLevelCandidates.Count:N0} previous level nodes...");
                var nextLevelCandidates = new ConcurrentBag<PathCandidate>();
                var parallelOptions = new ParallelOptions { CancellationToken = token };
                if (_params.LimitCpuUsage) parallelOptions.MaxDegreeOfParallelism = Math.Max(1, Environment.ProcessorCount / 2);

                // Report progress based on iterating through the current level's candidates for a smoother UI experience.
                long processedCandidates = 0;
                ReportProgress($"Searching Level {level}...", 0, currentLevelCandidates.Count, (int)_foundPathsCounter);

                Parallel.ForEach(currentLevelCandidates, parallelOptions, (candidate, loopState) =>
                {
                    // Use the new MaxCandidates parameter instead of MaxResults.
                    if (_candidatesGenerated >= _params.MaxCandidates)
                    {
                        loopState.Stop();
                        return;
                    }
                    if (candidate.HeadAddress >= _params.StaticBaseStart && candidate.HeadAddress <= _params.StaticBaseEnd)
                    {
                        // No need to search deeper from a static address.
                    }
                    else
                    {
                        int candidatesFoundThisLevel = 0;
                        for (int offset = 0; offset <= _params.MaxOffset; offset += 4)
                        {
                            // Use the new MaxCandidates parameter instead of MaxResults.
                            if (token.IsCancellationRequested || _candidatesGenerated >= _params.MaxCandidates)
                            {
                                loopState.Stop();
                                return;
                            }
                            if (candidatesFoundThisLevel >= _params.CandidatesPerLevel) break;

                            uint valueToFind = candidate.HeadAddress - (uint)offset;
                            var sources = FindSourcesForValue(valueToFind).ToList();
                            if (sources.Any())
                            {
                                foreach (var source in sources)
                                {
                                    var newOffsets = new List<int>(candidate.ReverseOffsets);
                                    newOffsets.Add(offset);
                                    nextLevelCandidates.Add(new PathCandidate { HeadAddress = source, ReverseOffsets = newOffsets });
                                    Interlocked.Increment(ref _candidatesGenerated);
                                }
                                candidatesFoundThisLevel++;
                            }
                        }
                    }

                    long currentProcessed = Interlocked.Increment(ref processedCandidates);
                    if (currentProcessed % 256 == 0) // Update progress bar periodically
                    {
                        ReportProgress($"Searching Level {level}...", currentProcessed, currentLevelCandidates.Count, (int)_foundPathsCounter);
                    }
                });

                currentLevelCandidates = nextLevelCandidates.ToList();
                logger.Log($"[Phase 2] Found {currentLevelCandidates.Count:N0} Level {level} candidates.");

                foreach (var candidate in currentLevelCandidates)
                {
                    if (token.IsCancellationRequested) break;
                    if (candidate.HeadAddress >= _params.StaticBaseStart && candidate.HeadAddress <= _params.StaticBaseEnd)
                    {
                        ValidateAndAdd(candidate);
                    }
                }
            }
            // Use the new MaxCandidates parameter instead of MaxResults.
            ReportProgress("Search phase complete.", _params.MaxCandidates, _params.MaxCandidates, (int)_foundPathsCounter);
            return Task.CompletedTask;
        }

        private void ValidateAndAdd(PathCandidate candidate)
        {
            if (_stopOnFirstCts.IsCancellationRequested) return;
            Interlocked.Increment(ref _candidatesValidated);

            var finalPath = new PointerPath
            {
                BaseAddress = candidate.HeadAddress,
                Offsets = candidate.ReverseOffsets,
                FinalAddress = _params.FinalAddressTarget
            };
            finalPath.Offsets.Reverse();

            if (IsValidInAllStates(finalPath))
            {
                _foundPaths.Add(finalPath);
                long currentCount = Interlocked.Increment(ref _foundPathsCounter);

                // Use the new manager class to decide when to report progress.
                if (_progressThresholdManager.ShouldUpdate(currentCount))
                {
                    // Report a count-only update.
                    ReportProgress(null, -1, -1, (int)currentCount);
                }

                if (_params.StopOnFirstPathFound)
                {
                    _stopOnFirstCts.Cancel();
                }
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

            uint finalAddress = currentAddress.Value + (uint)path.Offsets.Last();
            logBuilder?.AppendLine($"  -> Final: 0x{currentAddress.Value:X8} + {path.Offsets.Last():+X;-X} = 0x{finalAddress:X8}");
            return finalAddress;
        }

        private uint? ReadValueFromState(uint address, byte[] memory, StringBuilder logBuilder, string levelId)
        {
            (uint normalizedReadAddress, bool isShort) = _manager.NormalizeAddressForRead(address);

            // Use the new manager method to get the correct index for any memory layout.
            long indexInDump = _manager.GetIndexForStateDump(normalizedReadAddress);

            if (indexInDump < 0 || indexInDump + 3 >= memory.Length)
            {
                logBuilder?.AppendLine($"  -> {levelId} FAIL: Address 0x{address:X8} is out of memory dump bounds.");
                return null;
            }

            // Handle Big-Endian conversion when reading directly from the memory dump.
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

        protected void ReportProgress(string message, long current, long max, int found)
        {
            var report = new ScanProgressReport
            {
                StatusMessage = message,
                CurrentValue = current,
                MaxValue = max,
                FoundCount = found
            };
            _progress?.Report(report);
        }
        #endregion
    }
}