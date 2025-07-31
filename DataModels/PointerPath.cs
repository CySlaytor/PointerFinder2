using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace PointerFinder2.DataModels
{
    // Represents a single, complete pointer path from a static base address to a final target address.
    public class PointerPath
    {
        // The static base address that the path starts from.
        public uint BaseAddress { get; set; }

        // The sequence of offsets to traverse from the base address.
        public List<int> Offsets { get; set; } = new List<int>();

        // The final, dynamic address that this path resolves to.
        public uint FinalAddress { get; set; }

        // Returns a user-friendly string of the offsets (e.g., "+C, +F40, -1A").
        public string GetOffsetsString()
        {
            return string.Join(", ", Offsets.Select(o => (o < 0) ? $"-{Math.Abs(o):X}" : $"+{o:X}"));
        }

        // Formats the path into a string for RetroAchievements memory scripts.
        public string ToRetroAchievementsString(IEmulatorManager manager)
        {
            var sb = new StringBuilder();
            string prefix = manager.RetroAchievementsPrefix;

            // 1. Base Address (indirect memory reference).
            string normalizedBaseAddress = manager.FormatDisplayAddress(this.BaseAddress);
            sb.Append($"I:0x{prefix}{normalizedBaseAddress}");

            // 2. Intermediate Pointer Offsets (all but the last one).
            for (int i = 0; i < Offsets.Count - 1; i++)
            {
                string formattedOffset = $"{Math.Abs(Offsets[i]):x}";
                sb.Append($"_I:0x{prefix}{formattedOffset}");
            }

            // 3. Final Offset (direct offset).
            if (Offsets.Any())
            {
                string formattedLastOffset = $"{Math.Abs(Offsets.Last()):x}";
                sb.Append($"_0x{prefix}{formattedLastOffset}");
            }

            // 4. Add a default trigger condition.
            sb.Append("=1");

            return sb.ToString();
        }
    }
}