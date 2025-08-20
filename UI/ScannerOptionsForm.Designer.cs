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
            this.components = new System.ComponentModel.Container();
            this.txtTargetAddress = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.numMaxLevel = new System.Windows.Forms.NumericUpDown();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.txtMaxOffset = new System.Windows.Forms.TextBox();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.groupBoxRange = new System.Windows.Forms.GroupBox();
            this.label5 = new System.Windows.Forms.Label();
            this.txtStaticEnd = new System.Windows.Forms.TextBox();
            this.txtStaticStart = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.numMaxResults = new System.Windows.Forms.NumericUpDown();
            this.chkScanForStructureBase = new System.Windows.Forms.CheckBox();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.chkUse16ByteAlignment = new System.Windows.Forms.CheckBox();
            this.txtMaxNegativeOffset = new System.Windows.Forms.TextBox();
            this.chkUseSliderRange = new System.Windows.Forms.CheckBox();
            this.panelSlider = new System.Windows.Forms.Panel();
            this.lblSliderRange = new System.Windows.Forms.Label();
            this.rangeSlider = new PointerFinder2.UI.Controls.RangeSlider();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxLevel)).BeginInit();
            this.groupBoxRange.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxResults)).BeginInit();
            this.panelSlider.SuspendLayout();
            this.SuspendLayout();
            // 
            // txtTargetAddress
            // 
            this.txtTargetAddress.Location = new System.Drawing.Point(14, 28);
            this.txtTargetAddress.Name = "txtTargetAddress";
            this.txtTargetAddress.Size = new System.Drawing.Size(126, 22);
            this.txtTargetAddress.TabIndex = 0;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(11, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(130, 15);
            this.label1.TabIndex = 1;
            this.label1.Text = "Target Address (PS2, Hex)";
            // 
            // numMaxLevel
            // 
            this.numMaxLevel.Location = new System.Drawing.Point(14, 76);
            this.numMaxLevel.Maximum = new decimal(new int[] {
            100,
            0,
            0,
            0});
            this.numMaxLevel.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numMaxLevel.Name = "numMaxLevel";
            this.numMaxLevel.Size = new System.Drawing.Size(126, 22);
            this.numMaxLevel.TabIndex = 2;
            this.numMaxLevel.Value = new decimal(new int[] {
            7,
            0,
            0,
            0});
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(11, 57);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(61, 15);
            this.label2.TabIndex = 3;
            this.label2.Text = "Max Level";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(167, 9);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(94, 15);
            this.label3.TabIndex = 5;
            this.label3.Text = "Max Offset (Hex)";
            // 
            // txtMaxOffset
            // 
            this.txtMaxOffset.Location = new System.Drawing.Point(170, 28);
            this.txtMaxOffset.Name = "txtMaxOffset";
            this.txtMaxOffset.Size = new System.Drawing.Size(126, 22);
            this.txtMaxOffset.TabIndex = 1;
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.Location = new System.Drawing.Point(125, 275);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(83, 29);
            this.btnOK.TabIndex = 10;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(213, 275);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(83, 29);
            this.btnCancel.TabIndex = 11;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // groupBoxRange
            // 
            this.groupBoxRange.Controls.Add(this.label5);
            this.groupBoxRange.Controls.Add(this.txtStaticEnd);
            this.groupBoxRange.Controls.Add(this.txtStaticStart);
            this.groupBoxRange.Controls.Add(this.label4);
            this.groupBoxRange.Location = new System.Drawing.Point(14, 115);
            this.groupBoxRange.Name = "groupBoxRange";
            this.groupBoxRange.Size = new System.Drawing.Size(282, 59);
            this.groupBoxRange.TabIndex = 4;
            this.groupBoxRange.TabStop = false;
            this.groupBoxRange.Text = "Static Base Address Range (PS2, Hex)";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(135, 29);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(11, 15);
            this.label5.TabIndex = 12;
            this.label5.Text = "-";
            // 
            // txtStaticEnd
            // 
            this.txtStaticEnd.Location = new System.Drawing.Point(152, 26);
            this.txtStaticEnd.Name = "txtStaticEnd";
            this.txtStaticEnd.Size = new System.Drawing.Size(124, 22);
            this.txtStaticEnd.TabIndex = 1;
            // 
            // txtStaticStart
            // 
            this.txtStaticStart.Location = new System.Drawing.Point(6, 26);
            this.txtStaticStart.Name = "txtStaticStart";
            this.txtStaticStart.Size = new System.Drawing.Size(124, 22);
            this.txtStaticStart.TabIndex = 0;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(11, 57);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(61, 15);
            this.label4.TabIndex = 3;
            this.label4.Text = "Max Level";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(167, 57);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(73, 15);
            this.label6.TabIndex = 10;
            this.label6.Text = "Max Results";
            // 
            // numMaxResults
            // 
            this.numMaxResults.Location = new System.Drawing.Point(170, 76);
            this.numMaxResults.Maximum = new decimal(new int[] {
            100000000,
            0,
            0,
            0});
            this.numMaxResults.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numMaxResults.Name = "numMaxResults";
            this.numMaxResults.Size = new System.Drawing.Size(126, 22);
            this.numMaxResults.TabIndex = 3;
            this.numMaxResults.ThousandsSeparator = true;
            this.numMaxResults.Value = new decimal(new int[] {
            5000,
            0,
            0,
            0});
            // 
            // chkScanForStructureBase
            // 
            this.chkScanForStructureBase.AutoSize = true;
            this.chkScanForStructureBase.Location = new System.Drawing.Point(14, 219);
            this.chkScanForStructureBase.Name = "chkScanForStructureBase";
            this.chkScanForStructureBase.Size = new System.Drawing.Size(176, 19);
            this.chkScanForStructureBase.TabIndex = 6;
            this.chkScanForStructureBase.Text = "Scan for Structure Base (Hex)";
            this.toolTip1.SetToolTip(this.chkScanForStructureBase, "Performs an extra scan with negative offsets to find a pointer to the start of a" +
        " data structure, not just a member within it. Increases scan time.");
            this.chkScanForStructureBase.UseVisualStyleBackColor = true;
            this.chkScanForStructureBase.CheckedChanged += new System.EventHandler(this.chkScanForStructureBase_CheckedChanged);
            // 
            // chkUse16ByteAlignment
            // 
            this.chkUse16ByteAlignment.AutoSize = true;
            this.chkUse16ByteAlignment.Location = new System.Drawing.Point(14, 244);
            this.chkUse16ByteAlignment.Name = "chkUse16ByteAlignment";
            this.chkUse16ByteAlignment.Size = new System.Drawing.Size(193, 19);
            this.chkUse16ByteAlignment.TabIndex = 7;
            this.chkUse16ByteAlignment.Text = "Search using 16-byte alignment";
            this.toolTip1.SetToolTip(this.chkUse16ByteAlignment, "When enabled, the scanner descends through memory in 16-byte aligned steps.\r\nThi" +
        "s improves performance and avoids false positives.\r\nUncheck for a more thorough" +
        " 4-byte scan.");
            this.chkUse16ByteAlignment.UseVisualStyleBackColor = true;
            // 
            // txtMaxNegativeOffset
            // 
            this.txtMaxNegativeOffset.Location = new System.Drawing.Point(196, 217);
            this.txtMaxNegativeOffset.Name = "txtMaxNegativeOffset";
            this.txtMaxNegativeOffset.Size = new System.Drawing.Size(100, 22);
            this.txtMaxNegativeOffset.TabIndex = 11;
            // 
            // chkUseSliderRange
            // 
            this.chkUseSliderRange.AutoSize = true;
            this.chkUseSliderRange.Location = new System.Drawing.Point(14, 186);
            this.chkUseSliderRange.Name = "chkUseSliderRange";
            this.chkUseSliderRange.Size = new System.Drawing.Size(168, 19);
            this.chkUseSliderRange.TabIndex = 12;
            this.chkUseSliderRange.Text = "Use Visual Range Selector";
            this.chkUseSliderRange.UseVisualStyleBackColor = true;
            this.chkUseSliderRange.CheckedChanged += new System.EventHandler(this.chkUseSliderRange_CheckedChanged);
            // 
            // panelSlider
            // 
            this.panelSlider.Controls.Add(this.lblSliderRange);
            this.panelSlider.Controls.Add(this.rangeSlider);
            this.panelSlider.Location = new System.Drawing.Point(14, 115);
            this.panelSlider.Name = "panelSlider";
            this.panelSlider.Size = new System.Drawing.Size(282, 59);
            this.panelSlider.TabIndex = 13;
            this.panelSlider.Visible = false;
            // 
            // lblSliderRange
            // 
            this.lblSliderRange.Location = new System.Drawing.Point(3, 33);
            this.lblSliderRange.Name = "lblSliderRange";
            this.lblSliderRange.Size = new System.Drawing.Size(276, 15);
            this.lblSliderRange.TabIndex = 1;
            this.lblSliderRange.Text = "000000 - FFFFFF";
            this.lblSliderRange.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // rangeSlider
            // 
            this.rangeSlider.Location = new System.Drawing.Point(3, 4);
            this.rangeSlider.Maximum = 1024;
            this.rangeSlider.MaxRange = 256;
            this.rangeSlider.Minimum = 0;
            this.rangeSlider.Name = "rangeSlider";
            this.rangeSlider.Size = new System.Drawing.Size(276, 25);
            this.rangeSlider.TabIndex = 0;
            this.rangeSlider.Text = "rangeSlider1";
            this.rangeSlider.ThumbStep = 1;
            this.rangeSlider.TrackStep = 256;
            this.rangeSlider.Value1 = 256;
            this.rangeSlider.Value2 = 512;
            this.rangeSlider.ValueChanged += new System.EventHandler(this.rangeSlider_ValueChanged);
            // 
            // ScannerOptionsForm
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(310, 316);
            this.Controls.Add(this.chkUseSliderRange);
            this.Controls.Add(this.txtMaxNegativeOffset);
            this.Controls.Add(this.chkUse16ByteAlignment);
            this.Controls.Add(this.chkScanForStructureBase);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.numMaxResults);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.txtMaxOffset);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.numMaxLevel);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.txtTargetAddress);
            this.Controls.Add(this.panelSlider);
            this.Controls.Add(this.groupBoxRange);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ScannerOptionsForm";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Pointer Scan Options";
            this.Load += new System.EventHandler(this.ScannerOptionsForm_Load);
            ((System.ComponentModel.ISupportInitialize)(this.numMaxLevel)).EndInit();
            this.groupBoxRange.ResumeLayout(false);
            this.groupBoxRange.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxResults)).EndInit();
            this.panelSlider.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

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
    }
}