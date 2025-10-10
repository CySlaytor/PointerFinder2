namespace PointerFinder2
{
    partial class DebugLogForm
    {
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.TextBox txtLog;
        private System.Windows.Forms.Button btnClear;
        private System.Windows.Forms.Button btnToggleLogging;

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Ensure the timer is stopped and disposed of properly when the form is eventually closed.
                if (_logTimer != null)
                {
                    _logTimer.Stop();
                    _logTimer.Dispose();
                }
                if (components != null)
                {
                    components.Dispose();
                }
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code
        private void InitializeComponent()
        {
            this.txtLog = new System.Windows.Forms.TextBox();
            this.btnClear = new System.Windows.Forms.Button();
            this.btnToggleLogging = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // txtLog
            // 
            this.txtLog.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtLog.BackColor = System.Drawing.SystemColors.Window;
            this.txtLog.Font = new System.Drawing.Font("Consolas", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.txtLog.HideSelection = false;
            this.txtLog.Location = new System.Drawing.Point(0, 0);
            this.txtLog.Margin = new System.Windows.Forms.Padding(4);
            this.txtLog.Multiline = true;
            this.txtLog.Name = "txtLog";
            this.txtLog.ReadOnly = true;
            this.txtLog.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtLog.Size = new System.Drawing.Size(1067, 510);
            this.txtLog.TabIndex = 0;
            // 
            // btnClear
            // 
            this.btnClear.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClear.Location = new System.Drawing.Point(958, 517);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(97, 28);
            this.btnClear.TabIndex = 1;
            this.btnClear.Text = "Clear Log";
            this.btnClear.UseVisualStyleBackColor = true;
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
            // 
            // btnToggleLogging
            // 
            this.btnToggleLogging.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnToggleLogging.Location = new System.Drawing.Point(829, 517);
            this.btnToggleLogging.Name = "btnToggleLogging";
            this.btnToggleLogging.Size = new System.Drawing.Size(123, 28);
            this.btnToggleLogging.TabIndex = 2;
            this.btnToggleLogging.Text = "Pause Logging";
            this.btnToggleLogging.UseVisualStyleBackColor = true;
            this.btnToggleLogging.Click += new System.EventHandler(this.btnToggleLogging_Click);
            // 
            // DebugLogForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1067, 554);
            this.Controls.Add(this.btnToggleLogging);
            this.Controls.Add(this.btnClear);
            this.Controls.Add(this.txtLog);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.MinimumSize = new System.Drawing.Size(300, 150);
            this.Name = "DebugLogForm";
            this.Text = "Debug Console";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.DebugLogForm_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }
        #endregion
    }
}