using System.Reflection;
using System.Windows.Forms;

namespace PointerFinder2.UI.Controls
{
    // A static class containing extension methods for the DataGridView control.
    public static class DataGridViewExtensions
    {
        // An extension method to enable or disable double buffering on a DataGridView.
        // This is a common technique to reduce flickering and improve rendering performance when the grid has many rows or is updated frequently.
        // It works by accessing the private "DoubleBuffered" property of the control via reflection.
        public static void DoubleBuffered(this DataGridView dgv, bool setting)
        {
            // Get the PropertyInfo object for the non-public DoubleBuffered property.
            PropertyInfo pi = typeof(DataGridView).GetProperty("DoubleBuffered", BindingFlags.Instance | BindingFlags.NonPublic);
            if (pi != null)
            {
                // Set the value of the property.
                pi.SetValue(dgv, setting, null);
            }
        }
    }
}