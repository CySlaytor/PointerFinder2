using PointerFinder2.DataModels;
using System;
using System.IO;
using System.Text.Json;
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    // Handles saving and loading session files.
    public class SessionManager
    {
        private readonly DebugLogForm _logger = DebugLogForm.Instance;

        public bool SaveSession(SessionData sessionData, IWin32Window owner)
        {
            using (var sfd = new SaveFileDialog())
            {
                sfd.Filter = "Pointer Finder Session (*.pfs)|*.pfs";
                sfd.Title = "Save Session";
                sfd.FileName = $"session_{DateTime.Now:yyyyMMdd_HHmmss}.pfs";

                if (sfd.ShowDialog(owner) == DialogResult.OK)
                {
                    try
                    {
                        var options = new JsonSerializerOptions { WriteIndented = true };
                        string json = JsonSerializer.Serialize(sessionData, options);
                        File.WriteAllText(sfd.FileName, json);
                        return true;
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show($"Failed to save session: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        _logger.Log($"[ERROR] Session save failed: {ex.Message}");
                        return false;
                    }
                }
            }
            return false;
        }

        public SessionData LoadSession(IWin32Window owner)
        {
            using (var ofd = new OpenFileDialog())
            {
                ofd.Filter = "Pointer Finder Session (*.pfs)|*.pfs|All files (*.*)|*.*";
                ofd.Title = "Load Session";
                if (ofd.ShowDialog(owner) == DialogResult.OK)
                {
                    try
                    {
                        string json = File.ReadAllText(ofd.FileName);
                        return JsonSerializer.Deserialize<SessionData>(json);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show($"Failed to load session: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        _logger.Log($"[ERROR] Session load failed: {ex.Message}");
                    }
                }
            }
            return null;
        }
    }
}