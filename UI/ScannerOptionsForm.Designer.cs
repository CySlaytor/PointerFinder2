namespace PointerFinder2
{
    partial class ScannerOptionsForm
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
            txtTargetAddress = new TextBox();
            label1 = new Label();
            numMaxLevel = new NumericUpDown();
            label2 = new Label();
            label3 = new Label();
            txtMaxOffset = new TextBox();
            btnOK = new Button();
            btnCancel = new Button();
            groupBoxRange = new GroupBox();
            label5 = new Label();
            txtStaticEnd = new TextBox();
            txtStaticStart = new TextBox();
            label4 = new Label();
            btnResetRange = new Button();
            label6 = new Label();
            numMaxResults = new NumericUpDown();
            chkScanForStructureBase = new CheckBox();
            toolTip1 = new ToolTip(components);
            chkUse16ByteAlignment = new CheckBox();
            txtMaxNegativeOffset = new TextBox();
            chkUseSliderRange = new CheckBox();
            panelSlider = new Panel();
            lblSliderRange = new Label();
            rangeSlider = new PointerFinder2.UI.Controls.RangeSlider();
            ((System.ComponentModel.ISupportInitialize)numMaxLevel).BeginInit();
            groupBoxRange.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)numMaxResults).BeginInit();
            panelSlider.SuspendLayout();
            SuspendLayout();
            // 
            // txtTargetAddress
            // 
            txtTargetAddress.Location = new Point(14, 28);
            txtTargetAddress.Name = "txtTargetAddress";
            txtTargetAddress.Size = new Size(126, 23);
            txtTargetAddress.TabIndex = 0;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(11, 9);
            label1.Name = "label1";
            label1.Size = new Size(141, 15);
            label1.TabIndex = 1;
            label1.Text = "Target Address (PS2, Hex)";
            // 
            // numMaxLevel
            // 
            numMaxLevel.Location = new Point(14, 76);
            numMaxLevel.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            numMaxLevel.Name = "numMaxLevel";
            numMaxLevel.Size = new Size(126, 23);
            numMaxLevel.TabIndex = 2;
            numMaxLevel.Value = new decimal(new int[] { 7, 0, 0, 0 });
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new Point(11, 57);
            label2.Name = "label2";
            label2.Size = new Size(59, 15);
            label2.TabIndex = 3;
            label2.Text = "Max Level";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new Point(167, 9);
            label3.Name = "label3";
            label3.Size = new Size(95, 15);
            label3.TabIndex = 5;
            label3.Text = "Max Offset (Hex)";
            // 
            // txtMaxOffset
            // 
            txtMaxOffset.Location = new Point(170, 28);
            txtMaxOffset.Name = "txtMaxOffset";
            txtMaxOffset.Size = new Size(126, 23);
            txtMaxOffset.TabIndex = 1;
            // 
            // btnOK
            // 
            btnOK.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnOK.Location = new Point(124, 275);
            btnOK.Name = "btnOK";
            btnOK.Size = new Size(83, 29);
            btnOK.TabIndex = 10;
            btnOK.Text = "OK";
            btnOK.UseVisualStyleBackColor = true;
            btnOK.Click += btnOK_Click;
            // 
            // btnCancel
            // 
            btnCancel.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnCancel.DialogResult = DialogResult.Cancel;
            btnCancel.Location = new Point(213, 275);
            btnCancel.Name = "btnCancel";
            btnCancel.Size = new Size(83, 29);
            btnCancel.TabIndex = 11;
            btnCancel.Text = "Cancel";
            btnCancel.UseVisualStyleBackColor = true;
            // 
            // groupBoxRange
            // 
            groupBoxRange.Controls.Add(label5);
            groupBoxRange.Controls.Add(txtStaticEnd);
            groupBoxRange.Controls.Add(txtStaticStart);
            groupBoxRange.Controls.Add(label4);
            groupBoxRange.Location = new Point(14, 115);
            groupBoxRange.Name = "groupBoxRange";
            groupBoxRange.Size = new Size(284, 59);
            groupBoxRange.TabIndex = 4;
            groupBoxRange.TabStop = false;
            groupBoxRange.Text = "Static Base Address Range (PS2, Hex)";
            // 
            // label5
            // 
            label5.AutoSize = true;
            label5.Location = new Point(135, 29);
            label5.Name = "label5";
            label5.Size = new Size(12, 15);
            label5.TabIndex = 12;
            label5.Text = "-";
            // 
            // txtStaticEnd
            // 
            txtStaticEnd.Location = new Point(152, 26);
            txtStaticEnd.Name = "txtStaticEnd";
            txtStaticEnd.Size = new Size(124, 23);
            txtStaticEnd.TabIndex = 1;
            // 
            // txtStaticStart
            // 
            txtStaticStart.Location = new Point(6, 26);
            txtStaticStart.Name = "txtStaticStart";
            txtStaticStart.Size = new Size(124, 23);
            txtStaticStart.TabIndex = 0;
            // 
            // label4
            // 
            label4.AutoSize = true;
            label4.Location = new Point(11, 57);
            label4.Name = "label4";
            label4.Size = new Size(59, 15);
            label4.TabIndex = 3;
            label4.Text = "Max Level";
            // 
            // btnResetRange
            // 
            btnResetRange.Location = new Point(218, 183);
            btnResetRange.Name = "btnResetRange";
            btnResetRange.Size = new Size(75, 23);
            btnResetRange.TabIndex = 13;
            btnResetRange.Text = "Reset";
            toolTip1.SetToolTip(btnResetRange, "Reset the range to the recommended default for this emulator.");
            btnResetRange.UseVisualStyleBackColor = true;
            btnResetRange.Click += btnResetRange_Click;
            // 
            // label6
            // 
            label6.AutoSize = true;
            label6.Location = new Point(167, 57);
            label6.Name = "label6";
            label6.Size = new Size(69, 15);
            label6.TabIndex = 10;
            label6.Text = "Max Results";
            // 
            // numMaxResults
            // 
            numMaxResults.Location = new Point(170, 76);
            numMaxResults.Maximum = new decimal(new int[] { 100000000, 0, 0, 0 });
            numMaxResults.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            numMaxResults.Name = "numMaxResults";
            numMaxResults.Size = new Size(126, 23);
            numMaxResults.TabIndex = 3;
            numMaxResults.ThousandsSeparator = true;
            numMaxResults.Value = new decimal(new int[] { 5000, 0, 0, 0 });
            // 
            // chkScanForStructureBase
            // 
            chkScanForStructureBase.AutoSize = true;
            chkScanForStructureBase.Location = new Point(14, 219);
            chkScanForStructureBase.Name = "chkScanForStructureBase";
            chkScanForStructureBase.Size = new Size(198, 19);
            chkScanForStructureBase.TabIndex = 6;
            chkScanForStructureBase.Text = "Scan with Negative Offsets (Hex)";
            toolTip1.SetToolTip(chkScanForStructureBase, "Performs an extra search using negative offsets up to the specified maximum value (in hex). Increases scan time.");
            chkScanForStructureBase.UseVisualStyleBackColor = true;
            chkScanForStructureBase.CheckedChanged += chkScanForStructureBase_CheckedChanged;
            // 
            // chkUse16ByteAlignment
            // 
            chkUse16ByteAlignment.AutoSize = true;
            chkUse16ByteAlignment.Location = new Point(14, 244);
            chkUse16ByteAlignment.Name = "chkUse16ByteAlignment";
            chkUse16ByteAlignment.Size = new Size(193, 19);
            chkUse16ByteAlignment.TabIndex = 7;
            chkUse16ByteAlignment.Text = "Search using 16-byte alignment";
            toolTip1.SetToolTip(chkUse16ByteAlignment, "When enabled, the scanner descends through memory in 16-byte aligned steps.\r\nThis improves performance and avoids false positives.\r\nUncheck for a more thorough 4-byte scan.");
            chkUse16ByteAlignment.UseVisualStyleBackColor = true;
            // 
            // txtMaxNegativeOffset
            // 
            txtMaxNegativeOffset.Location = new Point(218, 217);
            txtMaxNegativeOffset.Name = "txtMaxNegativeOffset";
            txtMaxNegativeOffset.Size = new Size(75, 23);
            txtMaxNegativeOffset.TabIndex = 11;
            // 
            // chkUseSliderRange
            // 
            chkUseSliderRange.AutoSize = true;
            chkUseSliderRange.Location = new Point(14, 186);
            chkUseSliderRange.Name = "chkUseSliderRange";
            chkUseSliderRange.Size = new Size(160, 19);
            chkUseSliderRange.TabIndex = 12;
            chkUseSliderRange.Text = "Use Visual Range Selector";
            chkUseSliderRange.UseVisualStyleBackColor = true;
            chkUseSliderRange.CheckedChanged += chkUseSliderRange_CheckedChanged;
            // 
            // panelSlider
            // 
            panelSlider.Controls.Add(lblSliderRange);
            panelSlider.Controls.Add(rangeSlider);
            panelSlider.Location = new Point(14, 115);
            panelSlider.Name = "panelSlider";
            panelSlider.Size = new Size(282, 59);
            panelSlider.TabIndex = 13;
            panelSlider.Visible = false;
            // 
            // lblSliderRange
            // 
            lblSliderRange.Location = new Point(3, 33);
            lblSliderRange.Name = "lblSliderRange";
            lblSliderRange.Size = new Size(276, 15);
            lblSliderRange.TabIndex = 1;
            lblSliderRange.Text = "000000 - FFFFFF";
            lblSliderRange.TextAlign = ContentAlignment.MiddleCenter;
            // 
            // rangeSlider
            // 
            rangeSlider.Location = new Point(3, 4);
            rangeSlider.Maximum = 1024;
            rangeSlider.MaxRange = 256;
            rangeSlider.Minimum = 0;
            rangeSlider.Name = "rangeSlider";
            rangeSlider.Size = new Size(276, 25);
            rangeSlider.TabIndex = 0;
            rangeSlider.Text = "rangeSlider1";
            rangeSlider.ThumbStep = 1;
            rangeSlider.TrackStep = 256;
            rangeSlider.Value1 = 256;
            rangeSlider.Value2 = 512;
            rangeSlider.ValueChanged += rangeSlider_ValueChanged;
            // 
            // ScannerOptionsForm
            // 
            AcceptButton = btnOK;
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            CancelButton = btnCancel;
            ClientSize = new Size(307, 316);
            Controls.Add(btnResetRange);
            Controls.Add(chkUseSliderRange);
            Controls.Add(txtMaxNegativeOffset);
            Controls.Add(chkUse16ByteAlignment);
            Controls.Add(chkScanForStructureBase);
            Controls.Add(label6);
            Controls.Add(numMaxResults);
            Controls.Add(btnCancel);
            Controls.Add(btnOK);
            Controls.Add(label3);
            Controls.Add(txtMaxOffset);
            Controls.Add(label2);
            Controls.Add(numMaxLevel);
            Controls.Add(label1);
            Controls.Add(txtTargetAddress);
            Controls.Add(panelSlider);
            Controls.Add(groupBoxRange);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "ScannerOptionsForm";
            ShowInTaskbar = false;
            StartPosition = FormStartPosition.CenterParent;
            Text = "Pointer Scan Options";
            Load += ScannerOptionsForm_Load;
            ((System.ComponentModel.ISupportInitialize)numMaxLevel).EndInit();
            groupBoxRange.ResumeLayout(false);
            groupBoxRange.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)numMaxResults).EndInit();
            panelSlider.ResumeLayout(false);
            ResumeLayout(false);
            PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox txtTargetAddress;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.NumericUpDown numMaxLevel;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox txtMaxOffset;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.GroupBox groupBoxRange;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txtStaticEnd;
        private System.Windows.Forms.TextBox txtStaticStart;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.NumericUpDown numMaxResults;
        private System.Windows.Forms.CheckBox chkScanForStructureBase;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.CheckBox chkUse16ByteAlignment;
        private System.Windows.Forms.TextBox txtMaxNegativeOffset;
        private System.Windows.Forms.CheckBox chkUseSliderRange;
        private System.Windows.Forms.Panel panelSlider;
        private UI.Controls.RangeSlider rangeSlider;
        private System.Windows.Forms.Label lblSliderRange;
        private System.Windows.Forms.Button btnResetRange;
    }
}