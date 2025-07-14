using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace PointerFinder2
{
    public partial class EmulatorSelectionForm : Form
    {
        public EmulatorProfile SelectedProfile { get; private set; }
        private readonly List<EmulatorProfile> _foundProfiles;

        public EmulatorSelectionForm(List<EmulatorProfile> foundProfiles)
        {
            InitializeComponent();
            _foundProfiles = foundProfiles;
        }

        private void EmulatorSelectionForm_Load(object sender, EventArgs e)
        {
            listBoxEmulators.DataSource = _foundProfiles.Select(p => p.Name).ToList();
            if (listBoxEmulators.Items.Count > 0)
            {
                listBoxEmulators.SelectedIndex = 0;
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            if (listBoxEmulators.SelectedItem != null)
            {
                string selectedName = listBoxEmulators.SelectedItem.ToString();
                SelectedProfile = _foundProfiles.First(p => p.Name == selectedName);
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
            else
            {
                MessageBox.Show("Please select an emulator.", "Selection Required", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                this.DialogResult = DialogResult.None;
            }
        }
    }
}