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
            this.components = new System.ComponentModel.Container();
            this.btnFind = new System.Windows.Forms.Button();
            this.btnClose = new System.Windows.Forms.Button();
            this.btnApply = new System.Windows.Forms.Button();
            this.richLog = new System.Windows.Forms.RichTextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.lblApproxResult = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.lblAbsoluteResult = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnFind
            // 
            this.btnFind.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            this.btnFind.Location = new System.Drawing.Point(12, 12);
            this.btnFind.Name = "btnFind";
            this.btnFind.Size = new System.Drawing.Size(130, 28);
            this.btnFind.TabIndex = 0;
            this.btnFind.Text = "Find Static Range";
            this.btnFind.UseVisualStyleBackColor = true;
            this.btnFind.Click += new System.EventHandler(this.btnFind_Click);
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Location = new System.Drawing.Point(497, 401);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 28);
            this.btnClose.TabIndex = 1;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            // 
            // btnApply
            // 
            this.btnApply.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnApply.Enabled = false;
            this.btnApply.Location = new System.Drawing.Point(379, 401);
            this.btnApply.Name = "btnApply";
            this.btnApply.Size = new System.Drawing.Size(112, 28);
            this.btnApply.TabIndex = 2;
            this.btnApply.Text = "Apply and Close";
            this.toolTip1.SetToolTip(this.btnApply, "Saves the \'Approximate\' range to your PCSX2 settings profile.");
            this.btnApply.UseVisualStyleBackColor = true;
            this.btnApply.Click += new System.EventHandler(this.btnApply_Click);
            // 
            // richLog
            // 
            this.richLog.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.richLog.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))));
            this.richLog.Font = new System.Drawing.Font("Consolas", 9F);
            this.richLog.ForeColor = System.Drawing.Color.Gainsboro;
            this.richLog.Location = new System.Drawing.Point(12, 46);
            this.richLog.Name = "richLog";
            this.richLog.ReadOnly = true;
            this.richLog.Size = new System.Drawing.Size(560, 260);
            this.richLog.TabIndex = 3;
            this.richLog.Text = "";
            this.richLog.WordWrap = true;
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.lblApproxResult);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.lblAbsoluteResult);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(12, 312);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(560, 83);
            this.groupBox1.TabIndex = 4;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Results";
            // 
            // lblApproxResult
            // 
            this.lblApproxResult.AutoSize = true;
            this.lblApproxResult.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblApproxResult.Location = new System.Drawing.Point(212, 53);
            this.lblApproxResult.Name = "lblApproxResult";
            this.lblApproxResult.Size = new System.Drawing.Size(21, 15);
            this.lblApproxResult.TabIndex = 3;
            this.lblApproxResult.Text = "N/A";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(16, 53);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(190, 15);
            this.label3.TabIndex = 2;
            this.label3.Text = "Approximate 1MB Block (for scan):";
            this.toolTip1.SetToolTip(this.label3, "A calculated 1MB range that should contain all static addresses.\r\nThis is the ra" +
        "nge that will be applied to your settings.");
            // 
            // lblAbsoluteResult
            // 
            this.lblAbsoluteResult.AutoSize = true;
            this.lblAbsoluteResult.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblAbsoluteResult.Location = new System.Drawing.Point(212, 26);
            this.lblAbsoluteResult.Name = "lblAbsoluteResult";
            this.lblAbsoluteResult.Size = new System.Drawing.Size(21, 15);
            this.lblAbsoluteResult.TabIndex = 1;
            this.lblAbsoluteResult.Text = "N/A";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(16, 26);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(127, 15);
            this.label1.TabIndex = 0;
            this.label1.Text = "Absolute Static Range:";
            this.toolTip1.SetToolTip(this.label1, "The precise range from the start of the game executable to the last detected mar" +
        "ker.");
            // 
            // progressBar
            // 
            this.progressBar.Location = new System.Drawing.Point(148, 12);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(424, 28);
            this.progressBar.TabIndex = 5;
            // 
            // Pcsx2RamScanRangeFinderForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(584, 441);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.richLog);
            this.Controls.Add(this.btnApply);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.btnFind);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Pcsx2RamScanRangeFinderForm";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "PCSX2 Static Range Finder";
            this.Load += new System.EventHandler(this.Pcsx2RamScanRangeFinderForm_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

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