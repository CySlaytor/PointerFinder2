namespace PointerFinder2.UI
{
    partial class CodeNoteConverterForm
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
            btnClose = new Button();
            tabControl1 = new TabControl();
            tabToNote = new TabPage();
            btnCopyToClipboard = new Button();
            comboMemorySize = new ComboBox();
            txtDescription = new TextBox();
            label4 = new Label();
            label3 = new Label();
            richCodeNoteOutput = new RichTextBox();
            label2 = new Label();
            btnConvert = new Button();
            txtTriggerInput = new TextBox();
            label1 = new Label();
            tabToTrigger = new TabPage();
            chkUseMask = new CheckBox();
            btnCopyTrigger = new Button();
            label8 = new Label();
            txtTriggerOutput = new TextBox();
            btnReconvert = new Button();
            comboPointerPrefix = new ComboBox();
            label7 = new Label();
            txtBaseAddress = new TextBox();
            label6 = new Label();
            richCodeNoteInput = new RichTextBox();
            label5 = new Label();
            tabControl1.SuspendLayout();
            tabToNote.SuspendLayout();
            tabToTrigger.SuspendLayout();
            SuspendLayout();
            // 
            // btnClose
            // 
            btnClose.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnClose.BackColor = Color.Transparent;
            btnClose.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnClose.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnClose.ForeColor = SystemColors.ControlText;
            btnClose.Location = new Point(509, 365);
            btnClose.Name = "btnClose";
            btnClose.Size = new Size(75, 23);
            btnClose.TabIndex = 5;
            btnClose.Text = "Close";
            btnClose.UseVisualStyleBackColor = false;
            btnClose.Click += btnClose_Click;
            // 
            // tabControl1
            // 
            tabControl1.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            tabControl1.Controls.Add(tabToNote);
            tabControl1.Controls.Add(tabToTrigger);
            tabControl1.ForeColor = Color.White;
            tabControl1.Location = new Point(0, 0);
            tabControl1.Name = "tabControl1";
            tabControl1.SelectedIndex = 0;
            tabControl1.Size = new Size(597, 359);
            tabControl1.TabIndex = 6;
            // 
            // tabToNote
            // 
            tabToNote.BackColor = Color.Transparent;
            tabToNote.Controls.Add(btnCopyToClipboard);
            tabToNote.Controls.Add(comboMemorySize);
            tabToNote.Controls.Add(txtDescription);
            tabToNote.Controls.Add(label4);
            tabToNote.Controls.Add(label3);
            tabToNote.Controls.Add(richCodeNoteOutput);
            tabToNote.Controls.Add(label2);
            tabToNote.Controls.Add(btnConvert);
            tabToNote.Controls.Add(txtTriggerInput);
            tabToNote.Controls.Add(label1);
            tabToNote.ForeColor = SystemColors.ControlText;
            tabToNote.Location = new Point(4, 24);
            tabToNote.Name = "tabToNote";
            tabToNote.Padding = new Padding(3);
            tabToNote.Size = new Size(589, 331);
            tabToNote.TabIndex = 0;
            tabToNote.Text = "RA Trigger to Code Note";
            tabToNote.UseVisualStyleBackColor = true;
            tabToNote.Click += tabToNote_Click;
            // 
            // btnCopyToClipboard
            // 
            btnCopyToClipboard.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnCopyToClipboard.BackColor = Color.Transparent;
            btnCopyToClipboard.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnCopyToClipboard.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnCopyToClipboard.ForeColor = SystemColors.ControlText;
            btnCopyToClipboard.Location = new Point(457, 298);
            btnCopyToClipboard.Name = "btnCopyToClipboard";
            btnCopyToClipboard.Size = new Size(122, 23);
            btnCopyToClipboard.TabIndex = 15;
            btnCopyToClipboard.Text = "Copy to Clipboard";
            btnCopyToClipboard.UseVisualStyleBackColor = false;
            btnCopyToClipboard.Click += btnCopyToClipboard_Click;
            // 
            // comboMemorySize
            // 
            comboMemorySize.BackColor = SystemColors.Window;
            comboMemorySize.ForeColor = SystemColors.WindowText;
            comboMemorySize.FormattingEnabled = true;
            comboMemorySize.Location = new Point(10, 78);
            comboMemorySize.Name = "comboMemorySize";
            comboMemorySize.Size = new Size(127, 23);
            comboMemorySize.TabIndex = 12;
            comboMemorySize.TextChanged += comboMemorySize_TextChanged;
            // 
            // txtDescription
            // 
            txtDescription.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            txtDescription.BackColor = SystemColors.Window;
            txtDescription.ForeColor = SystemColors.WindowText;
            txtDescription.Location = new Point(143, 78);
            txtDescription.Name = "txtDescription";
            txtDescription.Size = new Size(436, 23);
            txtDescription.TabIndex = 13;
            txtDescription.TextChanged += txtDescription_TextChanged;
            // 
            // label4
            // 
            label4.AutoSize = true;
            label4.BackColor = Color.Transparent;
            label4.ForeColor = SystemColors.ControlText;
            label4.Location = new Point(143, 60);
            label4.Name = "label4";
            label4.Size = new Size(104, 15);
            label4.TabIndex = 18;
            label4.Text = "Description (final):";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.BackColor = Color.Transparent;
            label3.ForeColor = SystemColors.ControlText;
            label3.Location = new Point(10, 60);
            label3.Name = "label3";
            label3.Size = new Size(112, 15);
            label3.TabIndex = 16;
            label3.Text = "Memory Size (final):";
            // 
            // richCodeNoteOutput
            // 
            richCodeNoteOutput.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            richCodeNoteOutput.BackColor = SystemColors.Control;
            richCodeNoteOutput.Font = new Font("Consolas", 9F);
            richCodeNoteOutput.ForeColor = SystemColors.WindowText;
            richCodeNoteOutput.Location = new Point(10, 132);
            richCodeNoteOutput.Name = "richCodeNoteOutput";
            richCodeNoteOutput.ReadOnly = true;
            richCodeNoteOutput.Size = new Size(569, 160);
            richCodeNoteOutput.TabIndex = 14;
            richCodeNoteOutput.Text = "";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.BackColor = Color.Transparent;
            label2.ForeColor = SystemColors.ControlText;
            label2.Location = new Point(10, 114);
            label2.Name = "label2";
            label2.Size = new Size(125, 15);
            label2.TabIndex = 13;
            label2.Text = "Formatted Code Note:";
            // 
            // btnConvert
            // 
            btnConvert.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnConvert.BackColor = Color.Transparent;
            btnConvert.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnConvert.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnConvert.ForeColor = SystemColors.ControlText;
            btnConvert.Location = new Point(485, 25);
            btnConvert.Name = "btnConvert";
            btnConvert.Size = new Size(94, 23);
            btnConvert.TabIndex = 11;
            btnConvert.Text = "Convert";
            btnConvert.UseVisualStyleBackColor = false;
            btnConvert.Click += btnConvert_Click;
            // 
            // txtTriggerInput
            // 
            txtTriggerInput.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            txtTriggerInput.BackColor = SystemColors.Window;
            txtTriggerInput.ForeColor = SystemColors.WindowText;
            txtTriggerInput.Location = new Point(10, 25);
            txtTriggerInput.Name = "txtTriggerInput";
            txtTriggerInput.Size = new Size(469, 23);
            txtTriggerInput.TabIndex = 10;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.BackColor = Color.Transparent;
            label1.ForeColor = SystemColors.ControlText;
            label1.Location = new Point(10, 7);
            label1.Name = "label1";
            label1.Size = new Size(184, 15);
            label1.TabIndex = 9;
            label1.Text = "Input RetroAchievements Trigger:";
            // 
            // tabToTrigger
            // 
            tabToTrigger.BackColor = Color.Transparent;
            tabToTrigger.Controls.Add(chkUseMask);
            tabToTrigger.Controls.Add(btnCopyTrigger);
            tabToTrigger.Controls.Add(label8);
            tabToTrigger.Controls.Add(txtTriggerOutput);
            tabToTrigger.Controls.Add(btnReconvert);
            tabToTrigger.Controls.Add(comboPointerPrefix);
            tabToTrigger.Controls.Add(label7);
            tabToTrigger.Controls.Add(txtBaseAddress);
            tabToTrigger.Controls.Add(label6);
            tabToTrigger.Controls.Add(richCodeNoteInput);
            tabToTrigger.Controls.Add(label5);
            tabToTrigger.ForeColor = Color.White;
            tabToTrigger.Location = new Point(4, 24);
            tabToTrigger.Name = "tabToTrigger";
            tabToTrigger.Padding = new Padding(3);
            tabToTrigger.Size = new Size(589, 331);
            tabToTrigger.TabIndex = 1;
            tabToTrigger.Text = "Code Note to RA Trigger";
            tabToTrigger.Click += tabToTrigger_Click;
            // 
            // chkUseMask
            // 
            chkUseMask.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            chkUseMask.AutoSize = true;
            chkUseMask.BackColor = Color.Transparent;
            chkUseMask.ForeColor = SystemColors.ControlText;
            chkUseMask.Location = new Point(270, 245);
            chkUseMask.Name = "chkUseMask";
            chkUseMask.Size = new Size(76, 19);
            chkUseMask.TabIndex = 10;
            chkUseMask.Text = "Use Mask";
            chkUseMask.UseVisualStyleBackColor = false;
            // 
            // btnCopyTrigger
            // 
            btnCopyTrigger.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnCopyTrigger.BackColor = Color.Transparent;
            btnCopyTrigger.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnCopyTrigger.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnCopyTrigger.ForeColor = SystemColors.ControlText;
            btnCopyTrigger.Location = new Point(457, 298);
            btnCopyTrigger.Name = "btnCopyTrigger";
            btnCopyTrigger.Size = new Size(122, 23);
            btnCopyTrigger.TabIndex = 9;
            btnCopyTrigger.Text = "Copy to Clipboard";
            btnCopyTrigger.UseVisualStyleBackColor = false;
            btnCopyTrigger.Click += btnCopyTrigger_Click;
            // 
            // label8
            // 
            label8.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            label8.AutoSize = true;
            label8.BackColor = Color.Transparent;
            label8.ForeColor = SystemColors.ControlText;
            label8.Location = new Point(10, 274);
            label8.Name = "label8";
            label8.Size = new Size(140, 15);
            label8.TabIndex = 8;
            label8.Text = "Output RA Trigger String:";
            // 
            // txtTriggerOutput
            // 
            txtTriggerOutput.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            txtTriggerOutput.BackColor = SystemColors.Control;
            txtTriggerOutput.Font = new Font("Consolas", 9F);
            txtTriggerOutput.ForeColor = SystemColors.WindowText;
            txtTriggerOutput.Location = new Point(10, 292);
            txtTriggerOutput.Name = "txtTriggerOutput";
            txtTriggerOutput.ReadOnly = true;
            txtTriggerOutput.Size = new Size(441, 22);
            txtTriggerOutput.TabIndex = 7;
            // 
            // btnReconvert
            // 
            btnReconvert.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnReconvert.BackColor = Color.Transparent;
            btnReconvert.FlatAppearance.BorderColor = Color.FromArgb(55, 55, 55);
            btnReconvert.FlatAppearance.CheckedBackColor = Color.FromArgb(136, 54, 82, 104);
            btnReconvert.ForeColor = SystemColors.ControlText;
            btnReconvert.Location = new Point(485, 243);
            btnReconvert.Name = "btnReconvert";
            btnReconvert.Size = new Size(94, 23);
            btnReconvert.TabIndex = 6;
            btnReconvert.Text = "Re-Convert";
            btnReconvert.UseVisualStyleBackColor = false;
            btnReconvert.Click += btnReconvert_Click;
            // 
            // comboPointerPrefix
            // 
            comboPointerPrefix.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            comboPointerPrefix.BackColor = Color.FromArgb(55, 55, 55);
            comboPointerPrefix.DropDownStyle = ComboBoxStyle.DropDownList;
            comboPointerPrefix.ForeColor = Color.White;
            comboPointerPrefix.FormattingEnabled = true;
            comboPointerPrefix.Location = new Point(143, 243);
            comboPointerPrefix.Name = "comboPointerPrefix";
            comboPointerPrefix.Size = new Size(121, 23);
            comboPointerPrefix.TabIndex = 5;
            comboPointerPrefix.SelectedIndexChanged += comboPointerPrefix_SelectedIndexChanged;
            // 
            // label7
            // 
            label7.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            label7.AutoSize = true;
            label7.BackColor = Color.Transparent;
            label7.ForeColor = SystemColors.ControlText;
            label7.Location = new Point(143, 225);
            label7.Name = "label7";
            label7.Size = new Size(80, 15);
            label7.TabIndex = 4;
            label7.Text = "Pointer Prefix:";
            // 
            // txtBaseAddress
            // 
            txtBaseAddress.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            txtBaseAddress.BackColor = SystemColors.Window;
            txtBaseAddress.ForeColor = Color.White;
            txtBaseAddress.Location = new Point(10, 243);
            txtBaseAddress.Name = "txtBaseAddress";
            txtBaseAddress.Size = new Size(127, 23);
            txtBaseAddress.TabIndex = 3;
            // 
            // label6
            // 
            label6.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            label6.AutoSize = true;
            label6.BackColor = Color.Transparent;
            label6.ForeColor = SystemColors.ControlText;
            label6.Location = new Point(10, 225);
            label6.Name = "label6";
            label6.Size = new Size(110, 15);
            label6.TabIndex = 2;
            label6.Text = "Base Address (Hex):";
            // 
            // richCodeNoteInput
            // 
            richCodeNoteInput.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            richCodeNoteInput.BackColor = SystemColors.Window;
            richCodeNoteInput.Font = new Font("Consolas", 9F);
            richCodeNoteInput.ForeColor = SystemColors.WindowText;
            richCodeNoteInput.Location = new Point(10, 25);
            richCodeNoteInput.Name = "richCodeNoteInput";
            richCodeNoteInput.Size = new Size(569, 197);
            richCodeNoteInput.TabIndex = 1;
            richCodeNoteInput.Text = "";
            // 
            // label5
            // 
            label5.AutoSize = true;
            label5.BackColor = Color.Transparent;
            label5.ForeColor = SystemColors.ControlText;
            label5.Location = new Point(10, 7);
            label5.Name = "label5";
            label5.Size = new Size(98, 15);
            label5.TabIndex = 0;
            label5.Text = "Input Code Note:";
            // 
            // CodeNoteConverterForm
            // 
            AcceptButton = btnConvert;
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            CancelButton = btnClose;
            ClientSize = new Size(596, 400);
            Controls.Add(tabControl1);
            Controls.Add(btnClose);
            ForeColor = SystemColors.ControlText;
            MaximizeBox = false;
            MinimizeBox = false;
            MinimumSize = new Size(520, 420);
            Name = "CodeNoteConverterForm";
            ShowIcon = false;
            StartPosition = FormStartPosition.CenterParent;
            Text = "Code Note Converter";
            FormClosing += CodeNoteConverterForm_FormClosing;
            Load += CodeNoteConverterForm_Load;
            tabControl1.ResumeLayout(false);
            tabToNote.ResumeLayout(false);
            tabToNote.PerformLayout();
            tabToTrigger.ResumeLayout(false);
            tabToTrigger.PerformLayout();
            ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabToNote;
        private System.Windows.Forms.Button btnCopyToClipboard;
        private System.Windows.Forms.ComboBox comboMemorySize;
        private System.Windows.Forms.TextBox txtDescription;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.RichTextBox richCodeNoteOutput;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button btnConvert;
        private System.Windows.Forms.TextBox txtTriggerInput;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TabPage tabToTrigger;
        private System.Windows.Forms.Button btnCopyTrigger;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox txtTriggerOutput;
        private System.Windows.Forms.Button btnReconvert;
        private System.Windows.Forms.ComboBox comboPointerPrefix;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox txtBaseAddress;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.RichTextBox richCodeNoteInput;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.CheckBox chkUseMask;
    }
}