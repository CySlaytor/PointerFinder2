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
    // Added new form to find the static range for PPSSPP by scanning live memory for the game ID.
    public partial class PpssppRamScanRangeFinderForm : Form
    {
        private readonly IEmulatorManager _manager;
        private uint _foundStart = 0;
        private uint _foundEnd = 0;

        public PpssppRamScanRangeFinderForm(IEmulatorManager manager)
        {
            InitializeComponent();
            _manager = manager;
        }

        private void PpssppRamScanRangeFinderForm_Load(object sender, EventArgs e)
        {
            Log("--- Welcome to the PPSSPP Static Range Finder ---", Color.Cyan);
            Log("This tool attempts to automatically detect the static memory range of your game.", Color.White);
            Log("\nInstructions:", Color.Yellow);
            Log("1. Make sure you are fully in-game (e.g., controlling your character).", Color.Gainsboro);
            Log("2. Find your game's ID (e.g., 'UCUS98737'). This is usually in the PPSSPP window title.", Color.Gainsboro);
            Log("3. Enter the Game ID below, without dashes.", Color.Gainsboro);
            Log("4. Click 'Find Static Range' to begin.", Color.Gainsboro);
            Log("\nHow it works:", Color.Yellow);
            Log("The finder searches for your Game ID, which is often stored as a constant string at the end of the game's static data region. By finding the last occurrence, we can approximate the static memory range.", Color.Gainsboro);
        }

        private async void btnFind_Click(object sender, EventArgs e)
        {
            string gameId = txtGameId.Text.Trim().Replace("-", "");
            if (string.IsNullOrWhiteSpace(gameId))
            {
                MessageBox.Show("Please enter the game's ID (e.g., 'UCUS98737').", "Input Required", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // --- UI Prep ---
            btnFind.Enabled = false;
            btnApply.Enabled = false;
            richLog.Clear();
            lblApproxResult.Text = "N/A";
            progressBar.Value = 0;

            try
            {
                // --- Run Analysis on a background thread ---
                var result = await Task.Run(() => PerformAnalysis(gameId));

                // --- Update UI with results ---
                if (result.success)
                {
                    _foundStart = result.approxStart;
                    _foundEnd = result.approxEnd;
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

        private (bool success, uint approxStart, uint approxEnd) PerformAnalysis(string gameId)
        {
            Log("[INFO] Starting static range analysis for PPSSPP...", Color.Cyan);

            uint searchStart = 0x08800000;
            uint searchEnd = 0x09000000;
            uint searchSize = searchEnd - searchStart;

            Log($"[INFO] Reading {searchSize / 1024} KB of memory from PSP address range {searchStart:X} - {searchEnd:X}...", Color.White);
            byte[] searchBuffer = _manager.ReadMemory(searchStart, (int)searchSize);

            if (searchBuffer == null)
            {
                Log("[FAIL] Could not read the required memory block from the emulator. Is the game running?", Color.Red);
                return (false, 0, 0);
            }
            Log("[SUCCESS] Memory block read successfully.", Color.LimeGreen);

            Log($"[INFO] Searching for game ID marker: '{gameId}'...", Color.White);
            byte[] pattern = Encoding.ASCII.GetBytes(gameId);

            var foundIndices = FindAllOccurrences(searchBuffer, pattern);
            if (!foundIndices.Any())
            {
                Log($"\n[FAIL] Could not find the game ID string in the specified memory range.", Color.Red);
                Log("Please ensure you have entered the correct ID and are fully in-game.", Color.Yellow);
                return (false, 0, 0);
            }

            foundIndices.Sort();
            Log($"[INFO] Found {foundIndices.Count} candidate(s). Using the last one found.", Color.White);
            SetProgressBarMaximum(1);

            int lastIndex = foundIndices.Last();
            uint absoluteEndAddr = searchStart + (uint)lastIndex;

            Log($"  -> Last occurrence found at PSP address: {absoluteEndAddr:X}", Color.LimeGreen);
            UpdateProgressBar(1);

            // --- Result Calculation ---
            const uint MB_BLOCK_SIZE = 0x100000;

            // Replaced the flawed heuristic. The correct logic for PSP is to simply find the 1MB block
            // that contains the final marker address.
            uint approxStartAddr = (absoluteEndAddr / MB_BLOCK_SIZE) * MB_BLOCK_SIZE;
            uint approxEndAddr = approxStartAddr + MB_BLOCK_SIZE - 1;

            Log("\n--- [RESULTS] ---", Color.Cyan);
            Log($"Approximate 1MB Block (for scan): {_manager.FormatDisplayAddress(approxStartAddr)} to {_manager.FormatDisplayAddress(approxEndAddr)}", Color.White);

            return (true, approxStartAddr, approxEndAddr);
        }

        private void btnApply_Click(object sender, EventArgs e)
        {
            if (_foundStart == 0 || _foundEnd == 0) return;

            try
            {
                Log("\n[APPLY] Saving approximate range to settings.ini...", Color.Yellow);
                var settings = SettingsManager.Load(EmulatorTarget.PPSSPP, _manager.GetDefaultSettings());
                settings.StaticAddressStart = _manager.FormatDisplayAddress(_foundStart);
                settings.StaticAddressEnd = _manager.FormatDisplayAddress(_foundEnd);
                SettingsManager.Save(EmulatorTarget.PPSSPP, settings);

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
        // Replaced the infinite loop with a correct and efficient pattern search algorithm.
        private List<int> FindAllOccurrences(byte[] buffer, byte[] pattern)
        {
            var indices = new List<int>();
            if (pattern.Length == 0 || buffer.Length < pattern.Length)
            {
                return indices;
            }

            int searchIndex = 0;
            while (searchIndex < buffer.Length)
            {
                // Find the next occurrence starting from our current search index.
                int foundIndex = buffer.AsSpan(searchIndex).IndexOf(pattern);

                if (foundIndex == -1)
                {
                    // No more occurrences found, exit the loop.
                    break;
                }

                // The found index is relative to the slice, so we add our search start
                // to get the absolute index in the original buffer.
                int absoluteIndex = searchIndex + foundIndex;
                indices.Add(absoluteIndex);

                // For the next search, we start one position after the beginning of the
                // pattern we just found to avoid re-finding the same spot.
                searchIndex = absoluteIndex + 1;
            }
            return indices;
        }


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

        private void UpdateProgressBar(int value)
        {
            if (progressBar.InvokeRequired)
            {
                progressBar.Invoke((Action)(() => UpdateProgressBar(value)));
                return;
            }
            if (value <= progressBar.Maximum)
            {
                progressBar.Value = value;
            }
        }

        private void txtGameId_TextChanged(object sender, EventArgs e)
        {
            btnFind.Enabled = !string.IsNullOrWhiteSpace(txtGameId.Text);
        }
        #endregion
    }
}