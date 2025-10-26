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
            System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.videoTutorialToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            menuStrip1 = new System.Windows.Forms.MenuStrip();
            fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            menuAttach = new System.Windows.Forms.ToolStripMenuItem();
            saveSessionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            loadSessionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            menuExit = new System.Windows.Forms.ToolStripMenuItem();
            editToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            undoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            redoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            findToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            debugConsoleToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            debugOptionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            toolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            staticRangeFinderToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            panelBottom = new System.Windows.Forms.Panel();
            lblElapsedTime = new System.Windows.Forms.Label();
            lblProgressPercentage = new System.Windows.Forms.Label();
            btnStopScan = new System.Windows.Forms.Button();
            progressBar = new System.Windows.Forms.ProgressBar();
            tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            btnScan = new System.Windows.Forms.Button();
            btnRefineScan = new System.Windows.Forms.Button();
            btnFilter = new System.Windows.Forms.Button();
            btnCaptureState = new System.Windows.Forms.Button();
            tabControlMain = new System.Windows.Forms.TabControl();
            tabResults = new System.Windows.Forms.TabPage();
            dgvResults = new System.Windows.Forms.DataGridView();
            contextMenuResults = new System.Windows.Forms.ContextMenuStrip(components);
            copyBaseAddressToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            copyAsRetroAchievementsFormatToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            deleteSelectedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            sortByLowestOffsetsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            statusTableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            lblStatus = new System.Windows.Forms.Label();
            lblBaseAddress = new System.Windows.Forms.Label();
            lblResultCount = new System.Windows.Forms.Label();
            toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            menuStrip1.SuspendLayout();
            panelBottom.SuspendLayout();
            tableLayoutPanel1.SuspendLayout();
            tabControlMain.SuspendLayout();
            tabResults.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)dgvResults).BeginInit();
            contextMenuResults.SuspendLayout();
            statusTableLayoutPanel.SuspendLayout();
            SuspendLayout();
            // 
            // toolStripSeparator2
            // 
            toolStripSeparator2.Name = "toolStripSeparator2";
            toolStripSeparator2.Size = new System.Drawing.Size(180, 6);
            // 
            // toolStripSeparator1
            // 
            toolStripSeparator1.Name = "toolStripSeparator1";
            toolStripSeparator1.Size = new System.Drawing.Size(169, 6);
            // 
            // toolStripSeparator3
            // 
            toolStripSeparator3.Name = "toolStripSeparator3";
            toolStripSeparator3.Size = new System.Drawing.Size(334, 6);
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] { this.videoTutorialToolStripMenuItem });
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.helpToolStripMenuItem.Text = "&Help";
            // 
            // videoTutorialToolStripMenuItem
            // 
            this.videoTutorialToolStripMenuItem.Name = "videoTutorialToolStripMenuItem";
            this.videoTutorialToolStripMenuItem.Size = new System.Drawing.Size(170, 22);
            this.videoTutorialToolStripMenuItem.Text = "Video Tutorial...";
            this.videoTutorialToolStripMenuItem.Click += new System.EventHandler(this.videoTutorialToolStripMenuItem_Click);
            // 
            // menuStrip1
            // 
            menuStrip1.ImageScalingSize = new System.Drawing.Size(20, 20);
            menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] { fileToolStripMenuItem, editToolStripMenuItem, viewToolStripMenuItem, toolsToolStripMenuItem, this.helpToolStripMenuItem });
            menuStrip1.Location = new System.Drawing.Point(0, 0);
            menuStrip1.Name = "menuStrip1";
            menuStrip1.Padding = new System.Windows.Forms.Padding(4, 2, 0, 2);
            menuStrip1.Size = new System.Drawing.Size(966, 24);
            menuStrip1.TabIndex = 0;
            menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] { menuAttach, saveSessionToolStripMenuItem, loadSessionToolStripMenuItem, toolStripSeparator2, menuExit });
            fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            fileToolStripMenuItem.Text = "&File";
            fileToolStripMenuItem.DropDownOpening += new System.EventHandler(this.fileToolStripMenuItem_DropDownOpening);
            // 
            // menuAttach
            // 
            menuAttach.Name = "menuAttach";
            menuAttach.Size = new System.Drawing.Size(183, 22);
            menuAttach.Text = "Attach to Emulator...";
            menuAttach.Click += new System.EventHandler(this.menuAttach_Click);
            // 
            // saveSessionToolStripMenuItem
            // 
            saveSessionToolStripMenuItem.Name = "saveSessionToolStripMenuItem";
            saveSessionToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            saveSessionToolStripMenuItem.Size = new System.Drawing.Size(183, 22);
            saveSessionToolStripMenuItem.Text = "Save Session...";
            saveSessionToolStripMenuItem.Click += new System.EventHandler(this.saveSessionToolStripMenuItem_Click);
            // 
            // loadSessionToolStripMenuItem
            // 
            loadSessionToolStripMenuItem.Name = "loadSessionToolStripMenuItem";
            loadSessionToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            loadSessionToolStripMenuItem.Size = new System.Drawing.Size(183, 22);
            loadSessionToolStripMenuItem.Text = "Load Session...";
            loadSessionToolStripMenuItem.Click += new System.EventHandler(this.loadSessionToolStripMenuItem_Click);
            // 
            // menuExit
            // 
            menuExit.Name = "menuExit";
            menuExit.Size = new System.Drawing.Size(183, 22);
            menuExit.Text = "E&xit";
            menuExit.Click += new System.EventHandler(this.menuExit_Click);
            // 
            // editToolStripMenuItem
            // 
            editToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] { undoToolStripMenuItem, redoToolStripMenuItem, toolStripSeparator1, findToolStripMenuItem });
            editToolStripMenuItem.Name = "editToolStripMenuItem";
            editToolStripMenuItem.Size = new System.Drawing.Size(39, 20);
            editToolStripMenuItem.Text = "&Edit";
            editToolStripMenuItem.DropDownOpening += new System.EventHandler(this.editToolStripMenuItem_DropDownOpening);
            // 
            // undoToolStripMenuItem
            // 
            undoToolStripMenuItem.Name = "undoToolStripMenuItem";
            undoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z)));
            undoToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            undoToolStripMenuItem.Text = "Undo";
            undoToolStripMenuItem.Click += new System.EventHandler(this.undoToolStripMenuItem_Click);
            // 
            // redoToolStripMenuItem
            // 
            redoToolStripMenuItem.Name = "redoToolStripMenuItem";
            redoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift)
            | System.Windows.Forms.Keys.Z)));
            redoToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            redoToolStripMenuItem.Text = "Redo";
            redoToolStripMenuItem.Click += new System.EventHandler(this.redoToolStripMenuItem_Click);
            // 
            // findToolStripMenuItem
            // 
            findToolStripMenuItem.Name = "findToolStripMenuItem";
            findToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.F)));
            findToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            findToolStripMenuItem.Text = "Find...";
            findToolStripMenuItem.Click += new System.EventHandler(this.findToolStripMenuItem_Click);
            // 
            // viewToolStripMenuItem
            // 
            viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] { debugConsoleToolStripMenuItem, debugOptionsToolStripMenuItem });
            viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            viewToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            viewToolStripMenuItem.Text = "View";
            // 
            // debugConsoleToolStripMenuItem
            // 
            debugConsoleToolStripMenuItem.Name = "debugConsoleToolStripMenuItem";
            debugConsoleToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
            debugConsoleToolStripMenuItem.Text = "Debug Console";
            debugConsoleToolStripMenuItem.Click += new System.EventHandler(this.debugConsoleToolStripMenuItem_Click);
            // 
            // debugOptionsToolStripMenuItem
            // 
            debugOptionsToolStripMenuItem.Name = "debugOptionsToolStripMenuItem";
            debugOptionsToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
            debugOptionsToolStripMenuItem.Text = "Settings...";
            debugOptionsToolStripMenuItem.Click += new System.EventHandler(this.debugOptionsToolStripMenuItem_Click);
            // 
            // toolsToolStripMenuItem
            // 
            toolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] { staticRangeFinderToolStripMenuItem });
            toolsToolStripMenuItem.Name = "toolsToolStripMenuItem";
            toolsToolStripMenuItem.Size = new System.Drawing.Size(46, 20);
            toolsToolStripMenuItem.Text = "Tools";
            toolsToolStripMenuItem.DropDownOpening += new System.EventHandler(this.toolsToolStripMenuItem_DropDownOpening);
            // 
            // staticRangeFinderToolStripMenuItem
            // 
            staticRangeFinderToolStripMenuItem.Name = "staticRangeFinderToolStripMenuItem";
            staticRangeFinderToolStripMenuItem.Size = new System.Drawing.Size(181, 22);
            staticRangeFinderToolStripMenuItem.Text = "Static Range Finder...";
            staticRangeFinderToolStripMenuItem.Click += new System.EventHandler(this.staticRangeFinderToolStripMenuItem_Click);
            // 
            // panelBottom
            // 
            panelBottom.Controls.Add(lblElapsedTime);
            panelBottom.Controls.Add(lblProgressPercentage);
            panelBottom.Controls.Add(btnStopScan);
            panelBottom.Controls.Add(progressBar);
            panelBottom.Controls.Add(tableLayoutPanel1);
            panelBottom.Dock = System.Windows.Forms.DockStyle.Bottom;
            panelBottom.Location = new System.Drawing.Point(0, 545);
            panelBottom.Name = "panelBottom";
            panelBottom.Size = new System.Drawing.Size(966, 47);
            panelBottom.TabIndex = 2;
            // 
            // lblElapsedTime
            // 
            lblElapsedTime.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left;
            lblElapsedTime.AutoSize = true;
            lblElapsedTime.Location = new System.Drawing.Point(9, 27);
            lblElapsedTime.Name = "lblElapsedTime";
            lblElapsedTime.Size = new System.Drawing.Size(71, 15);
            lblElapsedTime.TabIndex = 5;
            lblElapsedTime.Text = "Time: 00:00.0";
            lblElapsedTime.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            lblElapsedTime.Visible = false;
            // 
            // lblProgressPercentage
            // 
            lblProgressPercentage.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right;
            lblProgressPercentage.Location = new System.Drawing.Point(748, 27);
            lblProgressPercentage.Name = "lblProgressPercentage";
            lblProgressPercentage.Size = new System.Drawing.Size(140, 18);
            lblProgressPercentage.TabIndex = 4;
            lblProgressPercentage.Text = "0 / 0";
            lblProgressPercentage.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            lblProgressPercentage.Visible = false;
            // 
            // btnStopScan
            // 
            btnStopScan.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right;
            btnStopScan.Location = new System.Drawing.Point(891, 3);
            btnStopScan.Name = "btnStopScan";
            btnStopScan.Size = new System.Drawing.Size(73, 41);
            btnStopScan.TabIndex = 2;
            btnStopScan.Text = "Stop";
            btnStopScan.UseVisualStyleBackColor = true;
            btnStopScan.Visible = false;
            btnStopScan.Click += new System.EventHandler(this.btnStopScan_Click);
            // 
            // progressBar
            // 
            progressBar.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right;
            progressBar.Location = new System.Drawing.Point(3, 3);
            progressBar.Name = "progressBar";
            progressBar.Size = new System.Drawing.Size(885, 21);
            progressBar.TabIndex = 1;
            progressBar.Visible = false;
            // 
            // tableLayoutPanel1
            // 
            tableLayoutPanel1.ColumnCount = 4;
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            tableLayoutPanel1.Controls.Add(btnScan, 0, 0);
            tableLayoutPanel1.Controls.Add(btnRefineScan, 1, 0);
            tableLayoutPanel1.Controls.Add(btnFilter, 3, 0);
            tableLayoutPanel1.Controls.Add(btnCaptureState, 2, 0);
            tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            tableLayoutPanel1.Name = "tableLayoutPanel1";
            tableLayoutPanel1.RowCount = 1;
            tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            tableLayoutPanel1.Size = new System.Drawing.Size(966, 47);
            tableLayoutPanel1.TabIndex = 0;
            // 
            // btnScan
            // 
            btnScan.Dock = System.Windows.Forms.DockStyle.Fill;
            btnScan.Enabled = false;
            btnScan.Location = new System.Drawing.Point(3, 3);
            btnScan.Name = "btnScan";
            btnScan.Size = new System.Drawing.Size(235, 41);
            btnScan.TabIndex = 0;
            btnScan.Text = "New Pointer Scan";
            btnScan.UseVisualStyleBackColor = true;
            btnScan.Click += new System.EventHandler(this.btnScan_Click);
            // 
            // btnRefineScan
            // 
            btnRefineScan.Dock = System.Windows.Forms.DockStyle.Fill;
            btnRefineScan.Enabled = false;
            btnRefineScan.Location = new System.Drawing.Point(244, 3);
            btnRefineScan.Name = "btnRefineScan";
            btnRefineScan.Size = new System.Drawing.Size(235, 41);
            btnRefineScan.TabIndex = 3;
            btnRefineScan.Text = "Refine with New Scan";
            btnRefineScan.UseVisualStyleBackColor = true;
            btnRefineScan.Click += new System.EventHandler(this.btnRefineScan_Click);
            // 
            // btnFilter
            // 
            btnFilter.Dock = System.Windows.Forms.DockStyle.Fill;
            btnFilter.Enabled = false;
            btnFilter.Location = new System.Drawing.Point(726, 3);
            btnFilter.Name = "btnFilter";
            btnFilter.Size = new System.Drawing.Size(237, 41);
            btnFilter.TabIndex = 2;
            btnFilter.Text = "Filter Dynamic Paths";
            btnFilter.UseVisualStyleBackColor = true;
            btnFilter.Click += new System.EventHandler(this.btnFilter_Click);
            // 
            // btnCaptureState
            // 
            btnCaptureState.Dock = System.Windows.Forms.DockStyle.Fill;
            btnCaptureState.Enabled = false;
            btnCaptureState.Location = new System.Drawing.Point(485, 3);
            btnCaptureState.Name = "btnCaptureState";
            btnCaptureState.Size = new System.Drawing.Size(235, 41);
            btnCaptureState.TabIndex = 4;
            btnCaptureState.Text = "Capture State";
            btnCaptureState.UseVisualStyleBackColor = true;
            btnCaptureState.Click += new System.EventHandler(this.btnCaptureState_Click);
            // 
            // tabControlMain
            // 
            tabControlMain.Controls.Add(tabResults);
            tabControlMain.Dock = System.Windows.Forms.DockStyle.Fill;
            tabControlMain.Location = new System.Drawing.Point(0, 24);
            tabControlMain.Name = "tabControlMain";
            tabControlMain.SelectedIndex = 0;
            tabControlMain.Size = new System.Drawing.Size(966, 499);
            tabControlMain.TabIndex = 3;
            // 
            // tabResults
            // 
            tabResults.Controls.Add(dgvResults);
            tabResults.Location = new System.Drawing.Point(4, 24);
            tabResults.Name = "tabResults";
            tabResults.Padding = new System.Windows.Forms.Padding(3);
            tabResults.Size = new System.Drawing.Size(958, 471);
            tabResults.TabIndex = 0;
            tabResults.Text = "Results";
            tabResults.UseVisualStyleBackColor = true;
            // 
            // dgvResults
            // 
            dgvResults.AllowUserToAddRows = false;
            dgvResults.AllowUserToDeleteRows = false;
            dgvResults.AllowUserToResizeRows = false;
            dgvResults.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            dgvResults.ContextMenuStrip = contextMenuResults;
            dgvResults.Dock = System.Windows.Forms.DockStyle.Fill;
            dgvResults.Location = new System.Drawing.Point(3, 3);
            dgvResults.Name = "dgvResults";
            dgvResults.ReadOnly = true;
            dgvResults.RowHeadersVisible = false;
            dgvResults.RowTemplate.Height = 24;
            dgvResults.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            dgvResults.Size = new System.Drawing.Size(952, 465);
            dgvResults.TabIndex = 0;
            dgvResults.KeyDown += new System.Windows.Forms.KeyEventHandler(this.dgvResults_KeyDown);
            // 
            // contextMenuResults
            // 
            contextMenuResults.ImageScalingSize = new System.Drawing.Size(20, 20);
            contextMenuResults.Items.AddRange(new System.Windows.Forms.ToolStripItem[] { copyBaseAddressToolStripMenuItem, copyAsRetroAchievementsFormatToolStripMenuItem, deleteSelectedToolStripMenuItem, toolStripSeparator3, sortByLowestOffsetsToolStripMenuItem });
            contextMenuResults.Name = "contextMenuResults";
            contextMenuResults.Size = new System.Drawing.Size(338, 98);
            contextMenuResults.Opening += new System.ComponentModel.CancelEventHandler(this.contextMenuResults_Opening);
            // 
            // copyBaseAddressToolStripMenuItem
            // 
            copyBaseAddressToolStripMenuItem.Name = "copyBaseAddressToolStripMenuItem";
            copyBaseAddressToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
            copyBaseAddressToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            copyBaseAddressToolStripMenuItem.Text = "Copy Base Address";
            copyBaseAddressToolStripMenuItem.Click += new System.EventHandler(this.copyBaseAddressToolStripMenuItem_Click);
            // 
            // copyAsRetroAchievementsFormatToolStripMenuItem
            // 
            copyAsRetroAchievementsFormatToolStripMenuItem.Name = "copyAsRetroAchievementsFormatToolStripMenuItem";
            copyAsRetroAchievementsFormatToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift)
            | System.Windows.Forms.Keys.C)));
            copyAsRetroAchievementsFormatToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            copyAsRetroAchievementsFormatToolStripMenuItem.Text = "Copy as RetroAchievements Format";
            copyAsRetroAchievementsFormatToolStripMenuItem.Click += new System.EventHandler(this.copyAsRetroAchievementsFormatToolStripMenuItem_Click);
            // 
            // deleteSelectedToolStripMenuItem
            // 
            deleteSelectedToolStripMenuItem.Name = "deleteSelectedToolStripMenuItem";
            deleteSelectedToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.Delete;
            deleteSelectedToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            deleteSelectedToolStripMenuItem.Text = "Delete Selected";
            deleteSelectedToolStripMenuItem.Click += new System.EventHandler(this.deleteSelectedToolStripMenuItem_Click);
            // 
            // sortByLowestOffsetsToolStripMenuItem
            // 
            this.sortByLowestOffsetsToolStripMenuItem.Name = "sortByLowestOffsetsToolStripMenuItem";
            this.sortByLowestOffsetsToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            this.sortByLowestOffsetsToolStripMenuItem.Text = "Sort by Lowest Offsets";
            this.sortByLowestOffsetsToolStripMenuItem.Click += new System.EventHandler(this.sortByLowestOffsetsToolStripMenuItem_Click);
            // 
            // statusTableLayoutPanel
            // 
            statusTableLayoutPanel.ColumnCount = 3;
            statusTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            statusTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            statusTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            statusTableLayoutPanel.Controls.Add(lblStatus, 0, 0);
            statusTableLayoutPanel.Controls.Add(lblBaseAddress, 1, 0);
            statusTableLayoutPanel.Controls.Add(lblResultCount, 2, 0);
            statusTableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            statusTableLayoutPanel.Location = new System.Drawing.Point(0, 523);
            statusTableLayoutPanel.Name = "statusTableLayoutPanel";
            statusTableLayoutPanel.RowCount = 1;
            statusTableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            statusTableLayoutPanel.Size = new System.Drawing.Size(966, 22);
            statusTableLayoutPanel.TabIndex = 5;
            // 
            // lblStatus
            // 
            lblStatus.Dock = System.Windows.Forms.DockStyle.Fill;
            lblStatus.Location = new System.Drawing.Point(3, 0);
            lblStatus.Name = "lblStatus";
            lblStatus.Padding = new System.Windows.Forms.Padding(2, 0, 0, 0);
            lblStatus.Size = new System.Drawing.Size(892, 22);
            lblStatus.TabIndex = 0;
            lblStatus.Text = "Status: Not Attached";
            lblStatus.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lblBaseAddress
            // 
            lblBaseAddress.AutoSize = true;
            lblBaseAddress.Dock = System.Windows.Forms.DockStyle.Fill;
            lblBaseAddress.Location = new System.Drawing.Point(901, 0);
            lblBaseAddress.Name = "lblBaseAddress";
            lblBaseAddress.Size = new System.Drawing.Size(1, 22);
            lblBaseAddress.TabIndex = 1;
            lblBaseAddress.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // lblResultCount
            // 
            lblResultCount.AutoSize = true;
            lblResultCount.Dock = System.Windows.Forms.DockStyle.Fill;
            lblResultCount.Location = new System.Drawing.Point(907, 0);
            lblResultCount.Name = "lblResultCount";
            lblResultCount.Size = new System.Drawing.Size(56, 22);
            lblResultCount.TabIndex = 2;
            lblResultCount.Text = "Results: 0";
            lblResultCount.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // MainForm
            // 
            AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            ClientSize = new System.Drawing.Size(966, 592);
            Controls.Add(tabControlMain);
            Controls.Add(statusTableLayoutPanel);
            Controls.Add(panelBottom);
            Controls.Add(menuStrip1);
            MainMenuStrip = menuStrip1;
            MinimumSize = new System.Drawing.Size(640, 480);
            Name = "MainForm";
            Text = "Pointer Finder 2.0";
            menuStrip1.ResumeLayout(false);
            menuStrip1.PerformLayout();
            panelBottom.ResumeLayout(false);
            panelBottom.PerformLayout();
            tableLayoutPanel1.ResumeLayout(false);
            tabControlMain.ResumeLayout(false);
            tabResults.ResumeLayout(false);
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
        private System.Windows.Forms.TabControl tabControlMain;
        private System.Windows.Forms.TabPage tabResults;
        private System.Windows.Forms.DataGridView dgvResults;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.Button btnScan;
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
        private System.Windows.Forms.Button btnRefineScan;
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
    }
}