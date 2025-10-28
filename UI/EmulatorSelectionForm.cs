using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Windows.Forms;
using PointerFinder2.UI;

namespace PointerFinder2
{
    // A form that is displayed when multiple supported emulators or instances are running.
    // It prompts the user to select which specific process they want to attach to.
    public partial class EmulatorSelectionForm : BaseForm
    {
        // Public properties to hold the user's final selection.
        public EmulatorProfile SelectedProfile { get; private set; }
        public Process SelectedProcess { get; private set; }

        // A flattened list of all found emulator instances to display.
        private readonly List<(EmulatorProfile profile, Process process)> _foundInstances;

        public EmulatorSelectionForm(Dictionary<EmulatorProfile, List<Process>> foundProcessMap)
        {
            InitializeComponent();
            _foundInstances = new List<(EmulatorProfile, Process)>();

            // Flatten the dictionary into a simple list for the ListBox.
            foreach (var kvp in foundProcessMap)
            {
                foreach (var process in kvp.Value)
                {
                    _foundInstances.Add((kvp.Key, process));
                }
            }
        }

        // When the form loads, populate the list box with user-friendly names for each process.
        private void EmulatorSelectionForm_Load(object sender, EventArgs e)
        {
            // Create a display string for each instance, including Name, PID, and Window Title.
            var displayItems = _foundInstances.Select(instance =>
            {
                string title = string.IsNullOrWhiteSpace(instance.process.MainWindowTitle) ? "" : $" - {instance.process.MainWindowTitle}";
                return $"{instance.profile.Name} (PID: {instance.process.Id}{title})";
            }).ToList();

            listBoxEmulators.DataSource = displayItems;

            // Default to selecting the first item in the list.
            if (listBoxEmulators.Items.Count > 0)
            {
                listBoxEmulators.SelectedIndex = 0;
            }
        }

        // When the user clicks OK, set the selected profile and process and close the dialog.
        private void btnOK_Click(object sender, EventArgs e)
        {
            if (listBoxEmulators.SelectedIndex != -1)
            {
                // Find the selected tuple from our original list based on the index.
                var selection = _foundInstances[listBoxEmulators.SelectedIndex];
                SelectedProfile = selection.profile;
                SelectedProcess = selection.process;

                this.DialogResult = DialogResult.OK;
                this.Close();
            }
            else
            {
                MessageBox.Show("Please select an emulator instance.", "Selection Required", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                this.DialogResult = DialogResult.None; // Prevent the dialog from closing.
            }
        }
    }
}