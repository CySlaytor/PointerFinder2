namespace PointerFinder2.UI.StaticRangeFinders
{
    partial class DolphinFileRangeFinderForm
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
            this.btnClose = new System.Windows.Forms.Button();
            this.btnApply = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.richLog = new System.Windows.Forms.RichTextBox();
            this.panelTop = new System.Windows.Forms.Panel();
            this.lblBorderSizeValue = new System.Windows.Forms.Label();
            this.trackBarBorderSize = new System.Windows.Forms.TrackBar();
            this.btnAnalyze = new System.Windows.Forms.Button();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.txtFilePath = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.lblResult = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.panelTop.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarBorderSize)).BeginInit();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Location = new System.Drawing.Point(597, 399);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 28);
            this.btnClose.TabIndex = 3;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            // 
            // btnApply
            // 
            this.btnApply.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnApply.Enabled = false;
            this.btnApply.Location = new System.Drawing.Point(479, 399);
            this.btnApply.Name = "btnApply";
            this.btnApply.Size = new System.Drawing.Size(112, 28);
            this.btnApply.TabIndex = 2;
            this.btnApply.Text = "Apply and Close";
            this.toolTip1.SetToolTip(this.btnApply, "Saves the suggested range to your Dolphin settings profile.");
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
            this.richLog.Location = new System.Drawing.Point(12, 89);
            this.richLog.Name = "richLog";
            this.richLog.ReadOnly = true;
            this.richLog.Size = new System.Drawing.Size(660, 239);
            this.richLog.TabIndex = 6;
            this.richLog.Text = "";
            this.richLog.WordWrap = true;
            // 
            // panelTop
            // 
            this.panelTop.Controls.Add(this.lblBorderSizeValue);
            this.panelTop.Controls.Add(this.trackBarBorderSize);
            this.panelTop.Controls.Add(this.btnAnalyze);
            this.panelTop.Controls.Add(this.btnBrowse);
            this.panelTop.Controls.Add(this.txtFilePath);
            this.panelTop.Controls.Add(this.label2);
            this.panelTop.Dock = System.Windows.Forms.DockStyle.Top;
            this.panelTop.Location = new System.Drawing.Point(0, 0);
            this.panelTop.Name = "panelTop";
            this.panelTop.Size = new System.Drawing.Size(684, 83);
            this.panelTop.TabIndex = 7;
            // 
            // lblBorderSizeValue
            // 
            this.lblBorderSizeValue.AutoSize = true;
            this.lblBorderSizeValue.Location = new System.Drawing.Point(12, 38);
            this.lblBorderSizeValue.Name = "lblBorderSizeValue";
            this.lblBorderSizeValue.Size = new System.Drawing.Size(109, 15);
            this.lblBorderSizeValue.TabIndex = 5;
            this.lblBorderSizeValue.Text = "Border Size: 1024 KB";
            // 
            // trackBarBorderSize
            // 
            this.trackBarBorderSize.LargeChange = 256;
            this.trackBarBorderSize.Location = new System.Drawing.Point(12, 56);
            this.trackBarBorderSize.Maximum = 2048;
            this.trackBarBorderSize.Name = "trackBarBorderSize";
            this.trackBarBorderSize.Size = new System.Drawing.Size(475, 45);
            this.trackBarBorderSize.SmallChange = 64;
            this.trackBarBorderSize.TabIndex = 4;
            this.trackBarBorderSize.TickFrequency = 128;
            this.toolTip1.SetToolTip(this.trackBarBorderSize, "The size of the memory border to add around the detected static region.\r\nHigher " +
        "values provide a safer, wider search range but may be less precise.");
            this.trackBarBorderSize.Value = 1024;
            this.trackBarBorderSize.Scroll += new System.EventHandler(this.trackBarBorderSize_Scroll);
            // 
            // btnAnalyze
            // 
            this.btnAnalyze.Enabled = false;
            this.btnAnalyze.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            this.btnAnalyze.Location = new System.Drawing.Point(574, 18);
            this.btnAnalyze.Name = "btnAnalyze";
            this.btnAnalyze.Size = new System.Drawing.Size(98, 23);
            this.btnAnalyze.TabIndex = 3;
            this.btnAnalyze.Text = "Analyze File";
            this.btnAnalyze.UseVisualStyleBackColor = true;
            this.btnAnalyze.Click += new System.EventHandler(this.btnAnalyze_Click);
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(493, 18);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(75, 23);
            this.btnBrowse.TabIndex = 2;
            this.btnBrowse.Text = "Browse...";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // txtFilePath
            // 
            this.txtFilePath.Location = new System.Drawing.Point(12, 19);
            this.txtFilePath.Name = "txtFilePath";
            this.txtFilePath.Size = new System.Drawing.Size(475, 23);
            this.txtFilePath.TabIndex = 1;
            this.txtFilePath.TextChanged += new System.EventHandler(this.txtFilePath_TextChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 1);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(84, 15);
            this.label2.TabIndex = 0;
            this.label2.Text = "main.dol File:";
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.lblResult);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(12, 334);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(660, 59);
            this.groupBox1.TabIndex = 8;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Results";
            // 
            // lblResult
            // 
            this.lblResult.AutoSize = true;
            this.lblResult.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblResult.Location = new System.Drawing.Point(180, 26);
            this.lblResult.Name = "lblResult";
            this.lblResult.Size = new System.Drawing.Size(21, 15);
            this.lblResult.TabIndex = 1;
            this.lblResult.Text = "N/A";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(16, 26);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(158, 15);
            this.label1.TabIndex = 0;
            this.label1.Text = "Suggested Range (for scan):";
            this.toolTip1.SetToolTip(this.label1, "The calculated range that should contain all static addresses.\r\nThis is the rang" +
        "e that will be applied to your settings.");
            // 
            // DolphinFileRangeFinderForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(684, 439);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.panelTop);
            this.Controls.Add(this.richLog);
            this.Controls.Add(this.btnApply);
            this.Controls.Add(this.btnClose);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DolphinFileRangeFinderForm";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Dolphin/GC Static Range Finder";
            this.Load += new System.EventHandler(this.DolphinFileRangeFinderForm_Load);
            this.panelTop.ResumeLayout(false);
            this.panelTop.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarBorderSize)).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.Button btnApply;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.RichTextBox richLog;
        private System.Windows.Forms.Panel panelTop;
        private System.Windows.Forms.Button btnAnalyze;
        private System.Windows.Forms.Button btnBrowse;
        private System.Windows.Forms.TextBox txtFilePath;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lblResult;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TrackBar trackBarBorderSize;
        private System.Windows.Forms.Label lblBorderSizeValue;
    }
}