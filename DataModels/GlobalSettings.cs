namespace PointerFinder2.DataModels
{
    // Holds global application settings that aren't tied to a specific emulator profile.
    public static class GlobalSettings
    {
        // If true, the app uses the default system notification sound instead of custom .wav files.
        public static bool UseWindowsDefaultSound { get; set; } = false;
    }
}