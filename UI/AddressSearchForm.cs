using System;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace PointerFinder2.UI
{
    // A dialog form for entering a hexadecimal address to search for in the results.
    public partial class AddressSearchForm : BaseForm
    {
        // The validated hexadecimal address string entered by the user.
        public string SearchAddress { get; private set; }

        public AddressSearchForm(string currentSearch)
        {
            InitializeComponent();
            txtAddress.Text = currentSearch;
        }

        private void btnFind_Click(object sender, EventArgs e)
        {
            string text = txtAddress.Text.Trim();
            if (string.IsNullOrEmpty(text))
            {
                MessageBox.Show("Please enter an address to find.", "Input Required", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // Validate that the input contains only valid hexadecimal characters after removing the prefix.
            if (!Regex.IsMatch(text.Replace("0x", ""), @"^[0-9a-fA-F]+$"))
            {
                MessageBox.Show("Invalid address format. Please use hexadecimal characters (0-9, A-F).", "Invalid Input", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Normalize the address by removing the prefix and any leading zeros.
            // This ensures that searches for "492e24", "0x492e24", and "00492e24" are treated identically.
            string normalizedAddress = text.Replace("0x", "").TrimStart('0');
            // If the result is an empty string (e.g., from an input of "0" or "00"), default it back to "0".
            if (string.IsNullOrEmpty(normalizedAddress))
            {
                normalizedAddress = "0";
            }

            SearchAddress = normalizedAddress;
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void txtAddress_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                btnFind.PerformClick();
                e.SuppressKeyPress = true;
            }
        }

        private void AddressSearchForm_Load(object sender, EventArgs e)
        {

        }
    }
}