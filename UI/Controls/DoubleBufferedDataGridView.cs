using System.Reflection;
using System.Windows.Forms;

namespace PointerFinder2.UI.Controls
{
    public static class DataGridViewExtensions
    {
        // This is an extension method to enable DoubleBuffering on a DataGridView
        public static void DoubleBuffered(this DataGridView dgv, bool setting)
        {
            PropertyInfo pi = typeof(DataGridView).GetProperty("DoubleBuffered", BindingFlags.Instance | BindingFlags.NonPublic);
            if (pi != null)
            {
                pi.SetValue(dgv, setting, null);
            }
        }
    }
}