using System;
using System.Collections.Concurrent;
using System.Text;
using System.Windows.Forms;
using PointerFinder2.DataModels;

namespace PointerFinder2
{
    // A form that displays real-time debug and logging messages from the application.
    // It is implemented as a singleton to ensure only one instance exists.
    public partial class DebugLogForm : Form
    {
        // The single, shared instance of the form.
        private static DebugLogForm _instance;
        // A thread-safe queue to hold incoming log messages from various threads.
        private readonly ConcurrentQueue<string> _logQueue = new ConcurrentQueue<string>();
        // A UI timer to periodically process the queue and update the textbox.
        private readonly System.Windows.Forms.Timer _logTimer = new System.Windows.Forms.Timer();

        // Public accessor for the singleton instance.
        public static DebugLogForm Instance
        {
            get
            {
                // Create a new instance if one doesn't exist or has been disposed.
                if (_instance == null || _instance.IsDisposed)
                {
                    _instance = new DebugLogForm();
                }
                return _instance;
            }
        }

        // Private constructor to enforce the singleton pattern.
        private DebugLogForm()
        {
            InitializeComponent();
            _logTimer.Interval = 100; // Update the log 10 times per second.
            _logTimer.Tick += LogTimer_Tick;
            _logTimer.Start();
        }

        // Public method for any part of the application to add a message to the log.
        public void Log(string message)
        {
            if (DebugSettings.IsLoggingPaused) return;
            // Add a timestamp and enqueue the formatted message.
            _logQueue.Enqueue($"[{DateTime.Now:HH:mm:ss.fff}] {message}{Environment.NewLine}");
        }

        // The timer's tick event handler, which runs on the UI thread.
        private void LogTimer_Tick(object sender, EventArgs e)
        {
            if (_logQueue.IsEmpty) return;

            // Use a StringBuilder for efficient string concatenation.
            var sb = new StringBuilder();
            // Dequeue all pending messages and append them.
            while (_logQueue.TryDequeue(out var message))
            {
                sb.Append(message);
            }
            // Update the textbox with the new messages.
            txtLog.AppendText(sb.ToString());
        }

        // Override the form closing event to simply hide the window instead of disposing of it.
        // This keeps the singleton instance alive for the duration of the application.
        private void DebugLogForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                Hide();
            }
        }

        // Event handler for the "Clear" button.
        private void btnClear_Click(object sender, EventArgs e)
        {
            // Empty the queue and clear the textbox.
            while (_logQueue.TryDequeue(out _)) { }
            txtLog.Clear();
        }

        // Event handler for the "Pause/Resume Logging" button.
        private void btnToggleLogging_Click(object sender, EventArgs e)
        {
            DebugSettings.IsLoggingPaused = !DebugSettings.IsLoggingPaused;
            btnToggleLogging.Text = DebugSettings.IsLoggingPaused ? "Resume Logging" : "Pause Logging";
        }
    }
}