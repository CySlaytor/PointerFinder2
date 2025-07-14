using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.Windows.Forms;

namespace PointerFinder2
{
    public partial class DebugOptionsForm : Form
    {
        public DebugOptionsForm()
        {
            InitializeComponent();
        }

        private void DebugOptionsForm_Load(object sender, EventArgs e)
        {
            chkLogLiveScan.Checked = DebugSettings.LogLiveScan;
            chkLogFilter.Checked = DebugSettings.LogFilterValidation;
            chkLogRefineScan.Checked = DebugSettings.LogRefineScan;
        }

        private void SaveDebugSettings()
        {
            SettingsManager.SaveDebugSettingsOnly();
        }

        private void chkLogLiveScan_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogLiveScan = chkLogLiveScan.Checked;
            SaveDebugSettings();
        }

        private void chkLogFilter_CheckedChanged(object sender, EventArgs e)
        {
            DebugSettings.LogFilterValidation = chkLogFilter.Checked;
            SaveDebugSettings();
        }

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