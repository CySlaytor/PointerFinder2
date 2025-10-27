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
            this.grpSorting = new System.Windows.Forms.GroupBox();
            this.chkSortByLevelFirst = new System.Windows.Forms.CheckBox();
            this.grpDangerZone = new System.Windows.Forms.GroupBox();
            this.btnRestartApp = new System.Windows.Forms.Button();
            this.btnResetAll = new System.Windows.Forms.Button();
            this.grpPerformance = new System.Windows.Forms.GroupBox();
            this.chkLimitCpuUsage = new System.Windows.Forms.CheckBox();
            this.grpSound = new System.Windows.Forms.GroupBox();
            this.chkUseDefaultSounds = new System.Windows.Forms.CheckBox();
            this.tabCodeNotes = new System.Windows.Forms.TabPage();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.richPreview = new System.Windows.Forms.RichTextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chkSuffixOnLastLine = new System.Windows.Forms.CheckBox();
            this.chkAlign = new System.Windows.Forms.CheckBox();
            this.txtSuffix = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.txtPrefix = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.tabDebug = new System.Windows.Forms.TabPage();
            this.grpLogging = new System.Windows.Forms.GroupBox();
            this.chkLogStateScanDetails = new System.Windows.Forms.CheckBox();
            this.chkLogRefineScan = new System.Windows.Forms.CheckBox();
            this.chkLogFilter = new System.Windows.Forms.CheckBox();
            this.chkLogLiveScan = new System.Windows.Forms.CheckBox();
            this.btnClose = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.btnPurgeMemory = new System.Windows.Forms.Button();
            this.tabControlSettings.SuspendLayout();
            this.tabGeneral.SuspendLayout();
            this.grpSorting.SuspendLayout();
            this.grpDangerZone.SuspendLayout();
            this.grpPerformance.SuspendLayout();
            this.grpSound.SuspendLayout();
            this.tabCodeNotes.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
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
            this.tabControlSettings.Controls.Add(this.tabCodeNotes);
            this.tabControlSettings.Controls.Add(this.tabDebug);
            this.tabControlSettings.Location = new System.Drawing.Point(12, 12);
            this.tabControlSettings.Name = "tabControlSettings";
            this.tabControlSettings.SelectedIndex = 0;
            this.tabControlSettings.Size = new System.Drawing.Size(460, 310);
            this.tabControlSettings.TabIndex = 0;
            // 
            // tabGeneral
            // 
            this.tabGeneral.Controls.Add(this.grpSorting);
            this.tabGeneral.Controls.Add(this.grpDangerZone);
            this.tabGeneral.Controls.Add(this.grpPerformance);
            this.tabGeneral.Controls.Add(this.grpSound);
            this.tabGeneral.Location = new System.Drawing.Point(4, 24);
            this.tabGeneral.Name = "tabGeneral";
            this.tabGeneral.Padding = new System.Windows.Forms.Padding(3);
            this.tabGeneral.Size = new System.Drawing.Size(452, 282);
            this.tabGeneral.TabIndex = 0;
            this.tabGeneral.Text = "General";
            this.tabGeneral.UseVisualStyleBackColor = true;
            // 
            // grpSorting
            // 
            this.grpSorting.Controls.Add(this.chkSortByLevelFirst);
            this.grpSorting.Location = new System.Drawing.Point(6, 77);
            this.grpSorting.Name = "grpSorting";
            this.grpSorting.Size = new System.Drawing.Size(439, 65);
            this.grpSorting.TabIndex = 3;
            this.grpSorting.TabStop = false;
            this.grpSorting.Text = "Sorting";
            // 
            // chkSortByLevelFirst
            // 
            this.chkSortByLevelFirst.AutoSize = true;
            this.chkSortByLevelFirst.Location = new System.Drawing.Point(16, 28);
            this.chkSortByLevelFirst.Name = "chkSortByLevelFirst";
            this.chkSortByLevelFirst.Size = new System.Drawing.Size(326, 19);
            this.chkSortByLevelFirst.TabIndex = 0;
            this.chkSortByLevelFirst.Text = "Prioritize shorter chains when sorting by lowest offsets";
            this.toolTip1.SetToolTip(this.chkSortByLevelFirst, "When checked, results are sorted by the number of offsets first, then by the off" +
        "set values.\r\nWhen unchecked, results are sorted purely by the offset values reg" +
        "ardless of chain length.");
            this.chkSortByLevelFirst.UseVisualStyleBackColor = true;
            this.chkSortByLevelFirst.CheckedChanged += new System.EventHandler(this.chkSortByLevelFirst_CheckedChanged);
            // 
            // grpDangerZone
            // 
            this.grpDangerZone.Controls.Add(this.btnRestartApp);
            this.grpDangerZone.Controls.Add(this.btnResetAll);
            this.grpDangerZone.Location = new System.Drawing.Point(6, 219);
            this.grpDangerZone.Name = "grpDangerZone";
            this.grpDangerZone.Size = new System.Drawing.Size(439, 59);
            this.grpDangerZone.TabIndex = 2;
            this.grpDangerZone.TabStop = false;
            this.grpDangerZone.Text = "Danger Zone";
            // 
            // btnRestartApp
            // 
            this.btnRestartApp.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            this.btnRestartApp.ForeColor = System.Drawing.Color.DarkOrange;
            this.btnRestartApp.Location = new System.Drawing.Point(228, 22);
            this.btnRestartApp.Name = "btnRestartApp";
            this.btnRestartApp.Size = new System.Drawing.Size(130, 23);
            this.btnRestartApp.TabIndex = 3;
            this.btnRestartApp.Text = "Restart Application...";
            this.toolTip1.SetToolTip(this.btnRestartApp, "Closes and re-opens the application to completely reset its memory state.");
            this.btnRestartApp.UseVisualStyleBackColor = true;
            this.btnRestartApp.Click += new System.EventHandler(this.btnRestartApp_Click);
            // 
            // btnResetAll
            // 
            this.btnResetAll.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            this.btnResetAll.ForeColor = System.Drawing.Color.Red;
            this.btnResetAll.Location = new System.Drawing.Point(16, 22);
            this.btnResetAll.Name = "btnResetAll";
            this.btnResetAll.Size = new System.Drawing.Size(186, 23);
            this.btnResetAll.TabIndex = 0;
            this.btnResetAll.Text = "Reset All Settings to Default...";
            this.toolTip1.SetToolTip(this.btnResetAll, "Deletes settings.ini and resets all saved options. The application will restart." +
        "");
            this.btnResetAll.UseVisualStyleBackColor = true;
            this.btnResetAll.Click += new System.EventHandler(this.btnResetAll_Click);
            // 
            // grpPerformance
            // 
            this.grpPerformance.Controls.Add(this.chkLimitCpuUsage);
            this.grpPerformance.Location = new System.Drawing.Point(6, 148);
            this.grpPerformance.Name = "grpPerformance";
            this.grpPerformance.Size = new System.Drawing.Size(439, 65);
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
            this.grpSound.Size = new System.Drawing.Size(439, 65);
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
            // tabCodeNotes
            // 
            this.tabCodeNotes.Controls.Add(this.groupBox2);
            this.tabCodeNotes.Controls.Add(this.groupBox1);
            this.tabCodeNotes.Location = new System.Drawing.Point(4, 24);
            this.tabCodeNotes.Name = "tabCodeNotes";
            this.tabCodeNotes.Padding = new System.Windows.Forms.Padding(3);
            this.tabCodeNotes.Size = new System.Drawing.Size(452, 282);
            this.tabCodeNotes.TabIndex = 2;
            this.tabCodeNotes.Text = "Code Notes";
            this.tabCodeNotes.UseVisualStyleBackColor = true;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.richPreview);
            this.groupBox2.Location = new System.Drawing.Point(220, 6);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(226, 201);
            this.groupBox2.TabIndex = 1;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Live Preview";
            // 
            // richPreview
            // 
            this.richPreview.BackColor = System.Drawing.SystemColors.ControlLight;
            this.richPreview.Dock = System.Windows.Forms.DockStyle.Fill;
            this.richPreview.Font = new System.Drawing.Font("Consolas", 9F);
            this.richPreview.Location = new System.Drawing.Point(3, 19);
            this.richPreview.Name = "richPreview";
            this.richPreview.ReadOnly = true;
            this.richPreview.Size = new System.Drawing.Size(220, 179);
            this.richPreview.TabIndex = 0;
            this.richPreview.Text = "";
            this.richPreview.WordWrap = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chkSuffixOnLastLine);
            this.groupBox1.Controls.Add(this.chkAlign);
            this.groupBox1.Controls.Add(this.txtSuffix);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.txtPrefix);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(6, 6);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(208, 201);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Formatting Options";
            // 
            // chkSuffixOnLastLine
            // 
            this.chkSuffixOnLastLine.AutoSize = true;
            this.chkSuffixOnLastLine.Location = new System.Drawing.Point(9, 153);
            this.chkSuffixOnLastLine.Name = "chkSuffixOnLastLine";
            this.chkSuffixOnLastLine.Size = new System.Drawing.Size(167, 19);
            this.chkSuffixOnLastLine.TabIndex = 5;
            this.chkSuffixOnLastLine.Text = "Apply Suffix to Last Line Only";
            this.toolTip1.SetToolTip(this.chkSuffixOnLastLine, "If checked, the suffix is only applied to the final line of the pointer chain.");
            this.chkSuffixOnLastLine.UseVisualStyleBackColor = true;
            this.chkSuffixOnLastLine.CheckedChanged += new System.EventHandler(this.CodeNoteSetting_Changed);
            // 
            // chkAlign
            // 
            this.chkAlign.AutoSize = true;
            this.chkAlign.Location = new System.Drawing.Point(9, 128);
            this.chkAlign.Name = "chkAlign";
            this.chkAlign.Size = new System.Drawing.Size(142, 19);
            this.chkAlign.TabIndex = 4;
            this.chkAlign.Text = "Align Suffixes Vertically";
            this.toolTip1.SetToolTip(this.chkAlign, "Adds padding spaces so that all suffixes line up in a column.");
            this.chkAlign.UseVisualStyleBackColor = true;
            this.chkAlign.CheckedChanged += new System.EventHandler(this.CodeNoteSetting_Changed);
            // 
            // txtSuffix
            // 
            this.txtSuffix.Location = new System.Drawing.Point(9, 89);
            this.txtSuffix.Name = "txtSuffix";
            this.txtSuffix.Size = new System.Drawing.Size(180, 23);
            this.txtSuffix.TabIndex = 3;
            this.toolTip1.SetToolTip(this.txtSuffix, "The character(s) to add to the end of each line (e.g., \" |\" or \"=\").");
            this.txtSuffix.TextChanged += new System.EventHandler(this.CodeNoteSetting_Changed);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 71);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(64, 15);
            this.label2.TabIndex = 2;
            this.label2.Text = "Line Suffix:";
            // 
            // txtPrefix
            // 
            this.txtPrefix.Location = new System.Drawing.Point(9, 45);
            this.txtPrefix.Name = "txtPrefix";
            this.txtPrefix.Size = new System.Drawing.Size(180, 23);
            this.txtPrefix.TabIndex = 1;
            this.toolTip1.SetToolTip(this.txtPrefix, "The character(s) used for indentation. Will be repeated for each level.\r\nExample" +
        "s: . or + or -");
            this.txtPrefix.TextChanged += new System.EventHandler(this.CodeNoteSetting_Changed);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 27);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(107, 15);
            this.label1.TabIndex = 0;
            this.label1.Text = "Indentation Prefix:";
            // 
            // tabDebug
            // 
            this.tabDebug.Controls.Add(this.grpLogging);
            this.tabDebug.Location = new System.Drawing.Point(4, 24);
            this.tabDebug.Name = "tabDebug";
            this.tabDebug.Padding = new System.Windows.Forms.Padding(3);
            this.tabDebug.Size = new System.Drawing.Size(452, 282);
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
            this.grpLogging.Size = new System.Drawing.Size(439, 154);
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
            this.btnClose.Location = new System.Drawing.Point(385, 328);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(87, 28);
            this.btnClose.TabIndex = 2;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // btnPurgeMemory
            // 
            this.btnPurgeMemory.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnPurgeMemory.Location = new System.Drawing.Point(12, 328);
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
            this.ClientSize = new System.Drawing.Size(484, 368);
            this.Controls.Add(this.btnPurgeMemory);
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
            this.grpSorting.ResumeLayout(false);
            this.grpSorting.PerformLayout();
            this.grpDangerZone.ResumeLayout(false);
            this.grpPerformance.ResumeLayout(false);
            this.grpPerformance.PerformLayout();
            this.grpSound.ResumeLayout(false);
            this.grpSound.PerformLayout();
            this.tabCodeNotes.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
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
        private System.Windows.Forms.TabPage tabCodeNotes;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.RichTextBox richPreview;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.CheckBox chkAlign;
        private System.Windows.Forms.TextBox txtSuffix;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox txtPrefix;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox chkSuffixOnLastLine;
        private System.Windows.Forms.GroupBox grpDangerZone;
        private System.Windows.Forms.Button btnResetAll;
        private System.Windows.Forms.GroupBox grpSorting;
        private System.Windows.Forms.CheckBox chkSortByLevelFirst;
    }
}