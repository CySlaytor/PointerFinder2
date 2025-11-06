using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
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
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2
{
    // The main window of the Pointer Finder application. Refactored to delegate logic to manager classes.
    public partial class MainForm : Form
    {
        #region Fields
        // --- Manager Classes ---
        private readonly ApplicationLifecycleManager _lifecycleManager;
        private readonly ResultsManager _resultsManager;
        private readonly ScanCoordinator _scanCoordinator;
        private readonly SessionManager _sessionManager;

        // --- Application State ---
        private IEmulatorManager _currentManager;
        private AppSettings _currentSettings;
        private EmulatorProfile _activeProfile;
        // Replaced the raw ScanState array with an encapsulated manager class.
        private readonly MultiScanState _multiScanState = new MultiScanState();
        private int _lastFoundCount = 0;
        private string _currentSearchTerm = string.Empty;

        // --- Timers ---
        private readonly System.Windows.Forms.Timer _scanTimer;
        private readonly Stopwatch _scanStopwatch = new Stopwatch();

        // --- Tool Window Instances ---
        private CodeNoteConverterForm _codeNoteConverterInstance;
        private CodeNoteHierarchyFixerForm _codeNoteHierarchyFixerInstance;
        private Form _staticRangeFinderInstance;

        // --- Debugging ---
        private readonly DebugLogForm logger = DebugLogForm.Instance;
        #endregion

        public MainForm(string[] args)
        {
            InitializeComponent();
            SettingsManager.InitializeGlobalSettings();

            // Initialize managers and subscribe to their events
            _lifecycleManager = new ApplicationLifecycleManager(this);
            _resultsManager = new ResultsManager();
            _scanCoordinator = new ScanCoordinator();
            _sessionManager = new SessionManager();

            _lifecycleManager.EmulatorProcessExited += OnEmulatorProcessExited;
            _resultsManager.ResultsChanged += OnResultsChanged;
            _resultsManager.SortChanged += OnSortChanged;
            _scanCoordinator.OperationStarted += OnOperationStarted;
            _scanCoordinator.OperationFinished += OnOperationFinished;
            _scanCoordinator.ProgressUpdated += OnProgressUpdated;
            _scanCoordinator.ScanCompleted += OnScanCompleted;
            _scanCoordinator.FoundCountUpdated += OnFoundCountUpdated;
            _scanCoordinator.FilterCompleted += OnFilterCompleted;

            var (pid, target) = _lifecycleManager.ParseCommandLineArgs(args);
            if (pid != -1)
            {
                this.Load += (s, e) => AutoAttachOnRestart(pid, target);
            }

            this.FormClosing += MainForm_FormClosing;
            dgvResults.DoubleBuffered(true);
            SetUIStateDetached();
            dgvResults.ColumnHeaderMouseClick += dgvResults_ColumnHeaderMouseClick;
            _scanTimer = new System.Windows.Forms.Timer { Interval = 100 };
            _scanTimer.Tick += ScanTimer_Tick;
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            Settings.Default.MainWindowState = this.WindowState;
            if (this.WindowState == FormWindowState.Normal)
            {
                Settings.Default.MainWindowLocation = this.Location;
                Settings.Default.MainWindowSize = this.Size;
            }
            else
            {
                Settings.Default.MainWindowLocation = this.RestoreBounds.Location;
                Settings.Default.MainWindowSize = this.RestoreBounds.Size;
            }
            Settings.Default.Save();
        }

        #region Attachment and State Management
        private bool PerformAttachment(EmulatorProfile profile, Process process)
        {
            if (profile == null || process == null) return false;

            _activeProfile = profile;
            _currentManager = _activeProfile.ManagerFactory();
            _lifecycleManager.SetCurrentManager(_currentManager);

            var defaultSettings = _currentManager.GetDefaultSettings();
            _currentSettings = SettingsManager.Load(_activeProfile.Target, defaultSettings);

            if (_currentManager.Attach(process))
            {
                string windowTitle = process.MainWindowTitle;
                this.Text = !string.IsNullOrWhiteSpace(windowTitle) ? $"Pointer Finder 2.0 - [{windowTitle}]" : $"Pointer Finder 2.0 - [{_currentManager.EmulatorName} Mode]";
                UpdateStatus($"Attached to {_currentManager.EmulatorName} (PID: {process.Id})");
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
            }
            catch (Exception ex)
            {
                logger.Log($"Auto-attach failed: {ex.Message}");
            }
        }

        private void DetachAndReset()
        {
            _scanCoordinator.StopCurrentOperation();
            _currentManager?.Detach();
            _activeProfile = null;
            _currentManager = null;
            _currentSettings = null;
            _lifecycleManager.SetCurrentManager(null);
            _resultsManager.ClearHistory();
            _resultsManager.ClearResults();
            // Use the new manager class to clear the state.
            _multiScanState.ClearAll();
            SetUIStateDetached();
        }

        private void OnEmulatorProcessExited(object sender, EventArgs e)
        {
            if (InvokeRequired)
            {
                Invoke((Action)(() => OnEmulatorProcessExited(sender, e)));
                return;
            }
            DetachAndReset();
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
            }
            else if (totalInstancesFound == 1)
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
            saveSessionToolStripMenuItem.Enabled = _currentManager != null && _currentManager.IsAttached && _resultsManager.HasResults;
            loadSessionToolStripMenuItem.Enabled = true;
        }
        private void saveSessionToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var sessionData = new SessionData
            {
                EmulatorTargetName = _activeProfile?.Target.ToString(),
                ProcessId = _currentManager?.EmulatorProcess?.Id ?? -1,
                LastScanParameters = _scanCoordinator.LastScanParams,
                Results = _resultsManager.CurrentResults.ToList(),
                SortedColumnName = _resultsManager.SortedColumnName,
                SortDirection = _resultsManager.SortOrder
            };

            if (_sessionManager.SaveSession(sessionData, this))
            {
                UpdateStatus("Session saved.");
            }
        }
        private void loadSessionToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var sessionData = _sessionManager.LoadSession(this);
            if (sessionData == null) return;

            if (_currentManager != null && _currentManager.IsAttached) DetachAndReset();

            var profile = EmulatorProfileRegistry.Profiles.FirstOrDefault(p => p.Target.ToString() == sessionData.EmulatorTargetName);
            if (profile != null)
            {
                _activeProfile = profile;
                _currentManager = _activeProfile.ManagerFactory();
                _lifecycleManager.SetCurrentManager(_currentManager);
                SetUIStateDetached();
                this.Text = $"Pointer Finder 2.0 - [Loaded Session]";
                menuAttach.Text = $"Attach to {_activeProfile.Name}";
            }

            _resultsManager.SetNewResults(sessionData.Results, true);
            _scanCoordinator.LastScanParams = sessionData.LastScanParameters;

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
                catch { /* Process not found */ }
            }
            UpdateStatus($"Session loaded with {sessionData.Results.Count:N0} results.");
        }
        private void editToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            findToolStripMenuItem.Enabled = _resultsManager.HasResults;
            undoToolStripMenuItem.Enabled = _resultsManager.CanUndo;
            redoToolStripMenuItem.Enabled = _resultsManager.CanRedo;
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
        private void undoToolStripMenuItem_Click(object sender, EventArgs e) => _resultsManager.Undo();
        private void redoToolStripMenuItem_Click(object sender, EventArgs e) => _resultsManager.Redo();
        private void debugConsoleToolStripMenuItem_Click(object sender, EventArgs e) { DebugLogForm.Instance.Show(); DebugLogForm.Instance.BringToFront(); }
        private void debugOptionsToolStripMenuItem_Click(object sender, EventArgs e) { new SettingsForm(this).ShowDialog(this); }
        private void toolsToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            bool isAttached = _currentManager != null && _currentManager.IsAttached;
            bool isStaticFinderSupported = _activeProfile?.SupportsStaticRangeFinder ?? false;

            staticRangeFinderToolStripMenuItem.Enabled = isAttached && isStaticFinderSupported && (_staticRangeFinderInstance == null || _staticRangeFinderInstance.IsDisposed);
            codeNoteConverterToolStripMenuItem.Enabled = (_codeNoteConverterInstance == null || _codeNoteConverterInstance.IsDisposed);
            codeNoteHierarchyFixerToolStripMenuItem.Enabled = (_codeNoteHierarchyFixerInstance == null || _codeNoteHierarchyFixerInstance.IsDisposed);
        }

        private void staticRangeFinderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_staticRangeFinderInstance != null && !_staticRangeFinderInstance.IsDisposed) return;
            if (_activeProfile?.StaticRangeFinderFactory == null) return;

            Form finderForm = _activeProfile.StaticRangeFinderFactory(_currentManager);

            if (finderForm != null)
            {
                _staticRangeFinderInstance = finderForm;
                _staticRangeFinderInstance.FormClosed += (s, args) =>
                {
                    if ((s as Form)?.DialogResult == DialogResult.OK)
                    {
                        this.Invoke((Action)(() => _currentSettings = SettingsManager.Load(_activeProfile.Target, _currentManager.GetDefaultSettings())));
                    }
                    _staticRangeFinderInstance = null;
                };
                _staticRangeFinderInstance.Show(this);
            }
        }
        private void codeNoteConverterToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_codeNoteConverterInstance != null && !_codeNoteConverterInstance.IsDisposed) return;
            _codeNoteConverterInstance = new CodeNoteConverterForm(_currentManager);
            _codeNoteConverterInstance.FormClosed += (s, args) => _codeNoteConverterInstance = null;
            _codeNoteConverterInstance.Show(this);
        }
        private void codeNoteHierarchyFixerToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (_codeNoteHierarchyFixerInstance != null && !_codeNoteHierarchyFixerInstance.IsDisposed) return;
            _codeNoteHierarchyFixerInstance = new CodeNoteHierarchyFixerForm();
            _codeNoteHierarchyFixerInstance.FormClosed += (s, args) => _codeNoteHierarchyFixerInstance = null;
            _codeNoteHierarchyFixerInstance.Show(this);
        }
        #endregion

        #region Scan Button Handlers
        private async void btnScan_Click(object sender, EventArgs e)
        {
            if (_scanCoordinator.IsBusy || _activeProfile == null || !_currentManager.IsAttached) return;

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    _resultsManager.SetNewResults(new List<PointerPath>(), true);
                    UpdateStatus("Purging application memory...");
                    await Task.Run(() => _lifecycleManager.PurgeMemory());

                    var scanParams = optionsForm.GetScanParameters();
                    if (scanParams == null) return;

                    optionsForm.UpdateSettings(_currentSettings);
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);

                    var scanner = _activeProfile.ScannerFactory();
                    await _scanCoordinator.StartScan(_currentManager, scanner, scanParams, isRefine: false);
                }
            }
        }

        private async void btnRefineScan_Click(object sender, EventArgs e)
        {
            if (_scanCoordinator.IsBusy || _activeProfile == null || !_currentManager.IsAttached || !_resultsManager.HasResults) return;

            UpdateStatus("Preparing existing results for refinement...");
            var existingPaths = new HashSet<PointerPath>(_resultsManager.CurrentResults);
            UpdateStatus("Ready for refinement. Please configure the new scan.");

            using (var optionsForm = new ScannerOptionsForm(_currentManager, _currentSettings))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    _resultsManager.SetNewResults(new List<PointerPath>(), true); // Clear UI temporarily
                    UpdateStatus("Purging application memory...");
                    await Task.Run(() => _lifecycleManager.PurgeMemory());

                    var scanParams = optionsForm.GetScanParameters();
                    if (scanParams == null) return;

                    optionsForm.UpdateSettings(_currentSettings);
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);

                    var scanner = _activeProfile.ScannerFactory();
                    await _scanCoordinator.StartScan(_currentManager, scanner, scanParams, isRefine: true, existingPaths);
                }
            }
        }

        private async void btnCaptureState_Click(object sender, EventArgs e)
        {
            if (_scanCoordinator.IsBusy || _activeProfile == null || !_currentManager.IsAttached) return;

            // Pass the encapsulated state manager to the form instead of a raw array.
            using (var optionsForm = new StateCaptureForm(_currentManager, _currentSettings, _multiScanState))
            {
                if (optionsForm.ShowDialog() == DialogResult.OK)
                {
                    var scanParams = optionsForm.GetScanParameters();
                    if (scanParams == null) return;

                    _resultsManager.SetNewResults(new List<PointerPath>(), true);
                    UpdateStatus("Purging application memory...");
                    await Task.Run(() => _lifecycleManager.PurgeMemory());

                    optionsForm.UpdateSettings(_currentSettings);
                    SettingsManager.Save(_activeProfile.Target, _currentSettings);

                    var scanner = _activeProfile.StateBasedScannerFactory();
                    await _scanCoordinator.StartScan(_currentManager, scanner, scanParams, isRefine: false);
                }
            }
        }
        private async void btnFilter_Click(object sender, EventArgs e)
        {
            if (_scanCoordinator.IsBusy || !_resultsManager.HasResults) return;
            await _scanCoordinator.StartFiltering(_currentManager, _resultsManager.CurrentResults);
        }

        private void btnStopScan_Click(object sender, EventArgs e)
        {
            _scanCoordinator.StopCurrentOperation();
        }
        #endregion

        #region Scan Coordinator Event Handlers
        private void OnOperationStarted(string message)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnOperationStarted(message))); return; }
            SwitchToScanUI(true, message);
            _scanStopwatch.Restart();
            _scanTimer.Start();
            _lastFoundCount = 0;
            progressBar.Value = 0;
        }

        private void OnOperationFinished(string finalStatusMessage)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnOperationFinished(finalStatusMessage))); return; }
            _scanTimer.Stop();
            _scanStopwatch.Stop();
            SwitchToScanUI(false);
            UpdateStatus(finalStatusMessage);
        }

        private void OnProgressUpdated(ScanProgressReport report)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnProgressUpdated(report))); return; }

            if (!string.IsNullOrEmpty(report.StatusMessage)) UpdateStatus(report.StatusMessage);

            int max = (int)Math.Min(report.MaxValue > 0 ? report.MaxValue : _scanCoordinator.LastScanParams?.MaxResults ?? int.MaxValue, int.MaxValue);
            if (progressBar.Maximum != max) progressBar.Maximum = max;

            long current = report.CurrentValue > 0 ? report.CurrentValue : report.FoundCount;
            progressBar.Value = (int)Math.Min(current, progressBar.Maximum);

            lblProgressPercentage.Text = (report.MaxValue > 0)
                ? $"{report.CurrentValue:N0} / {report.MaxValue:N0}"
                : $"{report.FoundCount:N0} / {_scanCoordinator.LastScanParams?.MaxResults:N0}";
        }

        private void OnScanCompleted(ScanCompletedEventArgs e)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnScanCompleted(e))); return; }

            _resultsManager.SetNewResults(e.Results, isNewDataSet: true);
            SwitchToScanUI(false);

            if (e.WasCancelled)
            {
                UpdateStatus($"Scan stopped. Found {e.Results.Count:N0} paths {FormatDuration(e.Duration)}.");
                SoundManager.PlayNotify();
            }
            else
            {
                if (!e.Results.Any())
                {
                    UpdateStatus($"Scan complete. No paths found {FormatDuration(e.Duration)}.");
                    if (e.IsRefineScan) SoundManager.PlayFail();
                    else SoundManager.PlayNotify();
                }
                else
                {
                    string message = e.IsRefineScan
                        ? $"Refine scan complete. Found {e.Results.Count:N0} matching paths {FormatDuration(e.Duration)}."
                        : $"Scan complete. Found {e.Results.Count:N0} paths {FormatDuration(e.Duration)}.";
                    UpdateStatus(message);

                    if (e.IsRefineScan || e.IsStateScan)
                    {
                        SoundManager.PlaySuccess();
                    }
                    else
                    {
                        SoundManager.PlayNotify();
                    }
                }
            }
        }

        private void OnFilterCompleted(List<PointerPath> filteredResults)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnFilterCompleted(filteredResults))); return; }
            _resultsManager.ApplyFilterResults(filteredResults);
        }


        private void OnFoundCountUpdated(int count)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnFoundCountUpdated(count))); return; }
            _lastFoundCount = count;
        }
        #endregion

        #region Results Manager Event Handlers
        private void OnResultsChanged(object sender, ResultsChangedEventArgs e)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnResultsChanged(sender, e))); return; }
            PopulateResultsGrid(_resultsManager.CurrentResults, e.IsNewDataSet);
            UpdateStatus($"{_resultsManager.ResultsCount:N0} results.");
            SwitchToScanUI(false);
        }

        private void OnSortChanged(object sender, SortChangedEventArgs e)
        {
            if (InvokeRequired) { Invoke((Action)(() => OnSortChanged(sender, e))); return; }

            foreach (DataGridViewColumn col in dgvResults.Columns)
            {
                col.HeaderCell.SortGlyphDirection = (col.Name == e.ColumnName) ? e.SortOrder : SortOrder.None;
            }
        }
        #endregion

        #region UI Updates and Interaction
        private void ScanTimer_Tick(object sender, EventArgs e)
        {
            if (_scanStopwatch.IsRunning)
            {
                TimeSpan ts = _scanStopwatch.Elapsed;
                string timeText = $"Time: {ts.Minutes:D2}:{ts.Seconds:D2}.{ts.Milliseconds / 100}";
                if (_lastFoundCount > 0)
                {
                    lblElapsedTime.Text = $"{timeText} | Found: {_lastFoundCount:N0}";
                }
                else
                {
                    lblElapsedTime.Text = timeText;
                }
            }
        }

        private void PopulateResultsGrid(IReadOnlyList<PointerPath> results, bool isNewDataSet)
        {
            dgvResults.SuspendLayout();
            dgvResults.CellValueNeeded -= dgvResults_CellValueNeeded;

            if (isNewDataSet)
            {
                dgvResults.Columns.Clear();
                _resultsManager.ClearSorting();
            }

            lblResultCount.Text = $"Results: {results.Count:N0}";
            lblResultCount.Visible = results.Any();

            if (!results.Any())
            {
                dgvResults.VirtualMode = false;
                dgvResults.RowCount = 0;
            }
            else
            {
                if (isNewDataSet)
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
                }
                dgvResults.VirtualMode = true;
                dgvResults.RowCount = results.Count;
                dgvResults.CellValueNeeded += dgvResults_CellValueNeeded;
            }
            dgvResults.ResumeLayout();
            dgvResults.Invalidate();
        }
        private void dgvResults_CellValueNeeded(object sender, DataGridViewCellValueEventArgs e)
        {
            if (_currentManager == null || _resultsManager.CurrentResults == null || e.RowIndex < 0 || e.RowIndex >= _resultsManager.ResultsCount) return;
            PointerPath path = _resultsManager.CurrentResults[e.RowIndex];
            string colName = dgvResults.Columns[e.ColumnIndex].Name;
            switch (colName)
            {
                case "colBase": e.Value = _currentManager.FormatDisplayAddress(path.BaseAddress); break;
                case "colFinal": e.Value = _currentManager.FormatDisplayAddress(path.FinalAddress); break;
                default:
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
        private void dgvResults_ColumnHeaderMouseClick(object sender, DataGridViewCellMouseEventArgs e)
        {
            _resultsManager.Sort(dgvResults.Columns[e.ColumnIndex].Name, _resultsManager.SortOrder);
        }

        private void SwitchToScanUI(bool scanningOrFilteringActive, string customMessage = null)
        {
            if (InvokeRequired) { Invoke((Action)(() => SwitchToScanUI(scanningOrFilteringActive, customMessage))); return; }

            bool isAttached = _currentManager != null && _currentManager.IsAttached;
            bool hasResults = _resultsManager.HasResults;

            tableLayoutPanel1.Visible = !scanningOrFilteringActive;
            progressBar.Visible = scanningOrFilteringActive;
            btnStopScan.Visible = scanningOrFilteringActive;
            lblProgressPercentage.Visible = scanningOrFilteringActive;
            lblResultCount.Visible = !scanningOrFilteringActive && hasResults;
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
                btnScan.Enabled = isAttached;
                btnRefineScan.Enabled = isAttached && hasResults;
                btnCaptureState.Enabled = isAttached;
                btnFilter.Enabled = isAttached && hasResults;
                if (!isAttached) SetUIStateDetached();
            }
        }

        private void SetUIStateDetached()
        {
            this.Text = "Pointer Finder 2.0";
            UpdateStatus("Not Attached");
            lblBaseAddress.Text = "";
            lblResultCount.Visible = false;
            menuAttach.Text = "Attach to Emulator...";
            btnScan.Enabled = false;
            btnFilter.Enabled = false;
            btnRefineScan.Enabled = false;
            btnCaptureState.Enabled = false;
        }

        private void UpdateStatus(string status)
        {
            if (InvokeRequired) Invoke((Action)(() => UpdateStatus(status)));
            else lblStatus.Text = "Status: " + status;
        }
        private string FormatDuration(TimeSpan duration)
        {
            var sb = new StringBuilder("in ");
            if (duration.TotalMinutes >= 1) sb.Append($"{(int)duration.TotalMinutes}m {(int)duration.Seconds}s");
            else if (duration.TotalSeconds >= 1) sb.Append($"{duration.TotalSeconds:F1}s");
            else sb.Append($"{duration.TotalMilliseconds:F0}ms");
            return sb.ToString();
        }
        private void deleteSelectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0) return;
            if (dgvResults.SelectedRows.Count == _resultsManager.ResultsCount)
            {
                _resultsManager.ClearResults();
            }
            else
            {
                var indicesToRemove = dgvResults.SelectedRows.Cast<DataGridViewRow>().Select(r => r.Index);
                _resultsManager.DeleteSelected(indicesToRemove);
            }
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
            if (e.Control && e.KeyCode == Keys.V)
            {
                e.SuppressKeyPress = true;
                string clipboardText = Clipboard.GetText();
                if (!string.IsNullOrEmpty(clipboardText) && (clipboardText.Contains("I:0x") || clipboardText.Contains("0xH")))
                {
                    if (_codeNoteConverterInstance == null || _codeNoteConverterInstance.IsDisposed)
                    {
                        _codeNoteConverterInstance = new CodeNoteConverterForm(_currentManager);
                        _codeNoteConverterInstance.FormClosed += (s, args) => _codeNoteConverterInstance = null;
                        _codeNoteConverterInstance.Show(this);
                    }
                    _codeNoteConverterInstance.ProcessTrigger(clipboardText);
                    _codeNoteConverterInstance.Activate();
                }
            }
        }

        private void PerformSearch(string searchText)
        {
            if (string.IsNullOrEmpty(searchText) || _currentManager == null)
            {
                dgvResults.ClearSelection();
                if (string.IsNullOrEmpty(searchText)) UpdateStatus("Search cleared.");
                return;
            }

            for (int i = 0; i < _resultsManager.ResultsCount; i++)
            {
                string baseAddr = _currentManager.FormatDisplayAddress(_resultsManager.CurrentResults[i].BaseAddress);
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
        #endregion

        #region Context Menu Handlers
        public void RestartApplication() => _lifecycleManager.RestartApplication(_activeProfile);
        public void PurgeMemory() => _lifecycleManager.PurgeMemory();
        private void videoTutorialToolStripMenuItem_Click(object sender, EventArgs e)
        {
            const string tutorialUrl = "https://youtu.be/QwHTML0kRtI";
            try
            {
                var ps = new ProcessStartInfo(tutorialUrl) { UseShellExecute = true, Verb = "open" };
                Process.Start(ps);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Could not open the video link automatically.\n\nPlease copy and paste this URL into your browser:\n\n{tutorialUrl}\n\nError: {ex.Message}", "Error Opening Link", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        private void contextMenuResults_Opening(object sender, CancelEventArgs e)
        {
            bool hasSelection = dgvResults.SelectedRows.Count > 0;
            copyBaseAddressToolStripMenuItem.Enabled = hasSelection;
            deleteSelectedToolStripMenuItem.Enabled = hasSelection;
            copyAsRetroAchievementsFormatToolStripMenuItem.Enabled = dgvResults.SelectedRows.Count == 1;
            copyAsCodeNoteToolStripMenuItem.Enabled = dgvResults.SelectedRows.Count == 1;
            sortByLowestOffsetsToolStripMenuItem.Enabled = _resultsManager.HasResults;
        }
        private void copyBaseAddressToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count == 0 || _currentManager == null) return;
            var addresses = dgvResults.SelectedRows.Cast<DataGridViewRow>()
                .Select(r => _currentManager.FormatDisplayAddress(_resultsManager.CurrentResults[r.Index].BaseAddress));
            if (addresses.Any())
            {
                Clipboard.SetText(string.Join(Environment.NewLine, addresses));
                UpdateStatus($"Copied {addresses.Count():N0} base address(es) to clipboard.");
            }
        }
        private void copyAsRetroAchievementsFormatToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count != 1) return;
            PointerPath path = _resultsManager.CurrentResults[dgvResults.SelectedRows[0].Index];
            if (path != null && _currentManager != null)
            {
                Clipboard.SetText(path.ToRetroAchievementsString(_currentManager));
                UpdateStatus("Copied path in RetroAchievements format.");
            }
        }
        private void copyAsCodeNoteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (dgvResults.SelectedRows.Count != 1) return;
            PointerPath path = _resultsManager.CurrentResults[dgvResults.SelectedRows[0].Index];
            if (path != null)
            {
                var settings = CodeNoteSettings.GetFromGlobalSettings();
                string codeNote = CodeNoteHelper.GenerateFromPointerPath(path, settings);
                Clipboard.SetText(codeNote);
                UpdateStatus("Copied path as custom code note.");
            }
        }
        private async void sortByLowestOffsetsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!_resultsManager.HasResults) return;

            this.Enabled = false;
            this.UseWaitCursor = true;
            UpdateStatus("Sorting by lowest offsets... this may take a moment.");

            try
            {
                await _resultsManager.SortByLowestOffsetsAsync();
                UpdateStatus($"Sorted by lowest offsets. {_resultsManager.ResultsCount:N0} results.");
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
        #endregion
    }
}