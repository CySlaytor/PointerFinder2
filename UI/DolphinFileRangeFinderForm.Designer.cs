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
            components = new System.ComponentModel.Container();
            btnClose = new Button();
            btnApply = new Button();
            toolTip1 = new ToolTip(components);
            trackBarBorderSize = new TrackBar();
            label1 = new Label();
            richLog = new RichTextBox();
            panelTop = new Panel();
            lblBorderSizeValue = new Label();
            btnAnalyze = new Button();
            btnBrowse = new Button();
            txtFilePath = new TextBox();
            label2 = new Label();
            groupBox1 = new GroupBox();
            lblResult = new Label();
            ((System.ComponentModel.ISupportInitialize)trackBarBorderSize).BeginInit();
            panelTop.SuspendLayout();
            groupBox1.SuspendLayout();
            SuspendLayout();
            // 
            // btnClose
            // 
            btnClose.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnClose.DialogResult = DialogResult.Cancel;
            btnClose.Location = new Point(597, 399);
            btnClose.Name = "btnClose";
            btnClose.Size = new Size(75, 28);
            btnClose.TabIndex = 3;
            btnClose.Text = "Close";
            btnClose.UseVisualStyleBackColor = true;
            // 
            // btnApply
            // 
            btnApply.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnApply.Enabled = false;
            btnApply.Location = new Point(479, 399);
            btnApply.Name = "btnApply";
            btnApply.Size = new Size(112, 28);
            btnApply.TabIndex = 2;
            btnApply.Text = "Apply and Close";
            toolTip1.SetToolTip(btnApply, "Saves the suggested range to your Dolphin settings profile.");
            btnApply.UseVisualStyleBackColor = true;
            btnApply.Click += btnApply_Click;
            // 
            // trackBarBorderSize
            // 
            trackBarBorderSize.LargeChange = 256;
            trackBarBorderSize.Location = new Point(12, 56);
            trackBarBorderSize.Maximum = 2048;
            trackBarBorderSize.Name = "trackBarBorderSize";
            trackBarBorderSize.Size = new Size(475, 45);
            trackBarBorderSize.SmallChange = 64;
            trackBarBorderSize.TabIndex = 4;
            trackBarBorderSize.TickFrequency = 128;
            toolTip1.SetToolTip(trackBarBorderSize, "The size of the memory border to add around the detected static region.\r\nHigher values provide a safer, wider search range but may be less precise.");
            trackBarBorderSize.Value = 1024;
            trackBarBorderSize.Scroll += trackBarBorderSize_Scroll;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(16, 26);
            label1.Name = "label1";
            label1.Size = new Size(154, 15);
            label1.TabIndex = 0;
            label1.Text = "Suggested Range (for scan):";
            toolTip1.SetToolTip(label1, "The calculated range that should contain all static addresses.\r\nThis is the range that will be applied to your settings.");
            // 
            // richLog
            // 
            richLog.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            richLog.BackColor = Color.FromArgb(40, 40, 40);
            richLog.Font = new Font("Consolas", 9F);
            richLog.ForeColor = SystemColors.ControlLight;
            richLog.Location = new Point(12, 89);
            richLog.Name = "richLog";
            richLog.ReadOnly = true;
            richLog.Size = new Size(660, 239);
            richLog.TabIndex = 6;
            richLog.Text = "";
            // 
            // panelTop
            // 
            panelTop.Controls.Add(lblBorderSizeValue);
            panelTop.Controls.Add(trackBarBorderSize);
            panelTop.Controls.Add(btnAnalyze);
            panelTop.Controls.Add(btnBrowse);
            panelTop.Controls.Add(txtFilePath);
            panelTop.Controls.Add(label2);
            panelTop.Dock = DockStyle.Top;
            panelTop.Location = new Point(0, 0);
            panelTop.Name = "panelTop";
            panelTop.Size = new Size(684, 83);
            panelTop.TabIndex = 7;
            // 
            // lblBorderSizeValue
            // 
            lblBorderSizeValue.AutoSize = true;
            lblBorderSizeValue.Location = new Point(12, 38);
            lblBorderSizeValue.Name = "lblBorderSizeValue";
            lblBorderSizeValue.Size = new Size(112, 15);
            lblBorderSizeValue.TabIndex = 5;
            lblBorderSizeValue.Text = "Border Size: 1024 KB";
            // 
            // btnAnalyze
            // 
            btnAnalyze.Enabled = false;
            btnAnalyze.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            btnAnalyze.Location = new Point(574, 18);
            btnAnalyze.Name = "btnAnalyze";
            btnAnalyze.Size = new Size(98, 23);
            btnAnalyze.TabIndex = 3;
            btnAnalyze.Text = "Analyze File";
            btnAnalyze.UseVisualStyleBackColor = true;
            btnAnalyze.Click += btnAnalyze_Click;
            // 
            // btnBrowse
            // 
            btnBrowse.Location = new Point(493, 18);
            btnBrowse.Name = "btnBrowse";
            btnBrowse.Size = new Size(75, 23);
            btnBrowse.TabIndex = 2;
            btnBrowse.Text = "Browse...";
            btnBrowse.UseVisualStyleBackColor = true;
            btnBrowse.Click += btnBrowse_Click;
            // 
            // txtFilePath
            // 
            txtFilePath.Location = new Point(12, 19);
            txtFilePath.Name = "txtFilePath";
            txtFilePath.Size = new Size(475, 23);
            txtFilePath.TabIndex = 1;
            txtFilePath.TextChanged += txtFilePath_TextChanged;
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new Point(12, 1);
            label2.Name = "label2";
            label2.Size = new Size(78, 15);
            label2.TabIndex = 0;
            label2.Text = "main.dol File:";
            // 
            // groupBox1
            // 
            groupBox1.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            groupBox1.Controls.Add(lblResult);
            groupBox1.Controls.Add(label1);
            groupBox1.Location = new Point(12, 334);
            groupBox1.Name = "groupBox1";
            groupBox1.Size = new Size(660, 59);
            groupBox1.TabIndex = 8;
            groupBox1.TabStop = false;
            groupBox1.Text = "Results";
            // 
            // lblResult
            // 
            lblResult.AutoSize = true;
            lblResult.Font = new Font("Consolas", 9.75F, FontStyle.Bold);
            lblResult.Location = new Point(180, 26);
            lblResult.Name = "lblResult";
            lblResult.Size = new Size(28, 15);
            lblResult.TabIndex = 1;
            lblResult.Text = "N/A";
            // 
            // DolphinFileRangeFinderForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(684, 439);
            Controls.Add(groupBox1);
            Controls.Add(panelTop);
            Controls.Add(richLog);
            Controls.Add(btnApply);
            Controls.Add(btnClose);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "DolphinFileRangeFinderForm";
            ShowInTaskbar = false;
            StartPosition = FormStartPosition.CenterParent;
            Text = "Dolphin/GC Static Range Finder";
            Load += DolphinFileRangeFinderForm_Load;
            ((System.ComponentModel.ISupportInitialize)trackBarBorderSize).EndInit();
            panelTop.ResumeLayout(false);
            panelTop.PerformLayout();
            groupBox1.ResumeLayout(false);
            groupBox1.PerformLayout();
            ResumeLayout(false);

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