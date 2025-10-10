using PointerFinder2.Core;
using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2.UI.StaticRangeFinders
{
    // A form that automatically analyzes emulator memory to find the static address range for a game.
    public partial class Pcsx2RamScanRangeFinderForm : Form
    {
        // --- Heuristic Constants (from python script) ---
        private static readonly byte[][] SearchStrings = { Encoding.ASCII.GetBytes("BASLUS"), Encoding.ASCII.GetBytes("BESLES") };
        private const uint Ps2ExecutableStart = 0x100000;
        private const uint Ps2SearchStart = 0x20300000;
        private const uint Ps2SearchEnd = 0x207FFFFF;
        private const int ValidationOffset = 0x30;
        private const int ZeroCheckBytes = 64;
        private static readonly byte[] ZeroComparisonBlock = new byte[ZeroCheckBytes]; // Default is all zeros.

        private readonly IEmulatorManager _manager;
        private uint _foundStart = 0;
        private uint _foundEnd = 0;

        public Pcsx2RamScanRangeFinderForm(IEmulatorManager manager)
        {
            InitializeComponent();
            _manager = manager;
        }

        private void Pcsx2RamScanRangeFinderForm_Load(object sender, EventArgs e)
        {
            // Added introductory text to guide the user.
            Log("--- Welcome to the PCSX2 Static Range Finder ---", Color.Cyan);
            Log("This tool attempts to automatically detect the static memory range of your game.", Color.White);
            Log("\nFor best results:", Color.Yellow);
            Log("1. Please make sure you are fully in-game (e.g., controlling your character).", Color.Gainsboro);
            Log("   Avoid running this tool from the main menu or during loading screens.", Color.Gainsboro);
            Log("\nHow it works:", Color.Yellow);
            Log("The finder searches for a known pattern (e.g., 'BASLUS', 'BESLES') which often corresponds to memory card file paths. These paths are typically loaded at the very end of the game's static data region.", Color.Gainsboro);
            Log("By finding the last valid occurrence of this pattern, we can make a very good approximation of the static range.", Color.Gainsboro);
            Log("\nClick 'Find Static Range' to begin the analysis.", Color.LimeGreen);
        }

        private async void btnFind_Click(object sender, EventArgs e)
        {
            // --- UI Prep ---
            btnFind.Enabled = false;
            btnApply.Enabled = false;
            richLog.Clear();
            lblAbsoluteResult.Text = "N/A";
            lblApproxResult.Text = "N/A";
            progressBar.Value = 0;

            try
            {
                // --- Run Analysis on a background thread ---
                var result = await Task.Run(() => PerformAnalysis());

                // --- Update UI with results ---
                if (result.success)
                {
                    _foundStart = result.approxStart;
                    _foundEnd = result.approxEnd;

                    lblAbsoluteResult.Text = $"{Ps2ExecutableStart:X} - {_manager.FormatDisplayAddress(result.absoluteEnd)}";
                    lblApproxResult.Text = $"{_manager.FormatDisplayAddress(result.approxStart)} - {_manager.FormatDisplayAddress(result.approxEnd)}";
                    btnApply.Enabled = true;
                }
            }
            catch (Exception ex)
            {
                Log($"[FATAL] An unexpected error occurred: {ex.Message}", Color.Red);
            }
            finally
            {
                btnFind.Enabled = true;
            }
        }

        // The core analysis logic, designed to run on a background thread.
        private (bool success, uint absoluteEnd, uint approxStart, uint approxEnd) PerformAnalysis()
        {
            Log("[INFO] Starting static range analysis for PCSX2...", Color.Cyan);

            uint searchSize = Ps2SearchEnd - Ps2SearchStart;
            Log($"[INFO] Reading {searchSize / 1024} KB of memory from PS2 address range {Ps2SearchStart:X} - {Ps2SearchEnd:X}...", Color.White);
            byte[] searchBuffer = _manager.ReadMemory(Ps2SearchStart, (int)searchSize);

            if (searchBuffer == null)
            {
                Log("[FAIL] Could not read the required memory block from the emulator. Is the game running?", Color.Red);
                return (false, 0, 0, 0);
            }
            Log("[SUCCESS] Memory block read successfully.", Color.LimeGreen);

            Log($"[INFO] Searching for markers: {string.Join(", ", SearchStrings.Select(s => Encoding.ASCII.GetString(s)))}...", Color.White);

            var foundIndices = FindAllOccurrences(searchBuffer, SearchStrings);
            if (!foundIndices.Any())
            {
                Log($"\n[FAIL] Could not find any target strings in the specified range.", Color.Red);
                return (false, 0, 0, 0);
            }

            foundIndices.Sort();
            Log($"\n[INFO] Found {foundIndices.Count} total candidate(s). Validating in reverse order...", Color.Yellow);

            SetProgressBarMaximum(foundIndices.Count);

            // --- Validation Loop ---
            foreach (int indexInBuffer in foundIndices.AsEnumerable().Reverse())
            {
                uint ps2StringAddr = Ps2SearchStart + (uint)indexInBuffer;
                Log($"\n  -> Testing candidate at PS2 address: {ps2StringAddr:X}", Color.White);

                uint validationAddr = ps2StringAddr + ValidationOffset;
                byte[] validationBuffer = _manager.ReadMemory(validationAddr, ZeroCheckBytes);

                if (validationBuffer == null)
                {
                    Log("     [FAIL] Could not read memory for zero-check.", Color.OrangeRed);
                    UpdateProgressBar(progressBar.Value + 1); // Still update progress on failure
                    continue;
                }

                if (validationBuffer.SequenceEqual(ZeroComparisonBlock))
                {
                    Log("     [SUCCESS] Validation passed! A solid block of zeros was found.", Color.LimeGreen);

                    // --- Result Calculation ---
                    uint ps2AbsoluteEndAddr = ps2StringAddr;
                    const uint MB_BLOCK_SIZE = 0x100000;
                    const uint HALF_MB_THRESHOLD = MB_BLOCK_SIZE / 2;

                    uint currentBlockStart = (ps2AbsoluteEndAddr / MB_BLOCK_SIZE) * MB_BLOCK_SIZE;
                    uint offsetInBlock = ps2AbsoluteEndAddr - currentBlockStart;

                    uint approxStartAddr = (offsetInBlock >= HALF_MB_THRESHOLD)
                        ? currentBlockStart
                        : currentBlockStart - MB_BLOCK_SIZE;

                    uint approxEndAddr = approxStartAddr + MB_BLOCK_SIZE - 1;

                    Log("\n--- [RESULTS] ---", Color.Cyan);
                    Log($"Absolute Static Range (PS2):   {Ps2ExecutableStart:X} to {_manager.FormatDisplayAddress(ps2AbsoluteEndAddr)}", Color.White);
                    Log($"Approximate 1MB Block (PS2): {_manager.FormatDisplayAddress(approxStartAddr)} to {_manager.FormatDisplayAddress(approxEndAddr)}", Color.White);

                    UpdateProgressBar(progressBar.Maximum);
                    return (true, ps2AbsoluteEndAddr, approxStartAddr, approxEndAddr);
                }
                else
                {
                    Log("     [FAIL] Validation failed: The memory block was not all zeros.", Color.OrangeRed);
                }
                UpdateProgressBar(progressBar.Value + 1);
            }

            Log("\n[FINAL RESULT] Scanned all candidates, but none passed the validation.", Color.Red);
            return (false, 0, 0, 0);
        }

        // Applies the found range to the PCSX2 settings profile.
        private void btnApply_Click(object sender, EventArgs e)
        {
            if (_foundStart == 0 || _foundEnd == 0) return;

            try
            {
                Log("\n[APPLY] Saving approximate range to settings.ini...", Color.Yellow);
                var settings = SettingsManager.Load(EmulatorTarget.PCSX2, _manager.GetDefaultSettings());
                settings.StaticAddressStart = _manager.FormatDisplayAddress(_foundStart);
                settings.StaticAddressEnd = _manager.FormatDisplayAddress(_foundEnd);
                SettingsManager.Save(EmulatorTarget.PCSX2, settings);

                MessageBox.Show("Static range applied successfully!\nThe new range will be used the next time you open a scan window.",
                    "Settings Applied", MessageBoxButtons.OK, MessageBoxIcon.Information);

                this.DialogResult = DialogResult.OK;
                this.Close();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to save settings: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Log($"[FATAL] Failed to save settings: {ex.Message}", Color.Red);
            }
        }

        #region Helper Methods

        // A helper to find all occurrences of multiple byte patterns in a buffer.
        private List<int> FindAllOccurrences(byte[] buffer, byte[][] patterns)
        {
            var indices = new List<int>();
            for (int i = 0; i <= buffer.Length - 6; i++) // Patterns are 6 bytes long
            {
                foreach (var pattern in patterns)
                {
                    if (buffer.Skip(i).Take(pattern.Length).SequenceEqual(pattern))
                    {
                        indices.Add(i);
                        break;
                    }
                }
            }
            return indices;
        }

        // Thread-safe method to append colored text to the RichTextBox.
        private void Log(string message, Color color)
        {
            if (richLog.InvokeRequired)
            {
                richLog.Invoke((Action)(() => Log(message, color)));
                return;
            }
            richLog.SelectionStart = richLog.TextLength;
            richLog.SelectionLength = 0;
            richLog.SelectionColor = color;
            richLog.AppendText(message + Environment.NewLine);
            richLog.ScrollToCaret();
        }

        private void SetProgressBarMaximum(int value)
        {
            if (progressBar.InvokeRequired)
            {
                progressBar.Invoke((Action)(() => SetProgressBarMaximum(value)));
                return;
            }
            progressBar.Maximum = value;
        }

        // Thread-safe progress bar update.
        private void UpdateProgressBar(int value)
        {
            if (progressBar.InvokeRequired)
            {
                progressBar.Invoke((Action)(() => UpdateProgressBar(value)));
                return;
            }
            // Ensure value does not exceed Maximum, which can happen in edge cases
            if (value <= progressBar.Maximum)
            {
                progressBar.Value = value;
            }
        }

        #endregion
    }
}