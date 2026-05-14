namespace PointerFinder2
{
    partial class StateCaptureForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(StateCaptureForm));
            this.numMaxLevel = new System.Windows.Forms.NumericUpDown();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.txtMaxOffset = new System.Windows.Forms.TextBox();
            this.chkUseLastOffsetHint = new System.Windows.Forms.CheckBox();
            this.txtLastOffsetHint = new System.Windows.Forms.TextBox();
            this.btnScan = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.groupBoxRange = new System.Windows.Forms.GroupBox();
            this.btnResetRange = new System.Windows.Forms.Button();
            this.label5 = new System.Windows.Forms.Label();
            this.txtStaticEnd = new System.Windows.Forms.TextBox();
            this.txtStaticStart = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.numMaxCandidates = new System.Windows.Forms.NumericUpDown();
            this.dgvStates = new System.Windows.Forms.DataGridView();
            this.colSlot = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colAddress = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colStatus = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colAction = new System.Windows.Forms.DataGridViewButtonColumn();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chkPrintPartialPaths = new System.Windows.Forms.CheckBox();
            this.chkFastScanMode = new System.Windows.Forms.CheckBox();
            this.chkFindAllLevels = new System.Windows.Forms.CheckBox();
            this.chkStopOnFirst = new System.Windows.Forms.CheckBox();
            this.btnClearAll = new System.Windows.Forms.Button();
            this.btnHelp = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.numCandidatesPerLevel = new System.Windows.Forms.NumericUpDown();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.numMaxLevel)).BeginInit();
            this.groupBoxRange.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxCandidates)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvStates)).BeginInit();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numCandidatesPerLevel)).BeginInit();
            this.SuspendLayout();
            // 
            // numMaxLevel
            // 
            this.numMaxLevel.Location = new System.Drawing.Point(12, 304);
            this.numMaxLevel.Maximum = new decimal(new int[] { 50, 0, 0, 0 });
            this.numMaxLevel.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            this.numMaxLevel.Name = "numMaxLevel";
            this.numMaxLevel.Size = new System.Drawing.Size(96, 23);
            this.numMaxLevel.TabIndex = 6;
            this.numMaxLevel.Value = new decimal(new int[] { 7, 0, 0, 0 });
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 285);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(59, 15);
            this.label2.TabIndex = 5;
            this.label2.Text = "Max Level";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(9, 183);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(95, 15);
            this.label3.TabIndex = 1;
            this.label3.Text = "Max Offset (Hex)";
            // 
            // txtMaxOffset
            // 
            this.txtMaxOffset.Location = new System.Drawing.Point(12, 202);
            this.txtMaxOffset.Name = "txtMaxOffset";
            this.txtMaxOffset.Size = new System.Drawing.Size(96, 23);
            this.txtMaxOffset.TabIndex = 2;
            // 
            // chkUseLastOffsetHint
            // 
            this.chkUseLastOffsetHint.AutoSize = true;
            this.chkUseLastOffsetHint.Location = new System.Drawing.Point(9, 234);
            this.chkUseLastOffsetHint.Name = "chkUseLastOffsetHint";
            this.chkUseLastOffsetHint.Size = new System.Drawing.Size(139, 19);
            this.chkUseLastOffsetHint.TabIndex = 3;
            this.chkUseLastOffsetHint.Text = "Last Offset Hint (Hex)";
            this.toolTip1.SetToolTip(this.chkUseLastOffsetHint, "Optional. Enforces a specific final offset (e.g. from a breakpoint), massively speeding up the search.");
            this.chkUseLastOffsetHint.UseVisualStyleBackColor = true;
            this.chkUseLastOffsetHint.CheckedChanged += new System.EventHandler(this.chkUseLastOffsetHint_CheckedChanged);
            // 
            // txtLastOffsetHint
            // 
            this.txtLastOffsetHint.Location = new System.Drawing.Point(12, 254);
            this.txtLastOffsetHint.Name = "txtLastOffsetHint";
            this.txtLastOffsetHint.Size = new System.Drawing.Size(96, 23);
            this.txtLastOffsetHint.TabIndex = 4;
            this.toolTip1.SetToolTip(this.txtLastOffsetHint, "Optional. Enforces a specific final offset (e.g. from a breakpoint), massively speeding up the search.");
            // 
            // btnScan
            // 
            this.btnScan.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnScan.Location = new System.Drawing.Point(232, 469);
            this.btnScan.Name = "btnScan";
            this.btnScan.Size = new System.Drawing.Size(83, 29);
            this.btnScan.TabIndex = 13;
            this.btnScan.Text = "Scan";
            this.btnScan.UseVisualStyleBackColor = true;
            this.btnScan.Click += new System.EventHandler(this.btnScan_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(321, 469);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(83, 29);
            this.btnCancel.TabIndex = 14;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // groupBoxRange
            // 
            this.groupBoxRange.Controls.Add(this.btnResetRange);
            this.groupBoxRange.Controls.Add(this.label5);
            this.groupBoxRange.Controls.Add(this.txtStaticEnd);
            this.groupBoxRange.Controls.Add(this.txtStaticStart);
            this.groupBoxRange.Location = new System.Drawing.Point(114, 178);
            this.groupBoxRange.Name = "groupBoxRange";
            this.groupBoxRange.Size = new System.Drawing.Size(290, 53);
            this.groupBoxRange.TabIndex = 9;
            this.groupBoxRange.TabStop = false;
            this.groupBoxRange.Text = "Static Base Address Range ({sys}, Hex)";
            // 
            // btnResetRange
            // 
            this.btnResetRange.Location = new System.Drawing.Point(228, 21);
            this.btnResetRange.Name = "btnResetRange";
            this.btnResetRange.Size = new System.Drawing.Size(55, 25);
            this.btnResetRange.TabIndex = 13;
            this.btnResetRange.Text = "Reset";
            this.toolTip1.SetToolTip(this.btnResetRange, "Reset the range to the recommended default for this emulator.");
            this.btnResetRange.UseVisualStyleBackColor = true;
            this.btnResetRange.Click += new System.EventHandler(this.btnResetRange_Click);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(110, 26);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(12, 15);
            this.label5.TabIndex = 12;
            this.label5.Text = "-";
            // 
            // txtStaticEnd
            // 
            this.txtStaticEnd.Location = new System.Drawing.Point(124, 22);
            this.txtStaticEnd.Name = "txtStaticEnd";
            this.txtStaticEnd.Size = new System.Drawing.Size(98, 23);
            this.txtStaticEnd.TabIndex = 1;
            // 
            // txtStaticStart
            // 
            this.txtStaticStart.Location = new System.Drawing.Point(6, 22);
            this.txtStaticStart.Name = "txtStaticStart";
            this.txtStaticStart.Size = new System.Drawing.Size(98, 23);
            this.txtStaticStart.TabIndex = 0;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(267, 285);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(118, 15);
            this.label6.TabIndex = 10;
            this.label6.Text = "Max Candidate Paths";
            // 
            // numMaxCandidates
            // 
            this.numMaxCandidates.Location = new System.Drawing.Point(270, 304);
            this.numMaxCandidates.Maximum = new decimal(new int[] { 100000000, 0, 0, 0 });
            this.numMaxCandidates.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            this.numMaxCandidates.Name = "numMaxCandidates";
            this.numMaxCandidates.Size = new System.Drawing.Size(134, 23);
            this.numMaxCandidates.TabIndex = 8;
            this.numMaxCandidates.ThousandsSeparator = true;
            this.numMaxCandidates.Value = new decimal(new int[] { 500000, 0, 0, 0 });
            // 
            // dgvStates
            // 
            this.dgvStates.AllowUserToAddRows = false;
            this.dgvStates.AllowUserToDeleteRows = false;
            this.dgvStates.AllowUserToResizeColumns = false;
            this.dgvStates.AllowUserToResizeRows = false;
            this.dgvStates.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgvStates.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] { this.colSlot, this.colAddress, this.colStatus, this.colAction });
            this.dgvStates.Dock = System.Windows.Forms.DockStyle.Top;
            this.dgvStates.Location = new System.Drawing.Point(0, 0);
            this.dgvStates.Name = "dgvStates";
            this.dgvStates.RowHeadersVisible = false;
            this.dgvStates.Size = new System.Drawing.Size(416, 172);
            this.dgvStates.TabIndex = 0;
            this.dgvStates.CellContentClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.dgvStates_CellContentClick);
            // 
            // colSlot
            // 
            this.colSlot.HeaderText = "Slot";
            this.colSlot.Name = "colSlot";
            this.colSlot.ReadOnly = true;
            this.colSlot.Width = 40;
            // 
            // colAddress
            // 
            this.colAddress.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colAddress.HeaderText = "Target Address (Hex)";
            this.colAddress.Name = "colAddress";
            // 
            // colStatus
            // 
            this.colStatus.HeaderText = "Status";
            this.colStatus.Name = "colStatus";
            this.colStatus.ReadOnly = true;
            this.colStatus.Width = 80;
            // 
            // colAction
            // 
            this.colAction.HeaderText = "Action";
            this.colAction.Name = "colAction";
            this.colAction.Width = 80;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chkPrintPartialPaths);
            this.groupBox1.Controls.Add(this.chkFastScanMode);
            this.groupBox1.Controls.Add(this.chkFindAllLevels);
            this.groupBox1.Controls.Add(this.chkStopOnFirst);
            this.groupBox1.Location = new System.Drawing.Point(12, 335);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(392, 125);
            this.groupBox1.TabIndex = 11;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Scan Options";
            // 
            // chkPrintPartialPaths
            // 
            this.chkPrintPartialPaths.AutoSize = true;
            this.chkPrintPartialPaths.Location = new System.Drawing.Point(12, 97);
            this.chkPrintPartialPaths.Name = "chkPrintPartialPaths";
            this.chkPrintPartialPaths.Size = new System.Drawing.Size(256, 19);
            this.chkPrintPartialPaths.TabIndex = 4;
            this.chkPrintPartialPaths.Text = "Output partial/broken paths";
            this.toolTip1.SetToolTip(this.chkPrintPartialPaths, "Outputs dead-end paths that never reach the static range. Useful for analyzing dynamic addresses and arrays.");
            this.chkPrintPartialPaths.UseVisualStyleBackColor = true;
            // 
            // chkFastScanMode
            // 
            this.chkFastScanMode.AutoSize = true;
            this.chkFastScanMode.Location = new System.Drawing.Point(12, 72);
            this.chkFastScanMode.Name = "chkFastScanMode";
            this.chkFastScanMode.Size = new System.Drawing.Size(193, 19);
            this.chkFastScanMode.TabIndex = 3;
            this.chkFastScanMode.Text = "Fast Mode (Aggressive Pruning)";
            this.toolTip1.SetToolTip(this.chkFastScanMode, "Dramatically speeds up deep scans and prevents memory explosion by skipping redundant pointer paths.\r\nUncheck to perform an exhaustive (but much slower) search.");
            this.chkFastScanMode.UseVisualStyleBackColor = true;
            // 
            // chkFindAllLevels
            // 
            this.chkFindAllLevels.AutoSize = true;
            this.chkFindAllLevels.Location = new System.Drawing.Point(12, 47);
            this.chkFindAllLevels.Name = "chkFindAllLevels";
            this.chkFindAllLevels.Size = new System.Drawing.Size(168, 19);
            this.chkFindAllLevels.TabIndex = 2;
            this.chkFindAllLevels.Text = "Find all path levels (slower)";
            this.toolTip1.SetToolTip(this.chkFindAllLevels, resources.GetString("chkFindAllLevels.ToolTip"));
            this.chkFindAllLevels.UseVisualStyleBackColor = true;
            // 
            // chkStopOnFirst
            // 
            this.chkStopOnFirst.AutoSize = true;
            this.chkStopOnFirst.Location = new System.Drawing.Point(12, 22);
            this.chkStopOnFirst.Name = "chkStopOnFirst";
            this.chkStopOnFirst.Size = new System.Drawing.Size(167, 19);
            this.chkStopOnFirst.TabIndex = 1;
            this.chkStopOnFirst.Text = "Stop when first path found";
            this.chkStopOnFirst.UseVisualStyleBackColor = true;
            // 
            // btnHelp
            // 
            this.btnHelp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnHelp.Location = new System.Drawing.Point(12, 469);
            this.btnHelp.Name = "btnHelp";
            this.btnHelp.Size = new System.Drawing.Size(29, 29);
            this.btnHelp.TabIndex = 15;
            this.btnHelp.Text = "?";
            this.btnHelp.UseVisualStyleBackColor = true;
            this.btnHelp.Click += new System.EventHandler(this.btnHelp_Click);
            // 
            // btnClearAll
            // 
            this.btnClearAll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnClearAll.Location = new System.Drawing.Point(47, 469);
            this.btnClearAll.Name = "btnClearAll";
            this.btnClearAll.Size = new System.Drawing.Size(83, 29);
            this.btnClearAll.TabIndex = 12;
            this.btnClearAll.Text = "Clear All";
            this.btnClearAll.UseVisualStyleBackColor = true;
            this.btnClearAll.Click += new System.EventHandler(this.btnClearAll_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(111, 285);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(116, 15);
            this.label1.TabIndex = 15;
            this.label1.Text = "Candidates per Level";
            // 
            // numCandidatesPerLevel
            // 
            this.numCandidatesPerLevel.Location = new System.Drawing.Point(114, 304);
            this.numCandidatesPerLevel.Maximum = new decimal(new int[] { 1000, 0, 0, 0 });
            this.numCandidatesPerLevel.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            this.numCandidatesPerLevel.Name = "numCandidatesPerLevel";
            this.numCandidatesPerLevel.Size = new System.Drawing.Size(150, 23);
            this.numCandidatesPerLevel.TabIndex = 7;
            this.toolTip1.SetToolTip(this.numCandidatesPerLevel, "Limits how many candidate offsets are found for each pointer.\r\nA low value (e.g., 1-5) makes the scan faster and more targeted at the closest offsets.");
            this.numCandidatesPerLevel.Value = new decimal(new int[] { 10, 0, 0, 0 });
            // 
            // StateCaptureForm
            // 
            this.AcceptButton = this.btnScan;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(416, 510);
            this.Controls.Add(this.btnHelp);
            this.Controls.Add(this.chkUseLastOffsetHint);
            this.Controls.Add(this.txtLastOffsetHint);
            this.Controls.Add(this.numCandidatesPerLevel);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnClearAll);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.dgvStates);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.numMaxCandidates);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnScan);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.txtMaxOffset);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.numMaxLevel);
            this.Controls.Add(this.groupBoxRange);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "StateCaptureForm";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "State Capture";
            this.Load += new System.EventHandler(this.StateCaptureForm_Load);
            ((System.ComponentModel.ISupportInitialize)(this.numMaxLevel)).EndInit();
            this.groupBoxRange.ResumeLayout(false);
            this.groupBoxRange.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxCandidates)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgvStates)).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numCandidatesPerLevel)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.NumericUpDown numMaxLevel;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox txtMaxOffset;
        private System.Windows.Forms.CheckBox chkUseLastOffsetHint;
        private System.Windows.Forms.TextBox txtLastOffsetHint;
        private System.Windows.Forms.Button btnScan;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.GroupBox groupBoxRange;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txtStaticEnd;
        private System.Windows.Forms.TextBox txtStaticStart;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.NumericUpDown numMaxCandidates;
        private System.Windows.Forms.DataGridView dgvStates;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.CheckBox chkStopOnFirst;
        private System.Windows.Forms.DataGridViewTextBoxColumn colSlot;
        private System.Windows.Forms.DataGridViewTextBoxColumn colAddress;
        private System.Windows.Forms.DataGridViewTextBoxColumn colStatus;
        private System.Windows.Forms.DataGridViewButtonColumn colAction;
        private System.Windows.Forms.Button btnClearAll;
        private System.Windows.Forms.Button btnHelp;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.NumericUpDown numCandidatesPerLevel;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.CheckBox chkFindAllLevels;
        private System.Windows.Forms.Button btnResetRange;
        private System.Windows.Forms.CheckBox chkFastScanMode;
        private System.Windows.Forms.CheckBox chkPrintPartialPaths;
    }
}