using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Properties;
using PointerFinder2.UI;
using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.Button;

namespace PointerFinder2
{
    // The form for managing global application and debug settings.
    public partial class SettingsForm : BaseForm
    {
        private readonly MainForm _mainForm;
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
            _isInitializing = true;
            CheckDarkModeEnabled.Checked = Properties.Settings.Default.DarkModeEnabled;
            chkUseDefaultSounds.Checked = GlobalSettings.UseWindowsDefaultSound;
            chkLimitCpuUsage.Checked = GlobalSettings.LimitCpuUsage;
            chkSortByLevelFirst.Checked = GlobalSettings.SortByLevelFirst;
            chkLogLiveScan.Checked = DebugSettings.LogLiveScan;
            chkLogFilter.Checked = DebugSettings.LogFilterValidation;
            chkLogRefineScan.Checked = DebugSettings.LogRefineScan;
            chkLogStateScanDetails.Checked = DebugSettings.LogStateBasedScanDetails;
            txtPrefix.Text = GlobalSettings.CodeNotePrefix;
            txtSuffix.Text = GlobalSettings.CodeNoteSuffix;
            chkAlign.Checked = GlobalSettings.CodeNoteAlignSuffixes;
            chkSuffixOnLastLine.Checked = GlobalSettings.CodeNoteSuffixOnLastLineOnly;
            UpdateCodeNotePreview();
            _isInitializing = false;
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            Close();
        }

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

        private void btnPurgeMemory_Click(object sender, EventArgs e)
        {
            _mainForm?.PurgeMemory();
            MessageBox.Show(
                "Application memory has been purged.\n\nYou should see the RAM usage drop in Task Manager.",
                "Success",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

        private void btnResetAll_Click(object sender, EventArgs e)
        {
            var result = MessageBox.Show(
                "This will reset ALL application settings, including saved scan parameters, window positions, and debug flags. The settings.ini file will be deleted.\n\n" +
                "The application will restart to apply the default settings.\n\n" +
                "Are you sure you want to proceed?",
                "Confirm Reset All Settings",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning,
                MessageBoxDefaultButton.Button2);

            if (result == DialogResult.Yes)
            {
                try
                {
                    Settings.Default.Reset();
                    Settings.Default.Save();

                    string settingsFile = Path.Combine(Application.StartupPath, "settings.ini");
                    if (File.Exists(settingsFile))
                    {
                        File.Delete(settingsFile);
                    }

                    MessageBox.Show("All settings have been reset to default. The application will now restart.", "Settings Reset", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    _mainForm?.RestartApplication();
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Failed to reset settings: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        #region Settings Event Handlers
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

        private void chkSortByLevelFirst_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            GlobalSettings.SortByLevelFirst = chkSortByLevelFirst.Checked;
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

        private void CodeNoteSetting_Changed(object sender, EventArgs e)
        {
            if (_isInitializing) return;

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
            richPreview.Text = CodeNoteHelper.BuildCodeNote(dummyOffsets, settings, "8-bit", "Description");
        }

        private void grpPerformance_Enter(object sender, EventArgs e)
        {

        }

        private void CheckDarkModeEnabled_CheckedChanged(object sender, EventArgs e)
        {
            if (_isInitializing) return;
            Properties.Settings.Default.DarkModeEnabled = CheckDarkModeEnabled.Checked;
            Properties.Settings.Default.Save();

            // Restart aplikacji, żeby wszystkie formy odczytały nowe ustawienie
            Application.Restart();
            Environment.Exit(0);
        }






        private void groupBox3_Enter(object sender, EventArgs e)
        {

        }

        private void tabGeneral_Click(object sender, EventArgs e)
        {

        }

        private void grpSorting_Enter(object sender, EventArgs e)
        {

        }

        private void groupBox1_Enter(object sender, EventArgs e)
        {

        }

        private void grpSound_Enter(object sender, EventArgs e)
        {

        }

        private void tabDebug_Click(object sender, EventArgs e)
        {

        }

        private void grpLogging_Enter(object sender, EventArgs e)
        {

        }
    }
}
