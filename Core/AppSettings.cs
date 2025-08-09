namespace PointerFinder2.Core
{
    // Holds the saved settings for a specific emulator profile.
    public class AppSettings
    {
        public string LastTargetAddress { get; set; } = "0";
        public int MaxOffset { get; set; }
        public int MaxLevel { get; set; }
        public int MaxResults { get; set; }
        public string StaticAddressStart { get; set; }
        public string StaticAddressEnd { get; set; }
        public bool AnalyzeStructures { get; set; }
        public bool ScanForStructureBase { get; set; }
        public int MaxNegativeOffset { get; set; }
        public bool Use16ByteAlignment { get; set; }
        public bool UseSliderRange { get; set; } // For persistence
    }
}