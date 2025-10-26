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
            this.btnClose = new System.Windows.Forms.Button();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabToNote = new System.Windows.Forms.TabPage();
            this.btnCopyToClipboard = new System.Windows.Forms.Button();
            this.comboMemorySize = new System.Windows.Forms.ComboBox();
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.richCodeNoteOutput = new System.Windows.Forms.RichTextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.btnConvert = new System.Windows.Forms.Button();
            this.txtTriggerInput = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.tabToTrigger = new System.Windows.Forms.TabPage();
            this.chkUseMask = new System.Windows.Forms.CheckBox();
            this.btnCopyTrigger = new System.Windows.Forms.Button();
            this.label8 = new System.Windows.Forms.Label();
            this.txtTriggerOutput = new System.Windows.Forms.TextBox();
            this.btnReconvert = new System.Windows.Forms.Button();
            this.comboPointerPrefix = new System.Windows.Forms.ComboBox();
            this.label7 = new System.Windows.Forms.Label();
            this.txtBaseAddress = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.richCodeNoteInput = new System.Windows.Forms.RichTextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.tabControl1.SuspendLayout();
            this.tabToNote.SuspendLayout();
            this.tabToTrigger.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Location = new System.Drawing.Point(509, 303);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 23);
            this.btnClose.TabIndex = 5;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            // 
            // tabControl1
            // 
            this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabControl1.Controls.Add(this.tabToNote);
            this.tabControl1.Controls.Add(this.tabToTrigger);
            this.tabControl1.Location = new System.Drawing.Point(0, 0);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(597, 297);
            this.tabControl1.TabIndex = 6;
            // 
            // tabToNote
            // 
            this.tabToNote.Controls.Add(this.btnCopyToClipboard);
            this.tabToNote.Controls.Add(this.comboMemorySize);
            this.tabToNote.Controls.Add(this.txtDescription);
            this.tabToNote.Controls.Add(this.label4);
            this.tabToNote.Controls.Add(this.label3);
            this.tabToNote.Controls.Add(this.richCodeNoteOutput);
            this.tabToNote.Controls.Add(this.label2);
            this.tabToNote.Controls.Add(this.btnConvert);
            this.tabToNote.Controls.Add(this.txtTriggerInput);
            this.tabToNote.Controls.Add(this.label1);
            this.tabToNote.Location = new System.Drawing.Point(4, 24);
            this.tabToNote.Name = "tabToNote";
            this.tabToNote.Padding = new System.Windows.Forms.Padding(3);
            this.tabToNote.Size = new System.Drawing.Size(589, 269);
            this.tabToNote.TabIndex = 0;
            this.tabToNote.Text = "RA Trigger to Code Note";
            this.tabToNote.UseVisualStyleBackColor = true;
            // 
            // btnCopyToClipboard
            // 
            this.btnCopyToClipboard.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCopyToClipboard.Location = new System.Drawing.Point(459, 238);
            this.btnCopyToClipboard.Name = "btnCopyToClipboard";
            this.btnCopyToClipboard.Size = new System.Drawing.Size(122, 23);
            this.btnCopyToClipboard.TabIndex = 15;
            this.btnCopyToClipboard.Text = "Copy to Clipboard";
            this.btnCopyToClipboard.UseVisualStyleBackColor = true;
            this.btnCopyToClipboard.Click += new System.EventHandler(this.btnCopyToClipboard_Click);
            // 
            // comboMemorySize
            // 
            this.comboMemorySize.FormattingEnabled = true;
            this.comboMemorySize.Location = new System.Drawing.Point(10, 78);
            this.comboMemorySize.Name = "comboMemorySize";
            this.comboMemorySize.Size = new System.Drawing.Size(127, 23);
            this.comboMemorySize.TabIndex = 12;
            this.comboMemorySize.TextChanged += new System.EventHandler(this.comboMemorySize_TextChanged);
            // 
            // txtDescription
            // 
            this.txtDescription.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtDescription.Location = new System.Drawing.Point(143, 78);
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.Size = new System.Drawing.Size(438, 23);
            this.txtDescription.TabIndex = 13;
            this.txtDescription.TextChanged += new System.EventHandler(this.txtDescription_TextChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(143, 60);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(103, 15);
            this.label4.TabIndex = 18;
            this.label4.Text = "Description (final):";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(10, 60);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(109, 15);
            this.label3.TabIndex = 16;
            this.label3.Text = "Memory Size (final):";
            // 
            // richCodeNoteOutput
            // 
            this.richCodeNoteOutput.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.richCodeNoteOutput.Font = new System.Drawing.Font("Consolas", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.richCodeNoteOutput.Location = new System.Drawing.Point(10, 132);
            this.richCodeNoteOutput.Name = "richCodeNoteOutput";
            this.richCodeNoteOutput.ReadOnly = true;
            this.richCodeNoteOutput.Size = new System.Drawing.Size(571, 100);
            this.richCodeNoteOutput.TabIndex = 14;
            this.richCodeNoteOutput.Text = "";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(10, 114);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(127, 15);
            this.label2.TabIndex = 13;
            this.label2.Text = "Formatted Code Note:";
            // 
            // btnConvert
            // 
            this.btnConvert.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnConvert.Location = new System.Drawing.Point(487, 25);
            this.btnConvert.Name = "btnConvert";
            this.btnConvert.Size = new System.Drawing.Size(94, 23);
            this.btnConvert.TabIndex = 11;
            this.btnConvert.Text = "Convert";
            this.btnConvert.UseVisualStyleBackColor = true;
            this.btnConvert.Click += new System.EventHandler(this.btnConvert_Click);
            // 
            // txtTriggerInput
            // 
            this.txtTriggerInput.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtTriggerInput.Location = new System.Drawing.Point(10, 25);
            this.txtTriggerInput.Name = "txtTriggerInput";
            this.txtTriggerInput.Size = new System.Drawing.Size(471, 23);
            this.txtTriggerInput.TabIndex = 10;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 7);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(183, 15);
            this.label1.TabIndex = 9;
            this.label1.Text = "Input RetroAchievements Trigger:";
            // 
            // tabToTrigger
            // 
            this.tabToTrigger.Controls.Add(this.chkUseMask);
            this.tabToTrigger.Controls.Add(this.btnCopyTrigger);
            this.tabToTrigger.Controls.Add(this.label8);
            this.tabToTrigger.Controls.Add(this.txtTriggerOutput);
            this.tabToTrigger.Controls.Add(this.btnReconvert);
            this.tabToTrigger.Controls.Add(this.comboPointerPrefix);
            this.tabToTrigger.Controls.Add(this.label7);
            this.tabToTrigger.Controls.Add(this.txtBaseAddress);
            this.tabToTrigger.Controls.Add(this.label6);
            this.tabToTrigger.Controls.Add(this.richCodeNoteInput);
            this.tabToTrigger.Controls.Add(this.label5);
            this.tabToTrigger.Location = new System.Drawing.Point(4, 24);
            this.tabToTrigger.Name = "tabToTrigger";
            this.tabToTrigger.Padding = new System.Windows.Forms.Padding(3);
            this.tabToTrigger.Size = new System.Drawing.Size(589, 269);
            this.tabToTrigger.TabIndex = 1;
            this.tabToTrigger.Text = "Code Note to RA Trigger";
            this.tabToTrigger.UseVisualStyleBackColor = true;
            // 
            // chkUseMask
            // 
            this.chkUseMask.AutoSize = true;
            this.chkUseMask.Location = new System.Drawing.Point(270, 163);
            this.chkUseMask.Name = "chkUseMask";
            this.chkUseMask.Size = new System.Drawing.Size(77, 19);
            this.chkUseMask.TabIndex = 10;
            this.chkUseMask.Text = "Use Mask";
            this.chkUseMask.UseVisualStyleBackColor = true;
            // 
            // btnCopyTrigger
            // 
            this.btnCopyTrigger.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCopyTrigger.Location = new System.Drawing.Point(459, 238);
            this.btnCopyTrigger.Name = "btnCopyTrigger";
            this.btnCopyTrigger.Size = new System.Drawing.Size(122, 23);
            this.btnCopyTrigger.TabIndex = 9;
            this.btnCopyTrigger.Text = "Copy to Clipboard";
            this.btnCopyTrigger.UseVisualStyleBackColor = true;
            this.btnCopyTrigger.Click += new System.EventHandler(this.btnCopyTrigger_Click);
            // 
            // label8
            // 
            this.label8.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(10, 192);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(141, 15);
            this.label8.TabIndex = 8;
            this.label8.Text = "Output RA Trigger String:";
            // 
            // txtTriggerOutput
            // 
            this.txtTriggerOutput.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtTriggerOutput.Font = new System.Drawing.Font("Consolas", 9F);
            this.txtTriggerOutput.Location = new System.Drawing.Point(10, 210);
            this.txtTriggerOutput.Name = "txtTriggerOutput";
            this.txtTriggerOutput.ReadOnly = true;
            this.txtTriggerOutput.Size = new System.Drawing.Size(571, 22);
            this.txtTriggerOutput.TabIndex = 7;
            // 
            // btnReconvert
            // 
            this.btnReconvert.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnReconvert.Location = new System.Drawing.Point(487, 161);
            this.btnReconvert.Name = "btnReconvert";
            this.btnReconvert.Size = new System.Drawing.Size(94, 23);
            this.btnReconvert.TabIndex = 6;
            this.btnReconvert.Text = "Re-Convert";
            this.btnReconvert.UseVisualStyleBackColor = true;
            this.btnReconvert.Click += new System.EventHandler(this.btnReconvert_Click);
            // 
            // comboPointerPrefix
            // 
            this.comboPointerPrefix.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboPointerPrefix.FormattingEnabled = true;
            this.comboPointerPrefix.Location = new System.Drawing.Point(143, 161);
            this.comboPointerPrefix.Name = "comboPointerPrefix";
            this.comboPointerPrefix.Size = new System.Drawing.Size(121, 23);
            this.comboPointerPrefix.TabIndex = 5;
            this.comboPointerPrefix.SelectedIndexChanged += new System.EventHandler(this.comboPointerPrefix_SelectedIndexChanged);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(143, 143);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(82, 15);
            this.label7.TabIndex = 4;
            this.label7.Text = "Pointer Prefix:";
            // 
            // txtBaseAddress
            // 
            this.txtBaseAddress.Location = new System.Drawing.Point(10, 161);
            this.txtBaseAddress.Name = "txtBaseAddress";
            this.txtBaseAddress.Size = new System.Drawing.Size(127, 23);
            this.txtBaseAddress.TabIndex = 3;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(10, 143);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(109, 15);
            this.label6.TabIndex = 2;
            this.label6.Text = "Base Address (Hex):";
            // 
            // richCodeNoteInput
            // 
            this.richCodeNoteInput.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.richCodeNoteInput.Font = new System.Drawing.Font("Consolas", 9F);
            this.richCodeNoteInput.Location = new System.Drawing.Point(10, 25);
            this.richCodeNoteInput.Name = "richCodeNoteInput";
            this.richCodeNoteInput.Size = new System.Drawing.Size(571, 115);
            this.richCodeNoteInput.TabIndex = 1;
            this.richCodeNoteInput.Text = "";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(10, 7);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(101, 15);
            this.label5.TabIndex = 0;
            this.label5.Text = "Input Code Note:";
            // 
            // CodeNoteConverterForm
            // 
            this.AcceptButton = this.btnConvert;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnClose;
            this.ClientSize = new System.Drawing.Size(596, 338);
            this.Controls.Add(this.tabControl1);
            this.Controls.Add(this.btnClose);
            this.MinimumSize = new System.Drawing.Size(450, 350);
            this.Name = "CodeNoteConverterForm";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Code Note Converter";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.CodeNoteConverterForm_FormClosing);
            this.Load += new System.EventHandler(this.CodeNoteConverterForm_Load);
            this.tabControl1.ResumeLayout(false);
            this.tabToNote.ResumeLayout(false);
            this.tabToNote.PerformLayout();
            this.tabToTrigger.ResumeLayout(false);
            this.tabToTrigger.PerformLayout();
            this.ResumeLayout(false);

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