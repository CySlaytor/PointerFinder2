using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace PointerFinder2.Core
{
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

        public string Read(string key, string section, string defaultValue = "")
        {
            var retVal = new StringBuilder(255);
            GetPrivateProfileString(section, key, defaultValue, retVal, 255, _path);
            return retVal.ToString();
        }

        public void Write(string key, string value, string section)
        {
            WritePrivateProfileString(section, key, value, _path);
        }
    }
}