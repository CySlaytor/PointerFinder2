


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
            components = new System.ComponentModel.Container();
            tabControlSettings = new TabControl();
            tabGeneral = new TabPage();
            groupBox3 = new GroupBox();
            CheckDarkModeEnabled = new CheckBox();
            grpSorting = new GroupBox();
            chkSortByLevelFirst = new CheckBox();
            grpDangerZone = new GroupBox();
            btnRestartApp = new Button();
            btnResetAll = new Button();
            grpPerformance = new GroupBox();
            chkLimitCpuUsage = new CheckBox();
            grpSound = new GroupBox();
            chkUseDefaultSounds = new CheckBox();
            tabCodeNotes = new TabPage();
            groupBox2 = new GroupBox();
            richPreview = new RichTextBox();
            groupBox1 = new GroupBox();
            chkSuffixOnLastLine = new CheckBox();
            chkAlign = new CheckBox();
            txtSuffix = new TextBox();
            label2 = new Label();
            txtPrefix = new TextBox();
            label1 = new Label();
            tabDebug = new TabPage();
            grpLogging = new GroupBox();
            chkLogStateScanDetails = new CheckBox();
            chkLogRefineScan = new CheckBox();
            chkLogFilter = new CheckBox();
            chkLogLiveScan = new CheckBox();
            btnClose = new Button();
            toolTip1 = new ToolTip(components);
            btnPurgeMemory = new Button();
            tabControlSettings.SuspendLayout();
            tabGeneral.SuspendLayout();
            groupBox3.SuspendLayout();
            grpSorting.SuspendLayout();
            grpDangerZone.SuspendLayout();
            grpPerformance.SuspendLayout();
            grpSound.SuspendLayout();
            tabCodeNotes.SuspendLayout();
            groupBox2.SuspendLayout();
            groupBox1.SuspendLayout();
            tabDebug.SuspendLayout();
            grpLogging.SuspendLayout();
            SuspendLayout();
            // 
            // tabControlSettings
            // 
            tabControlSettings.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            tabControlSettings.Controls.Add(tabGeneral);
            tabControlSettings.Controls.Add(tabCodeNotes);
            tabControlSettings.Controls.Add(tabDebug);
            tabControlSettings.ForeColor = Color.White;
            tabControlSettings.Location = new Point(12, 12);
            tabControlSettings.Name = "tabControlSettings";
            tabControlSettings.SelectedIndex = 0;
            tabControlSettings.Size = new Size(460, 310);
            tabControlSettings.TabIndex = 0;
            // 
            // tabGeneral
            // 
            tabGeneral.BackColor = Color.Transparent;
            tabGeneral.Controls.Add(groupBox3);
            tabGeneral.Controls.Add(grpSorting);
            tabGeneral.Controls.Add(grpDangerZone);
            tabGeneral.Controls.Add(grpPerformance);
            tabGeneral.Controls.Add(grpSound);
            tabGeneral.ForeColor = SystemColors.ControlText;
            tabGeneral.Location = new Point(4, 24);
            tabGeneral.Name = "tabGeneral";
            tabGeneral.Padding = new Padding(3);
            tabGeneral.Size = new Size(452, 282);
            tabGeneral.TabIndex = 0;
            tabGeneral.Text = "General";
            tabGeneral.UseVisualStyleBackColor = true;
            tabGeneral.Click += tabGeneral_Click;
            // 
            // groupBox3
            // 
            groupBox3.BackColor = Color.Transparent;
            groupBox3.Controls.Add(CheckDarkModeEnabled);
            groupBox3.ForeColor = SystemColors.ControlText;
            groupBox3.Location = new Point(6, 165);
            groupBox3.Name = "groupBox3";
            groupBox3.Size = new Size(439, 48);
            groupBox3.TabIndex = 2;
            groupBox3.TabStop = false;
            groupBox3.Text = "Theme";
            groupBox3.Enter += groupBox3_Enter;
            // 
            // CheckDarkModeEnabled
            // 
            CheckDarkModeEnabled.AutoSize = true;
            CheckDarkModeEnabled.BackColor = Color.Transparent;
            CheckDarkModeEnabled.ForeColor = SystemColors.ControlText;
            CheckDarkModeEnabled.Location = new Point(14, 22);
            CheckDarkModeEnabled.Name = "CheckDarkModeEnabled";
            CheckDarkModeEnabled.Size = new Size(94, 19);
            CheckDarkModeEnabled.TabIndex = 0;
            CheckDarkModeEnabled.Text = "Black Theme";
            toolTip1.SetToolTip(CheckDarkModeEnabled, "Black!");
            CheckDarkModeEnabled.UseVisualStyleBackColor = true;
            CheckDarkModeEnabled.CheckedChanged += CheckDarkModeEnabled_CheckedChanged;
            // 
            // grpSorting
            // 
            grpSorting.BackColor = Color.Transparent;
            grpSorting.Controls.Add(chkSortByLevelFirst);
            grpSorting.ForeColor = SystemColors.ControlText;
            grpSorting.Location = new Point(6, 58);
            grpSorting.Name = "grpSorting";
            grpSorting.Size = new Size(439, 47);
            grpSorting.TabIndex = 3;
            grpSorting.TabStop = false;
            grpSorting.Text = "Sorting";
            grpSorting.Enter += grpSorting_Enter;
            // 
            // chkSortByLevelFirst
            // 
            chkSortByLevelFirst.AutoSize = true;
            chkSortByLevelFirst.BackColor = Color.Transparent;
            chkSortByLevelFirst.ForeColor = SystemColors.ControlText;
            chkSortByLevelFirst.Location = new Point(13, 22);
            chkSortByLevelFirst.Name = "chkSortByLevelFirst";
            chkSortByLevelFirst.Size = new Size(312, 19);
            chkSortByLevelFirst.TabIndex = 0;
            chkSortByLevelFirst.Text = "Prioritize shorter chains when sorting by lowest offsets";
            toolTip1.SetToolTip(chkSortByLevelFirst, "When checked, results are sorted by the number of offsets first, then by the offset values.\r\nWhen unchecked, results are sorted purely by the offset values regardless of chain length.");
            chkSortByLevelFirst.UseVisualStyleBackColor = true;
            chkSortByLevelFirst.CheckedChanged += chkSortByLevelFirst_CheckedChanged;
            // 
            // grpDangerZone
            // 
            grpDangerZone.BackColor = Color.Transparent;
            grpDangerZone.Controls.Add(btnRestartApp);
            grpDangerZone.Controls.Add(btnResetAll);
            grpDangerZone.ForeColor = SystemColors.ControlText;
            grpDangerZone.Location = new Point(6, 219);
            grpDangerZone.Name = "grpDangerZone";
            grpDangerZone.Size = new Size(439, 59);
            grpDangerZone.TabIndex = 2;
            grpDangerZone.TabStop = false;
            grpDangerZone.Text = "Danger Zone";
            // 
            // btnRestartApp
            // 
            btnRestartApp.BackColor = Color.Transparent;
            btnRestartApp.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnRestartApp.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnRestartApp.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            btnRestartApp.ForeColor = Color.DarkOrange;
            btnRestartApp.Location = new Point(228, 22);
            btnRestartApp.Name = "btnRestartApp";
            btnRestartApp.Size = new Size(130, 23);
            btnRestartApp.TabIndex = 3;
            btnRestartApp.Text = "Restart Application...";
            toolTip1.SetToolTip(btnRestartApp, "Closes and re-opens the application to completely reset its memory state.");
            btnRestartApp.UseVisualStyleBackColor = false;
            btnRestartApp.Click += btnRestartApp_Click;
            // 
            // btnResetAll
            // 
            btnResetAll.BackColor = Color.Transparent;
            btnResetAll.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnResetAll.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnResetAll.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            btnResetAll.ForeColor = Color.Red;
            btnResetAll.Location = new Point(16, 22);
            btnResetAll.Name = "btnResetAll";
            btnResetAll.Size = new Size(186, 23);
            btnResetAll.TabIndex = 0;
            btnResetAll.Text = "Reset All Settings to Default...";
            toolTip1.SetToolTip(btnResetAll, "Deletes settings.ini and resets all saved options. The application will restart.");
            btnResetAll.UseVisualStyleBackColor = false;
            btnResetAll.Click += btnResetAll_Click;
            // 
            // grpPerformance
            // 
            grpPerformance.BackColor = Color.Transparent;
            grpPerformance.Controls.Add(chkLimitCpuUsage);
            grpPerformance.ForeColor = SystemColors.ControlText;
            grpPerformance.Location = new Point(5, 111);
            grpPerformance.Name = "grpPerformance";
            grpPerformance.Size = new Size(439, 48);
            grpPerformance.TabIndex = 1;
            grpPerformance.TabStop = false;
            grpPerformance.Text = "Performance";
            grpPerformance.Enter += grpPerformance_Enter;
            // 
            // chkLimitCpuUsage
            // 
            chkLimitCpuUsage.AutoSize = true;
            chkLimitCpuUsage.BackColor = Color.Transparent;
            chkLimitCpuUsage.ForeColor = SystemColors.ControlText;
            chkLimitCpuUsage.Location = new Point(14, 22);
            chkLimitCpuUsage.Name = "chkLimitCpuUsage";
            chkLimitCpuUsage.Size = new Size(189, 19);
            chkLimitCpuUsage.TabIndex = 0;
            chkLimitCpuUsage.Text = "Limit CPU Usage (approx. 50%)";
            toolTip1.SetToolTip(chkLimitCpuUsage, "Reduces the number of CPU cores used during the scan.\r\nHelpful for multitasking, but the scan will take longer.");
            chkLimitCpuUsage.UseVisualStyleBackColor = true;
            chkLimitCpuUsage.CheckedChanged += chkLimitCpuUsage_CheckedChanged;
            // 
            // grpSound
            // 
            grpSound.BackColor = Color.Transparent;
            grpSound.Controls.Add(chkUseDefaultSounds);
            grpSound.ForeColor = SystemColors.ControlText;
            grpSound.Location = new Point(6, 6);
            grpSound.Name = "grpSound";
            grpSound.Size = new Size(439, 46);
            grpSound.TabIndex = 0;
            grpSound.TabStop = false;
            grpSound.Text = "Sound";
            grpSound.Enter += grpSound_Enter;
            // 
            // chkUseDefaultSounds
            // 
            chkUseDefaultSounds.AutoSize = true;
            chkUseDefaultSounds.BackColor = Color.Transparent;
            chkUseDefaultSounds.ForeColor = SystemColors.ControlText;
            chkUseDefaultSounds.Location = new Point(13, 22);
            chkUseDefaultSounds.Name = "chkUseDefaultSounds";
            chkUseDefaultSounds.Size = new Size(237, 19);
            chkUseDefaultSounds.TabIndex = 0;
            chkUseDefaultSounds.Text = "Use Windows default notification sound";
            chkUseDefaultSounds.UseVisualStyleBackColor = true;
            chkUseDefaultSounds.CheckedChanged += chkUseDefaultSounds_CheckedChanged;
            // 
            // tabCodeNotes
            // 
            tabCodeNotes.BackColor = Color.Transparent;
            tabCodeNotes.BorderStyle = BorderStyle.FixedSingle;
            tabCodeNotes.Controls.Add(groupBox2);
            tabCodeNotes.Controls.Add(groupBox1);
            tabCodeNotes.ForeColor = Color.White;
            tabCodeNotes.Location = new Point(4, 24);
            tabCodeNotes.Name = "tabCodeNotes";
            tabCodeNotes.Padding = new Padding(3);
            tabCodeNotes.Size = new Size(452, 282);
            tabCodeNotes.TabIndex = 2;
            tabCodeNotes.Text = "Code Notes";
            tabCodeNotes.UseVisualStyleBackColor = true;
            // 
            // groupBox2
            // 
            groupBox2.BackColor = Color.Transparent;
            groupBox2.Controls.Add(richPreview);
            groupBox2.ForeColor = SystemColors.ControlText;
            groupBox2.Location = new Point(220, 6);
            groupBox2.Name = "groupBox2";
            groupBox2.Size = new Size(226, 201);
            groupBox2.TabIndex = 1;
            groupBox2.TabStop = false;
            groupBox2.Text = "Live Preview";
            // 
            // richPreview
            // 
            richPreview.BackColor = SystemColors.ControlLight;
            richPreview.Dock = DockStyle.Fill;
            richPreview.Font = new Font("Consolas", 9F);
            richPreview.ForeColor = SystemColors.WindowText;
            richPreview.Location = new Point(3, 19);
            richPreview.Name = "richPreview";
            richPreview.ReadOnly = true;
            richPreview.Size = new Size(220, 179);
            richPreview.TabIndex = 0;
            richPreview.Text = "";
            richPreview.WordWrap = false;
            // 
            // groupBox1
            // 
            groupBox1.BackColor = Color.Transparent;
            groupBox1.Controls.Add(chkSuffixOnLastLine);
            groupBox1.Controls.Add(chkAlign);
            groupBox1.Controls.Add(txtSuffix);
            groupBox1.Controls.Add(label2);
            groupBox1.Controls.Add(txtPrefix);
            groupBox1.Controls.Add(label1);
            groupBox1.ForeColor = SystemColors.ControlText;
            groupBox1.Location = new Point(6, 6);
            groupBox1.Name = "groupBox1";
            groupBox1.Size = new Size(208, 201);
            groupBox1.TabIndex = 0;
            groupBox1.TabStop = false;
            groupBox1.Text = "Formatting Options";
            groupBox1.Enter += groupBox1_Enter;
            // 
            // chkSuffixOnLastLine
            // 
            chkSuffixOnLastLine.AutoSize = true;
            chkSuffixOnLastLine.BackColor = Color.Transparent;
            chkSuffixOnLastLine.ForeColor = SystemColors.WindowText;
            chkSuffixOnLastLine.Location = new Point(9, 153);
            chkSuffixOnLastLine.Name = "chkSuffixOnLastLine";
            chkSuffixOnLastLine.Size = new Size(180, 19);
            chkSuffixOnLastLine.TabIndex = 5;
            chkSuffixOnLastLine.Text = "Apply Suffix to Last Line Only";
            toolTip1.SetToolTip(chkSuffixOnLastLine, "If checked, the suffix is only applied to the final line of the pointer chain.");
            chkSuffixOnLastLine.UseVisualStyleBackColor = false;
            chkSuffixOnLastLine.CheckedChanged += CodeNoteSetting_Changed;
            // 
            // chkAlign
            // 
            chkAlign.AutoSize = true;
            chkAlign.BackColor = Color.Transparent;
            chkAlign.ForeColor = SystemColors.WindowText;
            chkAlign.Location = new Point(9, 128);
            chkAlign.Name = "chkAlign";
            chkAlign.Size = new Size(147, 19);
            chkAlign.TabIndex = 4;
            chkAlign.Text = "Align Suffixes Vertically";
            toolTip1.SetToolTip(chkAlign, "Adds padding spaces so that all suffixes line up in a column.");
            chkAlign.UseVisualStyleBackColor = false;
            chkAlign.CheckedChanged += CodeNoteSetting_Changed;
            // 
            // txtSuffix
            // 
            txtSuffix.BackColor = SystemColors.Window;
            txtSuffix.ForeColor = SystemColors.WindowText;
            txtSuffix.Location = new Point(9, 89);
            txtSuffix.Name = "txtSuffix";
            txtSuffix.Size = new Size(180, 23);
            txtSuffix.TabIndex = 3;
            toolTip1.SetToolTip(txtSuffix, "The character(s) to add to the end of each line (e.g., \" |\" or \"=\").");
            txtSuffix.TextChanged += CodeNoteSetting_Changed;
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.BackColor = SystemColors.Window;
            label2.ForeColor = SystemColors.WindowText;
            label2.Location = new Point(6, 71);
            label2.Name = "label2";
            label2.Size = new Size(64, 15);
            label2.TabIndex = 2;
            label2.Text = "Line Suffix:";
            // 
            // txtPrefix
            // 
            txtPrefix.BackColor = SystemColors.Window;
            txtPrefix.ForeColor = SystemColors.WindowText;
            txtPrefix.Location = new Point(9, 45);
            txtPrefix.Name = "txtPrefix";
            txtPrefix.Size = new Size(180, 23);
            txtPrefix.TabIndex = 1;
            toolTip1.SetToolTip(txtPrefix, "The character(s) used for indentation. Will be repeated for each level.\r\nExamples: . or + or -");
            txtPrefix.TextChanged += CodeNoteSetting_Changed;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.BackColor = SystemColors.Window;
            label1.ForeColor = SystemColors.WindowText;
            label1.Location = new Point(6, 27);
            label1.Name = "label1";
            label1.Size = new Size(103, 15);
            label1.TabIndex = 0;
            label1.Text = "Indentation Prefix:";
            // 
            // tabDebug
            // 
            tabDebug.BackColor = Color.Transparent;
            tabDebug.Controls.Add(grpLogging);
            tabDebug.ForeColor = SystemColors.ControlText;
            tabDebug.Location = new Point(4, 24);
            tabDebug.Name = "tabDebug";
            tabDebug.Padding = new Padding(3);
            tabDebug.Size = new Size(452, 282);
            tabDebug.TabIndex = 1;
            tabDebug.Text = "Debug";
            tabDebug.UseVisualStyleBackColor = true;
            tabDebug.Click += tabDebug_Click;
            // 
            // grpLogging
            // 
            grpLogging.BackColor = Color.Transparent;
            grpLogging.Controls.Add(chkLogStateScanDetails);
            grpLogging.Controls.Add(chkLogRefineScan);
            grpLogging.Controls.Add(chkLogFilter);
            grpLogging.Controls.Add(chkLogLiveScan);
            grpLogging.ForeColor = SystemColors.ControlText;
            grpLogging.Location = new Point(6, 6);
            grpLogging.Name = "grpLogging";
            grpLogging.Size = new Size(439, 154);
            grpLogging.TabIndex = 1;
            grpLogging.TabStop = false;
            grpLogging.Text = "Logging and Debug Toggles";
            grpLogging.Enter += grpLogging_Enter;
            // 
            // chkLogStateScanDetails
            // 
            chkLogStateScanDetails.AutoSize = true;
            chkLogStateScanDetails.BackColor = Color.Transparent;
            chkLogStateScanDetails.ForeColor = SystemColors.ControlText;
            chkLogStateScanDetails.Location = new Point(18, 110);
            chkLogStateScanDetails.Name = "chkLogStateScanDetails";
            chkLogStateScanDetails.Size = new Size(177, 19);
            chkLogStateScanDetails.TabIndex = 3;
            chkLogStateScanDetails.Text = "Log State-Based Scan Details";
            toolTip1.SetToolTip(chkLogStateScanDetails, "Outputs an extremely verbose log of the state-based scan validation process.");
            chkLogStateScanDetails.UseVisualStyleBackColor = false;
            chkLogStateScanDetails.CheckedChanged += chkLogStateScanDetails_CheckedChanged;
            // 
            // chkLogRefineScan
            // 
            chkLogRefineScan.AutoSize = true;
            chkLogRefineScan.BackColor = Color.Transparent;
            chkLogRefineScan.ForeColor = SystemColors.ControlText;
            chkLogRefineScan.Location = new Point(18, 83);
            chkLogRefineScan.Name = "chkLogRefineScan";
            chkLogRefineScan.Size = new Size(183, 19);
            chkLogRefineScan.TabIndex = 2;
            chkLogRefineScan.Text = "Log Refine Scan (Intersection)";
            chkLogRefineScan.UseVisualStyleBackColor = false;
            chkLogRefineScan.CheckedChanged += chkLogRefineScan_CheckedChanged;
            // 
            // chkLogFilter
            // 
            chkLogFilter.AutoSize = true;
            chkLogFilter.BackColor = Color.Transparent;
            chkLogFilter.ForeColor = SystemColors.ControlText;
            chkLogFilter.Location = new Point(18, 56);
            chkLogFilter.Name = "chkLogFilter";
            chkLogFilter.Size = new Size(143, 19);
            chkLogFilter.TabIndex = 1;
            chkLogFilter.Text = "Log Live Path Filtering";
            chkLogFilter.UseVisualStyleBackColor = false;
            chkLogFilter.CheckedChanged += chkLogFilter_CheckedChanged;
            // 
            // chkLogLiveScan
            // 
            chkLogLiveScan.AutoSize = true;
            chkLogLiveScan.BackColor = Color.Transparent;
            chkLogLiveScan.ForeColor = SystemColors.ControlText;
            chkLogLiveScan.Location = new Point(18, 29);
            chkLogLiveScan.Name = "chkLogLiveScan";
            chkLogLiveScan.Size = new Size(98, 19);
            chkLogLiveScan.TabIndex = 0;
            chkLogLiveScan.Text = "Log Live Scan";
            chkLogLiveScan.UseVisualStyleBackColor = false;
            chkLogLiveScan.CheckedChanged += chkLogLiveScan_CheckedChanged;
            // 
            // btnClose
            // 
            btnClose.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnClose.BackColor = SystemColors.Control;
            btnClose.DialogResult = DialogResult.OK;
            btnClose.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnClose.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnClose.ForeColor = SystemColors.ControlText;
            btnClose.Location = new Point(385, 328);
            btnClose.Name = "btnClose";
            btnClose.Size = new Size(87, 28);
            btnClose.TabIndex = 2;
            btnClose.Text = "Close";
            btnClose.UseVisualStyleBackColor = true;
            btnClose.Click += btnClose_Click;
            // 
            // btnPurgeMemory
            // 
            btnPurgeMemory.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            btnPurgeMemory.BackColor = Color.Transparent;
            btnPurgeMemory.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnPurgeMemory.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnPurgeMemory.ForeColor = SystemColors.ControlText;
            btnPurgeMemory.Location = new Point(12, 328);
            btnPurgeMemory.Name = "btnPurgeMemory";
            btnPurgeMemory.Size = new Size(91, 28);
            btnPurgeMemory.TabIndex = 4;
            btnPurgeMemory.Text = "Purge Memory";
            toolTip1.SetToolTip(btnPurgeMemory, "Forces the application to release unused RAM back to the operating system.");
            btnPurgeMemory.UseVisualStyleBackColor = false;
            btnPurgeMemory.Click += btnPurgeMemory_Click;
            // 
            // SettingsForm
            // 
            AcceptButton = btnClose;
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(484, 368);
            Controls.Add(btnPurgeMemory);
            Controls.Add(btnClose);
            Controls.Add(tabControlSettings);
            ForeColor = SystemColors.ControlText;
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "SettingsForm";
            StartPosition = FormStartPosition.CenterParent;
            Text = "Settings";
            Load += SettingsForm_Load;
            tabControlSettings.ResumeLayout(false);
            tabGeneral.ResumeLayout(false);
            groupBox3.ResumeLayout(false);
            groupBox3.PerformLayout();
            grpSorting.ResumeLayout(false);
            grpSorting.PerformLayout();
            grpDangerZone.ResumeLayout(false);
            grpPerformance.ResumeLayout(false);
            grpPerformance.PerformLayout();
            grpSound.ResumeLayout(false);
            grpSound.PerformLayout();
            tabCodeNotes.ResumeLayout(false);
            groupBox2.ResumeLayout(false);
            groupBox1.ResumeLayout(false);
            groupBox1.PerformLayout();
            tabDebug.ResumeLayout(false);
            grpLogging.ResumeLayout(false);
            grpLogging.PerformLayout();
            ResumeLayout(false);

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
        private GroupBox groupBox3;
        private CheckBox CheckDarkModeEnabled;

    }
}