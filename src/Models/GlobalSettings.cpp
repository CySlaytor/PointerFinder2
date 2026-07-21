#include "GlobalSettings.h"

namespace PointerFinder2::DataModels {

	bool GlobalSettings::useWindowsDefaultSound = false;
	bool GlobalSettings::limitCpuUsage = false;
	QString GlobalSettings::codeNotePrefix = ".";
	QString GlobalSettings::codeNoteSuffix = " |";
	bool GlobalSettings::codeNoteAlignSuffixes = true;
	bool GlobalSettings::codeNoteSuffixOnLastLineOnly = false;
	bool GlobalSettings::sortByLevelFirst = true;
	QString GlobalSettings::activeTheme = "Dark";

}
