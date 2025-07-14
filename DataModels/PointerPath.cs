using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace PointerFinder2.DataModels
{
    public class PointerPath
    {
        public uint BaseAddress { get; set; }
        public List<int> Offsets { get; set; } = new List<int>();
        public uint FinalAddress { get; set; }

        public string GetOffsetsString()
        {
            return string.Join(", ", Offsets.Select(o => (o < 0) ? $"-{Math.Abs(o):X}" : $"+{o:X}"));
        }

        // Modified to accept the manager for correct address formatting
        public string ToRetroAchievementsString(IEmulatorManager manager)
        {
            var sb = new StringBuilder();
            // Get the correct prefix ("H" for PS2, "W" for PS1) from the manager
            string prefix = manager.RetroAchievementsPrefix;

            // 1. Base Address
            // Use the manager to get the user-friendly short address string (e.g., "618A4")
            string normalizedBaseAddress = manager.FormatDisplayAddress(this.BaseAddress);
            sb.Append($"I:0x{prefix}{normalizedBaseAddress}");

            // 2. Intermediate Pointer Offsets (all offsets except the last one)
            for (int i = 0; i < Offsets.Count - 1; i++)
            {
                // The RA format wants the hex value of the offset.
                // It handles negative offsets implicitly, so we use Abs().
                // RA uses lowercase hex, so we use ":x".
                string formattedOffset = $"{Math.Abs(Offsets[i]):x}";
                sb.Append($"_I:0x{prefix}{formattedOffset}");
            }

            // 3. Final Offset (the very last one in the list)
            if (Offsets.Any())
            {
                string formattedLastOffset = $"{Math.Abs(Offsets.Last()):x}";
                // The final part is just the offset with no "I:"
                sb.Append($"_0x{prefix}{formattedLastOffset}");
            }

            // 4. Add the default trigger condition.
            sb.Append("=1");

            return sb.ToString();
        }
    }
}