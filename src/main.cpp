#include "Core/SettingsManager.h"
#include "Core/SoundManager.h"
#include "Models/GlobalSettings.h"
#include "Models/PointerPath.h"
#include "Models/ScanProgressReport.h"
#include "UI/MainWindow.h"
#include "UI/Styles/ThemeManager.h"

#include <QApplication>

// This is the starting point of the application. It configures program-wide settings,
// registers pointer data types, loads the visual theme, and launches the main window.
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    qRegisterMetaType<std::vector<PointerFinder2::DataModels::PointerPath>>();
    qRegisterMetaType<PointerFinder2::DataModels::ScanProgressReport>();

    QCoreApplication::setOrganizationName("RetroAchievements");
    QCoreApplication::setApplicationName("PointerFinder2");

    PointerFinder2::Core::SettingsManager::initializeGlobalSettings();

    PointerFinder2::UI::ThemeManager::applyTheme(app, PointerFinder2::DataModels::GlobalSettings::activeTheme);

    QStringList args = app.arguments();

    PointerFinder2::UI::MainWindow mainWindow(args);
    mainWindow.show();

    return app.exec();
}
