namespace PointerFinder2.DataModels
{
    // A data class that encapsulates a full memory snapshot and the parameters
    // associated with it for state-based differential scanning.
    public class ScanState
    {
        // The full memory dump of the emulator's RAM at the time of capture.
        public byte[] MemoryDump { get; set; }

        // The specific parameters used for this state capture, including the target address.
        public ScanParameters Parameters { get; set; }
    }
}