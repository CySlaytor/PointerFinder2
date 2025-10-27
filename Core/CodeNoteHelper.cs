#nullable enable
using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace PointerFinder2.Core
{
    // A helper class to generate custom-formatted code notes for pointer chains.
    public static class CodeNoteHelper
    {
        private static readonly Dictionary<char, string> SizeMap = new()
        {
            {' ', "16-bit"}, {'H', "8-bit"}, {'X', "32-bit"}, {'W', "24-bit"},
            {'M', "Bit0"}, {'N', "Bit1"}, {'O', "Bit2"}, {'P', "Bit3"},
            {'Q', "Bit4"}, {'R', "Bit5"}, {'S', "Bit6"}, {'T', "Bit7"},
            {'L', "Lower4"}, {'U', "Upper4"}, {'K', "BitCount"},
            {'G', "32-bit BE"}, {'I', "16-bit BE"}, {'J', "24-bit BE"}
        };

        private static readonly Dictionary<string, char> ReverseSizeMap = SizeMap
            .ToDictionary(kvp => kvp.Value, kvp => kvp.Key);

        // Generates a code note from a PointerPath object using the provided settings.
        public static string GenerateFromPointerPath(PointerPath path, CodeNoteSettings settings)
        {
            if (path == null) return string.Empty;
            return BuildCodeNote(path.Offsets, settings);
        }

        public static (List<int> offsets, string? size) ParseTrigger(string trigger)
        {
            if (string.IsNullOrWhiteSpace(trigger)) return ([], null);

            try
            {
                var offsets = new List<int>();
                string? finalMemorySize = null;

                var parts = trigger.Split('_');
                foreach (var part in parts)
                {
                    var pointerMatch = Regex.Match(part, @"^I:0x[A-Z]?([0-9a-fA-F]+)", RegexOptions.IgnoreCase);
                    if (pointerMatch.Success)
                    {
                        if (offsets.Count != 0 || parts.First() != part)
                        {
                            offsets.Add(Convert.ToInt32(pointerMatch.Groups[1].Value, 16));
                        }
                        continue;
                    }

                    // Updated regex to handle optional prefixes like 'd', 'A:', 'p:', etc., before the final offset.
                    // This allows for parsing of more complex and realistic trigger strings.
                    var finalOffsetMatch = Regex.Match(part, @"(?:[A-Z]:)?[dpb~]?0x([A-Z])([0-9a-fA-F]+)", RegexOptions.IgnoreCase);
                    if (finalOffsetMatch.Success)
                    {
                        offsets.Add(Convert.ToInt32(finalOffsetMatch.Groups[2].Value, 16));
                        char sizeChar = char.ToUpper(finalOffsetMatch.Groups[1].Value[0]);
                        finalMemorySize = SizeMap.GetValueOrDefault(sizeChar, "Unknown Size");
                        break;
                    }
                }
                return (offsets, finalMemorySize);
            }
            catch
            {
                return ([], null);
            }
        }

        //Method signature updated to accept a mask flag.
        public static string GenerateTriggerFromCodeNote(string codeNote, string baseAddress, string pointerPrefix, bool useMask)
        {
            if (string.IsNullOrWhiteSpace(codeNote) || string.IsNullOrWhiteSpace(baseAddress))
            {
                return "Code note and base address cannot be empty.";
            }

            try
            {
                var offsets = new List<int>();
                string finalMemorySize = "8-bit"; // Default

                var lines = codeNote.Split(['\r', '\n'], StringSplitOptions.RemoveEmptyEntries);

                var offsetRegex = new Regex(@"(?:\+|-)?0x([0-9a-fA-F]+)");

                for (int i = 0; i < lines.Length; i++)
                {
                    var line = lines[i];
                    var match = offsetRegex.Match(line);
                    if (!match.Success) continue;

                    offsets.Add(Convert.ToInt32(match.Groups[1].Value, 16));

                    if (i == lines.Length - 1)
                    {
                        var sizeMatch = Regex.Match(line, @"\[(.*?)\]");
                        if (sizeMatch.Success)
                        {
                            finalMemorySize = sizeMatch.Groups[1].Value;
                        }
                    }
                }

                if (offsets.Count == 0)
                {
                    return "Could not parse any offsets from the code note.";
                }

                // Build the trigger string.
                var sb = new StringBuilder();
                string maskString = useMask ? "&536870911" : "";

                sb.Append($"I:0x{pointerPrefix}{baseAddress.Replace("0x", "")}{maskString}");

                char sizeChar = ReverseSizeMap.TryGetValue(finalMemorySize, out var c) ? c : 'H';

                for (int i = 0; i < offsets.Count - 1; i++)
                {
                    sb.Append($"_I:0x{pointerPrefix}{offsets[i]:x}{maskString}");
                }

                // Final offset does not get the mask.
                sb.Append($"_0x{sizeChar}{offsets.Last():x}");
                sb.Append(">=1");

                return sb.ToString();
            }
            catch (Exception ex)
            {
                return $"Error generating trigger: {ex.Message}";
            }
        }


        internal static string BuildCodeNote(List<int> offsets, CodeNoteSettings settings, string? finalMemorySize = null, string? finalDescription = null)
        {
            if (offsets == null || offsets.Count == 0) return string.Empty;

            var lines = new List<string>();

            // 1. Generate the base lines for each offset.
            for (int i = 0; i < offsets.Count; i++)
            {
                int offset = offsets[i];
                string prefix = (i == 0) ? "" : string.Concat(Enumerable.Repeat(settings.Prefix, i));
                string sign = "+";
                string offsetHex = $"{Math.Abs(offset):X}";
                lines.Add($"{prefix}{sign}0x{offsetHex}");
            }

            // 2. Apply alignment and suffixes.
            int maxLength = settings.AlignSuffixes ? lines.Max(l => l.Length) : 0;
            var finalLines = new List<string>();

            for (int i = 0; i < lines.Count; i++)
            {
                string line = lines[i];
                string currentSuffix = "";
                if (!settings.SuffixOnLastLineOnly || (i == lines.Count - 1))
                {
                    currentSuffix = settings.Suffix;
                }

                string padding = settings.AlignSuffixes ? new string(' ', maxLength - line.Length) : "";
                finalLines.Add($"{line}{padding}{currentSuffix}");
            }

            // 3. Handle the final line description.
            if (finalLines.Count != 0 && (!string.IsNullOrEmpty(finalMemorySize) || !string.IsNullOrEmpty(finalDescription)))
            {
                int lastIndex = finalLines.Count - 1;
                var sb = new StringBuilder(finalLines[lastIndex]);

                if (!string.IsNullOrEmpty(finalMemorySize) && finalMemorySize != "N/A")
                {
                    sb.Append($"[{finalMemorySize}] ");
                }
                if (!string.IsNullOrEmpty(finalDescription))
                {
                    sb.Append(finalDescription);
                }
                finalLines[lastIndex] = sb.ToString().TrimEnd();
            }

            return string.Join(Environment.NewLine, finalLines);
        }
    }
}