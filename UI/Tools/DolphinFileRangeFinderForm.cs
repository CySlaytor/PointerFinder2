using PointerFinder2.Core;
using PointerFinder2.Emulators;
using System;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2.UI.StaticRangeFinders
{
    // A form that analyzes a main.dol file to find the static address range for a GC/Wii game.
    public partial class DolphinFileRangeFinderForm : BaseForm
    {
        private readonly IEmulatorManager _manager;
        private uint _foundStart = 0;
        private uint _foundEnd = 0;
        private const int SLIDER_STEP = 64;

        public DolphinFileRangeFinderForm(IEmulatorManager manager)
        {
            InitializeComponent();
            _manager = manager;
        }

        private void DolphinFileRangeFinderForm_Load(object sender, EventArgs e)
        {
            Log("--- Welcome to the Dolphin/GC Static Range Finder ---", Color.Cyan);
            Log("This tool analyzes a main.dol file to automatically detect the static memory range of your game.", Color.White);
            Log("\nInstructions:", Color.Yellow);
            Log("1. In Dolphin, right-click your game in the game list and select 'Properties'.", Color.Gainsboro);
            Log("2. Go to the 'Filesystem' tab.", Color.Gainsboro);
            Log("3. Right-click on the top-level entry ('Disc' for GC, 'Data Partition' for Wii) and choose 'Extract System Data...'. Save the files to a folder.", Color.Gainsboro);
            Log("4. In this tool, click 'Browse...' and navigate to that folder, then open the 'sys\\main.dol' file.", Color.Gainsboro);
            Log("5. Adjust the 'Border Size' if needed (a larger border gives a wider, safer search range).", Color.Gainsboro);
            Log("6. Click 'Analyze File' to begin.", Color.Gainsboro);

            // Initialize the slider and its display label.
            trackBarBorderSize.Value = 1024;
            lblBorderSizeValue.Text = $"Border Size: {trackBarBorderSize.Value} KB";
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            using (var ofd = new OpenFileDialog())
            {
                ofd.Filter = "DOL Executable (*.dol)|*.dol|All files (*.*)|*.*";
                ofd.Title = "Select main.dol File";
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    txtFilePath.Text = ofd.FileName;
                }
            }
        }

        private async void btnAnalyze_Click(object sender, EventArgs e)
        {
            string filePath = txtFilePath.Text;
            if (string.IsNullOrWhiteSpace(filePath) || !File.Exists(filePath))
            {
                MessageBox.Show("Please select a valid main.dol file.", "File Not Found", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
            // Read value from the TrackBar instead of NumericUpDown.
            int borderSize = trackBarBorderSize.Value * 1024;

            // --- UI Prep ---
            btnAnalyze.Enabled = false;
            btnApply.Enabled = false;
            richLog.Clear();
            lblResult.Text = "N/A";

            try
            {
                // --- Run Analysis on a background thread ---
                var result = await Task.Run(() => PerformAnalysis(filePath, (uint)borderSize));

                // --- Update UI with results ---
                if (result.success)
                {
                    _foundStart = result.start;
                    _foundEnd = result.end;
                    lblResult.Text = $"{_manager.FormatDisplayAddress(result.start)} - {_manager.FormatDisplayAddress(result.end)}";
                    btnApply.Enabled = true;
                }
            }
            catch (Exception ex)
            {
                Log($"[FATAL] An unexpected error occurred: {ex.Message}", Color.Red);
            }
            finally
            {
                btnAnalyze.Enabled = true;
            }
        }

        private (bool success, uint start, uint end) PerformAnalysis(string filePath, uint borderSize)
        {
            const int NUM_TEXT_SECTIONS = 7;
            const int NUM_DATA_SECTIONS = 11;
            const uint MEM1_END = 0x81800000;

            try
            {
                using (var fs = new FileStream(filePath, FileMode.Open, FileAccess.Read))
                using (var br = new BinaryReader(fs))
                {
                    if (br.BaseStream.Length < 0x100)
                    {
                        Log("[FAIL] File is too small to be a valid DOL header.", Color.Red);
                        return (false, 0, 0);
                    }

                    Log($"--- Analyzing '{Path.GetFileName(filePath)}' ---", Color.Cyan);

                    // Read header and parse section data (Big-Endian)
                    var textOffsets = ReadUInt32Array(br, 0x00, NUM_TEXT_SECTIONS);
                    var dataOffsets = ReadUInt32Array(br, 0x1C, NUM_DATA_SECTIONS);
                    var textAddrs = ReadUInt32Array(br, 0x48, NUM_TEXT_SECTIONS);
                    var dataAddrs = ReadUInt32Array(br, 0x64, NUM_DATA_SECTIONS);
                    var textSizes = ReadUInt32Array(br, 0x90, NUM_TEXT_SECTIONS);
                    var dataSizes = ReadUInt32Array(br, 0xAC, NUM_DATA_SECTIONS);
                    var bssAddr = ReadUInt32(br, 0xD8);
                    var bssSize = ReadUInt32(br, 0xDC);

                    uint minAddr = uint.MaxValue;
                    uint maxAddr = 0;

                    // Analyze text sections
                    for (int i = 0; i < NUM_TEXT_SECTIONS; i++)
                    {
                        if (textSizes[i] > 0 && textAddrs[i] > 0)
                        {
                            minAddr = Math.Min(minAddr, textAddrs[i]);
                            maxAddr = Math.Max(maxAddr, textAddrs[i] + textSizes[i]);
                        }
                    }

                    // Analyze data sections
                    for (int i = 0; i < NUM_DATA_SECTIONS; i++)
                    {
                        if (dataSizes[i] > 0 && dataAddrs[i] > 0)
                        {
                            minAddr = Math.Min(minAddr, dataAddrs[i]);
                            maxAddr = Math.Max(maxAddr, dataAddrs[i] + dataSizes[i]);
                        }
                    }

                    // Analyze BSS section
                    if (bssSize > 0 && bssAddr > 0)
                    {
                        minAddr = Math.Min(minAddr, bssAddr);
                        maxAddr = Math.Max(maxAddr, bssAddr + bssSize);
                    }

                    if (maxAddr == 0)
                    {
                        Log("[FAIL] No active sections found in the DOL file.", Color.Red);
                        return (false, 0, 0);
                    }

                    Log("\n--- [Calculations] ---", Color.Yellow);
                    Log($"1. Base Static Endpoint:", Color.Gainsboro);
                    Log($"   The highest memory address used by any section is 0x{maxAddr:X8}.", Color.White);

                    Log($"\n2. Applying Symmetrical Border of {borderSize / 1024} KB (0x{borderSize:X}):", Color.Gainsboro);
                    uint halfBorder = borderSize / 2;
                    uint suggestedStart = (maxAddr > halfBorder) ? maxAddr - halfBorder : 0;
                    uint suggestedEnd = maxAddr + halfBorder;
                    Log($"   New Start = 0x{maxAddr:X8} - 0x{halfBorder:X} = 0x{suggestedStart:X8}", Color.White);
                    Log($"   New End   = 0x{maxAddr:X8} + 0x{halfBorder:X} = 0x{suggestedEnd:X8}", Color.White);

                    Log("\n3. Clamping to Valid Memory Bounds:", Color.Gainsboro);
                    uint finalStart = Math.Max(minAddr, suggestedStart);
                    uint finalEnd = Math.Min(suggestedEnd, MEM1_END);
                    Log($"   Clamped Start: max(Actual Start: 0x{minAddr:X8}, Suggested: 0x{suggestedStart:X8}) = 0x{finalStart:X8}", Color.White);
                    Log($"   Clamped End:   min(Suggested: 0x{suggestedEnd:X8}, MEM1 End: 0x{MEM1_END:X8}) = 0x{finalEnd:X8}", Color.White);

                    Log("\n4. Final Suggested Search Range (4KB Aligned):", Color.Gainsboro);
                    uint mask = 0xFFFFF000;
                    uint alignedStart = finalStart & mask;
                    uint alignedEnd = finalEnd & mask;
                    Log($"   Start: 0x{alignedStart:X8}", Color.White);
                    Log($"   End:   0x{alignedEnd:X8}", Color.White);

                    Log("\n--- [RESULT] ---", Color.Cyan);
                    Log($"Suggested range for Pointer Finder:", Color.White);
                    Log($"  Start: {_manager.FormatDisplayAddress(alignedStart)}", Color.LimeGreen);
                    Log($"  End:   {_manager.FormatDisplayAddress(alignedEnd)}", Color.LimeGreen);

                    return (true, alignedStart, alignedEnd);
                }
            }
            catch (Exception ex)
            {
                Log($"[FAIL] An error occurred while reading the file: {ex.Message}", Color.Red);
                return (false, 0, 0);
            }
        }

        private void btnApply_Click(object sender, EventArgs e)
        {
            if (_foundStart == 0 || _foundEnd == 0) return;

            try
            {
                Log("\n[APPLY] Saving approximate range to settings.ini...", Color.Yellow);
                var settings = SettingsManager.Load(EmulatorTarget.Dolphin, _manager.GetDefaultSettings());
                settings.StaticAddressStart = _manager.FormatDisplayAddress(_foundStart);
                settings.StaticAddressEnd = _manager.FormatDisplayAddress(_foundEnd);
                SettingsManager.Save(EmulatorTarget.Dolphin, settings);

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

        // New event handler to snap the slider value and update the display label.
        private void trackBarBorderSize_Scroll(object sender, EventArgs e)
        {
            int value = trackBarBorderSize.Value;
            int snappedValue = (int)(Math.Round((double)value / SLIDER_STEP) * SLIDER_STEP);
            if (trackBarBorderSize.Value != snappedValue)
            {
                trackBarBorderSize.Value = snappedValue;
            }
            lblBorderSizeValue.Text = $"Border Size: {trackBarBorderSize.Value} KB";
        }

        #region Helpers
        private uint ReadUInt32(BinaryReader br, long position)
        {
            br.BaseStream.Position = position;
            byte[] bytes = br.ReadBytes(4);
            Array.Reverse(bytes);
            return BitConverter.ToUInt32(bytes, 0);
        }

        private uint[] ReadUInt32Array(BinaryReader br, long position, int count)
        {
            uint[] result = new uint[count];
            br.BaseStream.Position = position;
            for (int i = 0; i < count; i++)
            {
                byte[] bytes = br.ReadBytes(4);
                Array.Reverse(bytes);
                result[i] = BitConverter.ToUInt32(bytes, 0);
            }
            return result;
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

        private void txtFilePath_TextChanged(object sender, EventArgs e)
        {
            btnAnalyze.Enabled = !string.IsNullOrWhiteSpace(txtFilePath.Text);
        }
        #endregion
    }
}