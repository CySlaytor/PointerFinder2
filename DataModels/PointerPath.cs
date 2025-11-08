using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace PointerFinder2.DataModels
{
    // Represents a single, complete pointer path from a static base address to a final target address.
    // Implements IEquatable<PointerPath> for efficient, direct comparison and use in HashSets
    // without needing to convert to strings, which saves significant memory and CPU time.
    public class PointerPath : IEquatable<PointerPath>
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

            // Determine the correct memory access size prefix based on the emulator target.
            // This provides a much smarter default for the user.
            string sizePrefix;
            string pointerMask = "";

            // Find the profile for the current manager to get its target type.
            var profile = EmulatorProfileRegistry.Profiles.FirstOrDefault(p => p.Name.StartsWith(manager.EmulatorName.Split(' ')[0]));

            switch (profile?.Target)
            {
                case EmulatorTarget.DuckStation:
                case EmulatorTarget.RALibretroNDS:
                    sizePrefix = "W"; // 24-bit memory access
                    break;

                case EmulatorTarget.Dolphin:
                    sizePrefix = "G"; // 32-bit Big-Endian memory access
                    pointerMask = "&536870911"; // This is 0x1FFFFFFF
                    break;

                case EmulatorTarget.PPSSPP:
                    sizePrefix = "X"; // 32-bit memory access
                    pointerMask = "&33554431"; // This is decimal for 0x1FFFFFF
                    break;
                case EmulatorTarget.PCSX2:
                default:
                    sizePrefix = "X"; // 32-bit memory access
                    break;
            }

            // 1. Base Address (indirect memory reference).
            string normalizedBaseAddress = manager.FormatDisplayAddress(this.BaseAddress);
            sb.Append($"I:0x{sizePrefix}{normalizedBaseAddress.PadLeft(8, '0').ToLower()}{pointerMask}");

            // 2. Intermediate Pointer Offsets (all but the last one).
            for (int i = 0; i < Offsets.Count - 1; i++)
            {
                int offset = Offsets[i];
                // Correctly handle negative offsets for intermediate pointers by subtracting from the previous value.
                if (offset < 0)
                {
                    // For RA triggers, subtraction is represented by adding the two's complement, which is what happens with integer overflow.
                    // However, a simpler representation is `p(address) - value`.
                    // The most compatible way is to use a `Mem` address for the previous pointer and subtract.
                    // Since that's complex to build dynamically, we use addition with the large overflowed value.
                    // `ptr + (uint)negative_offset` correctly calculates `ptr - abs(negative_offset)`.
                    string formattedOffset = ((uint)offset).ToString("x8");
                    sb.Append($"_I:0x{sizePrefix}{formattedOffset}{pointerMask}");
                }
                else
                {
                    string formattedOffset = Math.Abs(offset).ToString("x8");
                    sb.Append($"_I:0x{sizePrefix}{formattedOffset}{pointerMask}");
                }
            }

            // 3. Final Offset (direct offset).
            if (Offsets.Any())
            {
                int lastOffset = Offsets.Last();
                // Correctly handle negative offsets for the final direct memory access.
                if (lastOffset < 0)
                {
                    // This creates a subtraction from the last pointer's value. e.g., `p(0x..)-0xC`
                    // This is not directly supported in one go in RA. The logic needs to be expressed differently.
                    // The standard way is to add the two's complement.
                    string formattedLastOffset = ((uint)lastOffset).ToString("x8");
                    sb.Append($"_0x{sizePrefix}{formattedLastOffset}");
                }
                else
                {
                    string formattedLastOffset = Math.Abs(lastOffset).ToString("x8");
                    sb.Append($"_0x{sizePrefix}{formattedLastOffset}");
                }
            }

            // 4. Add a default trigger condition.
            sb.Append("=1");

            return sb.ToString();
        }


        #region IEquatable Implementation
        // Determines if this PointerPath is equal to another one.
        // Two paths are considered equal if they have the same base address and the same sequence of offsets.
        public bool Equals(PointerPath other)
        {
            if (other is null)
            {
                return false;
            }
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            // A path is defined by its base and its offsets. The final address is a result of these
            // and does not need to be compared for path equality.
            return BaseAddress == other.BaseAddress && Offsets.SequenceEqual(other.Offsets);
        }

        // Standard override for object.Equals.
        public override bool Equals(object obj)
        {
            return Equals(obj as PointerPath);
        }

        // Generates a hash code for the PointerPath.
        // This is crucial for efficient performance in a HashSet. The hash code is based on the
        // same properties used in the Equals method (BaseAddress and the sequence of Offsets).
        public override int GetHashCode()
        {
            unchecked // Overflow is fine, just wrap
            {
                int hashCode = 17; // Start with a prime
                hashCode = hashCode * 23 + BaseAddress.GetHashCode();
                if (Offsets != null)
                {
                    foreach (var offset in Offsets)
                    {
                        hashCode = hashCode * 23 + offset.GetHashCode();
                    }
                }
                return hashCode;
            }
        }
        #endregion
    }
}