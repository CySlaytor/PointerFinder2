using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace PointerFinder2.Core
{
    // A simple wrapper for reading from and writing to INI configuration files
    // using the Windows API for maximum compatibility.
    public class IniFile
    {
        private readonly string _path;

        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        private static extern long WritePrivateProfileString(string section, string key, string value, string filePath);

        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        private static extern int GetPrivateProfileString(string section, string key, string Default, StringBuilder retVal, int size, string filePath);

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