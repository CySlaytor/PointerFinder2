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
            System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.videoTutorialToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.menuAttach = new System.Windows.Forms.ToolStripMenuItem();
            this.saveSessionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.loadSessionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.menuExit = new System.Windows.Forms.ToolStripMenuItem();
            this.editToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.undoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.redoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.findToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.debugConsoleToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.debugOptionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.staticRangeFinderToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.codeNoteConverterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.panelBottom = new System.Windows.Forms.Panel();
            this.lblElapsedTime = new System.Windows.Forms.Label();
            this.lblProgressPercentage = new System.Windows.Forms.Label();
            this.btnStopScan = new System.Windows.Forms.Button();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.btnScan = new System.Windows.Forms.Button();
            this.btnRefineScan = new System.Windows.Forms.Button();
            this.btnFilter = new System.Windows.Forms.Button();
            this.btnCaptureState = new System.Windows.Forms.Button();
            this.tabControlMain = new System.Windows.Forms.TabControl();
            this.tabResults = new System.Windows.Forms.TabPage();
            this.dgvResults = new System.Windows.Forms.DataGridView();
            this.contextMenuResults = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.copyBaseAddressToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.copyAsRetroAchievementsFormatToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.copyAsCodeNoteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.deleteSelectedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.sortByLowestOffsetsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.statusTableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.lblStatus = new System.Windows.Forms.Label();
            this.lblBaseAddress = new System.Windows.Forms.Label();
            this.lblResultCount = new System.Windows.Forms.Label();
            toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.menuStrip1.SuspendLayout();
            this.panelBottom.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.tabControlMain.SuspendLayout();
            this.tabResults.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgvResults)).BeginInit();
            this.contextMenuResults.SuspendLayout();
            this.statusTableLayoutPanel.SuspendLayout();
            this.SuspendLayout();
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
            // toolStripSeparator4
            // 
            toolStripSeparator4.Name = "toolStripSeparator4";
            toolStripSeparator4.Size = new System.Drawing.Size(334, 6);
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.videoTutorialToolStripMenuItem});
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
            this.menuStrip1.ImageScalingSize = new System.Drawing.Size(20, 20);
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.editToolStripMenuItem,
            this.viewToolStripMenuItem,
            this.toolsToolStripMenuItem,
            this.helpToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Padding = new System.Windows.Forms.Padding(4, 2, 0, 2);
            this.menuStrip1.Size = new System.Drawing.Size(966, 24);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuAttach,
            this.saveSessionToolStripMenuItem,
            this.loadSessionToolStripMenuItem,
            toolStripSeparator2,
            this.menuExit});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            this.fileToolStripMenuItem.DropDownOpening += new System.EventHandler(this.fileToolStripMenuItem_DropDownOpening);
            // 
            // menuAttach
            // 
            this.menuAttach.Name = "menuAttach";
            this.menuAttach.Size = new System.Drawing.Size(183, 22);
            this.menuAttach.Text = "Attach to Emulator...";
            this.menuAttach.Click += new System.EventHandler(this.menuAttach_Click);
            // 
            // saveSessionToolStripMenuItem
            // 
            this.saveSessionToolStripMenuItem.Name = "saveSessionToolStripMenuItem";
            this.saveSessionToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            this.saveSessionToolStripMenuItem.Size = new System.Drawing.Size(183, 22);
            this.saveSessionToolStripMenuItem.Text = "Save Session...";
            this.saveSessionToolStripMenuItem.Click += new System.EventHandler(this.saveSessionToolStripMenuItem_Click);
            // 
            // loadSessionToolStripMenuItem
            // 
            this.loadSessionToolStripMenuItem.Name = "loadSessionToolStripMenuItem";
            this.loadSessionToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.loadSessionToolStripMenuItem.Size = new System.Drawing.Size(183, 22);
            this.loadSessionToolStripMenuItem.Text = "Load Session...";
            this.loadSessionToolStripMenuItem.Click += new System.EventHandler(this.loadSessionToolStripMenuItem_Click);
            // 
            // menuExit
            // 
            this.menuExit.Name = "menuExit";
            this.menuExit.Size = new System.Drawing.Size(183, 22);
            this.menuExit.Text = "E&xit";
            this.menuExit.Click += new System.EventHandler(this.menuExit_Click);
            // 
            // editToolStripMenuItem
            // 
            this.editToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.undoToolStripMenuItem,
            this.redoToolStripMenuItem,
            toolStripSeparator1,
            this.findToolStripMenuItem});
            this.editToolStripMenuItem.Name = "editToolStripMenuItem";
            this.editToolStripMenuItem.Size = new System.Drawing.Size(39, 20);
            this.editToolStripMenuItem.Text = "&Edit";
            this.editToolStripMenuItem.DropDownOpening += new System.EventHandler(this.editToolStripMenuItem_DropDownOpening);
            // 
            // undoToolStripMenuItem
            // 
            this.undoToolStripMenuItem.Name = "undoToolStripMenuItem";
            this.undoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z)));
            this.undoToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.undoToolStripMenuItem.Text = "Undo";
            this.undoToolStripMenuItem.Click += new System.EventHandler(this.undoToolStripMenuItem_Click);
            // 
            // redoToolStripMenuItem
            // 
            this.redoToolStripMenuItem.Name = "redoToolStripMenuItem";
            this.redoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift)
            | System.Windows.Forms.Keys.Z)));
            this.redoToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.redoToolStripMenuItem.Text = "Redo";
            this.redoToolStripMenuItem.Click += new System.EventHandler(this.redoToolStripMenuItem_Click);
            // 
            // findToolStripMenuItem
            // 
            this.findToolStripMenuItem.Name = "findToolStripMenuItem";
            this.findToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.F)));
            this.findToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.findToolStripMenuItem.Text = "Find...";
            this.findToolStripMenuItem.Click += new System.EventHandler(this.findToolStripMenuItem_Click);
            // 
            // viewToolStripMenuItem
            // 
            this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.debugConsoleToolStripMenuItem,
            this.debugOptionsToolStripMenuItem});
            this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            this.viewToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.viewToolStripMenuItem.Text = "View";
            // 
            // debugConsoleToolStripMenuItem
            // 
            this.debugConsoleToolStripMenuItem.Name = "debugConsoleToolStripMenuItem";
            this.debugConsoleToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
            this.debugConsoleToolStripMenuItem.Text = "Debug Console";
            this.debugConsoleToolStripMenuItem.Click += new System.EventHandler(this.debugConsoleToolStripMenuItem_Click);
            // 
            // debugOptionsToolStripMenuItem
            // 
            this.debugOptionsToolStripMenuItem.Name = "debugOptionsToolStripMenuItem";
            this.debugOptionsToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
            this.debugOptionsToolStripMenuItem.Text = "Settings...";
            this.debugOptionsToolStripMenuItem.Click += new System.EventHandler(this.debugOptionsToolStripMenuItem_Click);
            // 
            // toolsToolStripMenuItem
            // 
            this.toolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.staticRangeFinderToolStripMenuItem,
            this.codeNoteConverterToolStripMenuItem});
            this.toolsToolStripMenuItem.Name = "toolsToolStripMenuItem";
            this.toolsToolStripMenuItem.Size = new System.Drawing.Size(46, 20);
            this.toolsToolStripMenuItem.Text = "Tools";
            this.toolsToolStripMenuItem.DropDownOpening += new System.EventHandler(this.toolsToolStripMenuItem_DropDownOpening);
            // 
            // staticRangeFinderToolStripMenuItem
            // 
            this.staticRangeFinderToolStripMenuItem.Name = "staticRangeFinderToolStripMenuItem";
            this.staticRangeFinderToolStripMenuItem.Size = new System.Drawing.Size(193, 22);
            this.staticRangeFinderToolStripMenuItem.Text = "Static Range Finder...";
            this.staticRangeFinderToolStripMenuItem.Click += new System.EventHandler(this.staticRangeFinderToolStripMenuItem_Click);
            // 
            // codeNoteConverterToolStripMenuItem
            // 
            this.codeNoteConverterToolStripMenuItem.Name = "codeNoteConverterToolStripMenuItem";
            this.codeNoteConverterToolStripMenuItem.Size = new System.Drawing.Size(193, 22);
            this.codeNoteConverterToolStripMenuItem.Text = "Code Note Converter...";
            this.codeNoteConverterToolStripMenuItem.Click += new System.EventHandler(this.codeNoteConverterToolStripMenuItem_Click);
            // 
            // panelBottom
            // 
            this.panelBottom.Controls.Add(this.lblElapsedTime);
            this.panelBottom.Controls.Add(this.lblProgressPercentage);
            this.panelBottom.Controls.Add(this.btnStopScan);
            this.panelBottom.Controls.Add(this.progressBar);
            this.panelBottom.Controls.Add(this.tableLayoutPanel1);
            this.panelBottom.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panelBottom.Location = new System.Drawing.Point(0, 545);
            this.panelBottom.Name = "panelBottom";
            this.panelBottom.Size = new System.Drawing.Size(966, 47);
            this.panelBottom.TabIndex = 2;
            // 
            // lblElapsedTime
            // 
            this.lblElapsedTime.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)));
            this.lblElapsedTime.AutoSize = true;
            this.lblElapsedTime.Location = new System.Drawing.Point(9, 27);
            this.lblElapsedTime.Name = "lblElapsedTime";
            this.lblElapsedTime.Size = new System.Drawing.Size(71, 15);
            this.lblElapsedTime.TabIndex = 5;
            this.lblElapsedTime.Text = "Time: 00:00.0";
            this.lblElapsedTime.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.lblElapsedTime.Visible = false;
            // 
            // lblProgressPercentage
            // 
            this.lblProgressPercentage.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lblProgressPercentage.Location = new System.Drawing.Point(748, 27);
            this.lblProgressPercentage.Name = "lblProgressPercentage";
            this.lblProgressPercentage.Size = new System.Drawing.Size(140, 18);
            this.lblProgressPercentage.TabIndex = 4;
            this.lblProgressPercentage.Text = "0 / 0";
            this.lblProgressPercentage.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.lblProgressPercentage.Visible = false;
            // 
            // btnStopScan
            // 
            this.btnStopScan.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnStopScan.Location = new System.Drawing.Point(891, 3);
            this.btnStopScan.Name = "btnStopScan";
            this.btnStopScan.Size = new System.Drawing.Size(73, 41);
            this.btnStopScan.TabIndex = 2;
            this.btnStopScan.Text = "Stop";
            this.btnStopScan.UseVisualStyleBackColor = true;
            this.btnStopScan.Visible = false;
            this.btnStopScan.Click += new System.EventHandler(this.btnStopScan_Click);
            // 
            // progressBar
            // 
            this.progressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.progressBar.Location = new System.Drawing.Point(3, 3);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(885, 21);
            this.progressBar.TabIndex = 1;
            this.progressBar.Visible = false;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 4;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.Controls.Add(this.btnScan, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.btnRefineScan, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.btnFilter, 3, 0);
            this.tableLayoutPanel1.Controls.Add(this.btnCaptureState, 2, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(966, 47);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // btnScan
            // 
            this.btnScan.Dock = System.Windows.Forms.DockStyle.Fill;
            this.btnScan.Enabled = false;
            this.btnScan.Location = new System.Drawing.Point(3, 3);
            this.btnScan.Name = "btnScan";
            this.btnScan.Size = new System.Drawing.Size(235, 41);
            this.btnScan.TabIndex = 0;
            this.btnScan.Text = "New Pointer Scan";
            this.btnScan.UseVisualStyleBackColor = true;
            this.btnScan.Click += new System.EventHandler(this.btnScan_Click);
            // 
            // btnRefineScan
            // 
            this.btnRefineScan.Dock = System.Windows.Forms.DockStyle.Fill;
            this.btnRefineScan.Enabled = false;
            this.btnRefineScan.Location = new System.Drawing.Point(244, 3);
            this.btnRefineScan.Name = "btnRefineScan";
            this.btnRefineScan.Size = new System.Drawing.Size(235, 41);
            this.btnRefineScan.TabIndex = 3;
            this.btnRefineScan.Text = "Refine with New Scan";
            this.btnRefineScan.UseVisualStyleBackColor = true;
            this.btnRefineScan.Click += new System.EventHandler(this.btnRefineScan_Click);
            // 
            // btnFilter
            // 
            this.btnFilter.Dock = System.Windows.Forms.DockStyle.Fill;
            this.btnFilter.Enabled = false;
            this.btnFilter.Location = new System.Drawing.Point(726, 3);
            this.btnFilter.Name = "btnFilter";
            this.btnFilter.Size = new System.Drawing.Size(237, 41);
            this.btnFilter.TabIndex = 2;
            this.btnFilter.Text = "Filter Dynamic Paths";
            this.btnFilter.UseVisualStyleBackColor = true;
            this.btnFilter.Click += new System.EventHandler(this.btnFilter_Click);
            // 
            // btnCaptureState
            // 
            this.btnCaptureState.Dock = System.Windows.Forms.DockStyle.Fill;
            this.btnCaptureState.Enabled = false;
            this.btnCaptureState.Location = new System.Drawing.Point(485, 3);
            this.btnCaptureState.Name = "btnCaptureState";
            this.btnCaptureState.Size = new System.Drawing.Size(235, 41);
            this.btnCaptureState.TabIndex = 4;
            this.btnCaptureState.Text = "Capture State";
            this.btnCaptureState.UseVisualStyleBackColor = true;
            this.btnCaptureState.Click += new System.EventHandler(this.btnCaptureState_Click);
            // 
            // tabControlMain
            // 
            this.tabControlMain.Controls.Add(this.tabResults);
            this.tabControlMain.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControlMain.Location = new System.Drawing.Point(0, 24);
            this.tabControlMain.Name = "tabControlMain";
            this.tabControlMain.SelectedIndex = 0;
            this.tabControlMain.Size = new System.Drawing.Size(966, 499);
            this.tabControlMain.TabIndex = 3;
            // 
            // tabResults
            // 
            this.tabResults.Controls.Add(this.dgvResults);
            this.tabResults.Location = new System.Drawing.Point(4, 24);
            this.tabResults.Name = "tabResults";
            this.tabResults.Padding = new System.Windows.Forms.Padding(3);
            this.tabResults.Size = new System.Drawing.Size(958, 471);
            this.tabResults.TabIndex = 0;
            this.tabResults.Text = "Results";
            this.tabResults.UseVisualStyleBackColor = true;
            // 
            // dgvResults
            // 
            this.dgvResults.AllowUserToAddRows = false;
            this.dgvResults.AllowUserToDeleteRows = false;
            this.dgvResults.AllowUserToResizeRows = false;
            this.dgvResults.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgvResults.ContextMenuStrip = this.contextMenuResults;
            this.dgvResults.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dgvResults.Location = new System.Drawing.Point(3, 3);
            this.dgvResults.Name = "dgvResults";
            this.dgvResults.ReadOnly = true;
            this.dgvResults.RowHeadersVisible = false;
            this.dgvResults.RowTemplate.Height = 24;
            this.dgvResults.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.dgvResults.Size = new System.Drawing.Size(952, 465);
            this.dgvResults.TabIndex = 0;
            this.dgvResults.KeyDown += new System.Windows.Forms.KeyEventHandler(this.dgvResults_KeyDown);
            // 
            // contextMenuResults
            // 
            this.contextMenuResults.ImageScalingSize = new System.Drawing.Size(20, 20);
            this.contextMenuResults.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.copyBaseAddressToolStripMenuItem,
            this.copyAsRetroAchievementsFormatToolStripMenuItem,
            this.copyAsCodeNoteToolStripMenuItem,
            toolStripSeparator4,
            this.deleteSelectedToolStripMenuItem,
            toolStripSeparator3,
            this.sortByLowestOffsetsToolStripMenuItem});
            this.contextMenuResults.Name = "contextMenuResults";
            this.contextMenuResults.Size = new System.Drawing.Size(338, 132);
            this.contextMenuResults.Opening += new System.ComponentModel.CancelEventHandler(this.contextMenuResults_Opening);
            // 
            // copyBaseAddressToolStripMenuItem
            // 
            this.copyBaseAddressToolStripMenuItem.Name = "copyBaseAddressToolStripMenuItem";
            this.copyBaseAddressToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
            this.copyBaseAddressToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            this.copyBaseAddressToolStripMenuItem.Text = "Copy Base Address";
            this.copyBaseAddressToolStripMenuItem.Click += new System.EventHandler(this.copyBaseAddressToolStripMenuItem_Click);
            // 
            // copyAsRetroAchievementsFormatToolStripMenuItem
            // 
            this.copyAsRetroAchievementsFormatToolStripMenuItem.Name = "copyAsRetroAchievementsFormatToolStripMenuItem";
            this.copyAsRetroAchievementsFormatToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift)
            | System.Windows.Forms.Keys.C)));
            this.copyAsRetroAchievementsFormatToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            this.copyAsRetroAchievementsFormatToolStripMenuItem.Text = "Copy as RetroAchievements Format";
            this.copyAsRetroAchievementsFormatToolStripMenuItem.Click += new System.EventHandler(this.copyAsRetroAchievementsFormatToolStripMenuItem_Click);
            // 
            // copyAsCodeNoteToolStripMenuItem
            // 
            this.copyAsCodeNoteToolStripMenuItem.Name = "copyAsCodeNoteToolStripMenuItem";
            this.copyAsCodeNoteToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            this.copyAsCodeNoteToolStripMenuItem.Text = "Copy as Code Note...";
            this.copyAsCodeNoteToolStripMenuItem.Click += new System.EventHandler(this.copyAsCodeNoteToolStripMenuItem_Click);
            // 
            // deleteSelectedToolStripMenuItem
            // 
            this.deleteSelectedToolStripMenuItem.Name = "deleteSelectedToolStripMenuItem";
            this.deleteSelectedToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.Delete;
            this.deleteSelectedToolStripMenuItem.Size = new System.Drawing.Size(337, 22);
            this.deleteSelectedToolStripMenuItem.Text = "Delete Selected";
            this.deleteSelectedToolStripMenuItem.Click += new System.EventHandler(this.deleteSelectedToolStripMenuItem_Click);
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
            this.statusTableLayoutPanel.ColumnCount = 3;
            this.statusTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.statusTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.statusTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.statusTableLayoutPanel.Controls.Add(this.lblStatus, 0, 0);
            this.statusTableLayoutPanel.Controls.Add(this.lblBaseAddress, 1, 0);
            this.statusTableLayoutPanel.Controls.Add(this.lblResultCount, 2, 0);
            this.statusTableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.statusTableLayoutPanel.Location = new System.Drawing.Point(0, 523);
            this.statusTableLayoutPanel.Name = "statusTableLayoutPanel";
            this.statusTableLayoutPanel.RowCount = 1;
            this.statusTableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.statusTableLayoutPanel.Size = new System.Drawing.Size(966, 22);
            this.statusTableLayoutPanel.TabIndex = 5;
            // 
            // lblStatus
            // 
            this.lblStatus.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lblStatus.Location = new System.Drawing.Point(3, 0);
            this.lblStatus.Name = "lblStatus";
            this.lblStatus.Padding = new System.Windows.Forms.Padding(2, 0, 0, 0);
            this.lblStatus.Size = new System.Drawing.Size(892, 22);
            this.lblStatus.TabIndex = 0;
            this.lblStatus.Text = "Status: Not Attached";
            this.lblStatus.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lblBaseAddress
            // 
            this.lblBaseAddress.AutoSize = true;
            this.lblBaseAddress.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lblBaseAddress.Location = new System.Drawing.Point(901, 0);
            this.lblBaseAddress.Name = "lblBaseAddress";
            this.lblBaseAddress.Size = new System.Drawing.Size(1, 22);
            this.lblBaseAddress.TabIndex = 1;
            this.lblBaseAddress.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // lblResultCount
            // 
            this.lblResultCount.AutoSize = true;
            this.lblResultCount.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lblResultCount.Location = new System.Drawing.Point(907, 0);
            this.lblResultCount.Name = "lblResultCount";
            this.lblResultCount.Size = new System.Drawing.Size(56, 22);
            this.lblResultCount.TabIndex = 2;
            this.lblResultCount.Text = "Results: 0";
            this.lblResultCount.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(966, 592);
            this.Controls.Add(this.tabControlMain);
            this.Controls.Add(this.statusTableLayoutPanel);
            this.Controls.Add(this.panelBottom);
            this.Controls.Add(this.menuStrip1);
            this.MainMenuStrip = this.menuStrip1;
            this.MinimumSize = new System.Drawing.Size(640, 480);
            this.Name = "MainForm";
            this.Text = "Pointer Finder 2.0";
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.panelBottom.ResumeLayout(false);
            this.panelBottom.PerformLayout();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tabControlMain.ResumeLayout(false);
            this.tabResults.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dgvResults)).EndInit();
            this.contextMenuResults.ResumeLayout(false);
            this.statusTableLayoutPanel.ResumeLayout(false);
            this.statusTableLayoutPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

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
        private System.Windows.Forms.ToolStripMenuItem copyAsCodeNoteToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem codeNoteConverterToolStripMenuItem;
    }
}