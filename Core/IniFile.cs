using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace PointerFinder2.Core
{
    // A simple wrapper class for reading from and writing to INI configuration files.
    // It uses the Windows Kernel32 API for robust INI file handling.
    public class IniFile
    {
        private readonly string _path;

        // P/Invoke declaration for writing to a private profile (INI file).
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        private static extern long WritePrivateProfileString(string section, string key, string value, string filePath);

        // P/Invoke declaration for reading from a private profile (INI file).
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        private static extern int GetPrivateProfileString(string section, string key, string Default, StringBuilder retVal, int size, string filePath);

        // Constructor that takes the path to the INI file.
        public IniFile(string iniPath)
        {
            _path = new FileInfo(iniPath).FullName;
        }

        // Reads a value from the specified section and key.
        public string Read(string key, string section, string defaultValue = "")
        {
            var retVal = new StringBuilder(255);
            GetPrivateProfileString(section, key, defaultValue, retVal, 255, _path);
            return retVal.ToString();
        }

        // Writes a value to the specified section and key.
        public void Write(string key, string value, string section)
        {
            WritePrivateProfileString(section, key, value, _path);
        }
    }
}