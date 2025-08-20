namespace PointerFinder2.DataModels
{
    // A data class that holds all the user-configurable parameters for a pointer scan.
    public class ScanParameters
    {
        // The dynamic address in the emulated memory that we are trying to find a pointer to.
        public uint TargetAddress { get; set; }

        // The maximum number of pointers in a chain (e.g., Base -> Ptr1 -> Ptr2 -> Target is Level 3).
        public int MaxLevel { get; set; } = 7;

        // The maximum positive offset to search backwards from an address.
        public int MaxOffset { get; set; } = 0xFFF;

        // The maximum negative offset to search, used for finding the base of a data structure.
        public int MaxNegativeOffset { get; set; } = 0x400;

        // The start of the memory range where static base addresses are expected to be found.
        public uint StaticBaseStart { get; set; } = 0x20100000;

        // The end of the memory range for static base addresses.
        public uint StaticBaseEnd { get; set; } = 0x21FFFFFF;

        // The maximum number of pointer paths to find before stopping the scan.
        public int MaxResults { get; set; } = 5000;

        // If true, the scanner will also search using negative offsets on the first level.
        public bool ScanForStructureBase { get; set; } = true;

        // If true, the scanner will use 16-byte steps for faster, aligned searching (primarily for PCSX2).
        public bool Use16ByteAlignment { get; set; } = true;

        // NEW: If true, the scanner will limit the number of threads used for parallel operations.
        public bool LimitCpuUsage { get; set; } = false;
    }
}