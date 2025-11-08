using System;
using System.Windows.Forms;
using DarkModeForms;

namespace PointerFinder2.UI
{
    internal static class ThemeBlack
    {
        public static bool Enabled { get; private set; }

        public static void Apply(Form form)
        {
            if (!Enabled)
                return; 

            _ = new DarkModeCS(form)
            {
                ColorMode = DarkModeCS.DisplayMode.DarkMode
            };
        }

        public static void SetEnabled(bool enabled)
        {
            Enabled = enabled;
            Properties.Settings.Default.DarkModeEnabled = enabled;
            Properties.Settings.Default.Save();
        }

        public static void Initialize()
        {
            Enabled = Properties.Settings.Default.DarkModeEnabled;
        }
    }
}
