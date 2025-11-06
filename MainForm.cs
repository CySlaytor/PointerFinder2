using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using PointerFinder2.Emulators.LiveScan;
using PointerFinder2.Emulators.StateBased;
using PointerFinder2.Properties;
using PointerFinder2.UI;
using PointerFinder2.UI.Controls;
using PointerFinder2.UI.StaticRangeFinders;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
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

        // Add P/Invoke to get total system physical RAM.
        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetPhysicallyInstalledSystemMemory(out long TotalMemoryInKilobytes);


        #region Fields
        // --- Application State ---
        private IEmulatorManager _currentManager;
        private IPointerScannerStrategy _currentScanner;
        private AppSettings _currentSettings;
        private EmulatorProfile _activeProfile;

        // An array to hold captured memory states, allowing them to persist within a session.
        private readonly ScanState[] _multiStateCaptures = new ScanState[4];

        // --- Timers ---
        private readonly System.Windows.Forms.Timer _processMonitorTimer; // Checks if the emulator process is still running.
        private readonly System.Windows.Forms.Timer _filterRefreshTimer;  // Periodically refreshes the UI during continuous filtering.
        private readonly System.Windows.Forms.Timer _scanTimer;           // Updates the elapsed time label during scans.
        private readonly Stopwatch _scanStopwatch = new Stopwatch();     // Measures the duration of scans.
        private readonly System.Windows.Forms.Timer _memoryMonitorTimer;  // Periodically checks system RAM usage during scans.
        // Fields re-purposed for physical RAM monitoring.
        private PerformanceCounter _performanceCounter;                  // The counter for available physical memory.
        private float _totalSystemMemoryMB;                              // Total physical RAM in Megabytes.
        private bool _stoppedByMemoryMonitor = false;                    // Flag to indicate why a scan was stopped.

        // --- Data & Threading ---
        private List<PointerPath> _currentResults = new List<PointerPath>();
        private ConcurrentBag<PointerPath> _validFilteredPaths; // A thread-safe collection for the background filter task.
        private CancellationTokenSource _scanCts;
        private CancellationTokenSource _filterCts;
        private ScanParameters _lastScanParams;
        private bool _isRefining = false;
        private readonly DebugLogForm logger = DebugLogForm.Instance;
        private int _lastFoundCount = 0;

        // --- Tool Window Instances ---
        // Added fields to track open tool windows to prevent duplicate creation from the menu.
        private CodeNoteConverterForm _codeNoteConverterInstance;
        private CodeNoteHierarchyFixerForm _codeNoteHierarchyFixerInstance;
        private Form _staticRangeFinderInstance;

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

            // Load global settings on startup to ensure consistency.
            SettingsManager.InitializeGlobalSettings();

            // Wire up the Load event to our auto-attach method. This is crucial for the smart restart to work.
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);

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

            _memoryMonitorTimer = new System.Windows.Forms.Timer { Interval = 2000 }; // Check every 2 seconds.
            _memoryMonitorTimer.Tick += MemoryMonitorTimer_Tick;
            try
            {
                // Get total physical RAM and set up the counter for "Available MBytes".
                if (GetPhysicallyInstalledSystemMemory(out long totalRamKB))
                {
                    _totalSystemMemoryMB = totalRamKB / 1024f;
                    logger.Log($"[INFO] Detected total system RAM: {_totalSystemMemoryMB:N0} MB.");
                }
                _performanceCounter = new PerformanceCounter("Memory", "Available MBytes");
            }
            catch (Exception ex)
            {
                logger.Log($"[WARNING] Could not initialize PerformanceCounter for memory monitoring. The feature will be disabled. Error: {ex.Message}");
                _performanceCounter = null;
            }
        }

        // New method to handle the memory monitor timer tick.
        private void MemoryMonitorTimer_Tick(object sender, EventArgs e)
        {
            if (_performanceCounter == null || _totalSystemMemoryMB <= 0 || _scanCts == null || _scanCts.IsCancellationRequested)
            {
                _memoryMonitorTimer.Stop();
                return;
            }

            try
            {
                float availableMB = _performanceCounter.NextValue();
                float usedMB = _totalSystemMemoryMB - availableMB;
                float usagePercentage = (usedMB / _totalSystemMemoryMB) * 100.0f;

                if (usagePercentage > 80.0f)
                {
                    logger.Log($"[PERFORMANCE] High physical RAM usage detected ({usagePercentage:F1}%). Stopping scan to prevent system instability.");
                    _stoppedByMemoryMonitor = true;
                    _scanCts?.Cancel();
                    _memoryMonitorTimer.Stop();
                }
            }
            catch (Exception ex)
            {
                logger.Log($"[ERROR] Failed to read memory performance counter. Disabling monitor. Error: {ex.Message}");
                _performanceCounter = null;
                _memoryMonitorTimer.Stop();
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Save window state, size, and location
            Settings.Default.MainWindowState = this.WindowState;
            if (this.WindowState == FormWindowState.Normal)
            {
                Settings.Default.MainWindowLocation = this.Location;
                Settings.Default.MainWindowSize = this.Size;
            }
            else
            {
                // If minimized or maximized, save the restored bounds
                Settings.Default.MainWindowLocation = this.RestoreBounds.Location;
                Settings.Default.MainWindowSize = this.RestoreBounds.Size;
            }
            Settings.Default.Save();
        }

        // After the form loads, check if we need to auto-attach from a restart.
        private void MainForm_Load(object sender, EventArgs e)
        {
            // Only apply saved settings if the position hasn't been manually set by restart logic.
            if (this.StartPosition != FormStartPosition.Manual)
            {
                if (Settings.Default.MainWindowSize.Width > 0 && Settings.Default.MainWindowSize.Height > 0)
                {
                    this.StartPosition = FormStartPosition.Manual;
                    // Ensure the form is visible on a screen before setting its location.
                    Point location = Settings.Default.MainWindowLocation;
                    Size size = Settings.Default.MainWindowSize;
                    bool isVisible = false;
                    foreach (Screen screen in Screen.AllScreens)
                    {
                        if (screen.WorkingArea.IntersectsWith(new Rectangle(location, size)))
                        {
                            isVisible = true;
                            break;
                        }
                    }
                    if (isVisible)
                    {
                        this.Location = location;
                        this.Size = size;
                        // Only set window state if it's not Minimized, to avoid starting hidden.
                        this.WindowState = (Settings.Default.MainWindowState == FormWindowState.Minimized) ? FormWindowState.Normal : Settings.Default.MainWindowState;
                    }
                }
            }

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
        public void PurgeMemory()
        {
            try
            {
                if (DebugSettings.LogLiveScan) logger.Log("--- Purging Application Memory ---");
                // Force a full garbage collection. This tells the .NET runtime to find and
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
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

        // A lightweight cleanup routine used between normal scans.
        private void ClearSessionDataOnly()
        {
            _filterCts?.Cancel();
            PushUndoState();
            _currentResults = new List<PointerPath>();
            _validFilteredPaths = null;
            dgvResults.RowCount = 0;
            dgvResults.Columns.Clear();

            // Reset sorting state variables whenever columns are cleared to prevent crashes.
            _sortedColumn = null;
            _sortOrder = SortOrder.None;

            lblResultCount.Visible = false;
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
                this.Text = !string.IsNullOrWhiteSpace(windowTitle) ? $"Pointer Finder 2.0 - [{windowTitle}]" : $"Pointer Finder 2.0 - [{_currentManager.EmulatorName} Mode]";
                lblStatus.Text = $"Status: Attached to {_currentManager.EmulatorName} (PID: {process.Id})";
                lblBaseAddress.Text = $"{_currentManager.EmulatorName.Split(' ')[0]} Base (PC): {_currentManager.MemoryBasePC:X}";
                menuAttach.Text = $"Detach from {_currentManager.EmulatorName}";
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
        private void ScanTimer_Tick(object sender, EventArgs e)
        {
            if (_scanStopwatch.IsRunning)
            {
                TimeSpan ts = _scanStopwatch.Elapsed;
                string timeText = $"Time: {ts.Minutes:D2}:{ts.Seconds:D2}.{ts.Milliseconds / 100}";

                bool isStateScan = _activeProfile?.StateBasedScannerFactory != null && _currentScanner != null && _currentScanner.GetType().IsSubclassOf(typeof(StateBasedScannerStrategyBase));

                if (isStateScan && _lastFoundCount > 0)
                {
                    lblElapsedTime.Text = $"{timeText} | Found: {_lastFoundCount:N0}";
                }
                else
                {
                    lblElapsedTime.Text = timeText;
                }
            }
        }

        // Periodically called by the filter timer to update the DataGridView with the latest valid paths.
        private void FilterRefreshTimer_Tick(object sender, EventArgs e)
        {
            if (_filterCts != null && !_filterCts.IsCancellationRequested)
            {
                UpdateStatus($"Filtering... {_validFilteredPaths.Count:N0} paths remain.");
                dgvResults.RowCount = _validFilteredPaths.Count;
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

            // Clear the captured states on detach to prevent stale data in a new session.
            for (int i = 0; i < _multiStateCaptures.Length; i++)
            {
                _multiStateCaptures[i] = null;
            }
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
            btnCaptureState.Enabled = false;
            btnCaptureState.Text = "Capture State";
            btnRefineScan.Text = "Refine with New Scan";
        }
        #endregion

        #region Menu Item Handlers
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
                        if (!foundProcessMap.ContainsKey(profile)) foundProcessMap[profile] = new List<Process>();
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

            if (totalInstancesFound == 1)
            {
                var entry = foundProcessMap.First();
                PerformAttachment(entry.Key, entry.Value.First());
            }
            else
            {
                using (var selectionForm = new EmulatorSelectionForm(foundProcessMap))
                {
                    if (selectionForm.ShowDialog() == DialogResult.OK)
                    {
                        PerformAttachment(selectionForm.SelectedProfile, selectionForm.SelectedProcess);
                    }
                }
            }
        }

        private void menuExit_Click(object sender, EventArgs e) { Application.Exit(); }
        private void fileToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            saveSessionToolStripMenuItem.Enabled = _currentManager != null && _currentManager.IsAttached && _currentResults.Any();
            loadSessionToolStripMenuItem.Enabled = true;
        }
        private void saveSessionToolStripMenuItem_Click(object sender, EventArgs e) { SaveSession(); }
        private void loadSessionToolStripMenuItem_Click(object sender, EventArgs e) { LoadSession(); }
        private void editToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            findToolStripMenuItem.Enabled = _currentResults.Any();
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
                _redoStack.Push(new List<PointerPath>(_currentResults));
                _currentResults = _undoStack.Pop();
                PopulateResultsGrid(_currentResults, false);
            }
        }
        private void redoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_redoStack.Any())
            {
                _undoStack.Push(new List<PointerPath>(_currentResults));
                _currentResults = _redoStack.Pop();
                PopulateResultsGrid(_currentResults, false);
            }
        }
        private void debugConsoleToolStripMenuItem_Click(object sender, EventArgs e) { DebugLogForm.Instance.Show(); DebugLogForm.Instance.BringToFront(); }
        private void debugOptionsToolStripMenuItem_Click(object sender, EventArgs e) { new SettingsForm(this).ShowDialog(this); }
        // Updated to disable tool menu items if their corresponding window is already open.
        private void toolsToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            // Fix: Expanded support to include PPSSPP.
            bool isSupported = _activeProfile?.Target == EmulatorTarget.PCSX2 ||
                               _activeProfile?.Target == EmulatorTarget.RALibretroNDS ||
                               _activeProfile?.Target == EmulatorTarget.Dolphin ||
                               _activeProfile?.Target == EmulatorTarget.PPSSPP;

            staticRangeFinderToolStripMenuItem.Enabled = _currentManager != null && _currentManager.IsAttached && isSupported && (_staticRangeFinderInstance == null || _staticRangeFinderInstance.IsDisposed);
            codeNoteConverterToolStripMenuItem.Enabled = (_codeNoteConverterInstance == null || _codeNoteConverterInstance.IsDisposed);
            codeNoteHierarchyFixerToolStripMenuItem.Enabled = (_codeNoteHierarchyFixerInstance == null || _codeNoteHierarchyFixerInstance.IsDisposed);
        }

        // Logic updated to manage a single instance of the finder form and handle settings reload on close.
        private void staticRangeFinderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_staticRangeFinderInstance != null && !_staticRangeFinderInstance.IsDisposed) return;

            // Added branching to open the correct finder for the active emulator.
            Form finderForm = null;
            if (_activeProfile.Target == EmulatorTarget.PCSX2)
            {
                finderForm = new Pcsx2RamScanRangeFinderForm(_currentManager);
            }
            else if (_activeProfile.Target == EmulatorTarget.RALibretroNDS)
            {
                finderForm = new NdsStaticRangeFinderForm(_currentManager);
            }
            else if (_activeProfile.Target == EmulatorTarget.Dolphin)
            {
                finderForm = new DolphinFileRangeFinderForm(_currentManager);
            }
            // Fix: Added branching for the new PPSSPP static range finder.
            else if (_activeProfile.Target == EmulatorTarget.PPSSPP)
            {
                finderForm = new PpssppRamScanRangeFinderForm(_currentManager);
            }


            if (finderForm != null)
            {
                _staticRangeFinderInstance = finderForm;
                _staticRangeFinderInstance.FormClosed += (s, args) =>
                {
                    var form = s as Form;
                    if (form != null && form.DialogResult == DialogResult.OK)
                    {
                        // If the user applied settings, reload them into the current session.
                        // We must invoke this on the UI thread.
                        this.Invoke((Action)(() =>
                        {
                            _currentSettings = SettingsManager.Load(_activeProfile.Target, _currentManager.GetDefaultSettings());
                        }));
                    }
                    _staticRangeFinderInstance = null; // Allow re-opening
                };
                _staticRangeFinderInstance.Show(this);
            }
        }
        // Changed to manage a single instance of the form.
        private void codeNoteConverterToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_codeNoteConverterInstance != null && !_codeNoteConverterInstance.IsDisposed) return;

            // Pass the current manager to the constructor for intelligent defaults.
            _codeNoteConverterInstance = new CodeNoteConverterForm(_currentManager);
            _codeNoteConverterInstance.FormClosed += (s, args) => _codeNoteConverterInstance = null;
            _codeNoteConverterInstance.Show(this);
        }

        // Changed to manage a single instance of the form.
        private void codeNoteHierarchyFixerToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_codeNoteHierarchyFixerInstance != null && !_codeNoteHierarchyFixerInstance.IsDisposed) return;

            _codeNoteHierarchyFixerInstance = new CodeNoteHierarchyFixerForm();
            _codeNoteHierarchyFixerInstance.FormClosed += (s, args) => _codeNoteHierarchyFixerInstance = null;
            _codeNoteHierarchyFixerInstance.Show(this);
        }
        #endregion

        #region Undo/Redo Management
        // Pushes the current result list onto the undo stack and clears the redo stack.
        // This should be called before any action that modifies the results.
        private void PushUndoState()
        {
            _undoStack.Push(new List<PointerPath>(_currentResults));
            _redoStack.Clear();
        }
        #endregion

        #region Scanning and Refining Logic
        // Handles the "New Pointer Scan" button click.
        private async void btnScan_Click(object sender, EventArgs e)
        {
            if (_scanCts != null || _activeProfile == null || !_currentManager.IsAttached) return;

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    ClearSessionDataOnly();
                    SwitchToScanUI(true, "Purging application memory...");
                    await Task.Run(() => PurgeMemory());

                    _lastScanParams = optionsForm.GetScanParameters();
                    if (_lastScanParams == null) return;

                    optionsForm.UpdateSettings(_currentSettings);
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);
                    try
                    {
                        _scanCts = new CancellationTokenSource();
                        var progress = new Progress<ScanProgressReport>(UpdateScanProgress);
                        _currentScanner = _activeProfile.ScannerFactory();
                        var scanTask = Task.Run(() => _currentScanner.Scan(_currentManager, _lastScanParams, progress, _scanCts.Token), _scanCts.Token);
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
            _lastFoundCount = 0;
            SwitchToScanUI(true);
            _scanStopwatch.Restart();
            _scanTimer.Start();
            _stoppedByMemoryMonitor = false;
            _memoryMonitorTimer.Start();

            try
            {
                var results = await scanTask;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                PopulateResultsGrid(results, true);
                SwitchToScanUI(false);

                if (_scanCts.IsCancellationRequested)
                {
                    if (_stoppedByMemoryMonitor)
                    {
                        UpdateStatus($"Scan stopped automatically due to high system memory usage. Found {results.Count:N0} paths {FormatDuration(elapsed)}.");
                        SoundManager.PlayFail();
                    }
                    else
                    {
                        UpdateStatus($"Scan stopped by user. Found {results.Count:N0} paths {FormatDuration(elapsed)}.");
                        SoundManager.PlayNotify();
                    }
                }
                else
                {
                    bool isStateScan = _activeProfile.StateBasedScannerFactory != null && _currentScanner.GetType().IsSubclassOf(typeof(StateBasedScannerStrategyBase));
                    SoundManager.PlayNotify();

                    if (!results.Any())
                    {
                        UpdateStatus($"Scan complete. No paths found {FormatDuration(elapsed)}.");
                    }
                    else
                    {
                        if (isStateScan)
                            UpdateStatus($"Scan complete. Found {results.Count:N0} valid paths {FormatDuration(elapsed)}.");
                        else
                            UpdateStatus($"Scan complete. Found {results.Count:N0} paths {FormatDuration(elapsed)}.");
                    }
                }
            }
            catch (OperationCanceledException)
            {
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                if (_stoppedByMemoryMonitor)
                {
                    UpdateStatus($"Scan stopped automatically due to high system memory usage {FormatDuration(elapsed)}.");
                    SoundManager.PlayFail();
                }
                else
                {
                    UpdateStatus($"Scan stopped by user {FormatDuration(elapsed)}.");
                    SoundManager.PlayNotify();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Scan failed with an error.");
            }
            finally
            {
                _memoryMonitorTimer.Stop();
                _scanStopwatch.Stop();
                _scanTimer.Stop();
                _scanCts?.Dispose();
                _scanCts = null;
                SwitchToScanUI(false);
            }
        }

        // Handles the "Refine with New Scan" button click.
        private async void btnRefineScan_Click(object sender, EventArgs e)
        {
            if (_scanCts != null || _activeProfile == null || !_currentManager.IsAttached || !_currentResults.Any())
            {
                MessageBox.Show("Please perform an initial scan and have results before refining.", "Initial Scan Required", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }

            UpdateStatus("Preparing existing results for refinement...");
            var existingPaths = await Task.Run(() => new HashSet<PointerPath>(_currentResults));
            UpdateStatus("Ready for refinement. Please configure the new scan.");

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    ClearSessionDataOnly();
                    SwitchToScanUI(true, "Purging application memory...");
                    await Task.Run(() => PurgeMemory());

                    var newScanParams = optionsForm.GetScanParameters();
                    if (newScanParams == null) return;
                    _lastScanParams = newScanParams;

                    optionsForm.UpdateSettings(_currentSettings);
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);
                    try
                    {
                        _scanCts = new CancellationTokenSource();
                        var progress = new Progress<ScanProgressReport>(UpdateScanProgress);
                        _currentScanner = _activeProfile.ScannerFactory();
                        var scanTask = Task.Run(() => _currentScanner.Scan(_currentManager, newScanParams, progress, _scanCts.Token), _scanCts.Token);
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
            _stoppedByMemoryMonitor = false;
            _memoryMonitorTimer.Start();
            bool shouldLog = DebugSettings.LogRefineScan;

            try
            {
                if (shouldLog) logger.Log($"--- STARTING REFINE SCAN: {existingPaths.Count:N0} initial paths to verify. ---");
                var newResults = await scanTask;
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();

                if (shouldLog) logger.Log($"New scan finished (cancelled={_scanCts.IsCancellationRequested}). Found {newResults.Count:N0} potential paths. Performing intersection...");
                var finalResults = newResults.Where(p => existingPaths.Contains(p)).ToList();
                PopulateResultsGrid(finalResults, true);
                SwitchToScanUI(false);

                if (_scanCts.IsCancellationRequested)
                {
                    if (_stoppedByMemoryMonitor)
                    {
                        UpdateStatus($"Refine scan stopped automatically due to high memory usage. Found {finalResults.Count:N0} matching paths {FormatDuration(elapsed)}.");
                        SoundManager.PlayFail();
                    }
                    else
                    {
                        UpdateStatus($"Refine scan stopped by user. Found {finalResults.Count:N0} matching paths {FormatDuration(elapsed)}.");
                        SoundManager.PlayNotify();
                    }
                }
                else
                {
                    if (finalResults.Any())
                    {
                        UpdateStatus($"Refine scan complete. Found {finalResults.Count:N0} matching paths {FormatDuration(elapsed)}.");
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
                var elapsed = _scanStopwatch.Elapsed;
                _scanTimer.Stop();
                if (_stoppedByMemoryMonitor)
                {
                    UpdateStatus($"Refine scan stopped automatically due to high system memory usage {FormatDuration(elapsed)}.");
                    SoundManager.PlayFail();
                }
                else
                {
                    UpdateStatus($"Refine scan stopped by user {FormatDuration(elapsed)}.");
                    SoundManager.PlayNotify();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("An error occurred during refine scan: " + ex.Message, "Scan Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                UpdateStatus("Refine scan failed with an error.");
            }
            finally
            {
                _isRefining = false;
                _memoryMonitorTimer.Stop();
                _scanStopwatch.Stop();
                _scanTimer.Stop();
                _scanCts?.Dispose();
                _scanCts = null;
                SwitchToScanUI(false);
            }
        }
        #endregion

        #region State-Based Scanning
        private async void btnCaptureState_Click(object sender, EventArgs e)
        {
            if (_scanCts != null || _activeProfile == null || !_currentManager.IsAttached) return;

            using (var optionsForm = new StateCaptureForm(_currentManager, _currentSettings, _multiStateCaptures))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    var scanParams = optionsForm.GetScanParameters();
                    if (scanParams == null) return;

                    ClearSessionDataOnly();

                    optionsForm.UpdateSettings(_currentSettings);
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);
                    _lastScanParams = scanParams;

                    UpdateStatus("Purging application memory...");
                    await Task.Run(() => PurgeMemory());

                    try
                    {
                        _scanCts = new CancellationTokenSource();
                        var progress = new Progress<ScanProgressReport>(UpdateScanProgress);
                        _currentScanner = _activeProfile.StateBasedScannerFactory();
                        var scanTask = Task.Run(() => _currentScanner.Scan(_currentManager, _lastScanParams, progress, _scanCts.Token), _scanCts.Token);
                        await RunScan(scanTask);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("An unexpected error occurred during state-based scan: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                        SwitchToScanUI(false);
                    }
                }
            }
        }
        #endregion

        #region Session Management
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

                        if (_currentManager != null && _currentManager.IsAttached && _activeProfile.Target.ToString() != sessionData.EmulatorTargetName)
                        {
                            var result = MessageBox.Show(
                                $"The session file is for '{sessionData.EmulatorTargetName}', but you are currently attached to '{_activeProfile.Name}'.\n\nDo you want to detach and continue loading the session?",
                                "Emulator Mismatch", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                            if (result == DialogResult.No) return;
                        }

                        if (_currentManager != null && _currentManager.IsAttached) DetachAndReset();
                        ClearSessionDataOnly();

                        _lastScanParams = sessionData.LastScanParameters;
                        _currentResults = sessionData.Results;

                        var profile = EmulatorProfileRegistry.Profiles.FirstOrDefault(p => p.Target.ToString() == sessionData.EmulatorTargetName);
                        if (profile != null)
                        {
                            _activeProfile = profile;
                            _currentManager = _activeProfile.ManagerFactory();
                            SetUIStateDetached();
                            this.Text = $"Pointer Finder 2.0 - [Loaded: {Path.GetFileName(ofd.FileName)}]";
                            menuAttach.Text = $"Attach to {_activeProfile.Name}";
                        }

                        PopulateResultsGrid(_currentResults, true);

                        if (profile != null && !string.IsNullOrEmpty(sessionData.SortedColumnName) && dgvResults.Columns.Contains(sessionData.SortedColumnName))
                        {
                            var column = dgvResults.Columns[sessionData.SortedColumnName];
                            SortResults(column, sessionData.SortDirection);
                        }

                        if (profile != null && sessionData.ProcessId != -1)
                        {
                            try
                            {
                                var process = Process.GetProcessById(sessionData.ProcessId);
                                if (profile.ProcessNames.Contains(process.ProcessName, StringComparer.OrdinalIgnoreCase))
                                {
                                    PerformAttachment(profile, process);
                                }
                            }
                            catch
                            {
                                UpdateStatus($"Session loaded. Emulator (PID: {sessionData.ProcessId}) not found. Attach manually.");
                                return;
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
                    else e.Value = string.Empty;
                    break;
            }
        }

        // Handles sorting when a column header is clicked.
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

            _currentResults.Sort((p1, p2) =>
            {
                int compareResult = 0;
                if (column.Name == "colBase") compareResult = p1.BaseAddress.CompareTo(p2.BaseAddress);
                else if (column.Name == "colFinal") compareResult = p1.FinalAddress.CompareTo(p2.FinalAddress);
                else if (column.Name.StartsWith("colOffset"))
                {
                    int offsetIndex = int.Parse(column.Name.Replace("colOffset", "")) - 1;
                    int offset1 = (offsetIndex < p1.Offsets.Count) ? p1.Offsets[offsetIndex] : int.MinValue;
                    int offset2 = (offsetIndex < p2.Offsets.Count) ? p2.Offsets[offsetIndex] : int.MinValue;
                    compareResult = offset1.CompareTo(offset2);
                }
                return (order == SortOrder.Descending) ? -compareResult : compareResult;
            });

            _sortedColumn = column;
            _sortOrder = order;
            _sortedColumn.HeaderCell.SortGlyphDirection = _sortOrder;
            dgvResults.Invalidate();
        }

        // Switches the UI between the idle state and the active scanning/filtering state.
        private void SwitchToScanUI(bool scanningOrFilteringActive, string customMessage = null)
        {
            // This method's logic was flawed. It has been rewritten to correctly use the
            // `scanningOrFilteringActive` parameter as the single source of truth for the UI state,
            // preventing the UI from getting "stuck" in a filtering state.
            // Determine the specific type of operation ONLY if we are in an active state.
            bool isScanning = scanningOrFilteringActive && _scanCts != null && !_scanCts.IsCancellationRequested;
            bool isFiltering = scanningOrFilteringActive && _filterCts != null && !_filterCts.IsCancellationRequested;

            tableLayoutPanel1.Visible = !scanningOrFilteringActive;
            progressBar.Visible = isScanning;
            btnStopScan.Visible = scanningOrFilteringActive;
            btnStopScan.Text = isFiltering ? "Stop Filtering" : "Stop Scan";
            lblProgressPercentage.Visible = isScanning;
            lblResultCount.Visible = !scanningOrFilteringActive && _currentResults.Any();
            lblElapsedTime.Visible = scanningOrFilteringActive;

            if (scanningOrFilteringActive)
            {
                if (!string.IsNullOrEmpty(customMessage)) UpdateStatus(customMessage);
                menuStrip1.Enabled = false;
                btnScan.Enabled = false;
                btnFilter.Enabled = false;
                btnRefineScan.Enabled = false;
                btnCaptureState.Enabled = false;
            }
            else
            {
                menuStrip1.Enabled = true;
                bool isAttached = _currentManager != null && _currentManager.IsAttached;
                bool hasResults = _currentResults.Any();

                btnScan.Enabled = isAttached;
                btnRefineScan.Enabled = isAttached && hasResults;
                btnRefineScan.Text = "Refine with New Scan";
                btnCaptureState.Enabled = isAttached;
                btnCaptureState.Text = "Capture State";
                btnFilter.Enabled = isAttached && hasResults;

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
            _lastFoundCount = report.FoundCount;

            if (_isRefining && report.CurrentValue > 0 && report.CurrentValue == report.MaxValue)
            {
                UpdateStatus("Scan phase complete, intersecting results...");
            }
            if (!string.IsNullOrEmpty(report.StatusMessage)) UpdateStatus(report.StatusMessage);

            if (_lastScanParams != null)
            {
                bool isStateScan = _activeProfile.StateBasedScannerFactory != null && _currentScanner.GetType().IsSubclassOf(typeof(StateBasedScannerStrategyBase));
                if (isStateScan)
                {
                    // Only update the main progress bar if valid Current/Max values are sent.
                    if (report.MaxValue > 0 && report.CurrentValue >= 0)
                    {
                        int max = (int)Math.Min(report.MaxValue, int.MaxValue);
                        if (progressBar.Maximum != max) progressBar.Maximum = max;
                        progressBar.Value = (int)Math.Min(report.CurrentValue, progressBar.Maximum);
                        lblProgressPercentage.Text = $"{report.CurrentValue:N0} / {report.MaxValue:N0}";
                    }
                    // The found count for state scans is now handled by the ScanTimer_Tick event.
                }
                else
                {
                    if (progressBar.Maximum != (int)_lastScanParams.MaxResults) progressBar.Maximum = (int)_lastScanParams.MaxResults;
                    progressBar.Value = (int)Math.Min(report.FoundCount, progressBar.Maximum);
                    lblProgressPercentage.Text = $"{report.FoundCount:N0} / {_lastScanParams.MaxResults:N0}";
                }
            }
        }

        // Updates the main status label.
        private void UpdateStatus(string status)
        {
            if (InvokeRequired) Invoke((Action)(() => UpdateStatus(status)));
            else lblStatus.Text = "Status: " + status;
        }

        // Populates the grid using Virtual Mode.
        private void PopulateResultsGrid(List<PointerPath> results, bool isNewDataSet)
        {
            // If this is a brand new data set (e.g., from a new scan), reset sorting and searching.
            if (isNewDataSet)
            {
                _sortedColumn = null;
                _sortOrder = SortOrder.None;
                _currentSearchTerm = string.Empty;
            }

            _currentResults = results;
            dgvResults.SuspendLayout();
            dgvResults.CellValueNeeded -= dgvResults_CellValueNeeded;
            dgvResults.Columns.Clear();

            // Just reset the state variables. The grid will handle removing the glyph.
            _sortedColumn = null;
            _sortOrder = SortOrder.None;

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
        }

        // Formats a TimeSpan into a user-friendly string.
        private string FormatDuration(TimeSpan duration)
        {
            var sb = new StringBuilder("in ");
            if (duration.TotalMinutes >= 1)
            {
                sb.Append($"{(int)duration.TotalMinutes} minute{((int)duration.TotalMinutes != 1 ? "s" : "")}");
                if (duration.Seconds > 0) sb.Append($" and {duration.Seconds} second{(duration.Seconds != 1 ? "s" : "")}");
            }
            else if (duration.TotalSeconds >= 1) sb.Append($"{duration.TotalSeconds:F1} seconds");
            else sb.Append($"{duration.TotalMilliseconds:F0} milliseconds");
            return sb.ToString();
        }
        #endregion

        #region Filtering Logic
        private async void btnFilter_Click(object sender, EventArgs e)
        {
            if (_filterCts != null) return;
            PushUndoState();
            _filterCts = new CancellationTokenSource();
            _validFilteredPaths = new ConcurrentBag<PointerPath>(_currentResults);
            SwitchToScanUI(true, "Starting filter...");
            _filterRefreshTimer.Start();
            _scanStopwatch.Restart();
            _scanTimer.Start();
            try
            {
                // Changed to call the new async version of FilterPathsContinuously.
                await FilterPathsContinuously(_filterCts.Token);
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
                SwitchToScanUI(false);
                UpdateStatus($"Filtering stopped {FormatDuration(elapsed)}. {_currentResults.Count:N0} paths remain.");
                _filterCts?.Dispose();
                _filterCts = null;
            }
        }

        // This method now uses a more memory-efficient "swap buffer" approach.
        // It avoids creating a new List<> on every iteration, reducing garbage collection pressure.
        private async Task FilterPathsContinuously(CancellationToken token)
        {
            // Use two bags for a swap-buffer pattern.
            var pathsToProcess = _validFilteredPaths;
            var validatedPaths = new ConcurrentBag<PointerPath>();

            while (!token.IsCancellationRequested)
            {
                if (pathsToProcess.IsEmpty)
                {
                    break;
                }

                // Process the current bag of paths in parallel chunks.
                await Task.Run(() =>
                {
                    Parallel.ForEach(pathsToProcess, path =>
                    {
                        if (token.IsCancellationRequested) return;
                        uint? calculatedAddress = _currentManager.RecalculateFinalAddress(path, path.FinalAddress);
                        if (calculatedAddress.HasValue && calculatedAddress.Value == path.FinalAddress)
                        {
                            validatedPaths.Add(path);
                        }
                    });
                }, token);

                if (token.IsCancellationRequested) break;

                // Swap the bags for the next iteration. The 'validatedPaths' becomes the new source,
                // and the old 'pathsToProcess' bag is cleared to become the new destination.
                _validFilteredPaths = validatedPaths;
                pathsToProcess = validatedPaths;
                validatedPaths = new ConcurrentBag<PointerPath>(); // Reset the destination bag.

                // Update the main results list for the UI refresh timer.
                _currentResults = pathsToProcess.ToList();

                // Calculate a dynamic delay between full filter passes to manage CPU usage.
                int pathCount = _currentResults.Count;
                int delayMs = (pathCount < 1000) ? 30 : (pathCount / 100);

                try
                {
                    await Task.Delay(delayMs, token);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
            }
        }
        #endregion

        #region General UI Event Handlers
        private void btnStopScan_Click(object sender, EventArgs e)
        {
            if (_filterCts != null) _filterCts.Cancel();
            else _scanCts?.Cancel();
        }

        private void PerformSearch(string searchText)
        {
            if (string.IsNullOrEmpty(searchText) || _currentManager == null)
            {
                dgvResults.ClearSelection();
                if (string.IsNullOrEmpty(searchText)) UpdateStatus("Search cleared.");
                return;
            }

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
            // Updated Ctrl+V logic to reuse an existing Code Note Converter window.
            if (e.Control && e.KeyCode == Keys.V)
            {
                e.SuppressKeyPress = true;
                string clipboardText = Clipboard.GetText();
                // Basic validation to see if it looks like a trigger.
                if (!string.IsNullOrEmpty(clipboardText) && (clipboardText.Contains("I:0x") || clipboardText.Contains("0xH")))
                {
                    // If an instance is not open, create it.
                    if (_codeNoteConverterInstance == null || _codeNoteConverterInstance.IsDisposed)
                    {
                        _codeNoteConverterInstance = new CodeNoteConverterForm(_currentManager);
                        _codeNoteConverterInstance.FormClosed += (s, args) => _codeNoteConverterInstance = null;
                        _codeNoteConverterInstance.Show(this);
                    }

                    // Pass the trigger to the (now guaranteed to be open) instance and focus it.
                    _codeNoteConverterInstance.ProcessTrigger(clipboardText);
                    _codeNoteConverterInstance.Activate();
                }
            }
        }

        // Helper method to reset the visual sorting indicators on the DataGridView headers.
        private void ClearSortGlyphs()
        {
            if (_sortedColumn != null)
            {
                _sortedColumn.HeaderCell.SortGlyphDirection = SortOrder.None;
            }
            _sortedColumn = null;
            _sortOrder = SortOrder.None;
        }

        private void contextMenuResults_Opening(object sender, CancelEventArgs e)
        {
            bool hasSelection = dgvResults.SelectedRows.Count > 0;
            bool hasResults = _currentResults.Any();
            copyBaseAddressToolStripMenuItem.Enabled = hasSelection;
            deleteSelectedToolStripMenuItem.Enabled = hasSelection;
            copyAsRetroAchievementsFormatToolStripMenuItem.Enabled = dgvResults.SelectedRows.Count == 1;
            copyAsCodeNoteToolStripMenuItem.Enabled = dgvResults.SelectedRows.Count == 1;
            // Enable the new sort option only if there are results to sort.
            sortByLowestOffsetsToolStripMenuItem.Enabled = hasResults;
        }

        private void copyBaseAddressToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0 || _currentManager == null) return;
            var addresses = dgvResults.SelectedRows.Cast<DataGridViewRow>()
                .Select(r => _currentManager.FormatDisplayAddress(_currentResults[r.Index].BaseAddress));
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

        private void copyAsCodeNoteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count != 1) return;

            PointerPath path = _currentResults[dgvResults.SelectedRows[0].Index];
            if (path != null)
            {
                var settings = CodeNoteSettings.GetFromGlobalSettings();
                string codeNote = CodeNoteHelper.GenerateFromPointerPath(path, settings);
                Clipboard.SetText(codeNote);
                UpdateStatus("Copied path as custom code note.");
            }
        }

        // This method has been refactored to be more memory-efficient.
        // It now builds a cache of sorted offsets first, then performs an in-place sort
        // on the main results list, avoiding the creation of a large intermediate list of objects.
        private async void sortByLowestOffsetsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!_currentResults.Any()) return;

            PushUndoState();
            ClearSortGlyphs();

            this.Enabled = false;
            this.UseWaitCursor = true;
            UpdateStatus("Sorting by lowest offsets... this may take a moment.");

            try
            {
                await Task.Run(() =>
                {
                    // Step 1: Create a cache of pre-sorted absolute offsets for each path.
                    // This is the memory-intensive part, but it's unavoidable if we want fast sorting.
                    var offsetCache = _currentResults.ToDictionary(
                        p => p,
                        p => p.Offsets.Select(o => Math.Abs(o)).OrderBy(o => o).ToList()
                    );

                    // Step 2: Sort the main list in-place using the cache.
                    _currentResults.Sort((p1, p2) =>
                    {
                        if (GlobalSettings.SortByLevelFirst)
                        {
                            int levelComparison = p1.Offsets.Count.CompareTo(p2.Offsets.Count);
                            if (levelComparison != 0) return levelComparison;
                        }

                        var p1Offsets = offsetCache[p1];
                        var p2Offsets = offsetCache[p2];

                        int minCount = Math.Min(p1Offsets.Count, p2Offsets.Count);
                        for (int i = 0; i < minCount; i++)
                        {
                            int comparison = p1Offsets[i].CompareTo(p2Offsets[i]);
                            if (comparison != 0) return comparison;
                        }

                        return p1.Offsets.Count.CompareTo(p2.Offsets.Count);
                    });
                });

                dgvResults.Invalidate();
                UpdateStatus($"Sorted by lowest offsets. {_currentResults.Count:N0} results.");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An error occurred during sorting: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                UpdateStatus("Sorting failed.");
            }
            finally
            {
                this.Enabled = true;
                this.UseWaitCursor = false;
            }
        }

        private void deleteSelectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0) return;
            PushUndoState();

            if (dgvResults.SelectedRows.Count == _currentResults.Count)
            {
                ClearSessionDataOnly();
                UpdateStatus("All results cleared.");
                SwitchToScanUI(false);
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
        private void videoTutorialToolStripMenuItem_Click(object sender, EventArgs e)
        {
            const string tutorialUrl = "https://youtu.be/QwHTML0kRtI";
            try
            {
                // This is the most reliable way to open a URL in the user's default browser.
                var ps = new ProcessStartInfo(tutorialUrl)
                {
                    UseShellExecute = true,
                    Verb = "open"
                };
                Process.Start(ps);
            }
            catch (Exception ex)
            {
                // If it fails for any reason, show an error with the link so the user can copy it.
                MessageBox.Show($"Could not open the video link automatically.\n\nPlease copy and paste this URL into your browser:\n\n{tutorialUrl}\n\nError: {ex.Message}",
                    "Error Opening Link",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }
        #endregion
    }
}