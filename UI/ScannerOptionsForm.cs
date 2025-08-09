using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.Globalization;
using System.Windows.Forms;

namespace PointerFinder2
{
    // A form for configuring the parameters of a pointer scan.
    public partial class ScannerOptionsForm : Form
    {
        private readonly EmulatorTarget _target;
        private readonly IEmulatorManager _manager;
        private readonly AppSettings _currentSettings;
        private readonly AppSettings _defaultSettings; // To store core defaults for reset.
        private bool _isUpdatingFromCode = false;

        // Use a 4KB granularity for the slider, a common memory page size.
        private const int SLIDER_GRANULARITY = 4096;
        private const int MAX_SLIDER_RANGE_BYTES = 1 * 1024 * 1024; // 1MB

        public ScannerOptionsForm(IEmulatorManager manager, AppSettings settings)
        {
            InitializeComponent();
            _manager = manager;
            _target = EmulatorProfileRegistry.Profiles.Find(p => p.Name == manager.EmulatorName).Target;
            _currentSettings = settings;
            _defaultSettings = _manager.GetDefaultSettings(); // Store the core defaults.

            this.rangeSlider.DoubleClick += new System.EventHandler(this.rangeSlider_DoubleClick);
            this.txtStaticStart.TextChanged += new System.EventHandler(this.txtStatic_TextChanged);
            this.txtStaticEnd.TextChanged += new System.EventHandler(this.txtStatic_TextChanged);
        }

        // When the form loads, populate the controls with the current settings for the active emulator.
        private void ScannerOptionsForm_Load(object sender, EventArgs e)
        {
            var logger = DebugLogForm.Instance;
            if (DebugSettings.LogLiveScan) logger.Log("ScannerOptionsForm loading...");

            // --- Configure UI based on Emulator Target ---
            this.Text = $"{_manager.EmulatorName} Scan Options";
            string targetSystem;
            switch (_target)
            {
                case EmulatorTarget.PCSX2: targetSystem = "PS2"; break;
                case EmulatorTarget.DuckStation: targetSystem = "PS1"; break;
                case EmulatorTarget.RALibretroNDS: targetSystem = "NDS"; break;
                default: targetSystem = "Mem"; break;
            }
            label1.Text = $"Target Address ({targetSystem}, Hex)";
            groupBoxRange.Text = $"Static Base Address Range ({targetSystem}, Hex)";

            chkUse16ByteAlignment.Visible = (_target == EmulatorTarget.PCSX2);

            // --- Populate controls with current settings ---
            txtTargetAddress.Text = _currentSettings.LastTargetAddress;
            txtMaxOffset.Text = _currentSettings.MaxOffset.ToString("X");
            numMaxLevel.Value = _currentSettings.MaxLevel;
            numMaxResults.Value = _currentSettings.MaxResults;
            txtStaticStart.Text = _currentSettings.StaticAddressStart;
            txtStaticEnd.Text = _currentSettings.StaticAddressEnd;
            chkAnalyzeStructures.Checked = _currentSettings.AnalyzeStructures;
            chkScanForStructureBase.Checked = _currentSettings.ScanForStructureBase;
            chkUse16ByteAlignment.Checked = _currentSettings.Use16ByteAlignment;
            txtMaxNegativeOffset.Text = _currentSettings.MaxNegativeOffset.ToString("X");
            txtMaxNegativeOffset.Enabled = chkScanForStructureBase.Checked;

            // --- Configure Slider and set initial state ---
            SetupRangeSlider();

            // Make the slider feature NDS-ONLY
            if (_target == EmulatorTarget.RALibretroNDS)
            {
                chkUseSliderRange.Visible = true;
                chkUseSliderRange.Checked = _currentSettings.UseSliderRange;
            }
            else
            {
                chkUseSliderRange.Visible = false;
                chkUseSliderRange.Checked = false;
            }
            chkUseSliderRange_CheckedChanged(null, null);
        }

        private void SetupRangeSlider()
        {
            rangeSlider.Minimum = 0;
            // Correctly use the MainMemorySize from the manager to set the slider's maximum.
            rangeSlider.Maximum = (int)(_manager.MainMemorySize / SLIDER_GRANULARITY);
            rangeSlider.MaxRange = MAX_SLIDER_RANGE_BYTES / SLIDER_GRANULARITY;
            rangeSlider.ThumbStep = 1; // Smallest possible step (4KB)
            rangeSlider.TrackStep = MAX_SLIDER_RANGE_BYTES / SLIDER_GRANULARITY;

            // Sync slider from textbox values on load. This now correctly updates the label.
            txtStatic_TextChanged(null, null);
        }

