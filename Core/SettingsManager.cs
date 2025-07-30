using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    // A data class to hold the application settings for a specific emulator profile.
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

    // A static class responsible for loading from and saving settings to the settings.ini file.
    public static class SettingsManager
    {
        private static readonly string _settingsFile = Path.Combine(Application.StartupPath, "settings.ini");

        // A dedicated method to save only the global debug settings.
        // This is called from the DebugOptionsForm to avoid rewriting the entire file.
        public static void SaveDebugSettingsOnly()
        {
            var logger = DebugLogForm.Instance;
            if (DebugSettings.LogLiveScan) logger.Log("Saving debug settings only.");
            var ini = new IniFile(_settingsFile);

            // Debug settings are global, not per-profile.
            ini.Write("LogLiveScan", DebugSettings.LogLiveScan.ToString(), "Debug");
            ini.Write("LogFilterValidation", DebugSettings.LogFilterValidation.ToString(), "Debug");
            ini.Write("LogRefineScan", DebugSettings.LogRefineScan.ToString(), "Debug");
            if (DebugSettings.LogLiveScan) logger.Log("Debug settings saved successfully.");
        }

        // Saves the application settings for a specific emulator target, as well as global debug settings.
        public static void Save(EmulatorTarget target, AppSettings settings)
        {
            var logger = DebugLogForm.Instance;
            if (DebugSettings.LogLiveScan) logger.Log($"Saving settings for profile: {target}");
            var ini = new IniFile(_settingsFile);
            string section = $"Scanner_{target}";

            // Write the settings for the specified emulator profile.
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

            // Also save the global debug settings every time.
            SaveDebugSettingsOnly();
            if (DebugSettings.LogLiveScan) logger.Log("Settings saved successfully.");
        }


        // Loads the application settings for a specific emulator target from the INI file.
        // It also loads the global debug settings.
        public static AppSettings Load(EmulatorTarget target, AppSettings defaultSettings)
        {
            var logger = DebugLogForm.Instance;
            if (DebugSettings.LogLiveScan) logger.Log($"Loading settings for profile: {target}");
            var settings = new AppSettings();
            var ini = new IniFile(_settingsFile);
            string section = $"Scanner_{target}";

            if (!File.Exists(_settingsFile))
            {
                if (DebugSettings.LogLiveScan) logger.Log("Settings file not found, using default settings.");
                return defaultSettings;
            }

            // --- Load scanner settings with robust TryParse to prevent crashes from corrupted INI file ---
            settings.LastTargetAddress = ini.Read("LastTargetAddress", section, defaultSettings.LastTargetAddress);

            if (!int.TryParse(ini.Read("MaxOffset", section, defaultSettings.MaxOffset.ToString()), out int maxOffset))
                maxOffset = defaultSettings.MaxOffset;
            settings.MaxOffset = maxOffset;

            if (!int.TryParse(ini.Read("MaxLevel", section, defaultSettings.MaxLevel.ToString()), out int maxLevel))
                maxLevel = defaultSettings.MaxLevel;
            settings.MaxLevel = maxLevel;

            if (!int.TryParse(ini.Read("MaxResults", section, defaultSettings.MaxResults.ToString()), out int maxResults))
                maxResults = defaultSettings.MaxResults;
            settings.MaxResults = maxResults;

            settings.StaticAddressStart = ini.Read("StaticAddressStart", section, defaultSettings.StaticAddressStart);
            settings.StaticAddressEnd = ini.Read("StaticAddressEnd", section, defaultSettings.StaticAddressEnd);

            if (!bool.TryParse(ini.Read("AnalyzeStructures", section, defaultSettings.AnalyzeStructures.ToString()), out bool analyzeStructures))
                analyzeStructures = defaultSettings.AnalyzeStructures;
            settings.AnalyzeStructures = analyzeStructures;

            if (!bool.TryParse(ini.Read("ScanForStructureBase", section, defaultSettings.ScanForStructureBase.ToString()), out bool scanForStructureBase))
                scanForStructureBase = defaultSettings.ScanForStructureBase;
            settings.ScanForStructureBase = scanForStructureBase;

            if (!bool.TryParse(ini.Read("Use16ByteAlignment", section, defaultSettings.Use16ByteAlignment.ToString()), out bool use16ByteAlignment))
                use16ByteAlignment = defaultSettings.Use16ByteAlignment;
            settings.Use16ByteAlignment = use16ByteAlignment;

            if (!int.TryParse(ini.Read("MaxNegativeOffset", section, defaultSettings.MaxNegativeOffset.ToString()), out int maxNegativeOffset))
                maxNegativeOffset = defaultSettings.MaxNegativeOffset;
            settings.MaxNegativeOffset = maxNegativeOffset;


            // --- Load global debug settings with robust TryParse ---
            if (!bool.TryParse(ini.Read("LogLiveScan", "Debug", DebugSettings.LogLiveScan.ToString()), out bool logLiveScan))
                logLiveScan = false;
            DebugSettings.LogLiveScan = logLiveScan;

            if (!bool.TryParse(ini.Read("LogFilterValidation", "Debug", DebugSettings.LogFilterValidation.ToString()), out bool logFilterValidation))
                logFilterValidation = false;
            DebugSettings.LogFilterValidation = logFilterValidation;

            if (!bool.TryParse(ini.Read("LogRefineScan", "Debug", DebugSettings.LogRefineScan.ToString()), out bool logRefineScan))
                logRefineScan = false;
            DebugSettings.LogRefineScan = logRefineScan;

            if (DebugSettings.LogLiveScan) logger.Log("Settings loaded successfully.");
            return settings;
        }
    }
}