#nullable enable
using System.Collections.Generic;

namespace PointerFinder2.DataModels
{
    // A data class that holds all the user-configurable parameters for a pointer scan.
    public class ScanParameters
    {
        // The dynamic address in the emulated memory that we are trying to find a pointer to.
        public uint TargetAddress { get; set; }
        // The maximum number of levels (pointers in a chain) to search for.
        public int MaxLevel { get; set; } = 7;
        // The maximum positive offset to search backwards from an address.
        public int MaxOffset { get; set; } = 0xFFF;
        // The start of the memory range where static base addresses are expected to be found.
        public uint StaticBaseStart { get; set; } = 0x20100000;
        // The end of the memory range for static base addresses.
        public uint StaticBaseEnd { get; set; } = 0x21FFFFFF;

        // If true, the scanner will limit the number of threads used for parallel operations.
        public bool LimitCpuUsage { get; set; } = false;

        // --- Properties for State-Based Scanning ---
        // If true, the state-based scan will stop as soon as the first valid path is found.
        public bool StopOnFirstPathFound { get; set; } = false;
        // If true, the state-based scan will return all valid paths, not just the shortest.
        public bool FindAllPathLevels { get; set; } = false;
        // The number of candidate offsets to find per address before moving on.
        public int CandidatesPerLevel { get; set; } = 10;
        // The maximum number of candidate paths to generate during a state-based scan.
        public int MaxCandidates { get; set; } = 500000;
        // The address to use for the final 'FinalAddress' column in the results grid.
        public uint FinalAddressTarget { get; set; }
        // If true, utilizes aggressive pruning to prevent memory explosion.
        public bool FastScanMode { get; set; } = true;
        // A list containing all the captured memory states for multi-state comparison.
        public List<ScanState>? CapturedStates { get; set; }

        // Creates a lightweight copy of the parameters for history tracking, 
        // deliberately dropping amount of memory dumps so the Garbage Collector can free them.
        public ScanParameters CloneWithoutStates()
        {
            return new ScanParameters
            {
                TargetAddress = TargetAddress,
                MaxLevel = MaxLevel,
                MaxOffset = MaxOffset,
                StaticBaseStart = StaticBaseStart,
                StaticBaseEnd = StaticBaseEnd,
                LimitCpuUsage = LimitCpuUsage,
                StopOnFirstPathFound = StopOnFirstPathFound,
                FindAllPathLevels = FindAllPathLevels,
                CandidatesPerLevel = CandidatesPerLevel,
                MaxCandidates = MaxCandidates,
                FastScanMode = FastScanMode,
                FinalAddressTarget = FinalAddressTarget,
                CapturedStates = null // Break the strong reference to the LOH memory dumps!
            };
        }
    }
}