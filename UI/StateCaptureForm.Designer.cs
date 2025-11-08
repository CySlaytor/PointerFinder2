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
            components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(StateCaptureForm));
            numMaxLevel = new NumericUpDown();
            label2 = new Label();
            label3 = new Label();
            txtMaxOffset = new TextBox();
            btnScan = new Button();
            btnCancel = new Button();
            groupBoxRange = new GroupBox();
            btnResetRange = new Button();
            label5 = new Label();
            txtStaticEnd = new TextBox();
            txtStaticStart = new TextBox();
            label6 = new Label();
            numMaxCandidates = new NumericUpDown();
            dgvStates = new DataGridView();
            colSlot = new DataGridViewTextBoxColumn();
            colAddress = new DataGridViewTextBoxColumn();
            colStatus = new DataGridViewTextBoxColumn();
            colAction = new DataGridViewButtonColumn();
            groupBox1 = new GroupBox();
            chkFindAllLevels = new CheckBox();
            chkStopOnFirst = new CheckBox();
            btnClearAll = new Button();
            label1 = new Label();
            numCandidatesPerLevel = new NumericUpDown();
            toolTip1 = new ToolTip(components);
            ((System.ComponentModel.ISupportInitialize)numMaxLevel).BeginInit();
            groupBoxRange.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)numMaxCandidates).BeginInit();
            ((System.ComponentModel.ISupportInitialize)dgvStates).BeginInit();
            groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)numCandidatesPerLevel).BeginInit();
            SuspendLayout();
            // 
            // numMaxLevel
            // 
            numMaxLevel.Location = new Point(12, 252);
            numMaxLevel.Maximum = new decimal(new int[] { 50, 0, 0, 0 });
            numMaxLevel.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            numMaxLevel.Name = "numMaxLevel";
            numMaxLevel.Size = new Size(96, 23);
            numMaxLevel.TabIndex = 4;
            numMaxLevel.Value = new decimal(new int[] { 7, 0, 0, 0 });
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new Point(9, 233);
            label2.Name = "label2";
            label2.Size = new Size(59, 15);
            label2.TabIndex = 3;
            label2.Text = "Max Level";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new Point(9, 183);
            label3.Name = "label3";
            label3.Size = new Size(95, 15);
            label3.TabIndex = 5;
            label3.Text = "Max Offset (Hex)";
            // 
            // txtMaxOffset
            // 
            txtMaxOffset.Location = new Point(12, 202);
            txtMaxOffset.Name = "txtMaxOffset";
            txtMaxOffset.Size = new Size(96, 23);
            txtMaxOffset.TabIndex = 1;
            // 
            // btnScan
            // 
            btnScan.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnScan.Location = new Point(232, 372);
            btnScan.Name = "btnScan";
            btnScan.Size = new Size(83, 29);
            btnScan.TabIndex = 10;
            btnScan.Text = "Scan";
            btnScan.UseVisualStyleBackColor = true;
            btnScan.Click += btnScan_Click;
            // 
            // btnCancel
            // 
            btnCancel.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnCancel.DialogResult = DialogResult.Cancel;
            btnCancel.Location = new Point(321, 372);
            btnCancel.Name = "btnCancel";
            btnCancel.Size = new Size(83, 29);
            btnCancel.TabIndex = 11;
            btnCancel.Text = "Cancel";
            btnCancel.UseVisualStyleBackColor = true;
            // 
            // groupBoxRange
            // 
            groupBoxRange.Controls.Add(btnResetRange);
            groupBoxRange.Controls.Add(label5);
            groupBoxRange.Controls.Add(txtStaticEnd);
            groupBoxRange.Controls.Add(txtStaticStart);
            groupBoxRange.Location = new Point(114, 178);
            groupBoxRange.Name = "groupBoxRange";
            groupBoxRange.Size = new Size(290, 53);
            groupBoxRange.TabIndex = 2;
            groupBoxRange.TabStop = false;
            groupBoxRange.Text = "Static Base Address Range ({sys}, Hex)";
            // 
            // btnResetRange
            // 
            btnResetRange.Location = new Point(228, 21);
            btnResetRange.Name = "btnResetRange";
            btnResetRange.Size = new Size(55, 25);
            btnResetRange.TabIndex = 13;
            btnResetRange.Text = "Reset";
            toolTip1.SetToolTip(btnResetRange, "Reset the range to the recommended default for this emulator.");
            btnResetRange.UseVisualStyleBackColor = true;
            btnResetRange.Click += btnResetRange_Click;
            // 
            // label5
            // 
            label5.AutoSize = true;
            label5.Location = new Point(110, 26);
            label5.Name = "label5";
            label5.Size = new Size(12, 15);
            label5.TabIndex = 12;
            label5.Text = "-";
            // 
            // txtStaticEnd
            // 
            txtStaticEnd.Location = new Point(124, 22);
            txtStaticEnd.Name = "txtStaticEnd";
            txtStaticEnd.Size = new Size(98, 23);
            txtStaticEnd.TabIndex = 1;
            // 
            // txtStaticStart
            // 
            txtStaticStart.Location = new Point(6, 22);
            txtStaticStart.Name = "txtStaticStart";
            txtStaticStart.Size = new Size(98, 23);
            txtStaticStart.TabIndex = 0;
            // 
            // label6
            // 
            label6.AutoSize = true;
            label6.Location = new Point(267, 234);
            label6.Name = "label6";
            label6.Size = new Size(118, 15);
            label6.TabIndex = 10;
            label6.Text = "Max Candidate Paths";
            // 
            // numMaxCandidates
            // 
            numMaxCandidates.Location = new Point(270, 252);
            numMaxCandidates.Maximum = new decimal(new int[] { 100000000, 0, 0, 0 });
            numMaxCandidates.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            numMaxCandidates.Name = "numMaxCandidates";
            numMaxCandidates.Size = new Size(134, 23);
            numMaxCandidates.TabIndex = 5;
            numMaxCandidates.ThousandsSeparator = true;
            numMaxCandidates.Value = new decimal(new int[] { 500000, 0, 0, 0 });
            // 
            // dgvStates
            // 
            dgvStates.AllowUserToAddRows = false;
            dgvStates.AllowUserToDeleteRows = false;
            dgvStates.AllowUserToResizeColumns = false;
            dgvStates.AllowUserToResizeRows = false;
            dgvStates.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            dgvStates.Columns.AddRange(new DataGridViewColumn[] { colSlot, colAddress, colStatus, colAction });
            dgvStates.Dock = DockStyle.Top;
            dgvStates.Location = new Point(0, 0);
            dgvStates.Name = "dgvStates";
            dgvStates.RowHeadersVisible = false;
            dgvStates.Size = new Size(416, 172);
            dgvStates.TabIndex = 12;
            dgvStates.CellContentClick += dgvStates_CellContentClick;
            // 
            // colSlot
            // 
            colSlot.HeaderText = "Slot";
            colSlot.Name = "colSlot";
            colSlot.ReadOnly = true;
            colSlot.Width = 40;
            // 
            // colAddress
            // 
            colAddress.AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
            colAddress.HeaderText = "Target Address (Hex)";
            colAddress.Name = "colAddress";
            // 
            // colStatus
            // 
            colStatus.HeaderText = "Status";
            colStatus.Name = "colStatus";
            colStatus.ReadOnly = true;
            colStatus.Width = 80;
            // 
            // colAction
            // 
            colAction.HeaderText = "Action";
            colAction.Name = "colAction";
            colAction.Width = 80;
            // 
            // groupBox1
            // 
            groupBox1.Controls.Add(chkFindAllLevels);
            groupBox1.Controls.Add(chkStopOnFirst);
            groupBox1.Location = new Point(12, 280);
            groupBox1.Name = "groupBox1";
            groupBox1.Size = new Size(392, 86);
            groupBox1.TabIndex = 13;
            groupBox1.TabStop = false;
            groupBox1.Text = "Scan Options";
            // 
            // chkFindAllLevels
            // 
            chkFindAllLevels.AutoSize = true;
            chkFindAllLevels.Location = new Point(12, 47);
            chkFindAllLevels.Name = "chkFindAllLevels";
            chkFindAllLevels.Size = new Size(168, 19);
            chkFindAllLevels.TabIndex = 2;
            chkFindAllLevels.Text = "Find all path levels (slower)";
            toolTip1.SetToolTip(chkFindAllLevels, resources.GetString("chkFindAllLevels.ToolTip"));
            chkFindAllLevels.UseVisualStyleBackColor = true;
            // 
            // chkStopOnFirst
            // 
            chkStopOnFirst.AutoSize = true;
            chkStopOnFirst.Location = new Point(12, 22);
            chkStopOnFirst.Name = "chkStopOnFirst";
            chkStopOnFirst.Size = new Size(167, 19);
            chkStopOnFirst.TabIndex = 1;
            chkStopOnFirst.Text = "Stop when first path found";
            chkStopOnFirst.UseVisualStyleBackColor = true;
            // 
            // btnClearAll
            // 
            btnClearAll.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            btnClearAll.Location = new Point(12, 372);
            btnClearAll.Name = "btnClearAll";
            btnClearAll.Size = new Size(83, 29);
            btnClearAll.TabIndex = 14;
            btnClearAll.Text = "Clear All";
            btnClearAll.UseVisualStyleBackColor = true;
            btnClearAll.Click += btnClearAll_Click;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(111, 233);
            label1.Name = "label1";
            label1.Size = new Size(116, 15);
            label1.TabIndex = 15;
            label1.Text = "Candidates per Level";
            // 
            // numCandidatesPerLevel
            // 
            numCandidatesPerLevel.Location = new Point(114, 252);
            numCandidatesPerLevel.Maximum = new decimal(new int[] { 1000, 0, 0, 0 });
            numCandidatesPerLevel.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            numCandidatesPerLevel.Name = "numCandidatesPerLevel";
            numCandidatesPerLevel.Size = new Size(150, 23);
            numCandidatesPerLevel.TabIndex = 16;
            toolTip1.SetToolTip(numCandidatesPerLevel, "Limits how many candidate offsets are found for each pointer.\r\nA low value (e.g., 1-5) makes the scan faster and more targeted at the closest offsets.");
            numCandidatesPerLevel.Value = new decimal(new int[] { 10, 0, 0, 0 });
            // 
            // StateCaptureForm
            // 
            AcceptButton = btnScan;
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            CancelButton = btnCancel;
            ClientSize = new Size(416, 413);
            Controls.Add(numCandidatesPerLevel);
            Controls.Add(label1);
            Controls.Add(btnClearAll);
            Controls.Add(groupBox1);
            Controls.Add(dgvStates);
            Controls.Add(label6);
            Controls.Add(numMaxCandidates);
            Controls.Add(btnCancel);
            Controls.Add(btnScan);
            Controls.Add(label3);
            Controls.Add(txtMaxOffset);
            Controls.Add(label2);
            Controls.Add(numMaxLevel);
            Controls.Add(groupBoxRange);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "StateCaptureForm";
            ShowInTaskbar = false;
            StartPosition = FormStartPosition.CenterParent;
            Text = "State Capture";
            Load += StateCaptureForm_Load;
            ((System.ComponentModel.ISupportInitialize)numMaxLevel).EndInit();
            groupBoxRange.ResumeLayout(false);
            groupBoxRange.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)numMaxCandidates).EndInit();
            ((System.ComponentModel.ISupportInitialize)dgvStates).EndInit();
            groupBox1.ResumeLayout(false);
            groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)numCandidatesPerLevel).EndInit();
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion
        private System.Windows.Forms.NumericUpDown numMaxLevel;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox txtMaxOffset;
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
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.NumericUpDown numCandidatesPerLevel;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.CheckBox chkFindAllLevels;
        private Button btnResetRange;
    }
}