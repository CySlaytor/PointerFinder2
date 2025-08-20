using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using PointerFinder2.UI;
using PointerFinder2.UI.Controls;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Media;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2
{
    // The main window of the Pointer Finder application.
    // It handles UI events, orchestrates scanning and filtering operations,
    // and manages the overall application state.
    public partial class MainForm : Form
    {
        // Win32 API function to force a process to release memory back to the OS.
        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetProcessWorkingSetSize(IntPtr hProcess, UIntPtr dwMinimumWorkingSetSize, UIntPtr dwMaximumWorkingSetSize);

        #region Fields
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

        // --- Restart State ---
        private int _pidToAutoAttach = -1;
        private string _targetToAutoAttach = null;

        // --- UI State for Sorting, Searching, and Undo/Redo ---
        private DataGridViewColumn _sortedColumn;
        private SortOrder _sortOrder = SortOrder.None;
        private string _currentSearchTerm = string.Empty;
        private readonly Stack<List<PointerPath>> _undoStack = new Stack<List<PointerPath>>();
        private readonly Stack<List<PointerPath>> _redoStack = new Stack<List<PointerPath>>();
        #endregion

        public MainForm(string[] args)
        {
            InitializeComponent();
            ParseCommandLineArgs(args);

            // Wire up the Load event to our auto-attach method. This is crucial for the smart restart to work.
            this.Load += new System.EventHandler(this.MainForm_Load);

            dgvResults.DoubleBuffered(true); // Enable double buffering to reduce flicker.
            SetUIStateDetached();
            dgvResults.ColumnHeaderMouseClick += dgvResults_ColumnHeaderMouseClick;
            _processMonitorTimer = new System.Windows.Forms.Timer { Interval = 2000 };
            _processMonitorTimer.Tick += ProcessMonitorTimer_Tick;
            _processMonitorTimer.Start();
            _filterRefreshTimer = new System.Windows.Forms.Timer { Interval = 1000 };
            _filterRefreshTimer.Tick += FilterRefreshTimer_Tick;
            _scanTimer = new System.Windows.Forms.Timer { Interval = 100 };
            _scanTimer.Tick += ScanTimer_Tick;
        }

        // After the form loads, check if we need to auto-attach from a restart.
        private void MainForm_Load(object sender, EventArgs e)
        {
            if (_pidToAutoAttach != -1 && !string.IsNullOrEmpty(_targetToAutoAttach))
            {
                AutoAttachOnRestart(_pidToAutoAttach, _targetToAutoAttach);
            }
        }

        // Checks for command-line arguments that signal a smart restart.
        private void ParseCommandLineArgs(string[] args)
        {
            try
            {
                bool isRestart = false;
                foreach (var arg in args)
                {
                    if (arg.Equals("/restart", StringComparison.OrdinalIgnoreCase))
                    {
                        isRestart = true;
                    }
                    else if (arg.StartsWith("/pid:", StringComparison.OrdinalIgnoreCase))
                    {
                        _pidToAutoAttach = int.Parse(arg.Substring(5));
                    }
                    else if (arg.StartsWith("/target:", StringComparison.OrdinalIgnoreCase))
                    {
                        _targetToAutoAttach = arg.Substring(8);
                    }
                }

                // If this is a restart, restore the previous window position.
                if (isRestart && File.Exists("restart.tmp"))
                {
                    var settings = File.ReadAllLines("restart.tmp");
                    this.StartPosition = FormStartPosition.Manual;
                    this.Left = int.Parse(settings[0]);
                    this.Top = int.Parse(settings[1]);
                    this.Width = int.Parse(settings[2]);
                    this.Height = int.Parse(settings[3]);
                    File.Delete("restart.tmp");
                }
            }
            catch (Exception ex)
            {
                logger.Log($"Error parsing command line args: {ex.Message}");
                _pidToAutoAttach = -1;
                _targetToAutoAttach = null;
            }
        }

        // The core logic for performing a smart, state-preserving self-restart.
        public void RestartApplication()
        {
            try
            {
                // Build the arguments to pass to the new process.
                var args = new List<string> { "/restart" };
                if (_currentManager != null && _currentManager.IsAttached)
                {
                    args.Add($"/pid:{_currentManager.EmulatorProcess.Id}");
                    args.Add($"/target:{_activeProfile.Target}");
                }

                // Save current window position to a temporary file.
                string[] settings = {
                    this.Left.ToString(),
                    this.Top.ToString(),
                    this.Width.ToString(),
                    this.Height.ToString()
                };
                File.WriteAllLines("restart.tmp", settings);

                // Start a new instance of this application and close the current one.
                Process.Start(Application.ExecutablePath, string.Join(" ", args));
                Application.Exit();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to restart the application: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        // A dedicated method to aggressively release memory back to the OS.
        // This is necessary because the .NET Garbage Collector is lazy and won't
        // immediately return memory, causing high RAM usage to persist.
        public void PurgeMemory()
        {
            try
            {
                if (DebugSettings.LogLiveScan) logger.Log("--- Purging Application Memory ---");

                // Force a full garbage collection. This tells the .NET runtime to find and
                // mark all unreferenced objects (like the huge pointer map from a previous scan) for cleanup.
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect(); // Call it a second time to catch objects finalized in the first pass.

                // Force the process to release the unused memory pages back to the OS.
                // This is the crucial step that makes the RAM usage drop in Task Manager.
                // Using -1 for both min and max is the documented way to tell the system to trim the working set.
                using (Process currentProcess = Process.GetCurrentProcess())
                {
                    SetProcessWorkingSetSize(currentProcess.Handle, (UIntPtr)0xFFFFFFFF, (UIntPtr)0xFFFFFFFF);
                }

                if (DebugSettings.LogLiveScan) logger.Log("--- Memory Purge Complete ---");
            }
            catch (Exception ex)
            {
                logger.Log($"[ERROR] Failed to purge memory: {ex.Message}");
            }
        }

        // A lightweight cleanup routine used between normal scans, which does not do a full memory purge.
        private void ClearSessionDataOnly()
        {
            _filterCts?.Cancel();
            if (tableLayoutPanel1.Visible == false)
            {
                SwitchToScanUI(isScanningOrFiltering: false);
            }

            PushUndoState(); // Save the state before clearing.

            _currentResults = new List<PointerPath>();
            _validFilteredPaths = null;
            PopulateResultsGrid(new List<PointerPath>(), true);
            GC.Collect();
        }

        // Centralized logic for attaching to a given emulator process.
        private bool PerformAttachment(EmulatorProfile profile, Process process)
        {
            if (profile == null || process == null) return false;

            _activeProfile = profile;
            _currentManager = _activeProfile.ManagerFactory();
            _currentScanner = _activeProfile.ScannerFactory();
            var defaultSettings = _currentManager.GetDefaultSettings();
            _currentSettings = SettingsManager.Load(_activeProfile.Target, defaultSettings);

            if (_currentManager.Attach(process))
            {
                string windowTitle = process.MainWindowTitle;
                if (!string.IsNullOrWhiteSpace(windowTitle))
                {
                    this.Text = $"Pointer Finder 2.0 - [{windowTitle}]";
                }
                else
                {
                    // Fallback if the title is empty, keep the old behavior
                    this.Text = $"Pointer Finder 2.0 - [{_activeProfile.Name} Mode]";
                }
                lblStatus.Text = $"Status: Attached to {_activeProfile.Name} (PID: {process.Id})";
                lblBaseAddress.Text = $"{_activeProfile.Name} Base (PC): {_currentManager.MemoryBasePC:X}";
                menuAttach.Text = $"Detach from {_activeProfile.Name}";

                // Centralize UI state updates
                SwitchToScanUI(false);
                return true;
            }
            else
            {
                MessageBox.Show($"Could not find required memory exports for {profile.Name}.", "Attach Failed", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                DetachAndReset();
                return false;
            }
        }

        // Automatically attaches to an emulator based on PID and target info from command-line args.
        private void AutoAttachOnRestart(int pid, string targetName)
        {
            try
            {
                var process = Process.GetProcessById(pid);
                var profile = EmulatorProfileRegistry.Profiles.FirstOrDefault(p => p.Target.ToString().Equals(targetName, StringComparison.OrdinalIgnoreCase));

                if (process != null && profile != null)
                {
                    PerformAttachment(profile, process);
                }
                else
                {
                    logger.Log($"Auto-attach failed: Could not find process with PID {pid} or profile for '{targetName}'.");
                }
            }
            catch (Exception ex)
            {
                logger.Log($"Auto-attach failed: {ex.Message}");
            }
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
                UpdateStatus($"Filtering... {_validFilteredPaths.Count:N0} paths remain.");
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
            _currentManager?.Detach();
            _activeProfile = null;
            _currentManager = null;
            _currentScanner = null;
            _currentSettings = null;
            _undoStack.Clear();
            _redoStack.Clear();
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

        #region Menu Item Handlers (File, Edit, View)

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

            PerformAttachment(selectedProfile, selectedProcess);
        }

        private void menuExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void fileToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            // Only allow saving if attached and there are results.
            saveSessionToolStripMenuItem.Enabled = _currentManager != null && _currentManager.IsAttached && _currentResults.Any();
            // Always allow loading a session.
            loadSessionToolStripMenuItem.Enabled = true;
        }

        private void saveSessionToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SaveSession();
        }

        private void loadSessionToolStripMenuItem_Click(object sender, EventArgs e)
        {
            LoadSession();
        }

        private void editToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            bool hasResults = _currentResults.Any();
            findToolStripMenuItem.Enabled = hasResults;
            undoToolStripMenuItem.Enabled = _undoStack.Any();
            redoToolStripMenuItem.Enabled = _redoStack.Any();
        }

        private void findToolStripMenuItem_Click(object sender, EventArgs e)
        {
            using (var searchForm = new AddressSearchForm(_currentSearchTerm))
            {
                if (searchForm.ShowDialog(this) == DialogResult.OK)
                {
                    _currentSearchTerm = searchForm.SearchAddress;
                    PerformSearch(_currentSearchTerm);
                }
            }
        }

        private void undoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_undoStack.Any())
            {
                // Push the current state to the redo stack before overwriting it.
                _redoStack.Push(new List<PointerPath>(_currentResults));
                // Pop the previous state from the undo stack.
                _currentResults = _undoStack.Pop();
                // Update the UI without resetting search/sort.
                PopulateResultsGrid(_currentResults, false);
            }
        }

        private void redoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_redoStack.Any())
            {
                // Push the current state to the undo stack before overwriting it.
                _undoStack.Push(new List<PointerPath>(_currentResults));
                // Pop the "future" state from the redo stack.
                _currentResults = _redoStack.Pop();
                // Update the UI without resetting search/sort.
                PopulateResultsGrid(_currentResults, false);
            }
        }

        private void debugConsoleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DebugLogForm.Instance.Show();
            DebugLogForm.Instance.BringToFront();
        }

        private void debugOptionsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new SettingsForm(this).ShowDialog(this);
        }

        #endregion

        #region Undo/Redo Management

        // Pushes the current result list onto the undo stack and clears the redo stack.
        // This should be called before any action that modifies the results.
        private void PushUndoState()
        {
            // Create a copy of the current list to save its state at this moment.
            _undoStack.Push(new List<PointerPath>(_currentResults));
            // Any new action clears the redo history.
            _redoStack.Clear();
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
                    PushUndoState();
                    ClearSessionDataOnly();
                    // Proactively purge memory before a new scan to ensure a clean slate
                    // and prevent RAM usage from compounding over multiple scans.
                    PurgeMemory();
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
            lblProgressPercentage.Text = $"0 / {_lastScanParams.MaxResults:N0}";
            progressBar.Maximum = (int)_lastScanParams.MaxResults;
            progressBar.Value = 0;

            SwitchToScanUI(isScanningOrFiltering: true);
            _scanStopwatch.Restart();
            _scanTimer.Start();

            try
            {
                var results = await scanTask;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                PopulateResultsGrid(results, true);

                if (scanTask.IsCanceled)
                {
                    UpdateStatus($"Scan stopped by user. Found {_currentResults.Count:N0} paths {FormatDuration(elapsed)}.");
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
                        UpdateStatus($"Scan complete. Found {_currentResults.Count:N0} paths {FormatDuration(elapsed)}.");
                    }
                }
            }
            catch (OperationCanceledException)
            {
                var results = scanTask.Result;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                PopulateResultsGrid(results, true);
                UpdateStatus($"Scan stopped by user. Found {_currentResults.Count:N0} paths {FormatDuration(elapsed)}.");
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
            }
        }

        // Handles the "Refine with New Scan" button click.
        // This method is now async to allow for non-blocking UI while preparing results.
        private async void btnRefineScan_Click(object sender, EventArgs e)
        {
            if (_scanCts != null || _activeProfile == null) return;
            if (!_currentManager.IsAttached || !_currentResults.Any())
            {
                MessageBox.Show("Please perform an initial scan and have results before refining.", "Initial Scan Required", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }

            // Prepare the existing results on a background thread to prevent UI hangs.
            // This is crucial when the result set is very large.
            UpdateStatus("Preparing existing results for refinement...");
            var existingPaths = await Task.Run(() => new HashSet<PointerPath>(_currentResults));
            UpdateStatus("Ready for refinement. Please configure the new scan.");

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    PushUndoState();
                    ClearSessionDataOnly();
                    // Also purge memory before a refine scan, as it performs a full new scan internally.
                    PurgeMemory();
                    var newScanParams = optionsForm.GetScanParameters();
                    if (newScanParams == null) return;
                    _lastScanParams = newScanParams;
                    _currentSettings = optionsForm.GetCurrentSettings();
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);
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
        // The signature is updated to accept a HashSet of PointerPath objects directly.
        private async Task RunRefineScan(Task<List<PointerPath>> scanTask, HashSet<PointerPath> existingPaths)
        {
            _isRefining = true;
            lblResultCount.Visible = false;
            lblProgressPercentage.Text = $"0 / {_lastScanParams.MaxResults:N0}";
            progressBar.Maximum = (int)_lastScanParams.MaxResults;
            progressBar.Value = 0;

            SwitchToScanUI(true, "Refining results...");
            _scanStopwatch.Restart();
            _scanTimer.Start();
            bool shouldLog = DebugSettings.LogRefineScan;

            try
            {
                if (shouldLog) logger.Log($"--- STARTING REFINE SCAN: {existingPaths.Count:N0} initial paths to verify. ---");
                var newResults = await scanTask;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                if (shouldLog) logger.Log($"New scan completed, found {newResults.Count:N0} potential paths. Performing intersection...");

                // The intersection logic is now much cleaner and more efficient.
                // It uses the custom Equals/GetHashCode methods on PointerPath for a fast lookup.
                var finalResults = newResults.Where(p => existingPaths.Contains(p)).ToList();
                PopulateResultsGrid(finalResults, true);

                if (scanTask.IsCanceled)
                {
                    UpdateStatus($"Refine scan stopped by user. Found {_currentResults.Count:N0} matching paths {FormatDuration(elapsed)}.");
                    SoundManager.PlayNotify();
                }
                else
                {
                    // For a refine scan, play success or fail based on results.
                    if (_currentResults.Any())
                    {
                        UpdateStatus($"Refine scan complete. Found {_currentResults.Count:N0} matching paths {FormatDuration(elapsed)}.");
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
                // Updated intersection logic for the cancellation case as well.
                var finalResults = newResults.Where(p => existingPaths.Contains(p)).ToList();
                PopulateResultsGrid(finalResults, true);
                UpdateStatus($"Refine scan stopped by user. Found {_currentResults.Count:N0} matching paths {FormatDuration(elapsed)}.");
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
            }
        }

        #endregion

        #region Session Management (Save/Load)

        // Saves the current results, scan parameters, and attachment state to a JSON file.
        private void SaveSession()
        {
            if (!_currentResults.Any())
            {
                MessageBox.Show("There are no results to save.", "Nothing to Save", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            using (var sfd = new SaveFileDialog())
            {
                sfd.Filter = "Pointer Finder Session (*.pfs)|*.pfs";
                sfd.Title = "Save Session";
                sfd.FileName = $"session_{DateTime.Now:yyyyMMdd_HHmmss}.pfs";

                if (sfd.ShowDialog(this) == DialogResult.OK)
                {
                    try
                    {
                        var sessionData = new SessionData
                        {
                            EmulatorTargetName = _activeProfile?.Target.ToString(),
                            ProcessId = _currentManager?.EmulatorProcess?.Id ?? -1,
                            LastScanParameters = _lastScanParams,
                            Results = _currentResults,
                            SortedColumnName = _sortedColumn?.Name,
                            SortDirection = _sortOrder
                        };

                        var options = new JsonSerializerOptions { WriteIndented = true };
                        string json = JsonSerializer.Serialize(sessionData, options);
                        File.WriteAllText(sfd.FileName, json);
                        UpdateStatus($"Session saved to {Path.GetFileName(sfd.FileName)}");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show($"Failed to save session: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        logger.Log($"[ERROR] Session save failed: {ex.Message}");
                    }
                }
            }
        }

        // Loads a session from a JSON file, restoring results and attempting to re-attach to the emulator.
        private void LoadSession()
        {
            using (var ofd = new OpenFileDialog())
            {
                ofd.Filter = "Pointer Finder Session (*.pfs)|*.pfs|All files (*.*)|*.*";
                ofd.Title = "Load Session";
                if (ofd.ShowDialog(this) == DialogResult.OK)
                {
                    try
                    {
                        string json = File.ReadAllText(ofd.FileName);
                        var sessionData = JsonSerializer.Deserialize<SessionData>(json);

                        // Validate session compatibility if an emulator is already attached.
                        if (_currentManager != null && _currentManager.IsAttached && _activeProfile.Target.ToString() != sessionData.EmulatorTargetName)
                        {
                            var result = MessageBox.Show(
                                $"The session file is for '{sessionData.EmulatorTargetName}', but you are currently attached to '{_activeProfile.Name}'.\n\nDo you want to detach and continue loading the session?",
                                "Emulator Mismatch",
                                MessageBoxButtons.YesNo,
                                MessageBoxIcon.Warning);

                            if (result == DialogResult.No)
                            {
                                return; // Abort loading.
                            }
                        }

                        PushUndoState();

                        // 1. Start with a clean slate
                        if (_currentManager != null && _currentManager.IsAttached) DetachAndReset();
                        ClearSessionDataOnly();

                        // 2. Load data into memory
                        _lastScanParams = sessionData.LastScanParameters;
                        _currentResults = sessionData.Results;

                        // 3. Set the emulator context (profile/manager) without attaching yet
                        var profile = EmulatorProfileRegistry.Profiles.FirstOrDefault(p => p.Target.ToString() == sessionData.EmulatorTargetName);
                        if (profile != null)
                        {
                            _activeProfile = profile;
                            _currentManager = _activeProfile.ManagerFactory();
                            SetUIStateDetached(); // Reset UI to a known state
                            this.Text = $"Pointer Finder 2.0 - [Loaded: {Path.GetFileName(ofd.FileName)}]";
                            menuAttach.Text = $"Attach to {_activeProfile.Name}";
                        }

                        // 4. Populate the grid (requires a valid _currentManager for formatting)
                        PopulateResultsGrid(_currentResults, true);

                        // 5. Restore sorting
                        if (profile != null && !string.IsNullOrEmpty(sessionData.SortedColumnName) && dgvResults.Columns.Contains(sessionData.SortedColumnName))
                        {
                            var column = dgvResults.Columns[sessionData.SortedColumnName];
                            SortResults(column, sessionData.SortDirection);
                        }

                        // 6. Attempt to auto-attach to the original process
                        if (profile != null && sessionData.ProcessId != -1)
                        {
                            try
                            {
                                var process = Process.GetProcessById(sessionData.ProcessId);
                                if (profile.ProcessNames.Contains(process.ProcessName, StringComparer.OrdinalIgnoreCase))
                                {
                                    PerformAttachment(profile, process); // This will update UI to "Attached"
                                }
                            }
                            catch
                            {
                                // Process not found or access denied, which is an expected scenario.
                                UpdateStatus($"Session loaded. Emulator (PID: {sessionData.ProcessId}) not found. Attach manually.");
                                return; // Exit after setting status
                            }
                        }
                        UpdateStatus($"Session loaded successfully. Found {sessionData.Results.Count:N0} results.");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show($"Failed to load session: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        logger.Log($"[ERROR] Session load failed: {ex.Message}");
                    }
                }
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
                case "colBase": e.Value = _currentManager.FormatDisplayAddress(path.BaseAddress); break;
                case "colFinal": e.Value = _currentManager.FormatDisplayAddress(path.FinalAddress); break;
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

            SortOrder newOrder;
            if (_sortedColumn == column)
            {
                newOrder = (_sortOrder == SortOrder.Ascending) ? SortOrder.Descending : SortOrder.Ascending;
            }
            else
            {
                newOrder = SortOrder.Ascending;
                if (_sortedColumn != null) _sortedColumn.HeaderCell.SortGlyphDirection = SortOrder.None;
            }

            SortResults(column, newOrder);
        }

        // Sorts the internal result list and updates the UI.
        private void SortResults(DataGridViewColumn column, SortOrder order)
        {
            if (column == null || order == SortOrder.None || !_currentResults.Any()) return;

            // Sort the backing data list based on the selected column and order
            _currentResults.Sort((p1, p2) =>
            {
                int compareResult = 0;
                if (column.Name == "colBase") compareResult = p1.BaseAddress.CompareTo(p2.BaseAddress);
                else if (column.Name == "colFinal") compareResult = p1.FinalAddress.CompareTo(p2.FinalAddress);
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
                return (order == SortOrder.Descending) ? -compareResult : compareResult;
            });

            _sortedColumn = column;
            _sortOrder = order;

            // Update the visual glyph on the column header and repaint the grid
            _sortedColumn.HeaderCell.SortGlyphDirection = _sortOrder;
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
                if (!string.IsNullOrEmpty(customMessage)) UpdateStatus(customMessage);
                menuStrip1.Enabled = false;
                btnScan.Enabled = false;
                btnFilter.Enabled = false;
                btnRefineScan.Enabled = false;
            }
            else
            {
                menuStrip1.Enabled = true;
                bool isAttached = _currentManager != null && _currentManager.IsAttached;
                btnScan.Enabled = isAttached;
                bool hasResults = _currentResults.Any();
                btnFilter.Enabled = isAttached && hasResults;
                btnRefineScan.Enabled = isAttached && hasResults;
                if (!isAttached)
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
            if (!string.IsNullOrEmpty(report.StatusMessage)) UpdateStatus(report.StatusMessage);
            if (_lastScanParams != null)
            {
                if (progressBar.Maximum != _lastScanParams.MaxResults) progressBar.Maximum = (int)_lastScanParams.MaxResults;
                progressBar.Value = (int)Math.Min(report.FoundCount, progressBar.Maximum);
                lblProgressPercentage.Text = $"{report.FoundCount:N0} / {_lastScanParams.MaxResults:N0}";
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
        private void PopulateResultsGrid(List<PointerPath> results, bool isNewDataSet)
        {
            // For a brand new data set, reset sorting and searching.
            if (isNewDataSet)
            {
                if (_sortedColumn != null)
                {
                    _sortedColumn.HeaderCell.SortGlyphDirection = SortOrder.None;
                    _sortedColumn = null;
                    _sortOrder = SortOrder.None;
                }
                _currentSearchTerm = string.Empty;
            }

            _currentResults = results;
            dgvResults.SuspendLayout();
            dgvResults.CellValueNeeded -= dgvResults_CellValueNeeded;
            dgvResults.Columns.Clear();
            lblResultCount.Text = $"Results: {results.Count:N0}";
            bool hasResults = results.Any();
            lblResultCount.Visible = hasResults;
            if (!hasResults)
            {
                dgvResults.VirtualMode = false;
                dgvResults.RowCount = 0;
            }
            else
            {
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

            // After populating, update the state of the buttons.
            SwitchToScanUI(false);
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

            PushUndoState();

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
            catch (TaskCanceledException) { }
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
                PopulateResultsGrid(_validFilteredPaths.ToList(), false);
                UpdateStatus($"Filtering stopped {FormatDuration(elapsed)}. {_currentResults.Count:N0} paths remain.");
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
            if (_filterCts != null) _filterCts.Cancel();
            else _scanCts?.Cancel();
        }

        // Finds and highlights a row with the given base address.
        private void PerformSearch(string searchText)
        {
            if (string.IsNullOrEmpty(searchText))
            {
                dgvResults.ClearSelection();
                UpdateStatus("Search cleared.");
                return;
            }

            if (_currentManager == null) return;

            for (int i = 0; i < _currentResults.Count; i++)
            {
                string baseAddr = _currentManager.FormatDisplayAddress(_currentResults[i].BaseAddress);
                if (baseAddr.Equals(searchText, StringComparison.OrdinalIgnoreCase))
                {
                    dgvResults.ClearSelection();
                    dgvResults.Rows[i].Selected = true;
                    dgvResults.CurrentCell = dgvResults.Rows[i].Cells[0];
                    UpdateStatus($"Found and selected base address '{searchText}'.");
                    return;
                }
            }
            UpdateStatus($"Base address '{searchText}' not found in results.");
        }


        // Handles keyboard shortcuts within the DataGridView.
        private void dgvResults_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Control && e.KeyCode == Keys.C)
            {
                e.SuppressKeyPress = true;
                if (e.Shift) copyAsRetroAchievementsFormatToolStripMenuItem_Click(sender, e);
                else copyBaseAddressToolStripMenuItem_Click(sender, e);
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
                UpdateStatus($"Copied {addresses.Count():N0} base address(es) to clipboard.");
            }
        }

        // Copies the selected path in RetroAchievements format.
        private void copyAsRetroAchievementsFormatToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count != 1)
            {
                if (dgvResults.SelectedRows.Count > 1) UpdateStatus("Cannot copy multiple rows in RetroAchievements format.");
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

            PushUndoState();

            if (dgvResults.SelectedRows.Count == _currentResults.Count)
            {
                ClearSessionDataOnly();
                UpdateStatus("All results cleared.");
                return;
            }
            int originalCount = _currentResults.Count;
            var indicesToRemove = new HashSet<int>(dgvResults.SelectedRows.Cast<DataGridViewRow>().Select(r => r.Index));
            _currentResults = _currentResults.Where((path, index) => !indicesToRemove.Contains(index)).ToList();
            if (_filterCts != null && _validFilteredPaths != null)
            {
                _validFilteredPaths = new ConcurrentBag<PointerPath>(_currentResults);
            }
            dgvResults.ClearSelection();
            dgvResults.RowCount = _currentResults.Count;
            dgvResults.Invalidate();
            lblResultCount.Text = $"Results: {_currentResults.Count:N0}";
            UpdateStatus($"Deleted {(originalCount - _currentResults.Count):N0} row(s).");
        }
        #endregion
    }
}