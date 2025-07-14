using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using PointerFinder2.UI.Controls;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2
{
    public partial class MainForm : Form
    {
        private IEmulatorManager _currentManager;
        private IPointerScannerStrategy _currentScanner;
        private AppSettings _currentSettings;
        private EmulatorProfile _activeProfile;

        // Timer to monitor the attached process
        private readonly System.Windows.Forms.Timer _processMonitorTimer;

        private List<PointerPath> _currentResults = new List<PointerPath>();
        private CancellationTokenSource _scanCts;
        private CancellationTokenSource _filterCts;
        private ScanParameters _lastScanParams;
        private bool _isRefining = false;
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        public MainForm()
        {
            InitializeComponent();
            dgvResults.DoubleBuffered(true);
            SetUIStateDetached();

            // Initialize and start the process monitor timer
            _processMonitorTimer = new System.Windows.Forms.Timer();
            _processMonitorTimer.Interval = 2000; // Check every 2 seconds
            _processMonitorTimer.Tick += ProcessMonitorTimer_Tick;
            _processMonitorTimer.Start();
        }

        // This event handler runs every 2 seconds
        private void ProcessMonitorTimer_Tick(object sender, EventArgs e)
        {
            // Only check if we think we are attached
            if (_currentManager != null && _currentManager.IsAttached)
            {
                try
                {
                    // The HasExited property tells us if the process has closed
                    if (_currentManager.EmulatorProcess.HasExited)
                    {
                        logger.Log($"Attached process '{_currentManager.EmulatorProcess.ProcessName}' has exited. Auto-detaching.");
                        DetachAndReset();
                    }
                }
                catch
                {
                    // If any error occurs trying to access the process, it's defunct. Detach.
                    logger.Log("Error checking process status. Forcing detach.");
                    DetachAndReset();
                }
            }
        }

        // Centralizes all detach logic
        private void DetachAndReset()
        {
            if (_currentManager != null)
            {
                _currentManager.Detach();
            }
            _activeProfile = null;
            _currentManager = null;
            _currentScanner = null;
            _currentSettings = null;
            SetUIStateDetached();
        }

        private void SetUIStateDetached()
        {
            this.Text = "Pointer Finder 2.0";
            lblStatus.Text = "Status: Not Attached";
            lblBaseAddress.Text = "";
            lblResultCount.Visible = false;
            menuAttach.Text = "Attach to Emulator...";

            btnScan.Enabled = false;
            btnFilter.Enabled = false;
            btnRefineScan.Enabled = false;
        }

        private void menuAttach_Click(object sender, EventArgs e)
        {
            if (_currentManager != null && _currentManager.IsAttached)
            {
                // Use the new helper method
                DetachAndReset();
                return;
            }

            logger.Log("Initiating Smart Attach...");
            var foundProfiles = new List<EmulatorProfile>();
            foreach (var profile in EmulatorProfileRegistry.Profiles)
            {
                foreach (var processName in profile.ProcessNames)
                {
                    if (Memory.GetProcess(processName) != null)
                    {
                        logger.Log($"Found running process for profile: {profile.Name}");
                        foundProfiles.Add(profile);
                        break;
                    }
                }
            }

            if (foundProfiles.Count == 0)
            {
                MessageBox.Show("No supported emulator process found. Please ensure a game is running in either PCSX2 or DuckStation.", "Attach Failed", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            EmulatorProfile selectedProfile;
            if (foundProfiles.Count == 1)
            {
                selectedProfile = foundProfiles[0];
            }
            else
            {
                using (var selectionForm = new EmulatorSelectionForm(foundProfiles))
                {
                    if (selectionForm.ShowDialog() == DialogResult.OK)
                    {
                        selectedProfile = selectionForm.SelectedProfile;
                    }
                    else
                    {
                        return;
                    }
                }
            }

            _activeProfile = selectedProfile;
            _currentManager = _activeProfile.ManagerFactory();
            _currentScanner = _activeProfile.ScannerFactory();
            var defaultSettings = _currentManager.GetDefaultSettings();
            _currentSettings = SettingsManager.Load(_activeProfile.Target, defaultSettings);

            if (_currentManager.Attach())
            {
                this.Text = $"Pointer Finder 2.0 - [{_activeProfile.Name} Mode]";
                lblStatus.Text = $"Status: Attached to {_activeProfile.Name}";
                lblBaseAddress.Text = $"{_activeProfile.Name} Base (PC): {_currentManager.MemoryBasePC:X}";
                menuAttach.Text = $"Detach from {_activeProfile.Name}";
                btnScan.Enabled = true;
                bool hasResults = dgvResults.Rows.Count > 0;
                btnFilter.Enabled = hasResults;
                btnRefineScan.Enabled = hasResults;
            }
            else
            {
                MessageBox.Show($"Could not find required memory exports for {selectedProfile.Name}. Ensure a game is fully loaded and the app is run as Administrator.", "Attach Failed", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                DetachAndReset(); // Use the helper on failure
            }
        }

        private async void btnScan_Click(object sender, EventArgs e)
        {
            if (_scanCts != null || _activeProfile == null) return;
            if (!_currentManager.IsAttached)
            {
                MessageBox.Show($"Please attach to {_activeProfile.Name} first.", "Not Attached", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    _lastScanParams = optionsForm.GetScanParameters();
                    if (_lastScanParams == null) return;

                    _currentSettings = optionsForm.GetCurrentSettings();
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);

                    _scanCts = new CancellationTokenSource();
                    var progress = new Progress<ScanProgressReport>(UpdateScanProgress);
                    await RunScan(_currentScanner.Scan(_currentManager, _lastScanParams, progress, _scanCts.Token));
                }
            }
        }

        private async Task RunScan(Task<List<PointerPath>> scanTask)
        {
            _currentResults.Clear();
            dgvResults.Rows.Clear();
            treeViewAnalysis.Nodes.Clear();
            lblResultCount.Visible = false;

            SwitchToScanUI(isScanning: true);

            try
            {
                _currentResults = await scanTask;
                PopulateResultsGrid(_currentResults);

                if (!_currentResults.Any())
                {
                    UpdateStatus("Scan complete. No paths found.");
                }
                else if ((_lastScanParams?.AnalyzeStructures ?? true))
                {
                    int structureCount = AnalyzeAndDisplayStructures(_currentResults);
                    UpdateStatus($"Scan complete. Found {_currentResults.Count} paths and {structureCount} potential structures.");
                }
                else
                {
                    UpdateStatus($"Scan complete. Found {_currentResults.Count} paths.");
                }
            }
            catch (TaskCanceledException)
            {
                UpdateStatus("Scan stopped by user.");
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Scan failed with an error.");
            }
            finally
            {
                SwitchToScanUI(isScanning: false);
                _scanCts?.Dispose();
                _scanCts = null;
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
            }
        }

        private async void btnRefineScan_Click(object sender, EventArgs e)
        {
            if (_scanCts != null || _activeProfile == null) return;
            if (!_currentManager.IsAttached || dgvResults.Rows.Count == 0)
            {
                MessageBox.Show("Please perform an initial scan and have results before refining.", "Initial Scan Required", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    var newScanParams = optionsForm.GetScanParameters();
                    if (newScanParams == null) return;

                    _lastScanParams = newScanParams;
                    _currentSettings = optionsForm.GetCurrentSettings();
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);

                    _scanCts = new CancellationTokenSource();
                    var progress = new Progress<ScanProgressReport>(UpdateScanProgress);
                    var scanTask = _currentScanner.Scan(_currentManager, newScanParams, progress, _scanCts.Token);

                    var existingPaths = new HashSet<string>(
                        dgvResults.Rows.Cast<DataGridViewRow>()
                        .Select(r => r.Tag as PointerPath)
                        .Where(p => p != null)
                        .Select(p => $"{p.BaseAddress:X8}:{p.GetOffsetsString()}")
                    );

                    await RunRefineScan(scanTask, existingPaths);
                }
            }
        }

        private async Task RunRefineScan(Task<List<PointerPath>> scanTask, HashSet<string> existingPaths)
        {
            _isRefining = true;
            SwitchToScanUI(true, "Refining results...");
            bool shouldLog = DebugSettings.LogRefineScan && !DebugSettings.IsLoggingPaused;

            try
            {
                if (shouldLog) logger.Log($"--- STARTING REFINE SCAN: {existingPaths.Count} initial paths to verify. ---");

                var newResults = await scanTask;
                if (shouldLog) logger.Log($"New scan completed, found {newResults.Count} potential paths. Performing intersection...");

                _currentResults = newResults.Where(p => existingPaths.Contains($"{p.BaseAddress:X8}:{p.GetOffsetsString()}")).ToList();

                if (shouldLog)
                {
                    logger.Log($"Intersection complete. Found {_currentResults.Count} common paths:");
                    foreach (var path in _currentResults)
                    {
                        logger.Log($"  -> {path.BaseAddress:X8} : {path.GetOffsetsString()}");
                    }
                }

                PopulateResultsGrid(_currentResults);

                if (_currentResults.Any())
                {
                    UpdateStatus($"Refine scan complete. Found {_currentResults.Count} matching paths.");
                    if (_lastScanParams?.AnalyzeStructures ?? true)
                    {
                        AnalyzeAndDisplayStructures(_currentResults);
                    }
                }
                else
                {
                    UpdateStatus("Refine scan complete. No matching paths found.");
                }
            }
            catch (TaskCanceledException)
            {
                UpdateStatus("Refine scan stopped by user.");
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during refine scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Refine scan failed with an error.");
            }
            finally
            {
                _isRefining = false;
                SwitchToScanUI(false);
                _scanCts?.Dispose();
                _scanCts = null;
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
            }
        }

        private void SwitchToScanUI(bool isScanning, string customMessage = null)
        {
            bool isFiltering = _filterCts != null && !_filterCts.IsCancellationRequested;

            tableLayoutPanel1.Visible = !isScanning && !isFiltering;
            progressBar.Visible = isScanning;
            btnStopScan.Visible = isScanning || isFiltering;

            btnStopScan.Text = isFiltering ? "Stop Filtering" : "Stop";

            lblProgressPercentage.Visible = isScanning;

            if (!isScanning && !isFiltering)
            {
                lblResultCount.Visible = dgvResults.Rows.Count > 0;
            }

            if (isScanning || isFiltering)
            {
                if (!string.IsNullOrEmpty(customMessage))
                {
                    UpdateStatus(customMessage);
                }
                menuStrip1.Enabled = false;
            }
            else
            {
                menuStrip1.Enabled = true;
                if (_currentManager != null && _currentManager.IsAttached)
                {
                    btnScan.Enabled = true;
                    bool hasResults = dgvResults.Rows.Count > 0;
                    btnFilter.Enabled = hasResults;
                    btnRefineScan.Enabled = hasResults;
                }
                else
                {
                    SetUIStateDetached();
                }
            }
        }

        private void UpdateScanProgress(ScanProgressReport report)
        {
            if (InvokeRequired)
            {
                Invoke((Action)(() => UpdateScanProgress(report)));
                return;
            }

            if (_isRefining && report.CurrentValue > 0 && report.CurrentValue == report.MaxValue)
            {
                UpdateStatus("Scan phase complete, intersecting results...");
                return;
            }

            lblResultCount.Visible = true;
            lblResultCount.Text = $"Results: {report.FoundCount}";

            if (!string.IsNullOrEmpty(report.StatusMessage))
            {
                UpdateStatus(report.StatusMessage);
            }

            if (report.MaxValue > 0)
            {
                progressBar.Maximum = 100;
                int val = (int)((double)report.CurrentValue / report.MaxValue * 100.0);
                progressBar.Value = Math.Min(100, Math.Max(0, val));
                lblProgressPercentage.Text = $"{report.CurrentValue} / {report.MaxValue}";
            }
        }

        private void UpdateStatus(string status)
        {
            if (InvokeRequired)
            {
                Invoke((Action)(() => UpdateStatus(status)));
            }
            else
            {
                lblStatus.Text = "Status: " + status;
            }
        }

        private void PopulateResultsGrid(List<PointerPath> results)
        {
            dgvResults.SuspendLayout();
            dgvResults.Rows.Clear();
            dgvResults.Columns.Clear();

            lblResultCount.Text = $"Results: {results.Count}";
            lblResultCount.Visible = results.Any();

            if (!results.Any())
            {
                dgvResults.ResumeLayout();
                if (_currentManager != null && _currentManager.IsAttached)
                {
                    btnFilter.Enabled = false;
                    btnRefineScan.Enabled = false;
                }
                return;
            }

            btnFilter.Enabled = true;
            btnRefineScan.Enabled = true;

            int maxOffsets = results.Max(p => p.Offsets.Count);
            if (maxOffsets == 0 && results.Any()) maxOffsets = 1;

            dgvResults.Columns.Add("colBase", "Base Address");
            dgvResults.Columns["colBase"].Width = 120;

            for (int i = 0; i < maxOffsets; i++)
            {
                var col = new DataGridViewTextBoxColumn
                {
                    Name = $"colOffset{i + 1}",
                    HeaderText = $"Offset {i + 1}",
                    Width = 80
                };
                dgvResults.Columns.Add(col);
            }

            dgvResults.Columns.Add("colFinal", "Final Address");
            dgvResults.Columns["colFinal"].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;

            var rows = new List<DataGridViewRow>();
            foreach (var result in results)
            {
                var row = new DataGridViewRow();
                row.CreateCells(dgvResults);
                row.Cells[0].Value = _currentManager.FormatDisplayAddress(result.BaseAddress);
                for (int i = 0; i < result.Offsets.Count; i++)
                {
                    int offset = result.Offsets[i];
                    row.Cells[i + 1].Value = (offset < 0) ? $"-{Math.Abs(offset):X}" : $"+{offset:X}";
                }
                row.Cells[dgvResults.Columns.Count - 1].Value = _currentManager.FormatDisplayAddress(result.FinalAddress);
                row.Tag = result;
                rows.Add(row);
            }
            dgvResults.Rows.AddRange(rows.ToArray());
            dgvResults.ResumeLayout();
        }

        private int AnalyzeAndDisplayStructures(List<PointerPath> results)
        {
            treeViewAnalysis.Nodes.Clear();
            treeViewAnalysis.BeginUpdate();

            var structureGroups = from p in results
                                  group p by p.GetOffsetsString() into g
                                  where g.Count() > 1
                                  orderby g.Count() descending
                                  select g;

            int structureCount = structureGroups.Count();

            foreach (var group in structureGroups)
            {
                var members = group.OrderBy(p => p.BaseAddress).ToList();
                if (members.Count < 2) continue;

                var deltas = new List<long>();
                for (int i = 1; i < members.Count; i++)
                {
                    deltas.Add((long)members[i].BaseAddress - members[i - 1].BaseAddress);
                }
                if (!deltas.Any()) continue;

                var commonDelta = deltas.GroupBy(d => d).OrderByDescending(g => g.Count()).First().Key;

                var rootNode = new TreeNode($"Structure Found ({group.Count()} members, Offsets: {group.Key})");
                rootNode.Nodes.Add($"Common Delta (Stride): 0x{commonDelta:X}");
                rootNode.Nodes.Add($"Base Address Range: {_currentManager.FormatDisplayAddress(members.First().BaseAddress)} - {_currentManager.FormatDisplayAddress(members.Last().BaseAddress)}");
                var memberNode = rootNode.Nodes.Add("Member Base Addresses");
                foreach (var member in members)
                {
                    memberNode.Nodes.Add(_currentManager.FormatDisplayAddress(member.BaseAddress));
                }
                treeViewAnalysis.Nodes.Add(rootNode);
            }
            treeViewAnalysis.EndUpdate();
            return structureCount;
        }

        private async void btnFilter_Click(object sender, EventArgs e)
        {
            if (_filterCts != null) return;

            _filterCts = new CancellationTokenSource();
            SwitchToScanUI(false);
            btnScan.Enabled = false;
            btnRefineScan.Enabled = false;
            btnFilter.Enabled = false;
            menuStrip1.Enabled = false;

            try
            {
                await FilterPathsContinuously(_filterCts.Token);
            }
            catch (TaskCanceledException)
            {
                UpdateStatus($"Filtering stopped. {dgvResults.Rows.Count} paths remain.");
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during filtering: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Filtering stopped with an error.");
            }
            finally
            {
                _filterCts?.Dispose();
                _filterCts = null;

                _currentResults = dgvResults.Rows.Cast<DataGridViewRow>()
                                                .Select(r => r.Tag as PointerPath)
                                                .Where(p => p != null)
                                                .ToList();
                lblResultCount.Text = $"Results: {_currentResults.Count}";

                SwitchToScanUI(false);
            }
        }

        private async Task FilterPathsContinuously(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                if (dgvResults.Rows.Count == 0)
                {
                    UpdateStatus("No paths left to filter. Stopping.");
                    break;
                }

                UpdateStatus($"Filtering {dgvResults.Rows.Count} paths...");

                var rowsToCheck = dgvResults.Rows.Cast<DataGridViewRow>().ToList();

                var rowsToRemove = await Task.Run(() =>
                {
                    var foundInvalid = new List<DataGridViewRow>();
                    foreach (var row in rowsToCheck)
                    {
                        if (token.IsCancellationRequested) break;

                        if (row.Tag is PointerPath path)
                        {
                            uint? calculatedAddress = _currentManager.RecalculateFinalAddress(path);
                            if (!calculatedAddress.HasValue || calculatedAddress.Value != path.FinalAddress)
                            {
                                foundInvalid.Add(row);
                            }
                        }
                    }
                    return foundInvalid;
                }, token);

                if (token.IsCancellationRequested) break;

                if (rowsToRemove.Any())
                {
                    Invoke((Action)(() =>
                    {
                        dgvResults.SuspendLayout();
                        foreach (var row in rowsToRemove)
                        {
                            if (!row.IsNewRow)
                            {
                                dgvResults.Rows.Remove(row);
                            }
                        }
                        dgvResults.ResumeLayout();

                        lblResultCount.Text = $"Results: {dgvResults.Rows.Count}";
                    }));
                }
            }
        }

        private void btnStopScan_Click(object sender, EventArgs e)
        {
            if (_filterCts != null)
            {
                _filterCts.Cancel();
            }
            else
            {
                _scanCts?.Cancel();
            }
        }

        private void btnSearch_Click(object sender, EventArgs e)
        {
            string searchText = txtSearchBaseAddress.Text.Trim();
            if (string.IsNullOrEmpty(searchText))
            {
                return;
            }

            foreach (DataGridViewRow row in dgvResults.Rows)
            {
                if (row.IsNewRow) continue;

                // The base address is in the first column (index 0)
                if (row.Cells[0].Value != null &&
                    row.Cells[0].Value.ToString().Equals(searchText, StringComparison.OrdinalIgnoreCase))
                {
                    // Found a match
                    dgvResults.ClearSelection();
                    row.Selected = true;
                    dgvResults.FirstDisplayedScrollingRowIndex = row.Index;
                    UpdateStatus($"Found and selected base address '{searchText}'.");
                    return; // Stop after finding the first one
                }
            }

            // If the loop completes, no match was found
            UpdateStatus($"Base address '{searchText}' not found in results.");
        }

        private void btnClearSearch_Click(object sender, EventArgs e)
        {
            txtSearchBaseAddress.Clear();
            dgvResults.ClearSelection();
            if (dgvResults.Rows.Count > 0)
            {
                dgvResults.FirstDisplayedScrollingRowIndex = 0;
            }
            UpdateStatus("Search cleared.");
            txtSearchBaseAddress.Focus();
        }

        private void txtSearchBaseAddress_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                btnSearch.PerformClick();
                e.SuppressKeyPress = true; // Prevents the 'ding' sound on enter
            }
        }

        private void dgvResults_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Control && e.KeyCode == Keys.C)
            {
                e.SuppressKeyPress = true;
                if (e.Shift)
                {
                    copyAsRetroAchievementsFormatToolStripMenuItem_Click(sender, e);
                }
                else
                {
                    copyBaseAddressToolStripMenuItem_Click(sender, e);
                }
            }
            if (e.KeyCode == Keys.Delete)
            {
                deleteSelectedToolStripMenuItem_Click(sender, e);
            }
        }

        private void contextMenuResults_Opening(object sender, CancelEventArgs e)
        {
            bool hasSelection = dgvResults.SelectedRows.Count > 0;
            bool singleSelection = dgvResults.SelectedRows.Count == 1;

            copyBaseAddressToolStripMenuItem.Enabled = hasSelection;
            deleteSelectedToolStripMenuItem.Enabled = hasSelection;
            copyAsRetroAchievementsFormatToolStripMenuItem.Enabled = singleSelection;
        }

        private void copyBaseAddressToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0 || _currentManager == null) return;

            var addresses = dgvResults.SelectedRows.Cast<DataGridViewRow>()
                .Select(r => r.Tag as PointerPath)
                .Where(p => p != null)
                .Select(p => $"0x{_currentManager.FormatDisplayAddress(p.BaseAddress)}");

            if (addresses.Any())
            {
                Clipboard.SetText(string.Join(Environment.NewLine, addresses));
                UpdateStatus($"Copied {addresses.Count()} base address(es) to clipboard.");
            }
        }

        private void copyAsRetroAchievementsFormatToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count != 1)
            {
                if (dgvResults.SelectedRows.Count > 1)
                    UpdateStatus("Cannot copy multiple rows in RetroAchievements format.");
                return;
            }

            if (dgvResults.SelectedRows[0].Tag is PointerPath path && _currentManager != null)
            {
                Clipboard.SetText(path.ToRetroAchievementsString(_currentManager));
                UpdateStatus("Copied path in RetroAchievements format.");
            }
        }

        private void deleteSelectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0)
            {
                return;
            }

            int originalCount = dgvResults.Rows.Count;
            dgvResults.SuspendLayout();
            foreach (DataGridViewRow item in dgvResults.SelectedRows.Cast<DataGridViewRow>().ToList())
            {
                if (!item.IsNewRow)
                {
                    dgvResults.Rows.Remove(item);
                }
            }
            dgvResults.ResumeLayout();

            _currentResults = dgvResults.Rows.Cast<DataGridViewRow>()
                                            .Select(r => r.Tag as PointerPath)
                                            .Where(p => p != null)
                                            .ToList();

            UpdateStatus($"Deleted {originalCount - _currentResults.Count} row(s).");
            lblResultCount.Text = $"Results: {_currentResults.Count}";
            btnFilter.Enabled = _currentResults.Any();
            btnRefineScan.Enabled = _currentResults.Any();
        }

        private void menuExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void debugConsoleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DebugLogForm.Instance.Show();
            DebugLogForm.Instance.BringToFront();
        }

        private void debugOptionsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new DebugOptionsForm().ShowDialog(this);
        }
    }
}