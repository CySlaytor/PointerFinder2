namespace PointerFinder2.DataModels
{
    public static class DebugSettings
    {
        public static bool LogLiveScan { get; set; } = false;
        public static bool LogFilterValidation { get; set; } = false;
        public static bool LogRefineScan { get; set; } = false;
        public static bool IsLoggingPaused { get; set; } = false;
    }
}