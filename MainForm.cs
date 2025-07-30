using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using PointerFinder2.UI.Controls;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Media;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2
{
    // The main window of the Pointer Finder application.
    // It handles UI events, orchestrates scanning and filtering operations, and manages the application state.
    public partial class MainForm : Form
    {
        // --- Application State ---
        private IEmulatorManager _currentManager;
        private IPointerScannerStrategy _currentScanner;
        private AppSettings _currentSettings;
        private EmulatorProfile _activeProfile;

        // --- Timers ---
        private readonly System.Windows.Forms.Timer _processMonitorTimer; // Checks if the emulator process is still running.
        private readonly System.Windows.Forms.Timer _filterRefreshTimer;  // Periodically refreshes the UI during continuous filtering.

        // --- Data & Threading ---
        private List<PointerPath> _currentResults = new List<PointerPath>();
        private ConcurrentBag<PointerPath> _validFilteredPaths; // A thread-safe collection for the background filter task.
        private CancellationTokenSource _scanCts;
        private CancellationTokenSource _filterCts;
        private ScanParameters _lastScanParams;
        private bool _isRefining = false;
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        public MainForm()
        {
            InitializeComponent();
            dgvResults.DoubleBuffered(true); // Enables double buffering on the grid to reduce flicker.
            SetUIStateDetached();

            // Configure the timer to monitor the attached emulator process.
            _processMonitorTimer = new System.Windows.Forms.Timer();
            _processMonitorTimer.Interval = 2000; // Check every 2 seconds.
            _processMonitorTimer.Tick += ProcessMonitorTimer_Tick;
            _processMonitorTimer.Start();

            // Configure the timer for refreshing the UI during filtering.
            _filterRefreshTimer = new System.Windows.Forms.Timer();
            _filterRefreshTimer.Interval = 1000; // Refresh the grid every second.
            _filterRefreshTimer.Tick += FilterRefreshTimer_Tick;
        }

        #region State Management & Timers

        // Periodically called by the filter timer to update the DataGridView with the latest valid paths.
        // This decouples the heavy filtering logic from the UI thread, ensuring responsiveness.
        private void FilterRefreshTimer_Tick(object sender, EventArgs e)
        {
            if (_filterCts != null && !_filterCts.IsCancellationRequested)
            {
                _currentResults = _validFilteredPaths.ToList();
                PopulateResultsGrid(_currentResults);
                UpdateStatus($"Filtering... {_currentResults.Count} paths remain.");
            }
        }

        // Periodically checks if the attached emulator process has closed unexpectedly.
        private void ProcessMonitorTimer_Tick(object sender, EventArgs e)
        {
            if (_currentManager != null && _currentManager.IsAttached)
            {
                try
                {
                    if (_currentManager.EmulatorProcess.HasExited)
                    {
                        if (DebugSettings.LogLiveScan) logger.Log($"Attached process '{_currentManager.EmulatorProcess.ProcessName}' has exited. Auto-detaching.");
                        DetachAndReset();
                    }
                }
                catch
                {
                    if (DebugSettings.LogLiveScan) logger.Log("Error checking process status. Forcing detach.");
                    DetachAndReset();
                }
            }
        }

        // Centralized method to detach from the emulator and reset the application's state.
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

        // Configures the UI for the "Not Attached" state.
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

        #endregion

        #region Attachment Logic

        // Handles the "Attach/Detach" menu item click.
        private void menuAttach_Click(object sender, EventArgs e)
        {
            // If already attached, this acts as a detach button.
            if (_currentManager != null && _currentManager.IsAttached)
            {
                DetachAndReset();
                return;
            }

            // --- Smart Attach Logic ---
            if (DebugSettings.LogLiveScan) logger.Log("Initiating Smart Attach...");
            // The key is the Emulator Profile, the value is a list of running process instances for that profile.
            var foundProcessMap = new Dictionary<EmulatorProfile, List<Process>>();

            // Check for running processes for each defined profile.
            foreach (var profile in EmulatorProfileRegistry.Profiles)
            {
                foreach (var processName in profile.ProcessNames)
                {
                    // Get all processes with that name, not just the first one.
                    Process[] processes = Process.GetProcessesByName(processName);
                    if (processes.Length > 0)
                    {
                        if (!foundProcessMap.ContainsKey(profile))
                        {
                            foundProcessMap[profile] = new List<Process>();
                        }
                        foundProcessMap[profile].AddRange(processes);
                    }
                }
            }

            int totalInstancesFound = foundProcessMap.Sum(kvp => kvp.Value.Count);

            if (totalInstancesFound == 0)
            {
                MessageBox.Show("No supported emulator process found.", "Attach Failed", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            Process selectedProcess;
            EmulatorProfile selectedProfile;

            // If only one total instance is found, attach to it automatically.
            if (totalInstancesFound == 1)
            {
                var entry = foundProcessMap.First();
                selectedProfile = entry.Key;
                selectedProcess = entry.Value.First();
            }
            else // If multiple instances are found (of the same or different emulators), prompt the user.
            {
                using (var selectionForm = new EmulatorSelectionForm(foundProcessMap))
                {
                    if (selectionForm.ShowDialog() == DialogResult.OK)
                    {
                        selectedProfile = selectionForm.SelectedProfile;
                        selectedProcess = selectionForm.SelectedProcess;
                    }
                    else
                    {
                        return; // User cancelled the selection.
                    }
                }
            }

            if (selectedProcess == null || selectedProfile == null) return;

            // Initialize the manager, scanner, and settings for the selected profile.
            _activeProfile = selectedProfile;
            _currentManager = _activeProfile.ManagerFactory();
            _currentScanner = _activeProfile.ScannerFactory();
            var defaultSettings = _currentManager.GetDefaultSettings();
            _currentSettings = SettingsManager.Load(_activeProfile.Target, defaultSettings);

            // Attempt to attach to the specific process instance and update the UI accordingly.
            if (_currentManager.Attach(selectedProcess))
            {
                this.Text = $"Pointer Finder 2.0 - [{_activeProfile.Name} Mode]";
                lblStatus.Text = $"Status: Attached to {_activeProfile.Name} (PID: {selectedProcess.Id})";
                lblBaseAddress.Text = $"{_activeProfile.Name} Base (PC): {_currentManager.MemoryBasePC:X}";
                menuAttach.Text = $"Detach from {_activeProfile.Name}";
                btnScan.Enabled = true;
                bool hasResults = dgvResults.Rows.Count > 0;
                btnFilter.Enabled = hasResults;
                btnRefineScan.Enabled = hasResults;
            }
            else
            {
                MessageBox.Show($"Could not find required memory exports for {selectedProfile.Name}.", "Attach Failed", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                DetachAndReset();
            }
        }

        #endregion

        #region Scanning and Refining Logic

        // Handles the "New Pointer Scan" button click.
        private async void btnScan_Click(object sender, EventArgs e)
        {
            if (_scanCts != null || _activeProfile == null) return;
            if (!_currentManager.IsAttached)
            {
                MessageBox.Show($"Please attach to {_activeProfile.Name} first.", "Not Attached", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }

            // Show the options form to get scan parameters from the user.
            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    _lastScanParams = optionsForm.GetScanParameters();
                    if (_lastScanParams == null) return;
                    _currentSettings = optionsForm.GetCurrentSettings();
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);

                    // Start the scan operation within a try-catch to handle any immediate errors.
                    try
                    {
                        _scanCts = new CancellationTokenSource();
                        var progress = new Progress<ScanProgressReport>(UpdateScanProgress);
                        var scanTask = _currentScanner.Scan(_currentManager, _lastScanParams, progress, _scanCts.Token);
                        await RunScan(scanTask);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("An unexpected error occurred: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                        SwitchToScanUI(false);
                    }
                }
            }
        }

        // Manages the execution of a scan task and handles its completion, cancellation, or failure.
        private async Task RunScan(Task<List<PointerPath>> scanTask)
        {
            // Reset UI for a new scan.
            _currentResults.Clear();
            dgvResults.Rows.Clear();
            treeViewAnalysis.Nodes.Clear();
            lblResultCount.Visible = false;
            lblProgressPercentage.Text = $"0 / {_lastScanParams.MaxResults}";
            progressBar.Maximum = _lastScanParams.MaxResults;
            progressBar.Value = 0;

            SwitchToScanUI(isScanningOrFiltering: true);

            try
            {
                // Await the scan task to complete. This will throw OperationCanceledException if cancelled.
                _currentResults = await scanTask;

                // This block is reached only if the scan completes normally (was not cancelled).
                PopulateResultsGrid(_currentResults);

                if (scanTask.IsCanceled)
                {
                    UpdateStatus($"Scan stopped by user. Found {_currentResults.Count} paths.");
                }
                else
                {
                    // Scan completed normally.
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
                    SystemSounds.Asterisk.Play(); // Play a sound on successful completion.
                }
            }
            catch (OperationCanceledException)
            {
                // The awaited task was cancelled, but the scanner strategy returns the partial results it found.
                // We can get these results from the task object itself.
                _currentResults = scanTask.Result;
                PopulateResultsGrid(_currentResults);
                UpdateStatus($"Scan stopped by user. Found {_currentResults.Count} paths.");
            }
            catch (Exception ex)
            {
                // This block is for genuine, unexpected errors.
                MessageBox.Show("An error occurred during scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Scan failed with an error.");
            }
            finally
            {
                // Always clean up and reset the UI, regardless of the outcome.
                _scanCts?.Dispose();
                _scanCts = null;
                SwitchToScanUI(isScanningOrFiltering: false);
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
            }
        }

        // Handles the "Refine with New Scan" button click.
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

                    // Get the set of existing paths to compare against.
                    var existingPaths = new HashSet<string>(
                        dgvResults.Rows.Cast<DataGridViewRow>()
                        .Select(r => r.Tag as PointerPath)
                        .Where(p => p != null)
                        .Select(p => $"{p.BaseAddress:X8}:{p.GetOffsetsString()}")
                    );

                    // Start the refine operation.
                    try
                    {
                        _scanCts = new CancellationTokenSource();
                        var progress = new Progress<ScanProgressReport>(UpdateScanProgress);
                        var scanTask = _currentScanner.Scan(_currentManager, newScanParams, progress, _scanCts.Token);
                        await RunRefineScan(scanTask, existingPaths);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("An unexpected error occurred: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                        SwitchToScanUI(false);
                    }
                }
            }
        }

        // Manages the execution of a refine scan and finds the intersection with existing results.
        private async Task RunRefineScan(Task<List<PointerPath>> scanTask, HashSet<string> existingPaths)
        {
            _isRefining = true;
            lblResultCount.Visible = false;
            lblProgressPercentage.Text = $"0 / {_lastScanParams.MaxResults}";
            progressBar.Maximum = _lastScanParams.MaxResults;
            progressBar.Value = 0;

            SwitchToScanUI(true, "Refining results...");
            bool shouldLog = DebugSettings.LogRefineScan;

            try
            {
                if (shouldLog) logger.Log($"--- STARTING REFINE SCAN: {existingPaths.Count} initial paths to verify. ---");
                var newResults = await scanTask; // Await the new scan.
                if (shouldLog) logger.Log($"New scan completed, found {newResults.Count} potential paths. Performing intersection...");

                // Find the common paths between the old and new results.
                _currentResults = newResults.Where(p => existingPaths.Contains($"{p.BaseAddress:X8}:{p.GetOffsetsString()}")).ToList();
                PopulateResultsGrid(_currentResults);

                if (scanTask.IsCanceled)
                {
                    UpdateStatus($"Refine scan stopped by user. Found {_currentResults.Count} matching paths.");
                }
                else
                {
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
                    SystemSounds.Asterisk.Play();
                }
            }
            catch (OperationCanceledException)
            {
                var newResults = scanTask.Result; // Get partial results on cancellation.
                _currentResults = newResults.Where(p => existingPaths.Contains($"{p.BaseAddress:X8}:{p.GetOffsetsString()}")).ToList();
                PopulateResultsGrid(_currentResults);
                UpdateStatus($"Refine scan stopped by user. Found {_currentResults.Count} matching paths.");
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during refine scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Refine scan failed with an error.");
            }
            finally
            {
                _isRefining = false;
                _scanCts?.Dispose();
                _scanCts = null;
                SwitchToScanUI(false);
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
            }
        }

        #endregion

        #region UI Updates and Interaction

        // Switches the UI between the idle state (buttons visible) and the active state (progress bar visible).
        private void SwitchToScanUI(bool isScanningOrFiltering, string customMessage = null)
        {
            bool isScanning = _scanCts != null;
            bool isFiltering = _filterCts != null;

            tableLayoutPanel1.Visible = !isScanningOrFiltering;
            progressBar.Visible = isScanning; // Progress bar is only shown for scanning, not filtering.
            btnStopScan.Visible = isScanningOrFiltering;
            btnStopScan.Text = isFiltering ? "Stop Filtering" : "Stop Scan";

            lblProgressPercentage.Visible = isScanning;
            lblResultCount.Visible = !isScanningOrFiltering && dgvResults.Rows.Count > 0;

            if (isScanningOrFiltering)
            {
                if (!string.IsNullOrEmpty(customMessage))
                {
                    UpdateStatus(customMessage);
                }
                menuStrip1.Enabled = false;
                btnScan.Enabled = false;
                btnFilter.Enabled = false;
                btnRefineScan.Enabled = false;
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

        // Callback method to update the progress bar and labels from a background thread.
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
            }
            if (!string.IsNullOrEmpty(report.StatusMessage))
            {
                UpdateStatus(report.StatusMessage);
            }
            if (_lastScanParams != null)
            {
                if (progressBar.Maximum != _lastScanParams.MaxResults)
                {
                    progressBar.Maximum = _lastScanParams.MaxResults;
                }
                progressBar.Value = Math.Min(report.FoundCount, progressBar.Maximum);
                lblProgressPercentage.Text = $"{report.FoundCount} / {_lastScanParams.MaxResults}";
            }
        }

        // Thread-safe method to update the main status label.
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

        // Clears and repopulates the results DataGridView.
        private void PopulateResultsGrid(List<PointerPath> results)
        {
            dgvResults.SuspendLayout();
            dgvResults.Rows.Clear();
            dgvResults.Columns.Clear();
            lblResultCount.Text = $"Results: {results.Count}";
            bool hasResults = results.Any();
            lblResultCount.Visible = hasResults;

            if (!hasResults)
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
                var col = new DataGridViewTextBoxColumn { Name = $"colOffset{i + 1}", HeaderText = $"Offset {i + 1}", Width = 80 };
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

        // Analyzes the results to find potential data structures (arrays) and displays them in the Analysis tab.
        private int AnalyzeAndDisplayStructures(List<PointerPath> results)
        {
            treeViewAnalysis.Nodes.Clear();
            treeViewAnalysis.BeginUpdate();
            // Group paths by their offset signature. A group with many members might be an array of objects.
            var structureGroups = from p in results group p by p.GetOffsetsString() into g where g.Count() > 1 orderby g.Count() descending select g;
            int structureCount = structureGroups.Count();
            foreach (var group in structureGroups)
            {
                var members = group.OrderBy(p => p.BaseAddress).ToList();
                if (members.Count < 2) continue;
                // Calculate the distance (stride) between consecutive members to find a common delta.
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

        #endregion

        #region Filtering Logic

        // Handles the "Filter Dynamic Paths" button click, starting the continuous filtering process.
        private async void btnFilter_Click(object sender, EventArgs e)
        {
            if (_filterCts != null) return;
            _filterCts = new CancellationTokenSource();
            _validFilteredPaths = new ConcurrentBag<PointerPath>(_currentResults);

            SwitchToScanUI(isScanningOrFiltering: true, customMessage: "Starting filter...");
            _filterRefreshTimer.Start();

            try
            {
                // Run the filtering logic on a background thread.
                await Task.Run(() => FilterPathsContinuously(_filterCts.Token), _filterCts.Token);
            }
            catch (TaskCanceledException)
            {
                // Expected when the user clicks "Stop Filtering".
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during filtering: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            finally
            {
                // Always clean up and reset the UI.
                _filterRefreshTimer.Stop();
                _currentResults = _validFilteredPaths.ToList();
                PopulateResultsGrid(_currentResults); // Perform one final update.
                UpdateStatus($"Filtering stopped. {_currentResults.Count} paths remain.");

                _filterCts?.Dispose();
                _filterCts = null;
                SwitchToScanUI(isScanningOrFiltering: false);
            }
        }

        // The background task that continuously validates pointer paths.
        // It operates on a separate thread-safe list to avoid interfering with the UI.
        private void FilterPathsContinuously(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                var pathsToCheck = _validFilteredPaths.ToList();
                if (pathsToCheck.Count == 0)
                {
                    break; // No more paths left to filter.
                }

                var stillValidPaths = new ConcurrentBag<PointerPath>();
                // Check all current paths in parallel for maximum speed.
                Parallel.ForEach(pathsToCheck, path =>
                {
                    if (token.IsCancellationRequested) return;
                    uint? calculatedAddress = _currentManager.RecalculateFinalAddress(path, path.FinalAddress);
                    if (calculatedAddress.HasValue && calculatedAddress.Value == path.FinalAddress)
                    {
                        stillValidPaths.Add(path);
                    }
                });

                _validFilteredPaths = stillValidPaths;

                // A small delay to prevent this background thread from hogging 100% of a CPU core.
                Thread.Sleep(250);
            }
        }

        #endregion

        #region General UI Event Handlers

        // Handles the "Stop" button click for both scanning and filtering.
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

        // Handles the "Find" button to search for a base address in the results.
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
                if (row.Cells[0].Value != null && row.Cells[0].Value.ToString().Equals(searchText, StringComparison.OrdinalIgnoreCase))
                {
                    dgvResults.ClearSelection();
                    row.Selected = true;
                    dgvResults.FirstDisplayedScrollingRowIndex = row.Index;
                    UpdateStatus($"Found and selected base address '{searchText}'.");
                    return;
                }
            }
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
                e.SuppressKeyPress = true; // Prevents the 'ding' sound.
            }
        }

        // Handles keyboard shortcuts within the DataGridView (Copy, Delete).
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

        // Configures the context menu items based on the current selection before it opens.
        private void contextMenuResults_Opening(object sender, CancelEventArgs e)
        {
            bool hasSelection = dgvResults.SelectedRows.Count > 0;
            bool singleSelection = dgvResults.SelectedRows.Count == 1;
            copyBaseAddressToolStripMenuItem.Enabled = hasSelection;
            deleteSelectedToolStripMenuItem.Enabled = hasSelection;
            copyAsRetroAchievementsFormatToolStripMenuItem.Enabled = singleSelection;
        }

        // Copies the selected base address(es) to the clipboard.
        private void copyBaseAddressToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0 || _currentManager == null) return;
            var addresses = dgvResults.SelectedRows.Cast<DataGridViewRow>()
                .Select(r => r.Tag as PointerPath).Where(p => p != null)
                .Select(p => $"0x{_currentManager.FormatDisplayAddress(p.BaseAddress)}");
            if (addresses.Any())
            {
                Clipboard.SetText(string.Join(Environment.NewLine, addresses));
                UpdateStatus($"Copied {addresses.Count()} base address(es) to clipboard.");
            }
        }

        // Copies the selected path in the format required by RetroAchievements.
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

        // Deletes the selected rows from the results. This action is immediate and responsive.
        private void deleteSelectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0) return;

            var pathsToRemove = new HashSet<PointerPath>(dgvResults.SelectedRows.Cast<DataGridViewRow>()
                .Select(r => r.Tag as PointerPath).Where(p => p != null));

            // Remove from the main backing list first.
            int originalCount = _currentResults.Count;
            _currentResults.RemoveAll(p => pathsToRemove.Contains(p));

            // If the filter is currently running, we must also update its source list.
            if (_filterCts != null && _validFilteredPaths != null)
            {
                _validFilteredPaths = new ConcurrentBag<PointerPath>(_currentResults);
            }

            // Now, refresh the UI with the modified list.
            PopulateResultsGrid(_currentResults);
            UpdateStatus($"Deleted {originalCount - _currentResults.Count} row(s).");
        }

        // Exits the application.
        private void menuExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        // Shows the debug console window.
        private void debugConsoleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DebugLogForm.Instance.Show();
            DebugLogForm.Instance.BringToFront();
        }

        // Shows the debug options window.
        private void debugOptionsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new DebugOptionsForm().ShowDialog(this);
        }

        #endregion
    }
}