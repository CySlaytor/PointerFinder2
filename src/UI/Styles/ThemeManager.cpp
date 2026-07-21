#include "ThemeManager.h"

#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyleFactory>
#include <QTextStream>
#include <QtSvg/QSvgRenderer>

namespace PointerFinder2::UI {

    QHash<QString, QIcon> ThemeManager::m_iconCache;

    // This function switches the program-wide visual style, loading stylesheet files 
    // and re-coloring active widgets depending on your selected theme (Light or Dark).
    void ThemeManager::applyTheme(QApplication& app, const QString& themeName) {
        clearCache();
        app.setStyle(QStyleFactory::create("Fusion"));

        if (themeName.compare("Light", Qt::CaseInsensitive) == 0) {
            applyLightPalette(app);
            loadStylesheet(app, ":/styles/light_theme.qss");
        }
        else {
            applyDarkPalette(app);
            loadStylesheet(app, ":/styles/dark_theme.qss");
        }
    }

    // Clears loaded icons from memory, forcing the application to re-load 
    // graphic resources from disk so color changes apply immediately.
    void ThemeManager::clearCache() {
        m_iconCache.clear();
    }

    // Resolves raw SVG XML data, colorizes paths dynamically using active visual themes, 
    // and caches standard raster sizes to minimize filesystem operations.
    QIcon ThemeManager::getIcon(const QString& resourcePath, const QString& themeName) {
        QString cacheKey = QString("%1_%2").arg(resourcePath, themeName);
        if (m_iconCache.contains(cacheKey)) {
            return m_iconCache.value(cacheKey);
        }

        QFile file(resourcePath);
        if (!file.open(QFile::ReadOnly)) {
            return QIcon(resourcePath);
        }
        QString svgContent = QString::fromUtf8(file.readAll());
        file.close();

        QString primaryColor;
        QString accentColor;

        if (themeName.compare("Light", Qt::CaseInsensitive) == 0) {
            if (resourcePath.contains("find.svg", Qt::CaseInsensitive)) {
                primaryColor = "#666666";
            }
            else {
                primaryColor = "#3a3a3a";
            }
            accentColor = "#0056b3";
        }
        else {
            primaryColor = "#e0e0e0";
            accentColor = "#7289da";
        }

        svgContent.replace("#e0e0e0", primaryColor, Qt::CaseInsensitive);
        svgContent.replace("#7289da", accentColor, Qt::CaseInsensitive);

        QByteArray svgData = svgContent.toUtf8();
        QIcon icon;

        const QList<int> standardSizes = { 16, 20, 24, 32, 48, 64, 128, 256 };
        for (int size : standardSizes) {
            QSvgRenderer renderer(svgData);
            if (renderer.isValid()) {
                QPixmap pixmap(size, size);
                pixmap.fill(Qt::transparent);
                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);
                renderer.render(&painter);
                painter.end();
                icon.addPixmap(pixmap);
            }
        }

        if (icon.isNull()) {
            return QIcon(resourcePath);
        }

        m_iconCache.insert(cacheKey, icon);
        return icon;
    }

    // Configures system-level interface colors (like buttons, window backgrounds, 
    // and highlighting borders) to match the dark color scheme.
    void ThemeManager::applyDarkPalette(QApplication& app) {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(32, 32, 32));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(32, 32, 32));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(55, 55, 55));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(3, 218, 198));

        darkPalette.setColor(QPalette::Highlight, QColor(58, 120, 203));
        darkPalette.setColor(QPalette::HighlightedText, Qt::white);

        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));

        app.setPalette(darkPalette);
    }

    // Configures system-level interface colors (like buttons, window backgrounds, 
    // and highlighting borders) to match the light color scheme.
    void ThemeManager::applyLightPalette(QApplication& app) {
        QPalette lightPalette;
        lightPalette.setColor(QPalette::Window, QColor(245, 245, 245));
        lightPalette.setColor(QPalette::WindowText, QColor(30, 30, 30));
        lightPalette.setColor(QPalette::Base, Qt::white);
        lightPalette.setColor(QPalette::AlternateBase, QColor(238, 238, 238));
        lightPalette.setColor(QPalette::ToolTipBase, QColor(30, 30, 30));
        lightPalette.setColor(QPalette::ToolTipText, Qt::white);
        lightPalette.setColor(QPalette::Text, QColor(30, 30, 30));
        lightPalette.setColor(QPalette::Button, QColor(230, 230, 230));
        lightPalette.setColor(QPalette::ButtonText, QColor(30, 30, 30));
        lightPalette.setColor(QPalette::BrightText, Qt::red);
        lightPalette.setColor(QPalette::Link, QColor(0, 102, 204));

        lightPalette.setColor(QPalette::Highlight, QColor(74, 144, 226));
        lightPalette.setColor(QPalette::HighlightedText, Qt::white);

        lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
        lightPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(160, 160, 160));
        lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));

        app.setPalette(lightPalette);
    }

    // Reads a style configuration stylesheet file from disk and applies 
    // its formatting rules to all active application windows.
    void ThemeManager::loadStylesheet(QApplication& app, const QString& resourcePath) {
        QFile file(resourcePath);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&file);
            app.setStyleSheet(stream.readAll());
            file.close();
        }
        else {
            app.setStyleSheet("");
        }
    }

}
