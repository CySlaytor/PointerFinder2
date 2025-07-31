using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Windows.Forms;

namespace PointerFinder2
{
    public partial class SettingsForm : Form
    {
        public SettingsForm()
        {
            InitializeComponent();
        }

        private void SettingsForm_Load(object sender, EventArgs e)
        {
            // --- Load General Settings ---
            chkUseDefaultSounds.Checked = GlobalSettings.UseWindowsDefaultSound;

            // --- Load Debug Settings ---
            chkLogLiveScan.Checked = DebugSettings.LogLiveScan;
            chkLogFilter.Checked = DebugSettings.LogFilterValidation;
            chkLogRefineScan.Checked = DebugSettings.LogRefineScan;
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            Close();
        }

        // --- General Settings Event Handlers ---
        private void chkUseDefaultSounds_CheckedChanged(object sender, EventArgs e)
        {
            GlobalSettings.UseWindowsDefaultSound = chkUseDefaultSounds.Checked;
            SettingsManager.SaveGlobalSettingsOnly();
        }

        // --- Debug Settings Event Handlers ---
        private void chkLogLiveScan_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogLiveScan = chkLogLiveScan.Checked;
            SettingsManager.SaveDebugSettingsOnly();
        }

        private void chkLogFilter_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogFilterValidation = chkLogFilter.Checked;
            SettingsManager.SaveDebugSettingsOnly();
        }

        private void chkLogRefineScan_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogRefineScan = chkLogRefineScan.Checked;
            SettingsManager.SaveDebugSettingsOnly();
        }
    }
}