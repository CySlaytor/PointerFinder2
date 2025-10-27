using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.Globalization;
using System.Text;
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
            _target = EmulatorProfileRegistry.Profiles.Find(p => p.Name.StartsWith(manager.EmulatorName.Split(' ')[0])).Target;
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
                // Added Dolphin target system name.
                case EmulatorTarget.Dolphin: targetSystem = "GC/Wii"; break;
                default: targetSystem = "Mem"; break;
            }
            label1.Text = $"Target Address ({targetSystem}, Hex)";
            groupBoxRange.Text = $"Static Base Address Range ({targetSystem}, Hex)";

            // Show PCSX2-specific options only when the target is PCSX2.
            bool isPcsx2 = (_target == EmulatorTarget.PCSX2);
            chkUse16ByteAlignment.Visible = isPcsx2;


            // --- Populate controls with current settings ---
            txtTargetAddress.Text = _currentSettings.LastTargetAddress;
            txtMaxOffset.Text = _currentSettings.MaxOffset.ToString("X");
            numMaxLevel.Value = _currentSettings.MaxLevel;
            numMaxResults.Value = _currentSettings.MaxResults;
            txtStaticStart.Text = _currentSettings.StaticAddressStart;
            txtStaticEnd.Text = _currentSettings.StaticAddressEnd;
            chkScanForStructureBase.Checked = _currentSettings.ScanForStructureBase;
            txtMaxNegativeOffset.Text = _currentSettings.MaxNegativeOffset.ToString("X");
            txtMaxNegativeOffset.Enabled = chkScanForStructureBase.Checked;
            // Populate PCSX2-specific controls
            if (isPcsx2)
            {
                chkUse16ByteAlignment.Checked = _currentSettings.Use16ByteAlignment;
            }


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

            // Wire up Leave events for automatic hex input sanitization.
            txtTargetAddress.Leave += SanitizeHexTextBox_Leave;
            txtMaxOffset.Leave += SanitizeHexTextBox_Leave;
            txtStaticStart.Leave += SanitizeHexTextBox_Leave;
            txtStaticEnd.Leave += SanitizeHexTextBox_Leave;
            txtMaxNegativeOffset.Leave += SanitizeHexTextBox_Leave;
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
                // Sanitize all hex input fields before parsing to enforce a clean format.
                txtTargetAddress.Text = SanitizeHexInput(txtTargetAddress.Text);
                txtMaxOffset.Text = SanitizeHexInput(txtMaxOffset.Text);
                txtStaticStart.Text = SanitizeHexInput(txtStaticStart.Text);
                txtStaticEnd.Text = SanitizeHexInput(txtStaticEnd.Text);
                txtMaxNegativeOffset.Text = SanitizeHexInput(txtMaxNegativeOffset.Text);


                // This correctly uses the manager's UnnormalizeAddress to handle all cases.
                uint targetAddress = _manager.UnnormalizeAddress(txtTargetAddress.Text);

                if (targetAddress % 4 != 0)
                {
                    uint correctedAddress = targetAddress & 0xFFFFFFFC; // Round down to the nearest multiple of 4.
                    string message = $"The target address 0x{targetAddress:X} is not 4-byte aligned. Pointer scans operate on 32-bit (4-byte) values, so the target address must be a multiple of 4.\n\n" +
                                     "This can happen on systems like Wii/GameCube when reading an 8-bit value from a Big-Endian address.\n\n" +
                                     $"The address will be automatically corrected to: 0x{correctedAddress:X}\n\n" +
                                     "Click OK to apply this correction and continue.";

                    MessageBox.Show(this, message, "Address Alignment Correction", MessageBoxButtons.OK, MessageBoxIcon.Information);

                    targetAddress = correctedAddress;
                    // Update the UI to reflect the corrected address.
                    txtTargetAddress.Text = _manager.FormatDisplayAddress(targetAddress);
                }

                uint staticStart = _manager.UnnormalizeAddress(txtStaticStart.Text);
                uint staticEnd = _manager.UnnormalizeAddress(txtStaticEnd.Text);

                // --- SAFETY CHECK ---
                // A dynamic target address should never fall within the static memory range where the search originates.
                if (targetAddress >= staticStart && targetAddress <= staticEnd)
                {
                    MessageBox.Show(
                        $"The Target Address (0x{_manager.FormatDisplayAddress(targetAddress)}) falls within the specified Static Base Address Range (0x{_manager.FormatDisplayAddress(staticStart)} - 0x{_manager.FormatDisplayAddress(staticEnd)}).\n\n" +
                        "A target address cannot be inside the static range where the search begins.\n\n" +
                        "Please adjust the static range to not include the target address.",
                        "Invalid Range",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                    return null; // Prevent the scan from starting.
                }

                if (DebugSettings.LogLiveScan) DebugLogForm.Instance.Log($"Parsed Scan Parameters: Target={targetAddress:X8}, StaticRange={staticStart:X8}-{staticEnd:X8}");

                return new ScanParameters
                {
                    TargetAddress = targetAddress,
                    MaxOffset = int.Parse(txtMaxOffset.Text, NumberStyles.HexNumber),
                    MaxLevel = (int)numMaxLevel.Value,
                    MaxResults = (int)numMaxResults.Value,
                    StaticBaseStart = staticStart,
                    StaticBaseEnd = staticEnd,
                    ScanForStructureBase = chkScanForStructureBase.Checked,
                    MaxNegativeOffset = int.Parse(txtMaxNegativeOffset.Text, NumberStyles.HexNumber),
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

        // This method now updates the existing settings object instead of creating a new one.
        public void UpdateSettings(AppSettings settings)
        {
            // Sanitize values before saving them back to settings.
            settings.LastTargetAddress = SanitizeHexInput(txtTargetAddress.Text);
            settings.MaxOffset = int.Parse(SanitizeHexInput(txtMaxOffset.Text), NumberStyles.HexNumber);
            settings.MaxLevel = (int)numMaxLevel.Value;
            settings.MaxResults = (int)numMaxResults.Value;
            settings.StaticAddressStart = SanitizeHexInput(txtStaticStart.Text);
            settings.StaticAddressEnd = SanitizeHexInput(txtStaticEnd.Text);
            settings.UseSliderRange = chkUseSliderRange.Checked;
            settings.ScanForStructureBase = chkScanForStructureBase.Checked;
            settings.MaxNegativeOffset = int.Parse(SanitizeHexInput(txtMaxNegativeOffset.Text), NumberStyles.HexNumber);
            settings.Use16ByteAlignment = chkUse16ByteAlignment.Checked;
        }


        // The OK button validates the input and closes the form if valid.
        private void btnOK_Click(object sender, EventArgs e)
        {
            // Try to parse values for the performance check first.
            if (!int.TryParse(SanitizeHexInput(txtMaxOffset.Text), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int maxOffset))
            {
                // If parsing fails, let GetScanParameters show the detailed error.
                if (GetScanParameters() != null) { DialogResult = DialogResult.OK; Close(); }
                return;
            }
            int maxLevel = (int)numMaxLevel.Value;

            string warningMessage = "";

            // Dolphin (Wii/GC) Warning
            if (_target == EmulatorTarget.Dolphin)
            {
                if (maxOffset >= 0xFFFF && maxLevel >= 3)
                {
                    warningMessage = "A Max Offset of 0xFFFF or higher with 3+ levels on this console will be extremely slow and may generate millions of useless results.\n\nFor such a broad search, using the State-Based Scan method is strongly recommended.\n\nDo you want to continue with this Live Scan anyway?";
                }
                else if (maxOffset >= 0xFFF && maxLevel >= 4)
                {
                    warningMessage = "A Max Offset of 0xFFF or higher with 4+ levels on this console may be very slow and produce a large number of results.\n\nFor broad searches, the State-Based Scan method is often more effective.\n\nDo you want to continue with this Live Scan?";
                }
            }
            // PCSX2 (PS2) Warning
            else if (_target == EmulatorTarget.PCSX2)
            {
                if (maxOffset >= 0xFFF && maxLevel >= 5)
                {
                    warningMessage = "A Max Offset of 0xFFF or higher with 5+ levels on PS2 can be very slow and may generate millions of results.\n\nFor such deep searches, consider using the State-Based Scan method for better performance and accuracy.\n\nDo you want to continue with this Live Scan?";
                }
            }

            if (!string.IsNullOrEmpty(warningMessage))
            {
                var result = MessageBox.Show(warningMessage, "Performance Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                if (result == DialogResult.No)
                {
                    return; // User cancelled, keep the form open.
                }
            }

            // If we passed the checks (or user clicked Yes), proceed with the normal validation and close.
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
                uint startAddr = uint.Parse(SanitizeHexInput(txtStaticStart.Text), NumberStyles.HexNumber);
                uint endAddr = uint.Parse(SanitizeHexInput(txtStaticEnd.Text), NumberStyles.HexNumber);
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

        // Added a helper method to sanitize hex strings by removing non-hex characters and converting to uppercase.
        private string SanitizeHexInput(string input)
        {
            if (string.IsNullOrWhiteSpace(input)) return string.Empty;
            var sb = new StringBuilder();
            foreach (char c in input.ToUpperInvariant())
            {
                if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
                {
                    sb.Append(c);
                }
            }
            return sb.ToString();
        }

        // Added a shared event handler to apply sanitization when a hex textbox loses focus.
        private void SanitizeHexTextBox_Leave(object sender, EventArgs e)
        {
            if (sender is TextBox tb)
            {
                tb.Text = SanitizeHexInput(tb.Text);
            }
        }
    }
}