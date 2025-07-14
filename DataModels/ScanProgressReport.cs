namespace PointerFinder2.DataModels
{
    public class ScanProgressReport
    {
        public long MaxValue { get; set; }
        public long CurrentValue { get; set; }
        public int FoundCount { get; set; }
        public string StatusMessage { get; set; } = string.Empty;
    }
}