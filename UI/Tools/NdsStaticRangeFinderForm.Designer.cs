namespace PointerFinder2.UI.StaticRangeFinders
{
    partial class NdsStaticRangeFinderForm
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
            label1 = new Label();
            richLog = new RichTextBox();
            panelTop = new Panel();
            btnAnalyze = new Button();
            btnBrowse = new Button();
            txtRomPath = new TextBox();
            label2 = new Label();
            groupBox1 = new GroupBox();
            lblResult = new Label();
            panelTop.SuspendLayout();
            groupBox1.SuspendLayout();
            SuspendLayout();
            // 
            // btnClose
            // 
            btnClose.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnClose.DialogResult = DialogResult.Cancel;
            btnClose.Location = new Point(497, 349);
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
            btnApply.Location = new Point(379, 349);
            btnApply.Name = "btnApply";
            btnApply.Size = new Size(112, 28);
            btnApply.TabIndex = 2;
            btnApply.Text = "Apply and Close";
            toolTip1.SetToolTip(btnApply, "Saves the suggested range to your NDS settings profile.");
            btnApply.UseVisualStyleBackColor = true;
            btnApply.Click += btnApply_Click;
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
            richLog.ForeColor = Color.Gainsboro;
            richLog.Location = new Point(12, 59);
            richLog.Name = "richLog";
            richLog.ReadOnly = true;
            richLog.Size = new Size(560, 219);
            richLog.TabIndex = 6;
            richLog.Text = "";
            // 
            // panelTop
            // 
            panelTop.Controls.Add(btnAnalyze);
            panelTop.Controls.Add(btnBrowse);
            panelTop.Controls.Add(txtRomPath);
            panelTop.Controls.Add(label2);
            panelTop.Dock = DockStyle.Top;
            panelTop.Location = new Point(0, 0);
            panelTop.Name = "panelTop";
            panelTop.Size = new Size(584, 53);
            panelTop.TabIndex = 7;
            // 
            // btnAnalyze
            // 
            btnAnalyze.Enabled = false;
            btnAnalyze.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
            btnAnalyze.Location = new Point(474, 18);
            btnAnalyze.Name = "btnAnalyze";
            btnAnalyze.Size = new Size(98, 23);
            btnAnalyze.TabIndex = 3;
            btnAnalyze.Text = "Analyze ROM";
            btnAnalyze.UseVisualStyleBackColor = true;
            btnAnalyze.Click += btnAnalyze_Click;
            // 
            // btnBrowse
            // 
            btnBrowse.Location = new Point(393, 18);
            btnBrowse.Name = "btnBrowse";
            btnBrowse.Size = new Size(75, 23);
            btnBrowse.TabIndex = 2;
            btnBrowse.Text = "Browse...";
            btnBrowse.UseVisualStyleBackColor = true;
            btnBrowse.Click += btnBrowse_Click;
            // 
            // txtRomPath
            // 
            txtRomPath.Location = new Point(12, 19);
            txtRomPath.Name = "txtRomPath";
            txtRomPath.Size = new Size(375, 23);
            txtRomPath.TabIndex = 1;
            txtRomPath.TextChanged += txtRomPath_TextChanged;
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new Point(12, 1);
            label2.Name = "label2";
            label2.Size = new Size(84, 15);
            label2.TabIndex = 0;
            label2.Text = "NDS ROM File:";
            // 
            // groupBox1
            // 
            groupBox1.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            groupBox1.Controls.Add(lblResult);
            groupBox1.Controls.Add(label1);
            groupBox1.Location = new Point(12, 284);
            groupBox1.Name = "groupBox1";
            groupBox1.Size = new Size(560, 59);
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
            // NdsStaticRangeFinderForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(584, 389);
            Controls.Add(groupBox1);
            Controls.Add(panelTop);
            Controls.Add(richLog);
            Controls.Add(btnApply);
            Controls.Add(btnClose);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "NdsStaticRangeFinderForm";
            ShowInTaskbar = false;
            StartPosition = FormStartPosition.CenterParent;
            Text = "NDS Static Range Finder";
            Load += NdsStaticRangeFinderForm_Load;
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
        private System.Windows.Forms.TextBox txtRomPath;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lblResult;
        private System.Windows.Forms.Label label1;
    }
}