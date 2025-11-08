#nullable enable
namespace PointerFinder2.Core
{
    // Holds the saved settings for a specific emulator profile.
    // This allows user preferences for each scanning mode to persist between sessions.
    public class AppSettings
    {
        // The last address the user searched for in a "New Pointer Scan".
        public string LastTargetAddress { get; set; } = "0";
        // The maximum positive offset to use during a scan.
        public int MaxOffset { get; set; }
        // The maximum number of levels (pointers in a chain) to search for.
        public int MaxLevel { get; set; }
        // Clarified that this setting is for the live scan.
        // The maximum number of results to find during a live scan.
        public int MaxResults { get; set; }
        // The start of the memory range to search for static base addresses.
        public string? StaticAddressStart { get; set; }
        // The end of the memory range to search for static base addresses.
        public string? StaticAddressEnd { get; set; }
        // If true, the scanner will also search using negative offsets on the first level.
        public bool ScanForStructureBase { get; set; }
        // The maximum negative offset to use when ScanForStructureBase is true.
        public int MaxNegativeOffset { get; set; }
        // If true, the live scanner will use 16-byte steps for faster, aligned searching.
        public bool Use16ByteAlignment { get; set; }
        // If true, the UI will show the visual range slider instead of textboxes (for NDS).
        public bool UseSliderRange { get; set; }
        // If true, the state-based scan will stop as soon as the first valid path is found.
        public bool StopOnFirstPathFound { get; set; } = false;
        // Added a setting to control whether the state-based scan returns all paths or only the shortest.
        // If true, the state-based scan will return all valid paths, not just the shortest ones.
        public bool FindAllPathLevels { get; set; } = false;
        // The number of candidate offsets to find per address in a state-based scan before moving on.
        public int CandidatesPerLevel { get; set; } = 10;
        // Added separate setting for State-Based scan candidate limit.
        // The maximum number of candidate paths to generate during a state-based scan.
        public int MaxCandidates { get; set; }
    }
}