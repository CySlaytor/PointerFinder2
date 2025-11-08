using PointerFinder2.Properties;
using PointerFinder2.UI;
using System;
using System.Windows.Forms;

namespace PointerFinder2
{
    internal static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        private static void Main(string[] args)
        {
            // For .NET Core/5+ applications, this is recommended for high DPI support.
            Application.SetHighDpiMode(HighDpiMode.SystemAware);

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            // Initialize theme settings from user preferences.
            ThemeBlack.Initialize();

            // Pass command-line arguments to the MainForm to handle smart restarts.
            Application.Run(new MainForm(args));
        }
    }
}