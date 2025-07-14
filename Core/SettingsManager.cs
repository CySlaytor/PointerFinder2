using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System.IO;
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    public class AppSettings
    {
        public string LastTargetAddress { get; set; } = "0";
        public int MaxOffset { get; set; }
        public int MaxLevel { get; set; }
        public int MaxResults { get; set; }
        public string StaticAddressStart { get; set; }
        public string StaticAddressEnd { get; set; }
        public bool AnalyzeStructures { get; set; }
        public bool ScanForStructureBase { get; set; }
        public int MaxNegativeOffset { get; set; }
        public bool Use16ByteAlignment { get; set; }
    }

    public static class SettingsManager
    {
        private static readonly string _settingsFile = Path.Combine(Application.StartupPath, "settings.ini");

        public static void SaveDebugSettingsOnly()
        {
            var logger = DebugLogForm.Instance;
            logger.Log("Saving debug settings only.");
            var ini = new IniFile(_settingsFile);

            // Debug settings are global, not per-profile
            ini.Write("LogLiveScan", DebugSettings.LogLiveScan.ToString(), "Debug");
            ini.Write("LogFilterValidation", DebugSettings.LogFilterValidation.ToString(), "Debug");
            ini.Write("LogRefineScan", DebugSettings.LogRefineScan.ToString(), "Debug");
            logger.Log("Debug settings saved successfully.");
        }

        public static void Save(EmulatorTarget target, AppSettings settings)
        {
            var logger = DebugLogForm.Instance;
            logger.Log($"Saving settings for profile: {target}");
            var ini = new IniFile(_settingsFile);
            string section = $"Scanner_{target}";

            if (settings != null)
            {
                ini.Write("LastTargetAddress", settings.LastTargetAddress, section);
                ini.Write("MaxOffset", settings.MaxOffset.ToString(), section);
                ini.Write("MaxLevel", settings.MaxLevel.ToString(), section);
                ini.Write("MaxResults", settings.MaxResults.ToString(), section);
                ini.Write("StaticAddressStart", settings.StaticAddressStart, section);
                ini.Write("StaticAddressEnd", settings.StaticAddressEnd, section);
                ini.Write("AnalyzeStructures", settings.AnalyzeStructures.ToString(), section);
                ini.Write("ScanForStructureBase", settings.ScanForStructureBase.ToString(), section);
                ini.Write("Use16ByteAlignment", settings.Use16ByteAlignment.ToString(), section);
                ini.Write("MaxNegativeOffset", settings.MaxNegativeOffset.ToString(), section);
            }

            // Debug settings are global, not per-profile
            ini.Write("LogLiveScan", DebugSettings.LogLiveScan.ToString(), "Debug");
            ini.Write("LogFilterValidation", DebugSettings.LogFilterValidation.ToString(), "Debug");
            ini.Write("LogRefineScan", DebugSettings.LogRefineScan.ToString(), "Debug");
            logger.Log("Settings saved successfully.");
        }

        public static AppSettings Load(EmulatorTarget target, AppSettings defaultSettings)
        {
            var logger = DebugLogForm.Instance;
            logger.Log($"Loading settings for profile: {target}");
            var settings = new AppSettings();
            var ini = new IniFile(_settingsFile);
            string section = $"Scanner_{target}";

            if (!File.Exists(_settingsFile))
            {
                logger.Log("Settings file not found, using default settings.");
                return defaultSettings;
            }

            // Load scanner settings from the profile's section, using defaults as a fallback
            settings.LastTargetAddress = ini.Read("LastTargetAddress", section, defaultSettings.LastTargetAddress);
            settings.MaxOffset = int.Parse(ini.Read("MaxOffset", section, defaultSettings.MaxOffset.ToString()));
            settings.MaxLevel = int.Parse(ini.Read("MaxLevel", section, defaultSettings.MaxLevel.ToString()));
            settings.MaxResults = int.Parse(ini.Read("MaxResults", section, defaultSettings.MaxResults.ToString()));
            settings.StaticAddressStart = ini.Read("StaticAddressStart", section, defaultSettings.StaticAddressStart);
            settings.StaticAddressEnd = ini.Read("StaticAddressEnd", section, defaultSettings.StaticAddressEnd);
            settings.AnalyzeStructures = bool.Parse(ini.Read("AnalyzeStructures", section, defaultSettings.AnalyzeStructures.ToString()));
            settings.ScanForStructureBase = bool.Parse(ini.Read("ScanForStructureBase", section, defaultSettings.ScanForStructureBase.ToString()));
            settings.Use16ByteAlignment = bool.Parse(ini.Read("Use16ByteAlignment", section, defaultSettings.Use16ByteAlignment.ToString()));
            settings.MaxNegativeOffset = int.Parse(ini.Read("MaxNegativeOffset", section, defaultSettings.MaxNegativeOffset.ToString()));

            // Load global debug settings
            DebugSettings.LogLiveScan = bool.Parse(ini.Read("LogLiveScan", "Debug", DebugSettings.LogLiveScan.ToString()));
            DebugSettings.LogFilterValidation = bool.Parse(ini.Read("LogFilterValidation", "Debug", DebugSettings.LogFilterValidation.ToString()));
            DebugSettings.LogRefineScan = bool.Parse(ini.Read("LogRefineScan", "Debug", DebugSettings.LogRefineScan.ToString()));

            logger.Log("Settings loaded successfully.");
            return settings;
        }
    }
}