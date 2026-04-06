namespace PointerFinder2.DataModels
{
    // A static class to hold global debug settings, allowing any part of the app
    // to check if a specific logging feature is enabled.
    public static class DebugSettings
    {
        // Toggles logging for general application events (like Emulator Attachment and Settings).
        public static bool LogGeneralEvents { get; set; } = false;
        // Toggles detailed logging for the pointer path validation (filtering) process.
        public static bool LogFilterValidation { get; set; } = false;
        // Toggles extremely detailed address-by-address logging for the state-based scan.
        public static bool LogStateBasedScanDetails { get; set; } = false;
        // A global flag to pause or resume logging in the debug console.
        public static bool IsLoggingPaused { get; set; } = false;
    }
}