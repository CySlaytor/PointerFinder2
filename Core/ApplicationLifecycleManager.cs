using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    // Handles application lifecycle events like restart, memory management, and process monitoring.
    public class ApplicationLifecycleManager : IDisposable
    {
        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetProcessWorkingSetSize(IntPtr hProcess, UIntPtr dwMinimumWorkingSetSize, UIntPtr dwMaximumWorkingSetSize);

        public event EventHandler EmulatorProcessExited;

        private readonly Form _mainForm;
        private IEmulatorManager _currentManager;
        // Explicitly use System.Windows.Forms.Timer to resolve ambiguity.
        private readonly System.Windows.Forms.Timer _processMonitorTimer;
        private readonly DebugLogForm _logger = DebugLogForm.Instance;

        public ApplicationLifecycleManager(Form mainForm)
        {
            _mainForm = mainForm;
            _processMonitorTimer = new System.Windows.Forms.Timer { Interval = 2000 };
            _processMonitorTimer.Tick += ProcessMonitorTimer_Tick;
            _processMonitorTimer.Start();
        }

        public void SetCurrentManager(IEmulatorManager manager)
        {
            _currentManager = manager;
        }

        public (int pid, string target) ParseCommandLineArgs(string[] args)
        {
            int pidToAutoAttach = -1;
            string targetToAutoAttach = null;
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
                        pidToAutoAttach = int.Parse(arg.Substring(5));
                    }
                    else if (arg.StartsWith("/target:", StringComparison.OrdinalIgnoreCase))
                    {
                        targetToAutoAttach = arg.Substring(8);
                    }
                }

                if (isRestart && File.Exists("restart.tmp"))
                {
                    var settings = File.ReadAllLines("restart.tmp");
                    _mainForm.StartPosition = FormStartPosition.Manual;
                    _mainForm.Left = int.Parse(settings[0]);
                    _mainForm.Top = int.Parse(settings[1]);
                    _mainForm.Width = int.Parse(settings[2]);
                    _mainForm.Height = int.Parse(settings[3]);
                    File.Delete("restart.tmp");
                }
            }
            catch (Exception ex)
            {
                _logger.Log($"Error parsing command line args: {ex.Message}");
                return (-1, null);
            }
            return (pidToAutoAttach, targetToAutoAttach);
        }

        public void RestartApplication()
        {
            try
            {
                var args = new List<string> { "/restart" };
                if (_currentManager != null && _currentManager.IsAttached)
                {
                    // Correctly get the target from the active profile, not the manager type name.
                    var profile = EmulatorProfileRegistry.Profiles.FirstOrDefault(p => p.ManagerFactory().GetType() == _currentManager.GetType());
                    if (profile != null)
                    {
                        args.Add($"/target:{profile.Target}");
                    }
                    args.Add($"/pid:{_currentManager.EmulatorProcess.Id}");
                }

                string[] settings = {
                    _mainForm.Left.ToString(),
                    _mainForm.Top.ToString(),
                    _mainForm.Width.ToString(),
                    _mainForm.Height.ToString()
                };
                File.WriteAllLines("restart.tmp", settings);

                Process.Start(Application.ExecutablePath, string.Join(" ", args));
                Application.Exit();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to restart the application: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        public void PurgeMemory()
        {
            try
            {
                _logger.Log("--- Purging Application Memory ---");
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                using (Process currentProcess = Process.GetCurrentProcess())
                {
                    SetProcessWorkingSetSize(currentProcess.Handle, (UIntPtr)0xFFFFFFFF, (UIntPtr)0xFFFFFFFF);
                }
                _logger.Log("--- Memory Purge Complete ---");
            }
            catch (Exception ex)
            {
                _logger.Log($"[ERROR] Failed to purge memory: {ex.Message}");
            }
        }

        private void ProcessMonitorTimer_Tick(object sender, EventArgs e)
        {
            if (_currentManager != null && _currentManager.IsAttached)
            {
                try
                {
                    if (_currentManager.EmulatorProcess.HasExited)
                    {
                        _logger.Log($"Attached process '{_currentManager.EmulatorProcess.ProcessName}' has exited. Auto-detaching.");
                        EmulatorProcessExited?.Invoke(this, EventArgs.Empty);
                    }
                }
                catch
                {
                    _logger.Log("Error checking process status. Forcing detach.");
                    EmulatorProcessExited?.Invoke(this, EventArgs.Empty);
                }
            }
        }

        public void Dispose()
        {
            _processMonitorTimer.Stop();
            _processMonitorTimer.Dispose();
        }
    }
}