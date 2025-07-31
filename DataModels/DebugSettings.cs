namespace PointerFinder2.DataModels
{
    // A static class to hold global debug settings, allowing any part of the app
    // to check if a specific logging feature is enabled.
    public static class DebugSettings
    {
        // Toggles detailed logging for the main pointer scanning process.
        public static bool LogLiveScan { get; set; } = false;

        // Toggles detailed logging for the pointer path validation (filtering) process.
        public static bool LogFilterValidation { get; set; } = false;

        // Toggles detailed logging for the refine scan (intersection) process.
        public static bool LogRefineScan { get; set; } = false;

        // A global flag to pause or resume logging in the debug console.
        public static bool IsLoggingPaused { get; set; } = false;
    }
}