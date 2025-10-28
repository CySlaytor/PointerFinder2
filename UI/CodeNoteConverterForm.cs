using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using PointerFinder2.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2.UI
{
    public partial class CodeNoteConverterForm : BaseForm
    {
        //Static fields to remember user inputs for the session.
        private static string _lastTriggerInput = "";
        private static string _lastCodeNoteInput = "";
        private static string _lastBaseAddress = "";
        private static int _lastPrefixIndex = 0;
        private static bool _lastUseMask = false;

        private List<int> _lastOffsets = new List<int>();
        private readonly IEmulatorManager _manager;

        // Base constructor
        public CodeNoteConverterForm(IEmulatorManager manager)
        {
            InitializeComponent();
            _manager = manager;
        }

        // This public method allows the main form to send a new trigger to an existing instance.
        public void ProcessTrigger(string trigger)
        {
            // Switch to the correct tab if not already selected
            if (tabControl1.SelectedTab != tabToNote)
            {
                tabControl1.SelectedTab = tabToNote;
            }

            txtTriggerInput.Text = trigger;
            btnConvert.PerformClick();
        }

        private void CodeNoteConverterForm_Load(object sender, EventArgs e)
        {
            if (Settings.Default.CodeNoteConverterSize.Width > 0 && Settings.Default.CodeNoteConverterSize.Height > 0)
            {
                this.StartPosition = FormStartPosition.Manual;
                Point location = Settings.Default.CodeNoteConverterLocation;
                Size size = Settings.Default.CodeNoteConverterSize;
                bool isVisible = false;
                foreach (Screen screen in Screen.AllScreens)
                {
                    if (screen.WorkingArea.IntersectsWith(new Rectangle(location, size)))
                    {
                        isVisible = true;
                        break;
                    }
                }
                if (isVisible)
                {
                    this.Location = location;
                    this.Size = size;
                }
            }

            comboMemorySize.Items.AddRange(new object[] {
                "8-bit", "16-bit", "32-bit", "24-bit", "Lower4", "Upper4",
                "16-bit BE", "32-bit BE", "24-bit BE",
                "Bit0", "Bit1", "Bit2", "Bit3", "Bit4", "Bit5", "Bit6", "Bit7",
                "BitCount",
                "Float", "Float BE",
                "Double32", "Double32 BE",
                "MBF32", "MBF32 LE",
                "ASCII Text"
            });

            comboPointerPrefix.Items.AddRange(new object[] {
                "X (32-bit)",
                "G (32-bit BE)",
                "W (24-bit)"
            });

            // Restore session state
            txtTriggerInput.Text = _lastTriggerInput;
            richCodeNoteInput.Text = _lastCodeNoteInput;
            txtBaseAddress.Text = _lastBaseAddress;
            comboPointerPrefix.SelectedIndex = _lastPrefixIndex;
            chkUseMask.Checked = _lastUseMask;

            // Apply intelligent defaults if an emulator is attached
            if (_manager != null && _manager.IsAttached)
            {
                ApplyEmulatorDefaults();
            }

            // Trigger initial conversion if there's text
            if (!string.IsNullOrEmpty(txtTriggerInput.Text))
            {
                btnConvert.PerformClick();
            }
            if (!string.IsNullOrEmpty(richCodeNoteInput.Text))
            {
                btnReconvert.PerformClick();
            }
        }

        private void CodeNoteConverterForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Save window state before saving other session state
            Settings.Default.CodeNoteConverterLocation = this.Location;
            Settings.Default.CodeNoteConverterSize = this.Size;
            Settings.Default.Save();

            // Save state for the session
            _lastTriggerInput = txtTriggerInput.Text;
            _lastCodeNoteInput = richCodeNoteInput.Text;
            _lastBaseAddress = txtBaseAddress.Text;
            _lastPrefixIndex = comboPointerPrefix.SelectedIndex;
            _lastUseMask = chkUseMask.Checked;
        }


        // Set defaults based on the currently attached emulator.
        private void ApplyEmulatorDefaults()
        {
            var profile = EmulatorProfileRegistry.Profiles.Find(p => p.Name.StartsWith(_manager.EmulatorName.Split(' ')[0]));
            if (profile == null) return;

            switch (profile.Target)
            {
                case EmulatorTarget.Dolphin:
                    comboPointerPrefix.SelectedIndex = 1; // G (32-bit BE)
                    chkUseMask.Checked = true;
                    chkUseMask.Enabled = false;
                    comboPointerPrefix.Enabled = false;
                    break;
                case EmulatorTarget.DuckStation:
                case EmulatorTarget.RALibretroNDS:
                    comboPointerPrefix.SelectedIndex = 2; // W (24-bit)
                    chkUseMask.Checked = false;
                    chkUseMask.Enabled = false;
                    comboPointerPrefix.Enabled = false;
                    break;
                case EmulatorTarget.PCSX2:
                    comboPointerPrefix.SelectedIndex = 0; // X (32-bit)
                    chkUseMask.Checked = false;
                    chkUseMask.Enabled = true;
                    comboPointerPrefix.Enabled = true;
                    break;
                default:
                    comboPointerPrefix.Enabled = true;
                    chkUseMask.Enabled = true;
                    break;
            }
        }


        private void btnConvert_Click(object sender, EventArgs e)
        {
            string trigger = txtTriggerInput.Text.Trim();
            if (string.IsNullOrEmpty(trigger))
            {
                richCodeNoteOutput.Text = "Please enter a trigger string to convert.";
                _lastOffsets.Clear();
                return;
            }

            var (offsets, parsedSize) = CodeNoteHelper.ParseTrigger(trigger);
            if (offsets == null || offsets.Count == 0)
            {
                richCodeNoteOutput.Text = "Could not find any valid offsets in the trigger string.";
                _lastOffsets.Clear();
                comboMemorySize.Text = "N/A";
                txtDescription.Clear();
                return;
            }

            _lastOffsets = offsets;
            comboMemorySize.Text = parsedSize ?? "N/A";
            txtDescription.Clear();

            UpdateNotePreview();
        }

        private void UpdateNotePreview()
        {
            if (_lastOffsets == null || _lastOffsets.Count == 0)
            {
                return;
            }

            var settings = CodeNoteSettings.GetFromGlobalSettings();
            string note = CodeNoteHelper.BuildCodeNote(_lastOffsets, settings, comboMemorySize.Text, txtDescription.Text);
            richCodeNoteOutput.Text = note;
        }

        private void txtDescription_TextChanged(object sender, EventArgs e)
        {
            UpdateNotePreview();
        }

        private void comboMemorySize_TextChanged(object sender, EventArgs e)
        {
            UpdateNotePreview();
        }

        private async void btnCopyToClipboard_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrWhiteSpace(richCodeNoteOutput.Text)) return;

            Clipboard.SetText(richCodeNoteOutput.Text);

            var originalText = btnCopyToClipboard.Text;
            var originalColor = btnCopyToClipboard.BackColor;

            btnCopyToClipboard.Text = "✔️ Copied!";
            btnCopyToClipboard.BackColor = Color.SeaGreen;
            btnCopyToClipboard.Enabled = false;

            await Task.Delay(2000);

            if (!this.IsDisposed)
            {
                btnCopyToClipboard.Text = originalText;
                btnCopyToClipboard.BackColor = originalColor;
                btnCopyToClipboard.Enabled = true;
            }
        }

        private void btnReconvert_Click(object sender, EventArgs e)
        {
            string pointerPrefix = comboPointerPrefix.Text.Split(' ')[0];
            txtTriggerOutput.Text = CodeNoteHelper.GenerateTriggerFromCodeNote(richCodeNoteInput.Text, txtBaseAddress.Text, pointerPrefix, chkUseMask.Checked);
        }

        private async void btnCopyTrigger_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrWhiteSpace(txtTriggerOutput.Text)) return;
            Clipboard.SetText(txtTriggerOutput.Text);

            var originalText = btnCopyTrigger.Text;
            var originalColor = btnCopyTrigger.BackColor;

            btnCopyTrigger.Text = "✔️ Copied!";
            btnCopyTrigger.BackColor = Color.SeaGreen;
            btnCopyTrigger.Enabled = false;

            await Task.Delay(2000);

            if (!this.IsDisposed)
            {
                btnCopyTrigger.Text = originalText;
                btnCopyTrigger.BackColor = originalColor;
                btnCopyTrigger.Enabled = true;
            }
        }

        private void comboPointerPrefix_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Disable mask checkbox if 24-bit is selected.
            if (comboPointerPrefix.SelectedIndex == 2) // W (24-bit)
            {
                chkUseMask.Checked = false;
                chkUseMask.Enabled = false;
            }
            // Only re-enable if not locked by an emulator default.
            else if (_manager == null || !_manager.IsAttached)
            {
                chkUseMask.Enabled = true;
            }
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}