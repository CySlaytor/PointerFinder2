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
    // A form that analyzes an NDS ROM header to find the static address range for a game.
    public partial class NdsStaticRangeFinderForm : BaseForm
    {
        private readonly IEmulatorManager _manager;
        private uint _foundStart = 0;
        private uint _foundEnd = 0;

        public NdsStaticRangeFinderForm(IEmulatorManager manager)
        {
            InitializeComponent();
            _manager = manager;
        }

        private void NdsStaticRangeFinderForm_Load(object sender, EventArgs e)
        {
            Log("--- Welcome to the NDS Static Range Finder ---", Color.Cyan);
            Log("This tool analyzes a .nds ROM file to automatically detect the static memory range of your game.", Color.White);
            Log("\nInstructions:", Color.Yellow);
            Log("1. Click 'Browse...' to select the .nds ROM file for the game you are running.", Color.Gainsboro);
            Log("2. Click 'Analyze ROM' to begin the analysis.", Color.Gainsboro);
            Log("\nHow it works:", Color.Yellow);
            Log("The finder reads the game's header to find where the ARM9 binary is loaded into RAM and its size. Based on this, it calculates a suggested search range that is most likely to contain the game's static data.", Color.Gainsboro);
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            using (var ofd = new OpenFileDialog())
            {
                ofd.Filter = "NDS ROM Files (*.nds)|*.nds|All files (*.*)|*.*";
                ofd.Title = "Select NDS ROM File";
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    txtRomPath.Text = ofd.FileName;
                }
            }
        }

        private async void btnAnalyze_Click(object sender, EventArgs e)
        {
            string romPath = txtRomPath.Text;
            if (string.IsNullOrWhiteSpace(romPath) || !File.Exists(romPath))
            {
                MessageBox.Show("Please select a valid .nds ROM file.", "File Not Found", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // --- UI Prep ---
            btnAnalyze.Enabled = false;
            btnApply.Enabled = false;
            richLog.Clear();
            lblResult.Text = "N/A";

            try
            {
                // --- Run Analysis on a background thread ---
                var result = await Task.Run(() => PerformAnalysis(romPath));

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

        private (bool success, uint start, uint end) PerformAnalysis(string romPath)
        {
            const uint NDS_RAM_START = 0x02000000;
            const uint NDS_RAM_END = 0x02400000;

            try
            {
                using (var fs = new FileStream(romPath, FileMode.Open, FileAccess.Read))
                using (var br = new BinaryReader(fs))
                {
                    if (br.BaseStream.Length < 0x80)
                    {
                        Log("[FAIL] ROM file is too small to be a valid NDS header.", Color.Red);
                        return (false, 0, 0);
                    }

                    byte[] header = br.ReadBytes(0x80);
                    string gameTitle = Encoding.ASCII.GetString(header, 0x00, 0x0C).TrimEnd('\0');
                    Log($"--- Analyzing Header for: {gameTitle} ---", Color.Cyan);

                    br.BaseStream.Position = 0x28;
                    uint arm9LoadAddr = br.ReadUInt32();
                    uint arm9Size = br.ReadUInt32();
                    Log($"[INFO] ARM9 Load Address: 0x{arm9LoadAddr:X8}", Color.White);
                    Log($"[INFO] ARM9 Size:         0x{arm9Size:X8} ({arm9Size} bytes)", Color.White);

                    if (!(arm9LoadAddr >= NDS_RAM_START && arm9LoadAddr < NDS_RAM_END))
                    {
                        Log("[FAIL] The 'ARM9 Load Address' is outside of main RAM. The header is highly unusual or corrupt.", Color.Red);
                        return (false, 0, 0);
                    }

                    uint calculatedEnd = arm9LoadAddr + arm9Size;
                    Log("\n--- [Calculations] ---", Color.Yellow);
                    Log($"1. Base Static Range (from header):", Color.Gainsboro);
                    Log($"   Start: 0x{arm9LoadAddr:X8}", Color.White);
                    Log($"   End:   0x{calculatedEnd:X8} (Load Address + ARM9 Size)", Color.White);

                    const uint paddingStart = 0x5000; // 20 KB
                    const uint paddingEnd = 0x10000;   // 64 KB, adjusted from script
                    Log("\n2. Applying Asymmetrical Search Buffer:", Color.Gainsboro);
                    Log($"   Buffer Before End: {paddingStart / 1024} KB (0x{paddingStart:X})", Color.White);
                    Log($"   Buffer After End:  {paddingEnd / 1024} KB (0x{paddingEnd:X})", Color.White);

                    uint suggestedStart = calculatedEnd - paddingStart;
                    uint suggestedEnd = calculatedEnd + paddingEnd;
                    Log($"   New Start = 0x{calculatedEnd:X8} - 0x{paddingStart:X} = 0x{suggestedStart:X8}", Color.White);
                    Log($"   New End   = 0x{calculatedEnd:X8} + 0x{paddingEnd:X} = 0x{suggestedEnd:X8}", Color.White);

                    Log("\n3. Clamping to Valid Memory Bounds:", Color.Gainsboro);
                    uint finalStart = Math.Max(arm9LoadAddr, suggestedStart);
                    uint finalEnd = Math.Min(suggestedEnd, NDS_RAM_END);
                    Log($"   Clamped Start: 0x{finalStart:X8}", Color.White);
                    Log($"   Clamped End:   0x{finalEnd:X8}", Color.White);

                    Log("\n4. Final Suggested Search Range (4KB Aligned):", Color.Gainsboro);
                    const uint mask = 0xFFFFF000;
                    uint alignedStart = finalStart & mask;
                    uint alignedEnd = finalEnd & mask;
                    Log($"   Start: 0x{alignedStart:X8} (from 0x{finalStart:X8})", Color.White);
                    Log($"   End:   0x{alignedEnd:X8} (from 0x{finalEnd:X8})", Color.White);

                    Log("\n--- [RESULT] ---", Color.Cyan);
                    Log($"Suggested range for Pointer Finder:", Color.White);
                    Log($"  Start: {_manager.FormatDisplayAddress(alignedStart)}", Color.LimeGreen);
                    Log($"  End:   {_manager.FormatDisplayAddress(alignedEnd)}", Color.LimeGreen);

                    return (true, alignedStart, alignedEnd);
                }
            }
            catch (Exception ex)
            {
                Log($"[FAIL] An error occurred while reading the ROM: {ex.Message}", Color.Red);
                return (false, 0, 0);
            }
        }

        private void btnApply_Click(object sender, EventArgs e)
        {
            if (_foundStart == 0 || _foundEnd == 0) return;

            try
            {
                Log("\n[APPLY] Saving approximate range to settings.ini...", Color.Yellow);
                var settings = SettingsManager.Load(EmulatorTarget.RALibretroNDS, _manager.GetDefaultSettings());
                settings.StaticAddressStart = _manager.FormatDisplayAddress(_foundStart);
                settings.StaticAddressEnd = _manager.FormatDisplayAddress(_foundEnd);
                SettingsManager.Save(EmulatorTarget.RALibretroNDS, settings);

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

        private void txtRomPath_TextChanged(object sender, EventArgs e)
        {
            btnAnalyze.Enabled = !string.IsNullOrWhiteSpace(txtRomPath.Text);
        }
    }
}