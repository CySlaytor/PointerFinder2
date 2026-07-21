#pragma once
#include "../Models/AppSettings.h"
#include "../Models/EmulatorTarget.h"

namespace PointerFinder2::Core {

	// This class manages configuration storage, reading and writing options 
	// to your local settings.ini file.
	class SettingsManager {
	public:
		static void initializeGlobalSettings();
		static void saveGlobalSettingsOnly();
		static void save(DataModels::EmulatorTarget target, const DataModels::AppSettings& settings);
		static DataModels::AppSettings load(DataModels::EmulatorTarget target, const DataModels::AppSettings& defaultSettings);

	private:
		static QString getSettingsFilePath();
				static QString getSectionName(DataModels::EmulatorTarget target);
	};

}
