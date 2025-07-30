using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.Windows.Forms;

namespace PointerFinder2
{
    // A simple form to allow the user to toggle debug logging settings at runtime.
    public partial class DebugOptionsForm : Form
    {
        public DebugOptionsForm()
        {
            InitializeComponent();
        }

        // When the form loads, populate the checkboxes with the current settings from the static DebugSettings class.
        private void DebugOptionsForm_Load(object sender, EventArgs e)
        {
            chkLogLiveScan.Checked = DebugSettings.LogLiveScan;
            chkLogFilter.Checked = DebugSettings.LogFilterValidation;
            chkLogRefineScan.Checked = DebugSettings.LogRefineScan;
        }

        // A helper method to persist the current state of the debug settings to the INI file.
        private void SaveDebugSettings()
        {
            // We only need to save the debug section, not the entire settings file.
            SettingsManager.SaveDebugSettingsOnly();
        }

        // Updates the LogLiveScan setting when the checkbox state changes.
        private void chkLogLiveScan_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogLiveScan = chkLogLiveScan.Checked;
            SaveDebugSettings();
        }

        // Updates the LogFilterValidation setting when the checkbox state changes.
        private void chkLogFilter_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogFilterValidation = chkLogFilter.Checked;
            SaveDebugSettings();
        }

        // Updates the LogRefineScan setting when the checkbox state changes.
        private void chkLogRefineScan_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogRefineScan = chkLogRefineScan.Checked;
            SaveDebugSettings();
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}