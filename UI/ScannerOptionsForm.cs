using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.Globalization;
using System.Windows.Forms;

namespace PointerFinder2
{
    public partial class ScannerOptionsForm : Form
    {
        private readonly EmulatorTarget _target;
        private readonly IEmulatorManager _manager;
        private readonly AppSettings _currentSettings;
        public ScannerOptionsForm(IEmulatorManager manager, AppSettings settings)
        {
            InitializeComponent();
            _manager = manager;
            _target = EmulatorProfileRegistry.Profiles.Find(p => p.Name == manager.EmulatorName).Target;
            _currentSettings = settings;
        }

        private void ScannerOptionsForm_Load(object sender, EventArgs e)
        {
            var logger = DebugLogForm.Instance;
            logger.Log("ScannerOptionsForm loading...");

            // --- Configure UI based on Emulator Target ---
            this.Text = $"{_manager.EmulatorName} Scan Options";
            string targetSystem = _target == EmulatorTarget.PCSX2 ? "PS2" : "PS1";
            label1.Text = $"Target Address ({targetSystem}, Hex)";
            groupBox1.Text = $"Static Base Address Range ({targetSystem}, Hex)";

            // Hide 16-byte alignment option for DuckStation as it's not applicable
            chkUse16ByteAlignment.Visible = (_target == EmulatorTarget.PCSX2);
            logger.Log($"UI configured for {_manager.EmulatorName}. 16-byte alignment visible: {chkUse16ByteAlignment.Visible}");

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
            logger.Log("Controls populated with current settings.");
        }

        // This method is now public so MainForm can call it after OK is clicked
        public ScanParameters GetScanParameters()
        {
            try
            {
                // Use the manager's UnnormalizeAddress for the target
                uint targetAddress = _manager.UnnormalizeAddress(txtTargetAddress.Text);
                // And for the static range
                uint staticStart = _manager.UnnormalizeAddress(txtStaticStart.Text);
                uint staticEnd = _manager.UnnormalizeAddress(txtStaticEnd.Text);

                DebugLogForm.Instance.Log($"Parsed Scan Parameters: Target={targetAddress:X8}, StaticRange={staticStart:X8}-{staticEnd:X8}");

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
                    Use16ByteAlignment = chkUse16ByteAlignment.Checked
                };
            }
            catch (Exception ex)
            {
                MessageBox.Show("Invalid input: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                return null;
            }
        }

        // This method is now public so MainForm can call it to save the settings
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
                AnalyzeStructures = chkAnalyzeStructures.Checked,
                ScanForStructureBase = chkScanForStructureBase.Checked,
                MaxNegativeOffset = int.Parse(txtMaxNegativeOffset.Text.Replace("0x", ""), NumberStyles.HexNumber),
                Use16ByteAlignment = chkUse16ByteAlignment.Checked
            };
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            // The OK button just validates and closes. The MainForm will handle getting the parameters and saving.
            if (GetScanParameters() != null)
            {
                DialogResult = DialogResult.OK;
                Close();
            }
        }

        private void chkScanForStructureBase_CheckedChanged(object sender, EventArgs e)
        {
            txtMaxNegativeOffset.Enabled = chkScanForStructureBase.Checked;
        }
    }
}