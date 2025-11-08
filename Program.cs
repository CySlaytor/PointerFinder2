using System;
using System.Windows.Forms;
using PointerFinder2.UI;
using PointerFinder2.Properties;

namespace PointerFinder2
{
    internal static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            bool darkMode = Settings.Default.DarkModeEnabled;

            if (Settings.Default.FirstRun)
            {
                darkMode = false;
                Settings.Default.FirstRun = false;
                Settings.Default.Save();
            }

            ThemeBlack.SetEnabled(darkMode);

            string[] args = Environment.GetCommandLineArgs();
            Application.Run(new MainForm(args));
        }
    }
}
