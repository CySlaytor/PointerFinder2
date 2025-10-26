using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace PointerFinder2
{
    // The form for managing global application and debug settings.
    public partial class SettingsForm : Form
    {
        private readonly MainForm _mainForm;
        //Add a flag to prevent event handlers from running during form initialization.
        private bool _isInitializing = true;

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
            chkLimitCpuUsage.Checked = GlobalSettings.LimitCpuUsage;

            // --- Load Debug Settings ---
            chkLogLiveScan.Checked = DebugSettings.LogLiveScan;
            chkLogFilter.Checked = DebugSettings.LogFilterValidation;
            chkLogRefineScan.Checked = DebugSettings.LogRefineScan;
            chkLogStateScanDetails.Checked = DebugSettings.LogStateBasedScanDetails;

            // Load Code Note settings
            txtPrefix.Text = GlobalSettings.CodeNotePrefix;
            txtSuffix.Text = GlobalSettings.CodeNoteSuffix;
            chkAlign.Checked = GlobalSettings.CodeNoteAlignSuffixes;
            chkSuffixOnLastLine.Checked = GlobalSettings.CodeNoteSuffixOnLastLineOnly;

            UpdateCodeNotePreview();

            // All controls have been populated, so now we can allow event handlers to run.
            _isInitializing = false;
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

        // Event handler for the new "Purge Memory" button.
        private void btnPurgeMemory_Click(object sender, EventArgs e)
        {
            // Call the public PurgeMemory method on the MainForm instance.
            _mainForm?.PurgeMemory();
            MessageBox.Show(
                "Application memory has been purged.\n\nYou should see the RAM usage drop in Task Manager.",
                "Success",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

        #region Settings Event Handlers
        //Add guard clause to all event handlers to prevent them from firing during Load.
        private void chkUseDefaultSounds_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            GlobalSettings.UseWindowsDefaultSound = chkUseDefaultSounds.Checked;
            SettingsManager.SaveGlobalSettingsOnly();
        }

        private void chkLimitCpuUsage_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            GlobalSettings.LimitCpuUsage = chkLimitCpuUsage.Checked;
            SettingsManager.SaveGlobalSettingsOnly();
        }

        private void chkLogLiveScan_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            DebugSettings.LogLiveScan = chkLogLiveScan.Checked;
            SettingsManager.SaveDebugSettingsOnly();
        }

        private void chkLogFilter_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            DebugSettings.LogFilterValidation = chkLogFilter.Checked;
            SettingsManager.SaveDebugSettingsOnly();
        }

        private void chkLogRefineScan_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            DebugSettings.LogRefineScan = chkLogRefineScan.Checked;
            SettingsManager.SaveDebugSettingsOnly();
        }

        private void chkLogStateScanDetails_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            DebugSettings.LogStateBasedScanDetails = chkLogStateScanDetails.Checked;
            SettingsManager.SaveDebugSettingsOnly();
        }
        #endregion

        // Add event handlers and preview logic for Code Notes tab
        private void CodeNoteSetting_Changed(object sender, EventArgs e)
        {
            // Add guard clause.
            if (_isInitializing) return;

            // Update Global Settings from UI controls
            GlobalSettings.CodeNotePrefix = txtPrefix.Text;
            GlobalSettings.CodeNoteSuffix = txtSuffix.Text;
            GlobalSettings.CodeNoteAlignSuffixes = chkAlign.Checked;
            GlobalSettings.CodeNoteSuffixOnLastLineOnly = chkSuffixOnLastLine.Checked;

            SettingsManager.SaveGlobalSettingsOnly();
            UpdateCodeNotePreview();
        }

        private void UpdateCodeNotePreview()
        {
            var settings = new CodeNoteSettings
            {
                Prefix = txtPrefix.Text,
                Suffix = txtSuffix.Text,
                AlignSuffixes = chkAlign.Checked,
                SuffixOnLastLineOnly = chkSuffixOnLastLine.Checked,
            };

            var dummyOffsets = new List<int> { 0x4, 0x2A0, -0x1C };
            // Add a dummy description to the last line of the preview for context.
            richPreview.Text = CodeNoteHelper.BuildCodeNote(dummyOffsets, settings, "8-bit", "Description");
        }
    }
}