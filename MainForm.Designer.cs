namespace PointerFinder2
{
    partial class MainForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            toolStripSeparator2 = new ToolStripSeparator();
            toolStripSeparator1 = new ToolStripSeparator();
            toolStripSeparator3 = new ToolStripSeparator();
            toolStripSeparator4 = new ToolStripSeparator();
            helpToolStripMenuItem = new ToolStripMenuItem();
            videoTutorialToolStripMenuItem = new ToolStripMenuItem();
            menuStrip1 = new MenuStrip();
            fileToolStripMenuItem = new ToolStripMenuItem();
            menuAttach = new ToolStripMenuItem();
            saveSessionToolStripMenuItem = new ToolStripMenuItem();
            loadSessionToolStripMenuItem = new ToolStripMenuItem();
            menuExit = new ToolStripMenuItem();
            editToolStripMenuItem = new ToolStripMenuItem();
            undoToolStripMenuItem = new ToolStripMenuItem();
            redoToolStripMenuItem = new ToolStripMenuItem();
            findToolStripMenuItem = new ToolStripMenuItem();
            viewToolStripMenuItem = new ToolStripMenuItem();
            debugConsoleToolStripMenuItem = new ToolStripMenuItem();
            debugOptionsToolStripMenuItem = new ToolStripMenuItem();
            toolsToolStripMenuItem = new ToolStripMenuItem();
            staticRangeFinderToolStripMenuItem = new ToolStripMenuItem();
            codeNoteConverterToolStripMenuItem = new ToolStripMenuItem();
            codeNoteHierarchyFixerToolStripMenuItem = new ToolStripMenuItem();
            panelBottom = new Panel();
            lblElapsedTime = new Label();
            lblProgressPercentage = new Label();
            btnStopScan = new Button();
            progressBar = new ProgressBar();
            tableLayoutPanel1 = new TableLayoutPanel();
            btnCaptureState = new Button();
            btnFilter = new Button();
            dgvResults = new DataGridView();
            contextMenuResults = new ContextMenuStrip(components);
            copyBaseAddressToolStripMenuItem = new ToolStripMenuItem();
            copyAsRetroAchievementsFormatToolStripMenuItem = new ToolStripMenuItem();
            copyAsCodeNoteToolStripMenuItem = new ToolStripMenuItem();
            deleteSelectedToolStripMenuItem = new ToolStripMenuItem();
            sortByLowestOffsetsToolStripMenuItem = new ToolStripMenuItem();
            statusTableLayoutPanel = new TableLayoutPanel();
            lblStatus = new Label();
            lblBaseAddress = new Label();
            lblResultCount = new Label();
            menuStrip1.SuspendLayout();
            panelBottom.SuspendLayout();
            tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)dgvResults).BeginInit();
            contextMenuResults.SuspendLayout();
            statusTableLayoutPanel.SuspendLayout();
            SuspendLayout();
            // 
            // toolStripSeparator2
            // 
            toolStripSeparator2.Name = "toolStripSeparator2";
            toolStripSeparator2.Size = new Size(191, 6);
            // 
            // toolStripSeparator1
            // 
            toolStripSeparator1.Name = "toolStripSeparator1";
            toolStripSeparator1.Size = new Size(171, 6);
            // 
            // toolStripSeparator3
            // 
            toolStripSeparator3.Name = "toolStripSeparator3";
            toolStripSeparator3.Size = new Size(334, 6);
            // 
            // toolStripSeparator4
            // 
            toolStripSeparator4.Name = "toolStripSeparator4";
            toolStripSeparator4.Size = new Size(334, 6);
            // 
            // helpToolStripMenuItem
            // 
            helpToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { videoTutorialToolStripMenuItem });
            helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            helpToolStripMenuItem.Size = new Size(44, 20);
            helpToolStripMenuItem.Text = "&Help";
            // 
            // videoTutorialToolStripMenuItem
            // 
            videoTutorialToolStripMenuItem.Name = "videoTutorialToolStripMenuItem";
            videoTutorialToolStripMenuItem.Size = new Size(156, 22);
            videoTutorialToolStripMenuItem.Text = "Video Tutorial...";
            videoTutorialToolStripMenuItem.Click += videoTutorialToolStripMenuItem_Click;
            // 
            // menuStrip1
            // 
            menuStrip1.ImageScalingSize = new Size(20, 20);
            menuStrip1.Items.AddRange(new ToolStripItem[] { fileToolStripMenuItem, editToolStripMenuItem, viewToolStripMenuItem, toolsToolStripMenuItem, helpToolStripMenuItem });
            menuStrip1.Location = new Point(0, 0);
            menuStrip1.Name = "menuStrip1";
            menuStrip1.Padding = new Padding(4, 2, 0, 2);
            menuStrip1.Size = new Size(966, 24);
            menuStrip1.TabIndex = 0;
            menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            fileToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { menuAttach, saveSessionToolStripMenuItem, loadSessionToolStripMenuItem, toolStripSeparator2, menuExit });
            fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            fileToolStripMenuItem.Size = new Size(37, 20);
            fileToolStripMenuItem.Text = "&File";
            fileToolStripMenuItem.DropDownOpening += fileToolStripMenuItem_DropDownOpening;
            // 
            // menuAttach
            // 
            menuAttach.Name = "menuAttach";
            menuAttach.Size = new Size(194, 22);
            menuAttach.Text = "Attach to Emulator...";
            menuAttach.Click += menuAttach_Click;
            // 
            // saveSessionToolStripMenuItem
            // 
            saveSessionToolStripMenuItem.Name = "saveSessionToolStripMenuItem";
            saveSessionToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.S;
            saveSessionToolStripMenuItem.Size = new Size(194, 22);
            saveSessionToolStripMenuItem.Text = "Save Session...";
            saveSessionToolStripMenuItem.Click += saveSessionToolStripMenuItem_Click;
            // 
            // loadSessionToolStripMenuItem
            // 
            loadSessionToolStripMenuItem.Name = "loadSessionToolStripMenuItem";
            loadSessionToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.O;
            loadSessionToolStripMenuItem.Size = new Size(194, 22);
            loadSessionToolStripMenuItem.Text = "Load Session...";
            loadSessionToolStripMenuItem.Click += loadSessionToolStripMenuItem_Click;
            // 
            // menuExit
            // 
            menuExit.Name = "menuExit";
            menuExit.Size = new Size(194, 22);
            menuExit.Text = "E&xit";
            menuExit.Click += menuExit_Click;
            // 
            // editToolStripMenuItem
            // 
            editToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { undoToolStripMenuItem, redoToolStripMenuItem, toolStripSeparator1, findToolStripMenuItem });
            editToolStripMenuItem.Name = "editToolStripMenuItem";
            editToolStripMenuItem.Size = new Size(39, 20);
            editToolStripMenuItem.Text = "&Edit";
            editToolStripMenuItem.DropDownOpening += editToolStripMenuItem_DropDownOpening;
            // 
            // undoToolStripMenuItem
            // 
            undoToolStripMenuItem.Name = "undoToolStripMenuItem";
            undoToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.Z;
            undoToolStripMenuItem.Size = new Size(174, 22);
            undoToolStripMenuItem.Text = "Undo";
            undoToolStripMenuItem.Click += undoToolStripMenuItem_Click;
            // 
            // redoToolStripMenuItem
            // 
            redoToolStripMenuItem.Name = "redoToolStripMenuItem";
            redoToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.Shift | Keys.Z;
            redoToolStripMenuItem.Size = new Size(174, 22);
            redoToolStripMenuItem.Text = "Redo";
            redoToolStripMenuItem.Click += redoToolStripMenuItem_Click;
            // 
            // findToolStripMenuItem
            // 
            findToolStripMenuItem.Name = "findToolStripMenuItem";
            findToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.F;
            findToolStripMenuItem.Size = new Size(174, 22);
            findToolStripMenuItem.Text = "Find...";
            findToolStripMenuItem.Click += findToolStripMenuItem_Click;
            // 
            // viewToolStripMenuItem
            // 
            viewToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { debugConsoleToolStripMenuItem, debugOptionsToolStripMenuItem });
            viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            viewToolStripMenuItem.Size = new Size(44, 20);
            viewToolStripMenuItem.Text = "View";
            // 
            // debugConsoleToolStripMenuItem
            // 
            debugConsoleToolStripMenuItem.Name = "debugConsoleToolStripMenuItem";
            debugConsoleToolStripMenuItem.Size = new Size(155, 22);
            debugConsoleToolStripMenuItem.Text = "Debug Console";
            debugConsoleToolStripMenuItem.Click += debugConsoleToolStripMenuItem_Click;
            // 
            // debugOptionsToolStripMenuItem
            // 
            debugOptionsToolStripMenuItem.Name = "debugOptionsToolStripMenuItem";
            debugOptionsToolStripMenuItem.Size = new Size(155, 22);
            debugOptionsToolStripMenuItem.Text = "Settings...";
            debugOptionsToolStripMenuItem.Click += debugOptionsToolStripMenuItem_Click;
            // 
            // toolsToolStripMenuItem
            // 
            toolsToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { staticRangeFinderToolStripMenuItem, codeNoteConverterToolStripMenuItem, codeNoteHierarchyFixerToolStripMenuItem });
            toolsToolStripMenuItem.Name = "toolsToolStripMenuItem";
            toolsToolStripMenuItem.Size = new Size(46, 20);
            toolsToolStripMenuItem.Text = "Tools";
            toolsToolStripMenuItem.DropDownOpening += toolsToolStripMenuItem_DropDownOpening;
            // 
            // staticRangeFinderToolStripMenuItem
            // 
            staticRangeFinderToolStripMenuItem.Name = "staticRangeFinderToolStripMenuItem";
            staticRangeFinderToolStripMenuItem.Size = new Size(222, 22);
            staticRangeFinderToolStripMenuItem.Text = "Static Range Finder...";
            staticRangeFinderToolStripMenuItem.Click += staticRangeFinderToolStripMenuItem_Click;
            // 
            // codeNoteConverterToolStripMenuItem
            // 
            codeNoteConverterToolStripMenuItem.Name = "codeNoteConverterToolStripMenuItem";
            codeNoteConverterToolStripMenuItem.Size = new Size(222, 22);
            codeNoteConverterToolStripMenuItem.Text = "Code Note Converter...";
            codeNoteConverterToolStripMenuItem.Click += codeNoteConverterToolStripMenuItem_Click;
            // 
            // codeNoteHierarchyFixerToolStripMenuItem
            // 
            codeNoteHierarchyFixerToolStripMenuItem.Name = "codeNoteHierarchyFixerToolStripMenuItem";
            codeNoteHierarchyFixerToolStripMenuItem.Size = new Size(222, 22);
            codeNoteHierarchyFixerToolStripMenuItem.Text = "Code Note Hierarchy Fixer...";
            codeNoteHierarchyFixerToolStripMenuItem.Click += codeNoteHierarchyFixerToolStripMenuItem_Click;
            // 
            // panelBottom
            // 
            panelBottom.Controls.Add(lblElapsedTime);
            panelBottom.Controls.Add(lblProgressPercentage);
            panelBottom.Controls.Add(btnStopScan);
            panelBottom.Controls.Add(progressBar);
            panelBottom.Controls.Add(tableLayoutPanel1);
            panelBottom.Dock = DockStyle.Bottom;
            panelBottom.Location = new Point(0, 545);
            panelBottom.Name = "panelBottom";
            panelBottom.Size = new Size(966, 47);
            panelBottom.TabIndex = 2;
            // 
            // lblElapsedTime
            // 
            lblElapsedTime.AutoSize = true;
            lblElapsedTime.Location = new Point(9, 27);
            lblElapsedTime.Name = "lblElapsedTime";
            lblElapsedTime.Size = new Size(75, 15);
            lblElapsedTime.TabIndex = 5;
            lblElapsedTime.Text = "Time: 00:00.0";
            lblElapsedTime.TextAlign = ContentAlignment.MiddleLeft;
            lblElapsedTime.Visible = false;
            // 
            // lblProgressPercentage
            // 
            lblProgressPercentage.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            lblProgressPercentage.Location = new Point(748, 27);
            lblProgressPercentage.Name = "lblProgressPercentage";
            lblProgressPercentage.Size = new Size(140, 18);
            lblProgressPercentage.TabIndex = 4;
            lblProgressPercentage.Text = "0 / 0";
            lblProgressPercentage.TextAlign = ContentAlignment.MiddleRight;
            lblProgressPercentage.Visible = false;
            // 
            // btnStopScan
            // 
            btnStopScan.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnStopScan.Location = new Point(891, 3);
            btnStopScan.Name = "btnStopScan";
            btnStopScan.Size = new Size(73, 41);
            btnStopScan.TabIndex = 2;
            btnStopScan.Text = "Stop";
            btnStopScan.UseVisualStyleBackColor = true;
            btnStopScan.Visible = false;
            btnStopScan.Click += btnStopScan_Click;
            // 
            // progressBar
            // 
            progressBar.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            progressBar.Location = new Point(3, 3);
            progressBar.Name = "progressBar";
            progressBar.Size = new Size(885, 21);
            progressBar.TabIndex = 1;
            progressBar.Visible = false;
            // 
            // tableLayoutPanel1
            // 
            tableLayoutPanel1.ColumnCount = 2;
            tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            tableLayoutPanel1.Controls.Add(btnCaptureState, 0, 0);
            tableLayoutPanel1.Controls.Add(btnFilter, 1, 0);
            tableLayoutPanel1.Dock = DockStyle.Fill;
            tableLayoutPanel1.Location = new Point(0, 0);
            tableLayoutPanel1.Name = "tableLayoutPanel1";
            tableLayoutPanel1.RowCount = 1;
            tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            tableLayoutPanel1.Size = new Size(966, 47);
            tableLayoutPanel1.TabIndex = 0;
            // 
            // btnCaptureState
            // 
            btnCaptureState.Dock = DockStyle.Fill;
            btnCaptureState.Enabled = false;
            btnCaptureState.Location = new Point(3, 3);
            btnCaptureState.Name = "btnCaptureState";
            btnCaptureState.Size = new Size(477, 41);
            btnCaptureState.TabIndex = 4;
            btnCaptureState.Text = "State-Based Scan";
            btnCaptureState.UseVisualStyleBackColor = true;
            btnCaptureState.Click += btnCaptureState_Click;
            // 
            // btnFilter
            // 
            btnFilter.Dock = DockStyle.Fill;
            btnFilter.Enabled = false;
            btnFilter.Location = new Point(486, 3);
            btnFilter.Name = "btnFilter";
            btnFilter.Size = new Size(477, 41);
            btnFilter.TabIndex = 2;
            btnFilter.Text = "Filter Dynamic Paths";
            btnFilter.UseVisualStyleBackColor = true;
            btnFilter.Click += btnFilter_Click;
            // 
            // dgvResults
            // 
            dgvResults.AllowUserToAddRows = false;
            dgvResults.AllowUserToDeleteRows = false;
            dgvResults.AllowUserToResizeRows = false;
            dgvResults.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            dgvResults.ContextMenuStrip = contextMenuResults;
            dgvResults.Dock = DockStyle.Fill;
            dgvResults.Location = new Point(0, 24);
            dgvResults.Name = "dgvResults";
            dgvResults.ReadOnly = true;
            dgvResults.RowHeadersVisible = false;
            dgvResults.RowTemplate.Height = 24;
            dgvResults.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
            dgvResults.Size = new Size(966, 499);
            dgvResults.TabIndex = 0;
            dgvResults.KeyDown += dgvResults_KeyDown;
            // 
            // contextMenuResults
            // 
            contextMenuResults.ImageScalingSize = new Size(20, 20);
            contextMenuResults.Items.AddRange(new ToolStripItem[] { copyBaseAddressToolStripMenuItem, copyAsRetroAchievementsFormatToolStripMenuItem, copyAsCodeNoteToolStripMenuItem, toolStripSeparator4, deleteSelectedToolStripMenuItem, toolStripSeparator3, sortByLowestOffsetsToolStripMenuItem });
            contextMenuResults.Name = "contextMenuResults";
            contextMenuResults.Size = new Size(338, 126);
            contextMenuResults.Opening += contextMenuResults_Opening;
            // 
            // copyBaseAddressToolStripMenuItem
            // 
            copyBaseAddressToolStripMenuItem.Name = "copyBaseAddressToolStripMenuItem";
            copyBaseAddressToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.C;
            copyBaseAddressToolStripMenuItem.Size = new Size(337, 22);
            copyBaseAddressToolStripMenuItem.Text = "Copy Base Address";
            copyBaseAddressToolStripMenuItem.Click += copyBaseAddressToolStripMenuItem_Click;
            // 
            // copyAsRetroAchievementsFormatToolStripMenuItem
            // 
            copyAsRetroAchievementsFormatToolStripMenuItem.Name = "copyAsRetroAchievementsFormatToolStripMenuItem";
            copyAsRetroAchievementsFormatToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.Shift | Keys.C;
            copyAsRetroAchievementsFormatToolStripMenuItem.Size = new Size(337, 22);
            copyAsRetroAchievementsFormatToolStripMenuItem.Text = "Copy as RetroAchievements Format";
            copyAsRetroAchievementsFormatToolStripMenuItem.Click += copyAsRetroAchievementsFormatToolStripMenuItem_Click;
            // 
            // copyAsCodeNoteToolStripMenuItem
            // 
            copyAsCodeNoteToolStripMenuItem.Name = "copyAsCodeNoteToolStripMenuItem";
            copyAsCodeNoteToolStripMenuItem.Size = new Size(337, 22);
            copyAsCodeNoteToolStripMenuItem.Text = "Copy as Code Note...";
            copyAsCodeNoteToolStripMenuItem.Click += copyAsCodeNoteToolStripMenuItem_Click;
            // 
            // deleteSelectedToolStripMenuItem
            // 
            deleteSelectedToolStripMenuItem.Name = "deleteSelectedToolStripMenuItem";
            deleteSelectedToolStripMenuItem.ShortcutKeys = Keys.Delete;
            deleteSelectedToolStripMenuItem.Size = new Size(337, 22);
            deleteSelectedToolStripMenuItem.Text = "Delete Selected";
            deleteSelectedToolStripMenuItem.Click += deleteSelectedToolStripMenuItem_Click;
            // 
            // sortByLowestOffsetsToolStripMenuItem
            // 
            sortByLowestOffsetsToolStripMenuItem.Name = "sortByLowestOffsetsToolStripMenuItem";
            sortByLowestOffsetsToolStripMenuItem.Size = new Size(337, 22);
            sortByLowestOffsetsToolStripMenuItem.Text = "Sort by Lowest Offsets";
            sortByLowestOffsetsToolStripMenuItem.Click += sortByLowestOffsetsToolStripMenuItem_Click;
            // 
            // statusTableLayoutPanel
            // 
            statusTableLayoutPanel.ColumnCount = 3;
            statusTableLayoutPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            statusTableLayoutPanel.ColumnStyles.Add(new ColumnStyle());
            statusTableLayoutPanel.ColumnStyles.Add(new ColumnStyle());
            statusTableLayoutPanel.Controls.Add(lblStatus, 0, 0);
            statusTableLayoutPanel.Controls.Add(lblBaseAddress, 1, 0);
            statusTableLayoutPanel.Controls.Add(lblResultCount, 2, 0);
            statusTableLayoutPanel.Dock = DockStyle.Bottom;
            statusTableLayoutPanel.Location = new Point(0, 523);
            statusTableLayoutPanel.Name = "statusTableLayoutPanel";
            statusTableLayoutPanel.RowCount = 1;
            statusTableLayoutPanel.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            statusTableLayoutPanel.Size = new Size(966, 22);
            statusTableLayoutPanel.TabIndex = 5;
            // 
            // lblStatus
            // 
            lblStatus.Dock = DockStyle.Fill;
            lblStatus.Location = new Point(3, 0);
            lblStatus.Name = "lblStatus";
            lblStatus.Padding = new Padding(2, 0, 0, 0);
            lblStatus.Size = new Size(892, 22);
            lblStatus.TabIndex = 0;
            lblStatus.Text = "Status: Not Attached";
            lblStatus.TextAlign = ContentAlignment.MiddleLeft;
            // 
            // lblBaseAddress
            // 
            lblBaseAddress.AutoSize = true;
            lblBaseAddress.Dock = DockStyle.Fill;
            lblBaseAddress.Location = new Point(901, 0);
            lblBaseAddress.Name = "lblBaseAddress";
            lblBaseAddress.Size = new Size(1, 22);
            lblBaseAddress.TabIndex = 1;
            lblBaseAddress.TextAlign = ContentAlignment.MiddleCenter;
            // 
            // lblResultCount
            // 
            lblResultCount.AutoSize = true;
            lblResultCount.Dock = DockStyle.Fill;
            lblResultCount.Location = new Point(907, 0);
            lblResultCount.Name = "lblResultCount";
            lblResultCount.Size = new Size(56, 22);
            lblResultCount.TabIndex = 2;
            lblResultCount.Text = "Results: 0";
            lblResultCount.TextAlign = ContentAlignment.MiddleRight;
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(966, 592);
            Controls.Add(dgvResults);
            Controls.Add(statusTableLayoutPanel);
            Controls.Add(panelBottom);
            Controls.Add(menuStrip1);
            Icon = (Icon)resources.GetObject("$this.Icon");
            MainMenuStrip = menuStrip1;
            MinimumSize = new Size(640, 480);
            Name = "MainForm";
            Text = "Pointer Finder 2.0";
            menuStrip1.ResumeLayout(false);
            menuStrip1.PerformLayout();
            panelBottom.ResumeLayout(false);
            panelBottom.PerformLayout();
            tableLayoutPanel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)dgvResults).EndInit();
            contextMenuResults.ResumeLayout(false);
            statusTableLayoutPanel.ResumeLayout(false);
            statusTableLayoutPanel.PerformLayout();
            ResumeLayout(false);
            PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem menuAttach;
        private System.Windows.Forms.ToolStripMenuItem menuExit;
        private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem debugConsoleToolStripMenuItem;
        private System.Windows.Forms.Panel panelBottom;
        private System.Windows.Forms.DataGridView dgvResults;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.Button btnFilter;
        private System.Windows.Forms.ToolStripMenuItem debugOptionsToolStripMenuItem;
        private System.Windows.Forms.ContextMenuStrip contextMenuResults;
        private System.Windows.Forms.ToolStripMenuItem copyBaseAddressToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem copyAsRetroAchievementsFormatToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem deleteSelectedToolStripMenuItem;
        private System.Windows.Forms.Button btnStopScan;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.Label lblProgressPercentage;
        private System.Windows.Forms.TableLayoutPanel statusTableLayoutPanel;
        private System.Windows.Forms.Label lblStatus;
        private System.Windows.Forms.Label lblBaseAddress;
        private System.Windows.Forms.Label lblResultCount;
        private System.Windows.Forms.Label lblElapsedTime;
        private System.Windows.Forms.ToolStripMenuItem saveSessionToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem loadSessionToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem editToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem undoToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem redoToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem findToolStripMenuItem;
        private System.Windows.Forms.Button btnCaptureState;
        private System.Windows.Forms.ToolStripMenuItem toolsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem staticRangeFinderToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem videoTutorialToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem sortByLowestOffsetsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem copyAsCodeNoteToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem codeNoteConverterToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem codeNoteHierarchyFixerToolStripMenuItem;
        private ToolStripSeparator toolStripSeparator2;
        private ToolStripSeparator toolStripSeparator1;
        private ToolStripSeparator toolStripSeparator3;
        private ToolStripSeparator toolStripSeparator4;
    }
}