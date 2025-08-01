using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Windows.Forms;

namespace PointerFinder2
{
    // The form for managing global application and debug settings.
    public partial class SettingsForm : Form
    {
        private readonly MainForm _mainForm;

        // The constructor now requires a reference to the MainForm
        // so it can call the public RestartApplication method.
        public SettingsForm(MainForm mainForm)
        {
            InitializeComponent();
            _mainForm = mainForm;
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

        // This button now triggers the smart self-restart to fully reset the application's memory.
        private void btnRestartApp_Click(object sender, EventArgs e)
        {
            var result = MessageBox.Show(
                "This will close and restart the application to reset its memory state. Your current emulator attachment will be preserved.\n\nAre you sure you want to continue?",
                "Confirm Application Restart",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning);

            if (result == DialogResult.Yes)
            {
                _mainForm?.RestartApplication();
            }
        }

        #region Settings Event Handlers
        private void chkUseDefaultSounds_CheckedChanged(object sender, EventArgs e)
        {
            GlobalSettings.UseWindowsDefaultSound = chkUseDefaultSounds.Checked;
            SettingsManager.SaveGlobalSettingsOnly();
        }

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
        #endregion
    }
}