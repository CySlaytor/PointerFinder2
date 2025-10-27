using System;
using System.Collections.Concurrent;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using PointerFinder2.DataModels;
using PointerFinder2.Properties;

namespace PointerFinder2
{
    // A form that displays real-time debug and logging messages from the application.
    // It is implemented as a singleton to ensure only one instance exists and that
    // logs from any thread can be safely displayed.
    public partial class DebugLogForm : Form
    {
        // The single, shared instance of the form.
        private static DebugLogForm _instance;
        // A thread-safe queue to hold incoming log messages from various threads.
        private readonly ConcurrentQueue<string> _logQueue = new ConcurrentQueue<string>();
        // A UI timer to periodically process the queue and update the textbox, preventing UI lockup.
        private readonly System.Windows.Forms.Timer _logTimer = new System.Windows.Forms.Timer();

        // Safeguards to prevent UI hanging from log floods.
        private const int MAX_QUEUE_SIZE = 50000;
        private const int MAX_TEXTBOX_LENGTH = 1_000_000;
        private const int TEXTBOX_TRIM_LENGTH = 500_000;
        private bool _isTrimming = false;

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
        private void DebugLogForm_Load(object sender, EventArgs e)
        {
            if (Settings.Default.DebugLogSize.Width > 0 && Settings.Default.DebugLogSize.Height > 0)
            {
                this.StartPosition = FormStartPosition.Manual;
                Point location = Settings.Default.DebugLogLocation;
                Size size = Settings.Default.DebugLogSize;
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
                }
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

            // If the queue is getting too large, start dropping old messages to prevent memory explosion.
            if (_logQueue.Count > MAX_QUEUE_SIZE)
            {
                _logQueue.TryDequeue(out _);
            }

            // Add a timestamp and enqueue the formatted message.
            _logQueue.Enqueue($"[{DateTime.Now:HH:mm:ss.fff}] {message}{Environment.NewLine}");
        }

        // Allows external code (like the MainForm's purge logic) to clear the log.
        public void ClearLog()
        {
            // Empty the queue to release memory from held strings.
            while (_logQueue.TryDequeue(out _)) { }

            // If the handle is created (form is visible or has been shown), clear the textbox.
            if (this.IsHandleCreated)
            {
                // Use Invoke to ensure thread safety if called from a non-UI thread.
                this.Invoke((Action)(() => txtLog.Clear()));
            }
        }

        // The timer's tick event handler, which runs on the UI thread.
        private void LogTimer_Tick(object sender, EventArgs e)
        {
            if (_logQueue.IsEmpty || _isTrimming) return;

            // Trim the textbox text if it gets too long, to keep the UI responsive.
            if (txtLog.TextLength > MAX_TEXTBOX_LENGTH)
            {
                _isTrimming = true; // Prevent re-entry while trimming
                // Use AppendText/Clear for better performance with large textboxes
                string textToKeep = txtLog.Text.Substring(txtLog.TextLength - TEXTBOX_TRIM_LENGTH);
                txtLog.Clear();
                txtLog.AppendText(textToKeep);
                _isTrimming = false;
            }

            // Use a StringBuilder for efficient string concatenation when batching messages.
            var sb = new StringBuilder();
            int messagesToProcess = 0;
            // Process messages in chunks to prevent the UI thread from being blocked for too long.
            while (_logQueue.TryDequeue(out var message) && messagesToProcess < 1000)
            {
                sb.Append(message);
                messagesToProcess++;
            }
            txtLog.AppendText(sb.ToString());
            txtLog.SelectionStart = txtLog.TextLength;
            txtLog.ScrollToCaret();
        }

        // Override the form closing event to simply hide the window instead of disposing it.
        // This keeps the singleton instance alive for the duration of the application.
        private void DebugLogForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Save window position and size before potentially canceling the close event.
            Settings.Default.DebugLogLocation = this.Location;
            Settings.Default.DebugLogSize = this.Size;
            Settings.Default.Save();

            if (e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                Hide();
            }
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            ClearLog();
        }

        private void btnToggleLogging_Click(object sender, EventArgs e)
        {
            DebugSettings.IsLoggingPaused = !DebugSettings.IsLoggingPaused;
            btnToggleLogging.Text = DebugSettings.IsLoggingPaused ? "Resume Logging" : "Pause Logging";
        }
    }
}