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
using System.Text;
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
        private readonly System.Windows.Forms.Timer _scanTimer;           // Updates the elapsed time label during scans.
        private readonly Stopwatch _scanStopwatch = new Stopwatch();     // Measures the duration of scans.

        // --- Data & Threading ---
        private List<PointerPath> _currentResults = new List<PointerPath>();
        private ConcurrentBag<PointerPath> _validFilteredPaths; // A thread-safe collection for the background filter task.
        private CancellationTokenSource _scanCts;
        private CancellationTokenSource _filterCts;
        private ScanParameters _lastScanParams;
        private bool _isRefining = false;
        private readonly DebugLogForm logger = DebugLogForm.Instance;

        // --- UI State for Sorting ---
        private DataGridViewColumn _sortedColumn;
        private SortOrder _sortOrder = SortOrder.None;

        public MainForm()
        {
            InitializeComponent();
            dgvResults.DoubleBuffered(true); // Enables double buffering on the grid to reduce flicker.
            SetUIStateDetached();

            // Attach event handler for manual sorting in Virtual Mode.
            dgvResults.ColumnHeaderMouseClick += dgvResults_ColumnHeaderMouseClick;

            // Configure the timer to monitor the attached emulator process.
            _processMonitorTimer = new System.Windows.Forms.Timer();
            _processMonitorTimer.Interval = 2000; // Check every 2 seconds.
            _processMonitorTimer.Tick += ProcessMonitorTimer_Tick;
            _processMonitorTimer.Start();

            // Configure the timer for refreshing the UI during filtering.
            _filterRefreshTimer = new System.Windows.Forms.Timer();
            _filterRefreshTimer.Interval = 1000; // Refresh the grid every second.
            _filterRefreshTimer.Tick += FilterRefreshTimer_Tick;

            // Configure the timer for updating the elapsed time display.
            _scanTimer = new System.Windows.Forms.Timer();
            _scanTimer.Interval = 100; // Update 10 times a second.
            _scanTimer.Tick += ScanTimer_Tick;
        }

        #region State Management & Timers

        // Periodically called by the scan timer to update the elapsed time label.
        private void ScanTimer_Tick(object sender, EventArgs e)
        {
            if (_scanStopwatch.IsRunning)
            {
                TimeSpan ts = _scanStopwatch.Elapsed;
                lblElapsedTime.Text = $"Time: {ts.Minutes:D2}:{ts.Seconds:D2}.{ts.Milliseconds / 100}";
            }
        }

        // Periodically called by the filter timer to update the DataGridView with the latest valid paths.
        private void FilterRefreshTimer_Tick(object sender, EventArgs e)
        {
            if (_filterCts != null && !_filterCts.IsCancellationRequested)
            {
                // In Virtual Mode, we just need to update the status and tell the grid its row count has changed.
                UpdateStatus($"Filtering... {_validFilteredPaths.Count} paths remain.");
                dgvResults.RowCount = _validFilteredPaths.Count;
                // Invalidate forces the grid to repaint, requesting cell values for the now-visible rows.
                dgvResults.Invalidate();
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
            if (_currentManager != null && _currentManager.IsAttached)
            {
                DetachAndReset();
                return;
            }

            if (DebugSettings.LogLiveScan) logger.Log("Initiating Smart Attach...");
            var foundProcessMap = new Dictionary<EmulatorProfile, List<Process>>();

            foreach (var profile in EmulatorProfileRegistry.Profiles)
            {
                foreach (var processName in profile.ProcessNames)
                {
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

            if (totalInstancesFound == 1)
            {
                var entry = foundProcessMap.First();
                selectedProfile = entry.Key;
                selectedProcess = entry.Value.First();
            }
            else
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

            _activeProfile = selectedProfile;
            _currentManager = _activeProfile.ManagerFactory();
            _currentScanner = _activeProfile.ScannerFactory();
            var defaultSettings = _currentManager.GetDefaultSettings();
            _currentSettings = SettingsManager.Load(_activeProfile.Target, defaultSettings);

            if (_currentManager.Attach(selectedProcess))
            {
                this.Text = $"Pointer Finder 2.0 - [{_activeProfile.Name} Mode]";
                lblStatus.Text = $"Status: Attached to {_activeProfile.Name} (PID: {selectedProcess.Id})";
                lblBaseAddress.Text = $"{_activeProfile.Name} Base (PC): {_currentManager.MemoryBasePC:X}";
                menuAttach.Text = $"Detach from {_activeProfile.Name}";
                btnScan.Enabled = true;
                bool hasResults = _currentResults.Any();
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

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    _lastScanParams = optionsForm.GetScanParameters();
                    if (_lastScanParams == null) return;
                    _currentSettings = optionsForm.GetCurrentSettings();
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);

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
            PopulateResultsGrid(new List<PointerPath>());
            treeViewAnalysis.Nodes.Clear();
            lblProgressPercentage.Text = $"0 / {_lastScanParams.MaxResults}";
            progressBar.Maximum = _lastScanParams.MaxResults;
            progressBar.Value = 0;

            SwitchToScanUI(isScanningOrFiltering: true);
            _scanStopwatch.Restart();
            _scanTimer.Start();

            try
            {
                var results = await scanTask;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                PopulateResultsGrid(results);

                if (scanTask.IsCanceled)
                {
                    UpdateStatus($"Scan stopped by user. Found {_currentResults.Count} paths {FormatDuration(elapsed)}.");
                    SoundManager.PlayNotify();
                }
                else
                {
                    // For a new scan, always play the general notification sound.
                    SoundManager.PlayNotify();

                    if (!_currentResults.Any())
                    {
                        UpdateStatus($"Scan complete. No paths found {FormatDuration(elapsed)}.");
                    }
                    else
                    {
                        int structureCount = 0;
                        if ((_lastScanParams?.AnalyzeStructures ?? true))
                        {
                            structureCount = AnalyzeAndDisplayStructures(_currentResults);
                            UpdateStatus($"Scan complete. Found {_currentResults.Count} paths and {structureCount} potential structures {FormatDuration(elapsed)}.");
                        }
                        else
                        {
                            UpdateStatus($"Scan complete. Found {_currentResults.Count} paths {FormatDuration(elapsed)}.");
                        }
                    }
                }
            }
            catch (OperationCanceledException)
            {
                var results = scanTask.Result;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                PopulateResultsGrid(results);
                UpdateStatus($"Scan stopped by user. Found {_currentResults.Count} paths {FormatDuration(elapsed)}.");
                SoundManager.PlayNotify();
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Scan failed with an error.");
            }
            finally
            {
                _scanStopwatch.Stop();
                _scanTimer.Stop();
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
            if (!_currentManager.IsAttached || !_currentResults.Any())
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

                    var existingPaths = new HashSet<string>(
                        _currentResults.Select(p => $"{p.BaseAddress:X8}:{p.GetOffsetsString()}")
                    );

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
            _scanStopwatch.Restart();
            _scanTimer.Start();
            bool shouldLog = DebugSettings.LogRefineScan;

            try
            {
                if (shouldLog) logger.Log($"--- STARTING REFINE SCAN: {existingPaths.Count} initial paths to verify. ---");
                var newResults = await scanTask;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                if (shouldLog) logger.Log($"New scan completed, found {newResults.Count} potential paths. Performing intersection...");

                var finalResults = newResults.Where(p => existingPaths.Contains($"{p.BaseAddress:X8}:{p.GetOffsetsString()}")).ToList();
                PopulateResultsGrid(finalResults);

                if (scanTask.IsCanceled)
                {
                    UpdateStatus($"Refine scan stopped by user. Found {_currentResults.Count} matching paths {FormatDuration(elapsed)}.");
                    SoundManager.PlayNotify();
                }
                else
                {
                    // For a refine scan, play success or fail based on results.
                    if (_currentResults.Any())
                    {
                        UpdateStatus($"Refine scan complete. Found {_currentResults.Count} matching paths {FormatDuration(elapsed)}.");
                        if (_lastScanParams?.AnalyzeStructures ?? true)
                        {
                            AnalyzeAndDisplayStructures(_currentResults);
                        }
                        SoundManager.PlaySuccess();
                    }
                    else
                    {
                        UpdateStatus($"Refine scan complete. No matching paths found {FormatDuration(elapsed)}.");
                        SoundManager.PlayFail();
                    }
                }
            }
            catch (OperationCanceledException)
            {
                var newResults = scanTask.Result;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                var finalResults = newResults.Where(p => existingPaths.Contains($"{p.BaseAddress:X8}:{p.GetOffsetsString()}")).ToList();
                PopulateResultsGrid(finalResults);
                UpdateStatus($"Refine scan stopped by user. Found {_currentResults.Count} matching paths {FormatDuration(elapsed)}.");
                SoundManager.PlayNotify();
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during refine scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Refine scan failed with an error.");
            }
            finally
            {
                _isRefining = false;
                _scanStopwatch.Stop();
                _scanTimer.Stop();
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

        // Event handler for Virtual Mode. It provides cell data to the DataGridView on demand.
        private void dgvResults_CellValueNeeded(object sender, DataGridViewCellValueEventArgs e)
        {
            if (_currentManager == null || _currentResults == null || e.RowIndex < 0 || e.RowIndex >= _currentResults.Count) return;

            PointerPath path = _currentResults[e.RowIndex];
            string colName = dgvResults.Columns[e.ColumnIndex].Name;

            switch (colName)
            {
                case "colBase":
                    e.Value = _currentManager.FormatDisplayAddress(path.BaseAddress);
                    break;

                case "colFinal":
                    e.Value = _currentManager.FormatDisplayAddress(path.FinalAddress);
                    break;

                default: // Handle Offset columns dynamically
                    int offsetIndex = e.ColumnIndex - 1;
                    if (offsetIndex >= 0 && offsetIndex < path.Offsets.Count)
                    {
                        int offset = path.Offsets[offsetIndex];
                        e.Value = (offset < 0) ? $"-{Math.Abs(offset):X}" : $"+{offset:X}";
                    }
                    else
                    {
                        e.Value = string.Empty;
                    }
                    break;
            }
        }

        // Handles sorting when a column header is clicked in Virtual Mode.
        private void dgvResults_ColumnHeaderMouseClick(object sender, DataGridViewCellMouseEventArgs e)
        {
            var column = dgvResults.Columns[e.ColumnIndex];
            if (column == null || !_currentResults.Any()) return;

            // Determine the new sort order
            if (_sortedColumn == column)
            {
                _sortOrder = (_sortOrder == SortOrder.Ascending) ? SortOrder.Descending : SortOrder.Ascending;
            }
            else
            {
                _sortOrder = SortOrder.Ascending;
                if (_sortedColumn != null)
                {
                    _sortedColumn.HeaderCell.SortGlyphDirection = SortOrder.None;
                }
            }

            _sortedColumn = column;

            // Sort the backing data list based on the selected column and order
            _currentResults.Sort((p1, p2) =>
            {
                int compareResult = 0;
                if (column.Name == "colBase")
                {
                    compareResult = p1.BaseAddress.CompareTo(p2.BaseAddress);
                }
                else if (column.Name == "colFinal")
                {
                    compareResult = p1.FinalAddress.CompareTo(p2.FinalAddress);
                }
                else if (column.Name.StartsWith("colOffset"))
                {
                    // This logic correctly handles numerical sorting for hex offsets
                    int offsetIndex = int.Parse(column.Name.Replace("colOffset", "")) - 1;

                    // Treat paths with fewer offsets as "smaller" for sorting purposes
                    int offset1 = (offsetIndex < p1.Offsets.Count) ? p1.Offsets[offsetIndex] : int.MinValue;
                    int offset2 = (offsetIndex < p2.Offsets.Count) ? p2.Offsets[offsetIndex] : int.MinValue;
                    compareResult = offset1.CompareTo(offset2);
                }

                // Reverse the result if sorting in descending order
                return (_sortOrder == SortOrder.Descending) ? -compareResult : compareResult;
            });

            // Update the visual glyph on the column header
            _sortedColumn.HeaderCell.SortGlyphDirection = _sortOrder;

            // Invalidate the grid to force it to repaint with the newly sorted data
            dgvResults.Invalidate();
        }

        // Switches the UI between the idle state and the active state.
        private void SwitchToScanUI(bool isScanningOrFiltering, string customMessage = null)
        {
            bool isScanning = _scanCts != null;
            bool isFiltering = _filterCts != null;

            tableLayoutPanel1.Visible = !isScanningOrFiltering;
            progressBar.Visible = isScanning;
            btnStopScan.Visible = isScanningOrFiltering;
            btnStopScan.Text = isFiltering ? "Stop Filtering" : "Stop Scan";

            lblProgressPercentage.Visible = isScanning;
            lblResultCount.Visible = !isScanningOrFiltering && _currentResults.Any();
            lblElapsedTime.Visible = isScanningOrFiltering;

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
                    bool hasResults = _currentResults.Any();
                    btnFilter.Enabled = hasResults;
                    btnRefineScan.Enabled = hasResults;
                }
                else
                {
                    SetUIStateDetached();
                }
            }
        }

        // Updates the progress bar and labels from a background thread.
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

        // Updates the main status label.
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

        // Populates the grid using Virtual Mode.
        private void PopulateResultsGrid(List<PointerPath> results)
        {
            // Reset sorting state when new data is loaded
            if (_sortedColumn != null)
            {
                _sortedColumn.HeaderCell.SortGlyphDirection = SortOrder.None;
                _sortedColumn = null;
                _sortOrder = SortOrder.None;
            }

            _currentResults = results;

            dgvResults.SuspendLayout();
            dgvResults.CellValueNeeded -= dgvResults_CellValueNeeded;

            dgvResults.Columns.Clear();

            lblResultCount.Text = $"Results: {results.Count}";
            bool hasResults = results.Any();
            lblResultCount.Visible = hasResults;

            if (!hasResults)
            {
                dgvResults.VirtualMode = false;
                dgvResults.RowCount = 0;
                btnFilter.Enabled = false;
                btnRefineScan.Enabled = false;
            }
            else
            {
                btnFilter.Enabled = true;
                btnRefineScan.Enabled = true;

                int maxOffsets = results.Max(p => p.Offsets.Count);
                if (maxOffsets == 0) maxOffsets = 1;

                dgvResults.Columns.Add("colBase", "Base Address");
                dgvResults.Columns["colBase"].Width = 120;
                for (int i = 0; i < maxOffsets; i++)
                {
                    var col = new DataGridViewTextBoxColumn { Name = $"colOffset{i + 1}", HeaderText = $"Offset {i + 1}", Width = 80 };
                    dgvResults.Columns.Add(col);
                }
                dgvResults.Columns.Add("colFinal", "Final Address");
                dgvResults.Columns["colFinal"].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;

                dgvResults.VirtualMode = true;
                dgvResults.RowCount = results.Count;
                dgvResults.CellValueNeeded += dgvResults_CellValueNeeded;
            }

            dgvResults.ResumeLayout();
            dgvResults.Invalidate();
        }

        // Analyzes results to find data structures.
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

        // Formats a TimeSpan into a user-friendly string.
        private string FormatDuration(TimeSpan duration)
        {
            var sb = new StringBuilder("in ");
            if (duration.TotalMinutes >= 1)
            {
                sb.Append($"{(int)duration.TotalMinutes} minute{((int)duration.TotalMinutes != 1 ? "s" : "")}");
                if (duration.Seconds > 0)
                {
                    sb.Append($" and {duration.Seconds} second{(duration.Seconds != 1 ? "s" : "")}");
                }
            }
            else if (duration.TotalSeconds >= 1)
            {
                sb.Append($"{duration.TotalSeconds:F1} seconds");
            }
            else
            {
                sb.Append($"{duration.TotalMilliseconds:F0} milliseconds");
            }
            return sb.ToString();
        }

        #endregion

        #region Filtering Logic

        // Handles the "Filter Dynamic Paths" button click.
        private async void btnFilter_Click(object sender, EventArgs e)
        {
            if (_filterCts != null) return;
            _filterCts = new CancellationTokenSource();
            _validFilteredPaths = new ConcurrentBag<PointerPath>(_currentResults);

            SwitchToScanUI(isScanningOrFiltering: true, customMessage: "Starting filter...");
            _filterRefreshTimer.Start();
            _scanStopwatch.Restart();
            _scanTimer.Start();

            try
            {
                await Task.Run(() => FilterPathsContinuously(_filterCts.Token), _filterCts.Token);
            }
            catch (TaskCanceledException)
            {
                // This is expected when the user clicks "Stop Filtering".
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during filtering: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            finally
            {
                var elapsed = _scanStopwatch.Elapsed;
                _scanStopwatch.Stop();
                _scanTimer.Stop();
                _filterRefreshTimer.Stop();

                PopulateResultsGrid(_validFilteredPaths.ToList());
                UpdateStatus($"Filtering stopped {FormatDuration(elapsed)}. {_currentResults.Count} paths remain.");

                _filterCts?.Dispose();
                _filterCts = null;
                SwitchToScanUI(isScanningOrFiltering: false);
            }
        }

        // The background task that continuously validates pointer paths.
        private void FilterPathsContinuously(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                var pathsToCheck = _validFilteredPaths.ToList();
                if (pathsToCheck.Count == 0) break;

                var stillValidPaths = new ConcurrentBag<PointerPath>();
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
                _currentResults = stillValidPaths.ToList();

                Thread.Sleep(250);
            }
        }

        #endregion

        #region General UI Event Handlers

        // Handles the "Stop" button click.
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

        // Handles the "Find" button to search for a base address.
        private void btnSearch_Click(object sender, EventArgs e)
        {
            string searchText = txtSearchBaseAddress.Text.Trim();
            if (string.IsNullOrEmpty(searchText)) return;

            for (int i = 0; i < _currentResults.Count; i++)
            {
                string baseAddr = _currentManager.FormatDisplayAddress(_currentResults[i].BaseAddress);
                if (baseAddr.Equals(searchText, StringComparison.OrdinalIgnoreCase))
                {
                    dgvResults.ClearSelection();
                    // In Virtual Mode, setting the CurrentCell is the most reliable way to navigate.
                    dgvResults.CurrentCell = dgvResults.Rows[i].Cells[0];
                    dgvResults.Rows[i].Selected = true;
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
                e.SuppressKeyPress = true;
            }
        }

        // Handles keyboard shortcuts within the DataGridView.
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

        // Configures the context menu before it opens.
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
                .Select(r => _currentResults[r.Index])
                .Select(p => _currentManager.FormatDisplayAddress(p.BaseAddress));
            if (addresses.Any())
            {
                Clipboard.SetText(string.Join(Environment.NewLine, addresses));
                UpdateStatus($"Copied {addresses.Count()} base address(es) to clipboard.");
            }
        }

        // Copies the selected path in RetroAchievements format.
        private void copyAsRetroAchievementsFormatToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count != 1)
            {
                if (dgvResults.SelectedRows.Count > 1)
                    UpdateStatus("Cannot copy multiple rows in RetroAchievements format.");
                return;
            }
            PointerPath path = _currentResults[dgvResults.SelectedRows[0].Index];
            if (path != null && _currentManager != null)
            {
                Clipboard.SetText(path.ToRetroAchievementsString(_currentManager));
                UpdateStatus("Copied path in RetroAchievements format.");
            }
        }

        // Deletes the selected rows from the results.
        private void deleteSelectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0) return;

            int originalCount = _currentResults.Count;

            // Get a set of the indices to be removed for faster lookup.
            var indicesToRemove = new HashSet<int>(dgvResults.SelectedRows.Cast<DataGridViewRow>().Select(r => r.Index));

            // Create a new list containing only the items we want to keep. This is much faster than RemoveAt in a loop.
            _currentResults = _currentResults.Where((path, index) => !indicesToRemove.Contains(index)).ToList();

            if (_filterCts != null && _validFilteredPaths != null)
            {
                _validFilteredPaths = new ConcurrentBag<PointerPath>(_currentResults);
            }

            // Unselect all rows to prevent graphical glitches.
            dgvResults.ClearSelection();

            // Update the grid's row count and force a full refresh.
            dgvResults.RowCount = _currentResults.Count;
            dgvResults.Invalidate();

            lblResultCount.Text = $"Results: {_currentResults.Count}";
            UpdateStatus($"Deleted {originalCount - _currentResults.Count} row(s).");
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
            new SettingsForm().ShowDialog(this);
        }

        #endregion
    }
}