        public ScanParameters GetScanParameters()
        {
            try
            {
                // This correctly uses the manager's UnnormalizeAddress to handle all cases.
                uint targetAddress = _manager.UnnormalizeAddress(txtTargetAddress.Text);
                uint staticStart = _manager.UnnormalizeAddress(txtStaticStart.Text);
                uint staticEnd = _manager.UnnormalizeAddress(txtStaticEnd.Text);

                if (DebugSettings.LogLiveScan) DebugLogForm.Instance.Log($"Parsed Scan Parameters: Target={targetAddress:X8}, StaticRange={staticStart:X8}-{staticEnd:X8}");

                return new ScanParameters
                {
                    TargetAddress = targetAddress,
                    MaxOffset = int.Parse(txtMaxOffset.Text.Replace("0x", ""), NumberStyles.HexNumber),
                    MaxLevel = (int)numMaxLevel.Value,
                    MaxResults = (int)numMaxResults.Value,
                    StaticBaseStart = staticStart,
                    StaticBaseEnd = staticEnd,
                    AnalyzeStructures = chkAnalyzeStructures.Checked,
                    ScanForStructureBase = chkScanForStructureBase.Checked,
                    MaxNegativeOffset = int.Parse(txtMaxNegativeOffset.Text.Replace("0x", ""), NumberStyles.HexNumber),
                    Use16ByteAlignment = chkUse16ByteAlignment.Checked,
                    // Get the CPU limit setting from the global settings instead of a local checkbox.
                    LimitCpuUsage = GlobalSettings.LimitCpuUsage
                };
            }
            catch (Exception ex)
            {
                MessageBox.Show("Invalid input: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                return null;
            }
        }

        public AppSettings GetCurrentSettings()
        {
            return new AppSettings
            {
                LastTargetAddress = txtTargetAddress.Text,
                MaxOffset = int.Parse(txtMaxOffset.Text.Replace("0x", ""), NumberStyles.HexNumber),
                MaxLevel = (int)numMaxLevel.Value,
                MaxResults = (int)numMaxResults.Value,
                StaticAddressStart = txtStaticStart.Text,
                StaticAddressEnd = txtStaticEnd.Text,
                UseSliderRange = chkUseSliderRange.Checked, // Save the checkbox state
                AnalyzeStructures = chkAnalyzeStructures.Checked,
                ScanForStructureBase = chkScanForStructureBase.Checked,
                MaxNegativeOffset = int.Parse(txtMaxNegativeOffset.Text.Replace("0x", ""), NumberStyles.HexNumber),
                Use16ByteAlignment = chkUse16ByteAlignment.Checked,
            };
        }

        // The OK button validates the input and closes the form if valid.
        private void btnOK_Click(object sender, EventArgs e)
        {
            if (GetScanParameters() != null)
            {
                DialogResult = DialogResult.OK;
                Close();
            }
        }

        // Toggles the enabled state of the negative offset textbox.
        private void chkScanForStructureBase_CheckedChanged(object sender, EventArgs e)
        {
            txtMaxNegativeOffset.Enabled = chkScanForStructureBase.Checked;
        }

        private void chkUseSliderRange_CheckedChanged(object sender, EventArgs e)
        {
            groupBoxRange.Visible = !chkUseSliderRange.Checked;
            panelSlider.Visible = chkUseSliderRange.Checked;
        }

        // --- Synchronization Logic ---

        private void txtStatic_TextChanged(object sender, EventArgs e)
        {
            if (_isUpdatingFromCode) return;
            _isUpdatingFromCode = true;

            try
            {
                uint startAddr = uint.Parse(txtStaticStart.Text, NumberStyles.HexNumber);
                uint endAddr = uint.Parse(txtStaticEnd.Text, NumberStyles.HexNumber);
                rangeSlider.SetRange(
                    (int)(startAddr / SLIDER_GRANULARITY),
                    (int)(endAddr / SLIDER_GRANULARITY)
                );
                // Explicitly update the label text here to fix the initial display bug.
                lblSliderRange.Text = $"{startAddr:X6} - {endAddr:X6}";
            }
            catch { /* Ignore parsing errors during typing */ }

            _isUpdatingFromCode = false;
        }

        private void rangeSlider_ValueChanged(object sender, EventArgs e)
        {
            if (_isUpdatingFromCode) return;
            _isUpdatingFromCode = true;

            uint startAddr = (uint)rangeSlider.Value1 * SLIDER_GRANULARITY;
            uint endAddr = (uint)rangeSlider.Value2 * SLIDER_GRANULARITY;

            txtStaticStart.Text = startAddr.ToString("X");
            txtStaticEnd.Text = endAddr.ToString("X");
            lblSliderRange.Text = $"{startAddr:X6} - {endAddr:X6}";

            _isUpdatingFromCode = false;
        }

        private void rangeSlider_DoubleClick(object sender, EventArgs e)
        {
            if (ModifierKeys == Keys.Control)
            {
                // Reset the textboxes to the application's core defaults
                txtStaticStart.Text = _defaultSettings.StaticAddressStart;
                txtStaticEnd.Text = _defaultSettings.StaticAddressEnd;
            }
        }
    }
}