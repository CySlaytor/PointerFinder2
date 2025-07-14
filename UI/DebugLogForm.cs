using System;
using System.Collections.Concurrent;
using System.Text;
using System.Windows.Forms;
using PointerFinder2.DataModels;

namespace PointerFinder2
{
    public partial class DebugLogForm : Form
    {
        private static DebugLogForm _instance;
        private readonly ConcurrentQueue<string> _logQueue = new ConcurrentQueue<string>();
        private readonly System.Windows.Forms.Timer _logTimer = new System.Windows.Forms.Timer();

        public static DebugLogForm Instance
        {
            get
            {
                if (_instance == null || _instance.IsDisposed)
                {
                    _instance = new DebugLogForm();
                }
                return _instance;
            }
        }

        private DebugLogForm()
        {
            InitializeComponent();
            _logTimer.Interval = 100; // Update the log 10 times per second
            _logTimer.Tick += LogTimer_Tick;
            _logTimer.Start();
        }

        public void Log(string message)
        {
            if (DebugSettings.IsLoggingPaused) return;
            _logQueue.Enqueue($"[{DateTime.Now:HH:mm:ss.fff}] {message}{Environment.NewLine}");
        }

        private void LogTimer_Tick(object sender, EventArgs e)
        {
            if (_logQueue.IsEmpty) return;

            var sb = new StringBuilder();
            while (_logQueue.TryDequeue(out var message))
            {
                sb.Append(message);
            }
            txtLog.AppendText(sb.ToString());
        }

        private void DebugLogForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                Hide();
            }
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            while (_logQueue.TryDequeue(out _)) { }
            txtLog.Clear();
        }

        private void btnToggleLogging_Click(object sender, EventArgs e)
        {
            DebugSettings.IsLoggingPaused = !DebugSettings.IsLoggingPaused;
            btnToggleLogging.Text = DebugSettings.IsLoggingPaused ? "Resume Logging" : "Pause Logging";
        }
    }
}