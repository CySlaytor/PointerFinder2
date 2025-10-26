using PointerFinder2.DataModels;
using PointerFinder2.Emulators;
using System;
using System.IO;
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    // Handles loading and saving all settings to the settings.ini file.
    public static class SettingsManager
    {
        private static readonly string _settingsFile = Path.Combine(Application.StartupPath, "settings.ini");

        // Saves only the global app settings (like sound preferences).
        public static void SaveGlobalSettingsOnly()
        {
            var logger = DebugLogForm.Instance;
            if (DebugSettings.LogLiveScan) logger.Log("Saving global settings only.");
            var ini = new IniFile(_settingsFile);
            ini.Write("UseWindowsDefaultSound", GlobalSettings.UseWindowsDefaultSound.ToString(), "Global");
            ini.Write("LimitCpuUsage", GlobalSettings.LimitCpuUsage.ToString(), "Global");

            // Save new Code Note settings
            ini.Write("CodeNotePrefix", GlobalSettings.CodeNotePrefix, "CodeNotes");
            ini.Write("CodeNoteSuffix", GlobalSettings.CodeNoteSuffix, "CodeNotes");
            ini.Write("CodeNoteAlignSuffixes", GlobalSettings.CodeNoteAlignSuffixes.ToString(), "CodeNotes");
            ini.Write("CodeNoteSuffixOnLastLineOnly", GlobalSettings.CodeNoteSuffixOnLastLineOnly.ToString(), "CodeNotes");

            if (DebugSettings.LogLiveScan) logger.Log("Global settings saved successfully.");
        }

        // Saves only the debug logging settings.
        public static void SaveDebugSettingsOnly()
        {
            var logger = DebugLogForm.Instance;
            if (DebugSettings.LogLiveScan) logger.Log("Saving debug settings only.");
            var ini = new IniFile(_settingsFile);
            ini.Write("LogLiveScan", DebugSettings.LogLiveScan.ToString(), "Debug");
            ini.Write("LogFilterValidation", DebugSettings.LogFilterValidation.ToString(), "Debug");
            ini.Write("LogRefineScan", DebugSettings.LogRefineScan.ToString(), "Debug");
            ini.Write("LogStateBasedScanDetails", DebugSettings.LogStateBasedScanDetails.ToString(), "Debug");
            if (DebugSettings.LogLiveScan) logger.Log("Debug settings saved successfully.");
        }

        // Saves settings for a specific emulator, plus all global settings.
        public static void Save(EmulatorTarget target, AppSettings settings)
        {
            var logger = DebugLogForm.Instance;
            if (DebugSettings.LogLiveScan) logger.Log($"Saving settings for profile: {target}");
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
                ini.Write("ScanForStructureBase", settings.ScanForStructureBase.ToString(), section);
                ini.Write("Use16ByteAlignment", settings.Use16ByteAlignment.ToString(), section);
                ini.Write("MaxNegativeOffset", settings.MaxNegativeOffset.ToString(), section);
                ini.Write("UseSliderRange", settings.UseSliderRange.ToString(), section);
                ini.Write("StopOnFirstPathFound", settings.StopOnFirstPathFound.ToString(), section);
                ini.Write("FindAllPathLevels", settings.FindAllPathLevels.ToString(), section);
                ini.Write("CandidatesPerLevel", settings.CandidatesPerLevel.ToString(), section);
            }

            SaveGlobalSettingsOnly();
            SaveDebugSettingsOnly();
            if (DebugSettings.LogLiveScan) logger.Log("Settings saved successfully.");
        }

        //New public method to be called once on application startup.
        public static void InitializeGlobalSettings()
        {
            if (!File.Exists(_settingsFile)) return;
            var ini = new IniFile(_settingsFile);
            LoadGlobalAndDebugSections(ini);
        }

        //New private helper to contain the shared loading logic for global settings.
        private static void LoadGlobalAndDebugSections(IniFile ini)
        {
            // Load global settings
            if (!bool.TryParse(ini.Read("UseWindowsDefaultSound", "Global", GlobalSettings.UseWindowsDefaultSound.ToString()), out bool useDefaultSound)) useDefaultSound = false;
            GlobalSettings.UseWindowsDefaultSound = useDefaultSound;
            if (!bool.TryParse(ini.Read("LimitCpuUsage", "Global", GlobalSettings.LimitCpuUsage.ToString()), out bool limitCpuUsage)) limitCpuUsage = false;
            GlobalSettings.LimitCpuUsage = limitCpuUsage;

            // Load Code Note settings
            GlobalSettings.CodeNotePrefix = ini.Read("CodeNotePrefix", "CodeNotes", ".");
            GlobalSettings.CodeNoteSuffix = ini.Read("CodeNoteSuffix", "CodeNotes", " |");
            if (!bool.TryParse(ini.Read("CodeNoteAlignSuffixes", "CodeNotes", "True"), out bool align)) align = true;
            GlobalSettings.CodeNoteAlignSuffixes = align;
            if (!bool.TryParse(ini.Read("CodeNoteSuffixOnLastLineOnly", "CodeNotes", "False"), out bool suffixLastLine)) suffixLastLine = false;
            GlobalSettings.CodeNoteSuffixOnLastLineOnly = suffixLastLine;

            // Load debug settings
            if (!bool.TryParse(ini.Read("LogLiveScan", "Debug", DebugSettings.LogLiveScan.ToString()), out bool logLiveScan)) logLiveScan = false;
            DebugSettings.LogLiveScan = logLiveScan;
            if (!bool.TryParse(ini.Read("LogFilterValidation", "Debug", DebugSettings.LogFilterValidation.ToString()), out bool logFilterValidation)) logFilterValidation = false;
            DebugSettings.LogFilterValidation = logFilterValidation;
            if (!bool.TryParse(ini.Read("LogRefineScan", "Debug", DebugSettings.LogRefineScan.ToString()), out bool logRefineScan)) logRefineScan = false;
            DebugSettings.LogRefineScan = logRefineScan;
            if (!bool.TryParse(ini.Read("LogStateBasedScanDetails", "Debug", DebugSettings.LogStateBasedScanDetails.ToString()), out bool logStateBasedScanDetails)) logStateBasedScanDetails = false;
            DebugSettings.LogStateBasedScanDetails = logStateBasedScanDetails;
        }


        // Loads settings for a specific emulator and all global settings from the INI file.
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

            // Load scanner settings with robust TryParse to prevent crashes from a corrupted INI.
            settings.LastTargetAddress = ini.Read("LastTargetAddress", section, defaultSettings.LastTargetAddress);
            if (!int.TryParse(ini.Read("MaxOffset", section, defaultSettings.MaxOffset.ToString()), out int maxOffset)) maxOffset = defaultSettings.MaxOffset;
            settings.MaxOffset = maxOffset;
            if (!int.TryParse(ini.Read("MaxLevel", section, defaultSettings.MaxLevel.ToString()), out int maxLevel)) maxLevel = defaultSettings.MaxLevel;
            settings.MaxLevel = maxLevel;
            if (!int.TryParse(ini.Read("MaxResults", section, defaultSettings.MaxResults.ToString()), out int maxResults)) maxResults = defaultSettings.MaxResults;
            settings.MaxResults = maxResults;
            settings.StaticAddressStart = ini.Read("StaticAddressStart", section, defaultSettings.StaticAddressStart);
            settings.StaticAddressEnd = ini.Read("StaticAddressEnd", section, defaultSettings.StaticAddressEnd);
            if (!bool.TryParse(ini.Read("ScanForStructureBase", section, defaultSettings.ScanForStructureBase.ToString()), out bool scanForStructureBase)) scanForStructureBase = defaultSettings.ScanForStructureBase;
            settings.ScanForStructureBase = scanForStructureBase;
            if (!bool.TryParse(ini.Read("Use16ByteAlignment", section, defaultSettings.Use16ByteAlignment.ToString()), out bool use16ByteAlignment)) use16ByteAlignment = defaultSettings.Use16ByteAlignment;
            settings.Use16ByteAlignment = use16ByteAlignment;
            if (!int.TryParse(ini.Read("MaxNegativeOffset", section, defaultSettings.MaxNegativeOffset.ToString()), out int maxNegativeOffset)) maxNegativeOffset = defaultSettings.MaxNegativeOffset;
            settings.MaxNegativeOffset = maxNegativeOffset;
            if (!bool.TryParse(ini.Read("UseSliderRange", section, defaultSettings.UseSliderRange.ToString()), out bool useSliderRange)) useSliderRange = defaultSettings.UseSliderRange;
            settings.UseSliderRange = useSliderRange;
            if (!bool.TryParse(ini.Read("StopOnFirstPathFound", section, defaultSettings.StopOnFirstPathFound.ToString()), out bool stopOnFirst)) stopOnFirst = defaultSettings.StopOnFirstPathFound;
            settings.StopOnFirstPathFound = stopOnFirst;
            if (!bool.TryParse(ini.Read("FindAllPathLevels", section, defaultSettings.FindAllPathLevels.ToString()), out bool findAllLevels)) findAllLevels = defaultSettings.FindAllPathLevels;
            settings.FindAllPathLevels = findAllLevels;
            if (!int.TryParse(ini.Read("CandidatesPerLevel", section, defaultSettings.CandidatesPerLevel.ToString()), out int cpl)) cpl = defaultSettings.CandidatesPerLevel;
            settings.CandidatesPerLevel = cpl;

            //Re-load all global sections to ensure they are up-to-date.
            LoadGlobalAndDebugSections(ini);

            if (DebugSettings.LogLiveScan) logger.Log("Settings loaded successfully.");
            return settings;
        }
    }
}