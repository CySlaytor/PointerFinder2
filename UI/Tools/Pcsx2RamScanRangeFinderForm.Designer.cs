namespace PointerFinder2.UI.StaticRangeFinders
{
    partial class Pcsx2RamScanRangeFinderForm
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
            btnFind = new Button();
            btnClose = new Button();
            btnApply = new Button();
            richLog = new RichTextBox();
            groupBox1 = new GroupBox();
            lblApproxResult = new Label();
            label3 = new Label();
            lblAbsoluteResult = new Label();
            label1 = new Label();
            toolTip1 = new ToolTip(components);
            progressBar = new ProgressBar();
            groupBox1.SuspendLayout();
            SuspendLayout();
            // 
            // btnFind
            // 
            btnFind.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            btnFind.Location = new Point(12, 12);
            btnFind.Name = "btnFind";
            btnFind.Size = new Size(130, 28);
            btnFind.TabIndex = 0;
            btnFind.Text = "Find Static Range";
            btnFind.UseVisualStyleBackColor = true;
            btnFind.Click += btnFind_Click;
            // 
            // btnClose
            // 
            btnClose.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnClose.DialogResult = DialogResult.Cancel;
            btnClose.Location = new Point(497, 401);
            btnClose.Name = "btnClose";
            btnClose.Size = new Size(75, 28);
            btnClose.TabIndex = 1;
            btnClose.Text = "Close";
            btnClose.UseVisualStyleBackColor = true;
            // 
            // btnApply
            // 
            btnApply.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnApply.Enabled = false;
            btnApply.Location = new Point(379, 401);
            btnApply.Name = "btnApply";
            btnApply.Size = new Size(112, 28);
            btnApply.TabIndex = 2;
            btnApply.Text = "Apply and Close";
            toolTip1.SetToolTip(btnApply, "Saves the 'Approximate' range to your PCSX2 settings profile.");
            btnApply.UseVisualStyleBackColor = true;
            btnApply.Click += btnApply_Click;
            // 
            // richLog
            // 
            richLog.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            richLog.BackColor = Color.FromArgb(40, 40, 40);
            richLog.Font = new Font("Consolas", 9F);
            richLog.ForeColor = Color.Gainsboro;
            richLog.Location = new Point(12, 46);
            richLog.Name = "richLog";
            richLog.ReadOnly = true;
            richLog.Size = new Size(560, 260);
            richLog.TabIndex = 3;
            richLog.Text = "";
            // 
            // groupBox1
            // 
            groupBox1.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            groupBox1.Controls.Add(lblApproxResult);
            groupBox1.Controls.Add(label3);
            groupBox1.Controls.Add(lblAbsoluteResult);
            groupBox1.Controls.Add(label1);
            groupBox1.Location = new Point(12, 312);
            groupBox1.Name = "groupBox1";
            groupBox1.Size = new Size(560, 83);
            groupBox1.TabIndex = 4;
            groupBox1.TabStop = false;
            groupBox1.Text = "Results";
            // 
            // lblApproxResult
            // 
            lblApproxResult.AutoSize = true;
            lblApproxResult.Font = new Font("Consolas", 9.75F, FontStyle.Bold);
            lblApproxResult.Location = new Point(212, 53);
            lblApproxResult.Name = "lblApproxResult";
            lblApproxResult.Size = new Size(28, 15);
            lblApproxResult.TabIndex = 3;
            lblApproxResult.Text = "N/A";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new Point(16, 53);
            label3.Name = "label3";
            label3.Size = new Size(190, 15);
            label3.TabIndex = 2;
            label3.Text = "Approximate 1MB Block (for scan):";
            toolTip1.SetToolTip(label3, "A calculated 1MB range that should contain all static addresses.\r\nThis is the range that will be applied to your settings.");
            // 
            // lblAbsoluteResult
            // 
            lblAbsoluteResult.AutoSize = true;
            lblAbsoluteResult.Font = new Font("Consolas", 9.75F, FontStyle.Bold);
            lblAbsoluteResult.Location = new Point(212, 26);
            lblAbsoluteResult.Name = "lblAbsoluteResult";
            lblAbsoluteResult.Size = new Size(28, 15);
            lblAbsoluteResult.TabIndex = 1;
            lblAbsoluteResult.Text = "N/A";
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(16, 26);
            label1.Name = "label1";
            label1.Size = new Size(125, 15);
            label1.TabIndex = 0;
            label1.Text = "Absolute Static Range:";
            toolTip1.SetToolTip(label1, "The precise range from the start of the game executable to the last detected marker.");
            // 
            // progressBar
            // 
            progressBar.Location = new Point(148, 12);
            progressBar.Name = "progressBar";
            progressBar.Size = new Size(424, 28);
            progressBar.TabIndex = 5;
            // 
            // Pcsx2RamScanRangeFinderForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(584, 441);
            Controls.Add(progressBar);
            Controls.Add(groupBox1);
            Controls.Add(richLog);
            Controls.Add(btnApply);
            Controls.Add(btnClose);
            Controls.Add(btnFind);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "Pcsx2RamScanRangeFinderForm";
            ShowInTaskbar = false;
            StartPosition = FormStartPosition.CenterParent;
            Text = "PCSX2 Static Range Finder";
            Load += Pcsx2RamScanRangeFinderForm_Load;
            groupBox1.ResumeLayout(false);
            groupBox1.PerformLayout();
            ResumeLayout(false);

        }
        #endregion

        private System.Windows.Forms.Button btnFind;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.Button btnApply;
        private System.Windows.Forms.RichTextBox richLog;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lblApproxResult;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lblAbsoluteResult;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.ProgressBar progressBar;
    }
}