namespace PointerFinder2.DataModels
{
    // A simple data class used to report scan progress from a background thread to the UI thread.
    public class ScanProgressReport
    {
        // The maximum value for the progress bar (e.g., total memory to scan).
        public long MaxValue { get; set; }

        // The current value for the progress bar (e.g., memory scanned so far).
        public long CurrentValue { get; set; }

        // The number of pointer paths found so far.
        public int FoundCount { get; set; }

        // A message to display in the status bar (e.g., "Building pointer map...").
        public string StatusMessage { get; set; } = string.Empty;
    }
}