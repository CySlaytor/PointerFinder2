#pragma once

#include <QApplication>
#include <QHash>
#include <QIcon>
#include <QString>

namespace PointerFinder2::UI {

    // This class manages the visual appearance of the application, applying 
    // stylesheet templates and color layouts when you toggle between Light and Dark modes.
    class ThemeManager {
    public:
        static void applyTheme(QApplication& app, const QString& themeName);
        static QIcon getIcon(const QString& resourcePath, const QString& themeName);
        static void clearCache();
    private:
        static void applyDarkPalette(QApplication& app);
        static void applyLightPalette(QApplication& app);
        static void loadStylesheet(QApplication& app, const QString& resourcePath);

        static QHash<QString, QIcon> m_iconCache;
    };

}
