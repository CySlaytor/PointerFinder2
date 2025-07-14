namespace PointerFinder2
{
    partial class DebugOptionsForm
    {
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.CheckBox chkLogFilter;
        private System.Windows.Forms.CheckBox chkLogLiveScan;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.CheckBox chkLogRefineScan;

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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chkLogRefineScan = new System.Windows.Forms.CheckBox();
            this.chkLogFilter = new System.Windows.Forms.CheckBox();
            this.chkLogLiveScan = new System.Windows.Forms.CheckBox();
            this.btnClose = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chkLogRefineScan);
            this.groupBox1.Controls.Add(this.chkLogFilter);
            this.groupBox1.Controls.Add(this.chkLogLiveScan);
            this.groupBox1.Location = new System.Drawing.Point(12, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(288, 120);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Logging and Debug Toggles";
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
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnClose.Location = new System.Drawing.Point(213, 138);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(87, 28);
            this.btnClose.TabIndex = 1;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // DebugOptionsForm
            // 
            this.AcceptButton = this.btnClose;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(312, 178);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.groupBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DebugOptionsForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Debug Options";
            this.Load += new System.EventHandler(this.DebugOptionsForm_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }
        #endregion
    }
}