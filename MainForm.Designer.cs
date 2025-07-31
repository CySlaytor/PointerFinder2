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
            menuStrip1 = new MenuStrip();
            fileToolStripMenuItem = new ToolStripMenuItem();
            menuAttach = new ToolStripMenuItem();
            menuExit = new ToolStripMenuItem();
            viewToolStripMenuItem = new ToolStripMenuItem();
            debugConsoleToolStripMenuItem = new ToolStripMenuItem();
            debugOptionsToolStripMenuItem = new ToolStripMenuItem();
            panelBottom = new Panel();
            lblElapsedTime = new Label();
            lblProgressPercentage = new Label();
            btnStopScan = new Button();
            progressBar = new ProgressBar();
            tableLayoutPanel1 = new TableLayoutPanel();
            btnScan = new Button();
            btnRefineScan = new Button();
            btnFilter = new Button();
            tabControlMain = new TabControl();
            tabResults = new TabPage();
            dgvResults = new DataGridView();
            contextMenuResults = new ContextMenuStrip(components);
            copyBaseAddressToolStripMenuItem = new ToolStripMenuItem();
            copyAsRetroAchievementsFormatToolStripMenuItem = new ToolStripMenuItem();
            deleteSelectedToolStripMenuItem = new ToolStripMenuItem();
            tabAnalysis = new TabPage();
            treeViewAnalysis = new TreeView();
            statusTableLayoutPanel = new TableLayoutPanel();
            lblStatus = new Label();
            lblBaseAddress = new Label();
            lblResultCount = new Label();
            panelSearch = new Panel();
            btnClearSearch = new Button();
            btnSearch = new Button();
            txtSearchBaseAddress = new TextBox();
            labelSearch = new Label();
            menuStrip1.SuspendLayout();
            panelBottom.SuspendLayout();
            tableLayoutPanel1.SuspendLayout();
            tabControlMain.SuspendLayout();
            tabResults.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)dgvResults).BeginInit();
            contextMenuResults.SuspendLayout();
            tabAnalysis.SuspendLayout();
            statusTableLayoutPanel.SuspendLayout();
            panelSearch.SuspendLayout();
            SuspendLayout();
            // 
            // menuStrip1
            // 
            menuStrip1.ImageScalingSize = new Size(20, 20);
            menuStrip1.Items.AddRange(new ToolStripItem[] { fileToolStripMenuItem, viewToolStripMenuItem });
            menuStrip1.Location = new Point(0, 0);
            menuStrip1.Name = "menuStrip1";
            menuStrip1.Padding = new Padding(4, 2, 0, 2);
            menuStrip1.Size = new Size(966, 24);
            menuStrip1.TabIndex = 0;
            menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            fileToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { menuAttach, menuExit });
            fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            fileToolStripMenuItem.Size = new Size(37, 20);
            fileToolStripMenuItem.Text = "&File";
            // 
            // menuAttach
            // 
            menuAttach.Name = "menuAttach";
            menuAttach.Size = new Size(183, 22);
            menuAttach.Text = "Attach to Emulator...";
            menuAttach.Click += menuAttach_Click;
            // 
            // menuExit
            // 
            menuExit.Name = "menuExit";
            menuExit.Size = new Size(183, 22);
            menuExit.Text = "E&xit";
            menuExit.Click += menuExit_Click;
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
            debugConsoleToolStripMenuItem.Size = new Size(180, 22);
            debugConsoleToolStripMenuItem.Text = "Debug Console";
            debugConsoleToolStripMenuItem.Click += debugConsoleToolStripMenuItem_Click;
            // 
            // debugOptionsToolStripMenuItem
            // 
            debugOptionsToolStripMenuItem.Name = "debugOptionsToolStripMenuItem";
            debugOptionsToolStripMenuItem.Size = new Size(180, 22);
            debugOptionsToolStripMenuItem.Text = "Settings...";
            debugOptionsToolStripMenuItem.Click += debugOptionsToolStripMenuItem_Click;
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
            lblElapsedTime.Anchor = AnchorStyles.Top | AnchorStyles.Left;
            lblElapsedTime.AutoSize = true;
            lblElapsedTime.Location = new Point(9, 27);
            lblElapsedTime.Name = "lblElapsedTime";
            lblElapsedTime.Size = new Size(71, 15);
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
            tableLayoutPanel1.ColumnCount = 3;
            tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 33.33333F));
            tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 33.33333F));
            tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 33.33333F));
            tableLayoutPanel1.Controls.Add(btnScan, 0, 0);
            tableLayoutPanel1.Controls.Add(btnRefineScan, 1, 0);
            tableLayoutPanel1.Controls.Add(btnFilter, 2, 0);
            tableLayoutPanel1.Dock = DockStyle.Fill;
            tableLayoutPanel1.Location = new Point(0, 0);
            tableLayoutPanel1.Name = "tableLayoutPanel1";
            tableLayoutPanel1.RowCount = 1;
            tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            tableLayoutPanel1.Size = new Size(966, 47);
            tableLayoutPanel1.TabIndex = 0;
            // 
            // btnScan
            // 
            btnScan.Dock = DockStyle.Fill;
            btnScan.Enabled = false;
            btnScan.Location = new Point(3, 3);
            btnScan.Name = "btnScan";
            btnScan.Size = new Size(316, 41);
            btnScan.TabIndex = 0;
            btnScan.Text = "New Pointer Scan";
            btnScan.UseVisualStyleBackColor = true;
            btnScan.Click += btnScan_Click;
            // 
            // btnRefineScan
            // 
            btnRefineScan.Dock = DockStyle.Fill;
            btnRefineScan.Enabled = false;
            btnRefineScan.Location = new Point(325, 3);
            btnRefineScan.Name = "btnRefineScan";
            btnRefineScan.Size = new Size(316, 41);
            btnRefineScan.TabIndex = 3;
            btnRefineScan.Text = "Refine with New Scan";
            btnRefineScan.UseVisualStyleBackColor = true;
            btnRefineScan.Click += btnRefineScan_Click;
            // 
            // btnFilter
            // 
            btnFilter.Dock = DockStyle.Fill;
            btnFilter.Enabled = false;
            btnFilter.Location = new Point(647, 3);
            btnFilter.Name = "btnFilter";
            btnFilter.Size = new Size(316, 41);
            btnFilter.TabIndex = 2;
            btnFilter.Text = "Filter Dynamic Paths";
            btnFilter.UseVisualStyleBackColor = true;
            btnFilter.Click += btnFilter_Click;
            // 
            // tabControlMain
            // 
            tabControlMain.Controls.Add(tabResults);
            tabControlMain.Controls.Add(tabAnalysis);
            tabControlMain.Dock = DockStyle.Fill;
            tabControlMain.Location = new Point(0, 59);
            tabControlMain.Name = "tabControlMain";
            tabControlMain.SelectedIndex = 0;
            tabControlMain.Size = new Size(966, 464);
            tabControlMain.TabIndex = 3;
            // 
            // tabResults
            // 
            tabResults.Controls.Add(dgvResults);
            tabResults.Location = new Point(4, 24);
            tabResults.Name = "tabResults";
            tabResults.Padding = new Padding(3);
            tabResults.Size = new Size(958, 436);
            tabResults.TabIndex = 0;
            tabResults.Text = "Results";
            tabResults.UseVisualStyleBackColor = true;
            // 
            // dgvResults
            // 
            dgvResults.AllowUserToAddRows = false;
            dgvResults.AllowUserToDeleteRows = false;
            dgvResults.AllowUserToResizeRows = false;
            dgvResults.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            dgvResults.ContextMenuStrip = contextMenuResults;
            dgvResults.Dock = DockStyle.Fill;
            dgvResults.Location = new Point(3, 3);
            dgvResults.Name = "dgvResults";
            dgvResults.ReadOnly = true;
            dgvResults.RowHeadersVisible = false;
            dgvResults.RowTemplate.Height = 24;
            dgvResults.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
            dgvResults.Size = new Size(952, 430);
            dgvResults.TabIndex = 0;
            dgvResults.KeyDown += dgvResults_KeyDown;
            // 
            // contextMenuResults
            // 
            contextMenuResults.ImageScalingSize = new Size(20, 20);
            contextMenuResults.Items.AddRange(new ToolStripItem[] { copyBaseAddressToolStripMenuItem, copyAsRetroAchievementsFormatToolStripMenuItem, deleteSelectedToolStripMenuItem });
            contextMenuResults.Name = "contextMenuResults";
            contextMenuResults.Size = new Size(338, 70);
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
            // deleteSelectedToolStripMenuItem
            // 
            deleteSelectedToolStripMenuItem.Name = "deleteSelectedToolStripMenuItem";
            deleteSelectedToolStripMenuItem.ShortcutKeys = Keys.Delete;
            deleteSelectedToolStripMenuItem.Size = new Size(337, 22);
            deleteSelectedToolStripMenuItem.Text = "Delete Selected";
            deleteSelectedToolStripMenuItem.Click += deleteSelectedToolStripMenuItem_Click;
            // 
            // tabAnalysis
            // 
            tabAnalysis.Controls.Add(treeViewAnalysis);
            tabAnalysis.Location = new Point(4, 24);
            tabAnalysis.Name = "tabAnalysis";
            tabAnalysis.Padding = new Padding(3);
            tabAnalysis.Size = new Size(958, 436);
            tabAnalysis.TabIndex = 1;
            tabAnalysis.Text = "Structure Analysis";
            tabAnalysis.UseVisualStyleBackColor = true;
            // 
            // treeViewAnalysis
            // 
            treeViewAnalysis.Dock = DockStyle.Fill;
            treeViewAnalysis.Location = new Point(3, 3);
            treeViewAnalysis.Name = "treeViewAnalysis";
            treeViewAnalysis.Size = new Size(952, 430);
            treeViewAnalysis.TabIndex = 0;
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
            // panelSearch
            // 
            panelSearch.Controls.Add(btnClearSearch);
            panelSearch.Controls.Add(btnSearch);
            panelSearch.Controls.Add(txtSearchBaseAddress);
            panelSearch.Controls.Add(labelSearch);
            panelSearch.Dock = DockStyle.Top;
            panelSearch.Location = new Point(0, 24);
            panelSearch.Name = "panelSearch";
            panelSearch.Size = new Size(966, 35);
            panelSearch.TabIndex = 6;
            // 
            // btnClearSearch
            // 
            btnClearSearch.Location = new Point(369, 6);
            btnClearSearch.Name = "btnClearSearch";
            btnClearSearch.Size = new Size(75, 23);
            btnClearSearch.TabIndex = 3;
            btnClearSearch.Text = "Clear";
            btnClearSearch.UseVisualStyleBackColor = true;
            btnClearSearch.Click += btnClearSearch_Click;
            // 
            // btnSearch
            // 
            btnSearch.Location = new Point(288, 7);
            btnSearch.Name = "btnSearch";
            btnSearch.Size = new Size(75, 23);
            btnSearch.TabIndex = 2;
            btnSearch.Text = "Find";
            btnSearch.UseVisualStyleBackColor = true;
            btnSearch.Click += btnSearch_Click;
            // 
            // txtSearchBaseAddress
            // 
            txtSearchBaseAddress.Location = new Point(150, 7);
            txtSearchBaseAddress.Name = "txtSearchBaseAddress";
            txtSearchBaseAddress.Size = new Size(132, 23);
            txtSearchBaseAddress.TabIndex = 1;
            txtSearchBaseAddress.KeyDown += txtSearchBaseAddress_KeyDown;
            // 
            // labelSearch
            // 
            labelSearch.AutoSize = true;
            labelSearch.Location = new Point(12, 10);
            labelSearch.Name = "labelSearch";
            labelSearch.Size = new Size(132, 15);
            labelSearch.TabIndex = 0;
            labelSearch.Text = "Highlight Base Address:";
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(966, 592);
            Controls.Add(tabControlMain);
            Controls.Add(panelSearch);
            Controls.Add(statusTableLayoutPanel);
            Controls.Add(panelBottom);
            Controls.Add(menuStrip1);
            MainMenuStrip = menuStrip1;
            MinimumSize = new Size(640, 480);
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
            tabAnalysis.ResumeLayout(false);
            statusTableLayoutPanel.ResumeLayout(false);
            statusTableLayoutPanel.PerformLayout();
            panelSearch.ResumeLayout(false);
            panelSearch.PerformLayout();
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
        private System.Windows.Forms.TabPage tabAnalysis;
        private System.Windows.Forms.DataGridView dgvResults;
        private System.Windows.Forms.TreeView treeViewAnalysis;
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
        private System.Windows.Forms.Panel panelSearch;
        private System.Windows.Forms.Button btnClearSearch;
        private System.Windows.Forms.Button btnSearch;
        private System.Windows.Forms.TextBox txtSearchBaseAddress;
        private System.Windows.Forms.Label labelSearch;
        private Label lblElapsedTime;
    }
}