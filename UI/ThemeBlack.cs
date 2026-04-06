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

        public static void Initialize()
        {
            Enabled = Properties.Settings.Default.DarkModeEnabled;
        }
    }
}