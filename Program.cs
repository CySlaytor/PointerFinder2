using System;
using System.Windows.Forms;

namespace PointerFinder2
{
    internal static class Program
    {
        // The main entry point for the application.
        [STAThread]
        private static void Main(string[] args)
        {
            ApplicationConfiguration.Initialize();
            // Pass command-line arguments to the MainForm to handle smart restarts.
            Application.Run(new MainForm(args));
        }
    }
}