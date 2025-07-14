namespace PointerFinder2.DataModels
{
    public class ScanParameters
    {
        public uint TargetAddress { get; set; }
        public int MaxLevel { get; set; } = 7;
        public int MaxOffset { get; set; } = 0xFFF;
        public int MaxNegativeOffset { get; set; } = 0x400;
        public uint StaticBaseStart { get; set; } = 0x20100000;
        public uint StaticBaseEnd { get; set; } = 0x21FFFFFF;
        public int MaxResults { get; set; } = 5000;
        public bool AnalyzeStructures { get; set; } = true;
        public bool ScanForStructureBase { get; set; } = true;
        public bool Use16ByteAlignment { get; set; } = true;
    }
}