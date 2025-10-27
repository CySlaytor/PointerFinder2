namespace PointerFinder2.DataModels
{
    // Holds global application settings that aren't tied to a specific emulator profile.
    public static class GlobalSettings
    {
        // If true, the app uses the default system notification sound instead of custom .wav files.
        public static bool UseWindowsDefaultSound { get; set; } = false;

        // If true, the scanner will limit the number of threads used for parallel operations.
        public static bool LimitCpuUsage { get; set; } = false;

        // --- New Settings for Code Note Formatting ---
        public static string CodeNotePrefix { get; set; } = ".";
        public static string CodeNoteSuffix { get; set; } = " |";
        public static bool CodeNoteAlignSuffixes { get; set; } = true;
        public static bool CodeNoteSuffixOnLastLineOnly { get; set; } = false;
        // Added a new setting to control the "Sort by Lowest Offsets" behavior.
        public static bool SortByLevelFirst { get; set; } = true;
    }
}