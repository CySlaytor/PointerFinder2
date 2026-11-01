#include "SettingsDialog.h"

#include "../../Core/CodeNoteHelper.h"
#include "../../Core/SettingsManager.h"
#include "../../Models/CodeNoteSettings.h"
#include "../../Models/GlobalSettings.h"
#include "../MainWindow.h"
#include "../Styles/ThemeManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::DataModels;

    SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent) {
        setWindowTitle("Settings");
        setModal(true);
        resize(500, 360);

        setupLayout();

        m_isInitializing = true;

        m_themeCombo->setCurrentText(GlobalSettings::activeTheme);
        m_useDefaultSoundsCheck->setChecked(GlobalSettings::useWindowsDefaultSound);
        m_limitCpuCheck->setChecked(GlobalSettings::limitCpuUsage);
        m_sortByLevelCheck->setChecked(GlobalSettings::sortByLevelFirst);

        m_prefixEdit->setText(GlobalSettings::codeNotePrefix);
        m_suffixEdit->setText(GlobalSettings::codeNoteSuffix);
        m_alignCheck->setChecked(GlobalSettings::codeNoteAlignSuffixes);
        m_suffixLastLineCheck->setChecked(GlobalSettings::codeNoteSuffixOnLastLineOnly);
        m_autoPopulatePointersCheck->setChecked(GlobalSettings::codeNoteAutoPopulatePointers);

        m_isInitializing = false;
        updateCodeNotePreview();
    }

    SettingsDialog::~SettingsDialog() = default;

    void SettingsDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        m_tabWidget = new QTabWidget(this);
        setupGeneralTab();
        setupCodeNotesTab();
        mainLayout->addWidget(m_tabWidget);

        m_closeButton = new QPushButton("Close", this);
        m_closeButton->setObjectName("closeButton");
        m_closeButton->setAutoDefault(false);
        m_closeButton->setDefault(false);
        mainLayout->addWidget(m_closeButton, 0, Qt::AlignRight);

        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    // Builds the first tab containing basic settings like theme choices,
    // sound controls, and target computer performance limits.
    void SettingsDialog::setupGeneralTab() {
        auto* tab = new QWidget(this);
        auto* layout = new QVBoxLayout(tab);

        auto* themeGroup = new QGroupBox("Appearance", tab);
        auto* themeLayout = new QHBoxLayout(themeGroup);
        themeLayout->addWidget(new QLabel("Active Theme:", themeGroup));
        m_themeCombo = new QComboBox(themeGroup);
        m_themeCombo->addItems({ "Dark", "Light" });
        themeLayout->addWidget(m_themeCombo);
        themeLayout->addStretch();
        layout->addWidget(themeGroup);

        auto* sortingGroup = new QGroupBox("Sorting", tab);
        auto* sortingLayout = new QVBoxLayout(sortingGroup);
        m_sortByLevelCheck = new QCheckBox("Prioritize shorter chains when sorting by lowest offsets", sortingGroup);
        sortingLayout->addWidget(m_sortByLevelCheck);
        layout->addWidget(sortingGroup);

        auto* perfGroup = new QGroupBox("Performance", tab);
        auto* perfLayout = new QVBoxLayout(perfGroup);
        m_limitCpuCheck = new QCheckBox("Limit CPU Usage (approx. 50%)", perfGroup);
        perfLayout->addWidget(m_limitCpuCheck);
        layout->addWidget(perfGroup);

        auto* soundGroup = new QGroupBox("Sound", tab);
        auto* soundLayout = new QVBoxLayout(soundGroup);
        m_useDefaultSoundsCheck = new QCheckBox("Use Windows default notification sound", soundGroup);
        soundLayout->addWidget(m_useDefaultSoundsCheck);
        layout->addWidget(soundGroup);

        layout->addStretch();

        m_resetAllButton = new QPushButton("Reset All Settings to Default...", tab);
        m_resetAllButton->setStyleSheet("color: #FF5555; font-weight: bold; padding: 4px;");
        m_resetAllButton->setAutoDefault(false);
        m_resetAllButton->setDefault(false);
        layout->addWidget(m_resetAllButton);

        m_tabWidget->addTab(tab, "General");

        connect(m_themeCombo, &QComboBox::currentTextChanged, this, &SettingsDialog::onGeneralSettingChanged);
        connect(m_sortByLevelCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onGeneralSettingChanged);
        connect(m_limitCpuCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onGeneralSettingChanged);
        connect(m_useDefaultSoundsCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onGeneralSettingChanged);
        connect(m_resetAllButton, &QPushButton::clicked, this, &SettingsDialog::onResetAllClicked);
    }

    // Builds the second settings tab, allowing you to configure indentation strings
    // with a live-rendering note simulator showing changes as you type.
    void SettingsDialog::setupCodeNotesTab() {
        auto* tab = new QWidget(this);
        auto* tabLayout = new QVBoxLayout(tab);

        // Header row with QA standards help button
        auto* headerRow = new QHBoxLayout();
        auto* noteHeaderDesc = new QLabel("Configure code note indirection levels and formatting standards:", tab);
        m_codeNoteHelpBtn = new QPushButton("?", tab);
        m_codeNoteHelpBtn->setFixedSize(22, 22);
        m_codeNoteHelpBtn->setAutoDefault(false);
        m_codeNoteHelpBtn->setDefault(false);
        m_codeNoteHelpBtn->setToolTip("Why standard indentation matters (QA & Code Review standards)");

        headerRow->addWidget(noteHeaderDesc);
        headerRow->addStretch();
        headerRow->addWidget(m_codeNoteHelpBtn);
        tabLayout->addLayout(headerRow);

        auto* contentLayout = new QHBoxLayout();

        auto* formattingGroup = new QGroupBox("Formatting Options", tab);
        auto* formLayout = new QFormLayout(formattingGroup);
        m_prefixEdit = new QLineEdit(formattingGroup);
        m_suffixEdit = new QLineEdit(formattingGroup);
        m_alignCheck = new QCheckBox("Align Suffixes Vertically", formattingGroup);
        m_suffixLastLineCheck = new QCheckBox("Apply Suffix to Last Line Only", formattingGroup);
        m_autoPopulatePointersCheck = new QCheckBox("Auto-populate intermediate\npointer levels with template", formattingGroup);

        formLayout->addRow("Indentation Prefix:", m_prefixEdit);
        formLayout->addRow("Line Suffix:", m_suffixEdit);
        formLayout->addRow(m_alignCheck);
        formLayout->addRow(m_suffixLastLineCheck);
        formLayout->addRow(m_autoPopulatePointersCheck);
        contentLayout->addWidget(formattingGroup, 3);

        auto* previewGroup = new QGroupBox("Live Preview", tab);
        auto* previewLayout = new QVBoxLayout(previewGroup);
        m_previewEdit = new QPlainTextEdit(previewGroup);
        m_previewEdit->setReadOnly(true);
        m_previewEdit->setMinimumWidth(210);

        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        m_previewEdit->setFont(monoFont);

        previewLayout->addWidget(m_previewEdit);
        contentLayout->addWidget(previewGroup, 2);

        tabLayout->addLayout(contentLayout, 1);
        m_tabWidget->addTab(tab, "Code Notes");

        connect(m_codeNoteHelpBtn, &QPushButton::clicked, this, &SettingsDialog::onCodeNoteHelpClicked);
        connect(m_prefixEdit, &QLineEdit::textChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
        connect(m_suffixEdit, &QLineEdit::textChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
        connect(m_alignCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
        connect(m_suffixLastLineCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
        connect(m_autoPopulatePointersCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
    }

    void SettingsDialog::onCodeNoteHelpClicked() {
        QDialog helpDialog(this);
        helpDialog.setWindowTitle("Recommended Code Note Structure");
        helpDialog.resize(620, 600);
        helpDialog.setWindowFlags(helpDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

        auto* layout = new QVBoxLayout(&helpDialog);
        layout->setSpacing(8);

        auto* titleLabel = new QLabel("<b>Why Hierarchical Indentation Matters</b>", &helpDialog);
        titleLabel->setStyleSheet("font-size: 13px;");
        layout->addWidget(titleLabel);

        bool isDark = (GlobalSettings::activeTheme.compare("Light", Qt::CaseInsensitive) != 0);
        QString codeBg = isDark ? "#2b2d31" : "#e3e5e8";
        QString codeFg = isDark ? "#e2e4e9" : "#2b2d31";
        QString codeBorder = isDark ? "#3d4149" : "#c4c7cc";

        // Discord-style inline code badge formatter
        auto codeBadge = [&](const QString& text) -> QString {
            return QString("<span style='background-color: %1; color: %2; border: 1px solid %3; font-family: Consolas, monospace; font-size: 11px;'>&nbsp;%4&nbsp;</span>")
                .arg(codeBg, codeFg, codeBorder, text.toHtmlEscaped());
        };

        auto* descLabel = new QLabel(
            QString(
                "<p style='line-height: 130%%; margin: 0;'>RetroAchievements QA and Code Review (CR) teams strongly encourage adopting standardized "
                "hierarchical code note formatting for all new sets and updates:</p>"
                "<ul style='margin-top: 8px; margin-bottom: 8px; margin-left: -22px; line-height: 135%%;'>"
                "<li style='margin-bottom: 7px;'><b>Older Sets of Inactive Developers:</b> Many legacy achievement sets were created years ago by "
                "developers who have long since become inactive, long before modern code note conventions were established. "
                "When these older sets require maintenance, logic upgrades, or bug fixes, current developers and reviewers "
                "must spend considerable time reverse-engineering flat, unformatted lists of offsets. Following this structure "
                "ensures sets remain transparent and maintainable by anyone in the future.</li>"
                "<li style='margin-bottom: 7px;'><b>Pointers (%1) vs. Enums (%2):</b> While dots were previously used for indirection, "
                "using plusses (%1) exclusively for pointer jumps and dots (%2) exclusively for value "
                "enumerations provides an immediate visual distinction between structural memory offsets and variable states.</li>"
                "<li style='margin-bottom: 7px;'><b>Leveling Up Enums:</b> Dots are used to level up enumerations directly under their respective variable "
                "at the current pointer level (e.g., %3 directly beneath %4), making complex "
                "structures intuitive to read.</li>"
                "<li style='margin-bottom: 7px;'><b>Explicit Pointer Annotations:</b> It is recommended to use %5 followed by %6 "
                "or %7 depending on the platform architecture.</li>"
                "<li style='margin-bottom: 4px;'><b>Why Pipelines (%8) Were Reallocated:</b> Using %8 as a pointer delimiter has been retired. "
                "Pipelines are now reserved exclusively to document <b>Array structures</b> and indexed element offsets "
                "(e.g., %9).</li>"
                "</ul>"
            ).arg(codeBadge("+"), codeBadge("."), codeBadge("...0x00=Idle"), codeBadge("++0x28"))
             .arg(codeBadge("="), codeBadge("[32-bit] Pointer"), codeBadge("[24-bit] Pointer"))
             .arg(codeBadge("|"), codeBadge("+++|0x00")),
            &helpDialog
        );
        descLabel->setWordWrap(true);
        layout->addWidget(descLabel);

        layout->addWidget(new QLabel("<b>Exemplary Recommended Structure:</b>", &helpDialog));

        auto* exampleEdit = new QPlainTextEdit(&helpDialog);
        exampleEdit->setReadOnly(true);
        exampleEdit->setMinimumHeight(240);
        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        exampleEdit->setFont(monoFont);
        exampleEdit->setPlainText(
            "[32-bit] Pointer to PlayerObject\n\n"
            "+0x0=[32-bit] Pointer to PlayerData\n"
            "++0x14=[32-bit] Pointer to Coordinates\n"
            "+++0x0=[Float] Position X\n"
            "+++0x4=[Float] Position Y\n"
            "+++0x8=[Float] Position Z\n"
            "++0x28=[32-bit] Player State\n"
            "...0x00=Idle\n"
            "...0x01=Running\n"
            "...0x02=Jumping\n"
            "...0xFF=Dead\n"
            "++0x30=[16-bit] Health\n"
            "++0x40=[30x16 bytes] Pointer to PlayerInventory Array\n"
            "+++|0x00=[32-bit] Weapon ID\n"
            "....0x00=Pistol\n"
            "....0x01=SMG\n"
            "....0x02=Sniper Rifle"
        );
        layout->addWidget(exampleEdit, 1);

        auto* closeBtn = new QPushButton("Close", &helpDialog);
        closeBtn->setObjectName("closeButton");
        closeBtn->setAutoDefault(false);
        closeBtn->setDefault(false);
        connect(closeBtn, &QPushButton::clicked, &helpDialog, &QDialog::accept);
        layout->addWidget(closeBtn, 0, Qt::AlignRight);

        helpDialog.exec();
    }

    void SettingsDialog::onGeneralSettingChanged() {
        if (m_isInitializing) return;
        GlobalSettings::useWindowsDefaultSound = m_useDefaultSoundsCheck->isChecked();
        GlobalSettings::limitCpuUsage = m_limitCpuCheck->isChecked();
        GlobalSettings::sortByLevelFirst = m_sortByLevelCheck->isChecked();

        if (GlobalSettings::activeTheme != m_themeCombo->currentText()) {
            GlobalSettings::activeTheme = m_themeCombo->currentText();
            ThemeManager::applyTheme(*qApp, GlobalSettings::activeTheme);

            auto* mainWindow = qobject_cast<MainWindow*>(parentWidget());
            if (mainWindow) {
                mainWindow->updateThemeIcons();
            }
        }

        SettingsManager::saveGlobalSettingsOnly();
    }

    void SettingsDialog::onCodeNoteSettingChanged() {
        if (m_isInitializing) return;
        GlobalSettings::codeNotePrefix = m_prefixEdit->text();
        GlobalSettings::codeNoteSuffix = m_suffixEdit->text();
        GlobalSettings::codeNoteAlignSuffixes = m_alignCheck->isChecked();
        GlobalSettings::codeNoteSuffixOnLastLineOnly = m_suffixLastLineCheck->isChecked();
        GlobalSettings::codeNoteAutoPopulatePointers = m_autoPopulatePointersCheck->isChecked();

        SettingsManager::saveGlobalSettingsOnly();
        updateCodeNotePreview();
    }

    void SettingsDialog::updateCodeNotePreview() {
        CodeNoteSettings settings;
        settings.Prefix = m_prefixEdit->text();
        settings.Suffix = m_suffixEdit->text();
        settings.AlignSuffixes = m_alignCheck->isChecked();
        settings.SuffixOnLastLineOnly = m_suffixLastLineCheck->isChecked();
        settings.AutoPopulatePointers = m_autoPopulatePointersCheck->isChecked();

        std::vector<int32_t> dummyOffsets = { 0x4, 0x2A0, 0x1C };
        QString preview = CodeNoteHelper::buildCodeNote(dummyOffsets, settings, "8-bit", "Description", "[32-bit] Pointer");
        m_previewEdit->setPlainText(preview);
    }

    // Clears your entire settings file and restarts the application to restore
    // original default factory configurations.
    void SettingsDialog::onResetAllClicked() {
        auto result = QMessageBox::critical(
            this,
            "Confirm Reset All Settings",
            "This will reset ALL application settings, including saved scan parameters, window positions, and styling preferences. The settings.ini file will be deleted.\n\n"
            "The application will restart to apply the default settings.\n\n"
            "Are you sure you want to proceed?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (result == QMessageBox::Yes) {
            QSettings settings;
            settings.clear();

            QString settingsFile = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
            if (QFile::exists(settingsFile)) {
                QFile::remove(settingsFile);
            }

            QMessageBox::information(this, "Settings Reset", "All settings have been reset to default. The application will now restart.");
            triggerApplicationSelfRestart();
        }
    }

    void SettingsDialog::triggerApplicationSelfRestart() {
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments().mid(1));
        qApp->quit();
        std::exit(0);
    }

}
