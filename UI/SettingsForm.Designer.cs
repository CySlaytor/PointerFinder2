namespace PointerFinder2
{
    partial class SettingsForm
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.tabControlSettings = new System.Windows.Forms.TabControl();
            this.tabGeneral = new System.Windows.Forms.TabPage();
            this.grpPerformance = new System.Windows.Forms.GroupBox();
            this.chkLimitCpuUsage = new System.Windows.Forms.CheckBox();
            this.grpSound = new System.Windows.Forms.GroupBox();
            this.chkUseDefaultSounds = new System.Windows.Forms.CheckBox();
            this.tabDebug = new System.Windows.Forms.TabPage();
            this.grpLogging = new System.Windows.Forms.GroupBox();
            this.chkLogStateScanDetails = new System.Windows.Forms.CheckBox();
            this.chkLogRefineScan = new System.Windows.Forms.CheckBox();
            this.chkLogFilter = new System.Windows.Forms.CheckBox();
            this.chkLogLiveScan = new System.Windows.Forms.CheckBox();
            this.btnClose = new System.Windows.Forms.Button();
            this.btnRestartApp = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.btnPurgeMemory = new System.Windows.Forms.Button();
            this.tabControlSettings.SuspendLayout();
            this.tabGeneral.SuspendLayout();
            this.grpPerformance.SuspendLayout();
            this.grpSound.SuspendLayout();
            this.tabDebug.SuspendLayout();
            this.grpLogging.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControlSettings
            // 
            this.tabControlSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabControlSettings.Controls.Add(this.tabGeneral);
            this.tabControlSettings.Controls.Add(this.tabDebug);
            this.tabControlSettings.Location = new System.Drawing.Point(12, 12);
            this.tabControlSettings.Name = "tabControlSettings";
            this.tabControlSettings.SelectedIndex = 0;
            this.tabControlSettings.Size = new System.Drawing.Size(320, 203);
            this.tabControlSettings.TabIndex = 0;
            // 
            // tabGeneral
            // 
            this.tabGeneral.Controls.Add(this.grpPerformance);
            this.tabGeneral.Controls.Add(this.grpSound);
            this.tabGeneral.Location = new System.Drawing.Point(4, 24);
            this.tabGeneral.Name = "tabGeneral";
            this.tabGeneral.Padding = new System.Windows.Forms.Padding(3);
            this.tabGeneral.Size = new System.Drawing.Size(312, 175);
            this.tabGeneral.TabIndex = 0;
            this.tabGeneral.Text = "General";
            this.tabGeneral.UseVisualStyleBackColor = true;
            // 
            // grpPerformance
            // 
            this.grpPerformance.Controls.Add(this.chkLimitCpuUsage);
            this.grpPerformance.Location = new System.Drawing.Point(6, 77);
            this.grpPerformance.Name = "grpPerformance";
            this.grpPerformance.Size = new System.Drawing.Size(299, 65);
            this.grpPerformance.TabIndex = 1;
            this.grpPerformance.TabStop = false;
            this.grpPerformance.Text = "Performance";
            // 
            // chkLimitCpuUsage
            // 
            this.chkLimitCpuUsage.AutoSize = true;
            this.chkLimitCpuUsage.Location = new System.Drawing.Point(16, 28);
            this.chkLimitCpuUsage.Name = "chkLimitCpuUsage";
            this.chkLimitCpuUsage.Size = new System.Drawing.Size(206, 19);
            this.chkLimitCpuUsage.TabIndex = 0;
            this.chkLimitCpuUsage.Text = "Limit CPU Usage (approx. 50%)";
            this.toolTip1.SetToolTip(this.chkLimitCpuUsage, "Reduces the number of CPU cores used during the scan.\r\nHelpful for multitasking," +
        " but the scan will take longer.");
            this.chkLimitCpuUsage.UseVisualStyleBackColor = true;
            this.chkLimitCpuUsage.CheckedChanged += new System.EventHandler(this.chkLimitCpuUsage_CheckedChanged);
            // 
            // grpSound
            // 
            this.grpSound.Controls.Add(this.chkUseDefaultSounds);
            this.grpSound.Location = new System.Drawing.Point(6, 6);
            this.grpSound.Name = "grpSound";
            this.grpSound.Size = new System.Drawing.Size(299, 65);
            this.grpSound.TabIndex = 0;
            this.grpSound.TabStop = false;
            this.grpSound.Text = "Sound";
            // 
            // chkUseDefaultSounds
            // 
            this.chkUseDefaultSounds.AutoSize = true;
            this.chkUseDefaultSounds.Location = new System.Drawing.Point(16, 28);
            this.chkUseDefaultSounds.Name = "chkUseDefaultSounds";
            this.chkUseDefaultSounds.Size = new System.Drawing.Size(217, 19);
            this.chkUseDefaultSounds.TabIndex = 0;
            this.chkUseDefaultSounds.Text = "Use Windows default notification sound";
            this.chkUseDefaultSounds.UseVisualStyleBackColor = true;
            this.chkUseDefaultSounds.CheckedChanged += new System.EventHandler(this.chkUseDefaultSounds_CheckedChanged);
            // 
            // tabDebug
            // 
            this.tabDebug.Controls.Add(this.grpLogging);
            this.tabDebug.Location = new System.Drawing.Point(4, 24);
            this.tabDebug.Name = "tabDebug";
            this.tabDebug.Padding = new System.Windows.Forms.Padding(3);
            this.tabDebug.Size = new System.Drawing.Size(312, 175);
            this.tabDebug.TabIndex = 1;
            this.tabDebug.Text = "Debug";
            this.tabDebug.UseVisualStyleBackColor = true;
            // 
            // grpLogging
            // 
            this.grpLogging.Controls.Add(this.chkLogStateScanDetails);
            this.grpLogging.Controls.Add(this.chkLogRefineScan);
            this.grpLogging.Controls.Add(this.chkLogFilter);
            this.grpLogging.Controls.Add(this.chkLogLiveScan);
            this.grpLogging.Location = new System.Drawing.Point(6, 6);
            this.grpLogging.Name = "grpLogging";
            this.grpLogging.Size = new System.Drawing.Size(299, 154);
            this.grpLogging.TabIndex = 1;
            this.grpLogging.TabStop = false;
            this.grpLogging.Text = "Logging and Debug Toggles";
            // 
            // chkLogStateScanDetails
            // 
            this.chkLogStateScanDetails.AutoSize = true;
            this.chkLogStateScanDetails.Location = new System.Drawing.Point(18, 110);
            this.chkLogStateScanDetails.Name = "chkLogStateScanDetails";
            this.chkLogStateScanDetails.Size = new System.Drawing.Size(199, 19);
            this.chkLogStateScanDetails.TabIndex = 3;
            this.chkLogStateScanDetails.Text = "Log State-Based Scan Details";
            this.toolTip1.SetToolTip(this.chkLogStateScanDetails, "Outputs an extremely verbose log of the state-based scan validation process.");
            this.chkLogStateScanDetails.UseVisualStyleBackColor = true;
            this.chkLogStateScanDetails.CheckedChanged += new System.EventHandler(this.chkLogStateScanDetails_CheckedChanged);
            // 
            // chkLogRefineScan
            // 
            this.chkLogRefineScan.AutoSize = true;
            this.chkLogRefineScan.Location = new System.Drawing.Point(18, 83);
            this.chkLogRefineScan.Name = "chkLogRefineScan";
            this.chkLogRefineScan.Size = new System.Drawing.Size(201, 19);
            this.chkLogRefineScan.TabIndex = 2;
            this.chkLogRefineScan.Text = "Log Refine Scan (Intersection)";
            this.chkLogRefineScan.UseVisualStyleBackColor = true;
            this.chkLogRefineScan.CheckedChanged += new System.EventHandler(this.chkLogRefineScan_CheckedChanged);
            // 
            // chkLogFilter
            // 
            this.chkLogFilter.AutoSize = true;
            this.chkLogFilter.Location = new System.Drawing.Point(18, 56);
            this.chkLogFilter.Name = "chkLogFilter";
            this.chkLogFilter.Size = new System.Drawing.Size(147, 19);
            this.chkLogFilter.TabIndex = 1;
            this.chkLogFilter.Text = "Log Live Path Filtering";
            this.chkLogFilter.UseVisualStyleBackColor = true;
            this.chkLogFilter.CheckedChanged += new System.EventHandler(this.chkLogFilter_CheckedChanged);
            // 
            // chkLogLiveScan
            // 
            this.chkLogLiveScan.AutoSize = true;
            this.chkLogLiveScan.Location = new System.Drawing.Point(18, 29);
            this.chkLogLiveScan.Name = "chkLogLiveScan";
            this.chkLogLiveScan.Size = new System.Drawing.Size(107, 19);
            this.chkLogLiveScan.TabIndex = 0;
            this.chkLogLiveScan.Text = "Log Live Scan";
            this.chkLogLiveScan.UseVisualStyleBackColor = true;
            this.chkLogLiveScan.CheckedChanged += new System.EventHandler(this.chkLogLiveScan_CheckedChanged);
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnClose.Location = new System.Drawing.Point(245, 221);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(87, 28);
            this.btnClose.TabIndex = 2;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // btnRestartApp
            // 
            this.btnRestartApp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnRestartApp.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            this.btnRestartApp.ForeColor = System.Drawing.Color.DarkRed;
            this.btnRestartApp.Location = new System.Drawing.Point(12, 221);
            this.btnRestartApp.Name = "btnRestartApp";
            this.btnRestartApp.Size = new System.Drawing.Size(130, 28);
            this.btnRestartApp.TabIndex = 3;
            this.btnRestartApp.Text = "Restart Application";
            this.toolTip1.SetToolTip(this.btnRestartApp, "Closes and re-opens the application to completely reset its memory state.");
            this.btnRestartApp.UseVisualStyleBackColor = true;
            this.btnRestartApp.Click += new System.EventHandler(this.btnRestartApp_Click);
            // 
            // btnPurgeMemory
            // 
            this.btnPurgeMemory.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnPurgeMemory.Location = new System.Drawing.Point(148, 221);
            this.btnPurgeMemory.Name = "btnPurgeMemory";
            this.btnPurgeMemory.Size = new System.Drawing.Size(91, 28);
            this.btnPurgeMemory.TabIndex = 4;
            this.btnPurgeMemory.Text = "Purge Memory";
            this.toolTip1.SetToolTip(this.btnPurgeMemory, "Forces the application to release unused RAM back to the operating system.");
            this.btnPurgeMemory.UseVisualStyleBackColor = true;
            this.btnPurgeMemory.Click += new System.EventHandler(this.btnPurgeMemory_Click);
            // 
            // SettingsForm
            // 
            this.AcceptButton = this.btnClose;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(344, 261);
            this.Controls.Add(this.btnPurgeMemory);
            this.Controls.Add(this.btnRestartApp);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.tabControlSettings);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "SettingsForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Settings";
            this.Load += new System.EventHandler(this.SettingsForm_Load);
            this.tabControlSettings.ResumeLayout(false);
            this.tabGeneral.ResumeLayout(false);
            this.grpPerformance.ResumeLayout(false);
            this.grpPerformance.PerformLayout();
            this.grpSound.ResumeLayout(false);
            this.grpSound.PerformLayout();
            this.tabDebug.ResumeLayout(false);
            this.grpLogging.ResumeLayout(false);
            this.grpLogging.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControlSettings;
        private System.Windows.Forms.TabPage tabGeneral;
        private System.Windows.Forms.TabPage tabDebug;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.GroupBox grpLogging;
        private System.Windows.Forms.CheckBox chkLogRefineScan;
        private System.Windows.Forms.CheckBox chkLogFilter;
        private System.Windows.Forms.CheckBox chkLogLiveScan;
        private System.Windows.Forms.GroupBox grpSound;
        private System.Windows.Forms.CheckBox chkUseDefaultSounds;
        private System.Windows.Forms.Button btnRestartApp;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.Button btnPurgeMemory;
        private System.Windows.Forms.GroupBox grpPerformance;
        private System.Windows.Forms.CheckBox chkLimitCpuUsage;
        private System.Windows.Forms.CheckBox chkLogStateScanDetails;
    }
}