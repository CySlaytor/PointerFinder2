using PointerFinder2.Core;
using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2
{
    public partial class StateCaptureForm : Form
    {
        private readonly IEmulatorManager _manager;
        private readonly AppSettings _currentSettings;
        private readonly ScanState[] _capturedStates;
        private int _lastCapturedSlotIndex = -1;

        public StateCaptureForm(IEmulatorManager manager, AppSettings settings, ScanState[] capturedStates)
        {
            InitializeComponent();
            _manager = manager;
            _currentSettings = settings;
            _capturedStates = capturedStates; // Keep a reference to MainForm's array
        }

        private void StateCaptureForm_Load(object sender, EventArgs e)
        {
            this.Text = $"{_manager.EmulatorName} State-Based Scan";
            string targetSystem;
            var profile = EmulatorProfileRegistry.Profiles.Find(p => p.Name.StartsWith(_manager.EmulatorName.Split(' ')[0]));
            switch (profile?.Target)
            {
                case EmulatorTarget.PCSX2: targetSystem = "PS2"; break;
                case EmulatorTarget.DuckStation: targetSystem = "PS1"; break;
                case EmulatorTarget.RALibretroNDS: targetSystem = "NDS"; break;
                case EmulatorTarget.Dolphin: targetSystem = "GC/Wii"; break;
                default: targetSystem = "Mem"; break;
            }
            groupBoxRange.Text = $"Static Base Address Range ({targetSystem}, Hex)";
            colAddress.HeaderText = $"Target Address ({targetSystem}, Hex)";

            // Populate settings
            txtMaxOffset.Text = _currentSettings.MaxOffset.ToString("X");
            numMaxLevel.Value = _currentSettings.MaxLevel;
            numMaxResults.Value = _currentSettings.MaxResults;
            txtStaticStart.Text = _currentSettings.StaticAddressStart;
            txtStaticEnd.Text = _currentSettings.StaticAddressEnd;
            chkStopOnFirst.Checked = _currentSettings.StopOnFirstPathFound;
            // Load the new FindAllPathLevels setting.
            chkFindAllLevels.Checked = _currentSettings.FindAllPathLevels;
            numCandidatesPerLevel.Value = _currentSettings.CandidatesPerLevel;

            // NOTE: 16-byte alignment checkbox is intentionally removed from this form,
            // as the state-based algorithm always uses a 4-byte search for maximum accuracy.

            // Initialize Grid from persistent state and find last captured slot
            for (int i = 0; i < 4; i++)
            {
                string address = "";
                string status = "Empty";
                string action = "Capture";
                Color statusColor = SystemColors.WindowText;

                if (_capturedStates[i] != null)
                {
                    // Display the full, absolute address to avoid confusion.
                    address = _capturedStates[i].Parameters.TargetAddress.ToString("X8");
                    status = "Captured";
                    action = "Release";
                    statusColor = Color.Green;
                    _lastCapturedSlotIndex = i; // Track the last valid slot
                }

                dgvStates.Rows.Add($"State {i + 1}", address, status, action);
                dgvStates.Rows[i].Cells["colStatus"].Style.ForeColor = statusColor;
            }

            dgvStates.ClearSelection();
            UpdateScanButtonState();
        }

        private async void dgvStates_CellContentClick(object sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex < 0 || e.ColumnIndex != dgvStates.Columns["colAction"].Index)
            {
                return;
            }

            var row = dgvStates.Rows[e.RowIndex];
            var actionCell = row.Cells["colAction"] as DataGridViewButtonCell;
            var statusCell = row.Cells["colStatus"];
            var addressCell = row.Cells["colAddress"];

            if (actionCell.Value.ToString() == "Release")
            {
                _capturedStates[e.RowIndex] = null;
                statusCell.Value = "Empty";
                statusCell.Style.ForeColor = SystemColors.WindowText;
                actionCell.Value = "Capture";
                addressCell.Value = "";

                // If we just released the last captured slot, find the new last one.
                if (e.RowIndex == _lastCapturedSlotIndex)
                {
                    _lastCapturedSlotIndex = -1; // Reset
                    for (int i = _capturedStates.Length - 1; i >= 0; i--)
                    {
                        if (_capturedStates[i] != null)
                        {
                            _lastCapturedSlotIndex = i;
                            break;
                        }
                    }
                }
                UpdateScanButtonState();
                return;
            }

            // --- Capture Logic ---
            string addressText = addressCell.Value?.ToString();

            if (string.IsNullOrWhiteSpace(addressText))
            {
                MessageBox.Show($"Please enter a target address for Slot {e.RowIndex + 1}.", "Input Required", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            uint targetAddress;
            try
            {
                targetAddress = _manager.UnnormalizeAddress(addressText);
            }
            catch
            {
                MessageBox.Show("Invalid address format. Please use hexadecimal characters.", "Invalid Input", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // UI feedback
            statusCell.Value = "Capturing...";
            statusCell.Style.ForeColor = Color.Orange;
            dgvStates.Enabled = false;
            this.UseWaitCursor = true;

            byte[] memoryDump = await Task.Run(() => _manager.ReadMemory(_manager.MainMemoryStart, (int)_manager.MainMemorySize));

            // Restore UI
            dgvStates.Enabled = true;
            this.UseWaitCursor = false;

            if (memoryDump == null)
            {
                MessageBox.Show("Failed to read emulator memory.", "Capture Failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
                statusCell.Value = "Failed";
                statusCell.Style.ForeColor = Color.Red;
                return;
            }

            _capturedStates[e.RowIndex] = new ScanState
            {
                MemoryDump = memoryDump,
                Parameters = new ScanParameters { TargetAddress = targetAddress }
            };

            // Display the full, absolute address after capture for clarity.
            addressCell.Value = targetAddress.ToString("X8");
            statusCell.Value = "Captured";
            statusCell.Style.ForeColor = Color.Green;
            actionCell.Value = "Release";
            _lastCapturedSlotIndex = e.RowIndex; // This is now the last captured slot.

            UpdateScanButtonState();
        }

        private void btnClearAll_Click(object sender, EventArgs e)
        {
            for (int i = 0; i < 4; i++)
            {
                _capturedStates[i] = null;
                var row = dgvStates.Rows[i];
                row.Cells["colAddress"].Value = "";
                row.Cells["colStatus"].Value = "Empty";
                row.Cells["colStatus"].Style.ForeColor = SystemColors.WindowText;
                (row.Cells["colAction"] as DataGridViewButtonCell).Value = "Capture";
            }
            _lastCapturedSlotIndex = -1; // Reset last captured state memory.
            UpdateScanButtonState();
        }

        private void UpdateScanButtonState()
        {
            int capturedCount = _capturedStates.Count(s => s != null);
            btnScan.Enabled = capturedCount >= 2;
        }

        public ScanParameters GetScanParameters()
        {
            try
            {
                uint finalTarget;
                if (_lastCapturedSlotIndex != -1 && _capturedStates[_lastCapturedSlotIndex] != null)
                {
                    finalTarget = _capturedStates[_lastCapturedSlotIndex].Parameters.TargetAddress;
                }
                else
                {
                    // Fallback to the first valid state if the last one isn't available for some reason.
                    var firstState = _capturedStates.FirstOrDefault(s => s != null);
                    finalTarget = firstState?.Parameters.TargetAddress ?? 0;
                }

                return new ScanParameters
                {
                    MaxOffset = int.Parse(txtMaxOffset.Text.Replace("0x", ""), NumberStyles.HexNumber),
                    StaticBaseStart = _manager.UnnormalizeAddress(txtStaticStart.Text),
                    StaticBaseEnd = _manager.UnnormalizeAddress(txtStaticEnd.Text),
                    MaxLevel = (int)numMaxLevel.Value,
                    MaxResults = (int)numMaxResults.Value,
                    Use16ByteAlignment = false, // Hardcoded to false for this algorithm for maximum accuracy.
                    StopOnFirstPathFound = chkStopOnFirst.Checked,
                    // Pass the new FindAllPathLevels setting to the scan parameters.
                    FindAllPathLevels = chkFindAllLevels.Checked,
                    LimitCpuUsage = GlobalSettings.LimitCpuUsage,
                    CandidatesPerLevel = (int)numCandidatesPerLevel.Value,
                    FinalAddressTarget = finalTarget,
                    CapturedStates = _capturedStates.Where(s => s != null).ToList()
                };
            }
            catch (Exception ex)
            {
                MessageBox.Show("Invalid input: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                return null;
            }
        }

        public AppSettings GetCurrentSettings()
        {
            _currentSettings.MaxOffset = int.Parse(txtMaxOffset.Text.Replace("0x", ""), NumberStyles.HexNumber);
            _currentSettings.StaticAddressStart = txtStaticStart.Text;
            _currentSettings.StaticAddressEnd = txtStaticEnd.Text;
            _currentSettings.MaxLevel = (int)numMaxLevel.Value;
            _currentSettings.MaxResults = (int)numMaxResults.Value;
            // Do not overwrite the user's preference for 16-byte alignment from this form.
            _currentSettings.StopOnFirstPathFound = chkStopOnFirst.Checked;
            // Save the new FindAllPathLevels setting.
            _currentSettings.FindAllPathLevels = chkFindAllLevels.Checked;
            _currentSettings.CandidatesPerLevel = (int)numCandidatesPerLevel.Value;
            return _currentSettings;
        }

        private void btnScan_Click(object sender, EventArgs e)
        {
            if (GetScanParameters() != null)
            {
                DialogResult = DialogResult.OK;
                Close();
            }
        }
    }
}