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
        resize(480, 350);

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
        auto* layout = new QHBoxLayout(tab);

        auto* formattingGroup = new QGroupBox("Formatting Options", tab);
        auto* formLayout = new QFormLayout(formattingGroup);
        m_prefixEdit = new QLineEdit(formattingGroup);
        m_suffixEdit = new QLineEdit(formattingGroup);
        m_alignCheck = new QCheckBox("Align Suffixes Vertically", formattingGroup);
        m_suffixLastLineCheck = new QCheckBox("Apply Suffix to Last Line Only", formattingGroup);

        formLayout->addRow("Indentation Prefix:", m_prefixEdit);
        formLayout->addRow("Line Suffix:", m_suffixEdit);
        formLayout->addRow(m_alignCheck);
        formLayout->addRow(m_suffixLastLineCheck);
        layout->addWidget(formattingGroup, 1);

        auto* previewGroup = new QGroupBox("Live Preview", tab);
        auto* previewLayout = new QVBoxLayout(previewGroup);
        m_previewEdit = new QPlainTextEdit(previewGroup);
        m_previewEdit->setReadOnly(true);

        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        m_previewEdit->setFont(monoFont);

        previewLayout->addWidget(m_previewEdit);
        layout->addWidget(previewGroup, 1);

        m_tabWidget->addTab(tab, "Code Notes");

        connect(m_prefixEdit, &QLineEdit::textChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
        connect(m_suffixEdit, &QLineEdit::textChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
        connect(m_alignCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
        connect(m_suffixLastLineCheck, &QCheckBox::checkStateChanged, this, &SettingsDialog::onCodeNoteSettingChanged);
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

        SettingsManager::saveGlobalSettingsOnly();
        updateCodeNotePreview();
    }

    void SettingsDialog::updateCodeNotePreview() {
        CodeNoteSettings settings;
        settings.Prefix = m_prefixEdit->text();
        settings.Suffix = m_suffixEdit->text();
        settings.AlignSuffixes = m_alignCheck->isChecked();
        settings.SuffixOnLastLineOnly = m_suffixLastLineCheck->isChecked();

        std::vector<int32_t> dummyOffsets = { 0x4, 0x2A0, -0x1C };
        QString preview = CodeNoteHelper::buildCodeNote(dummyOffsets, settings, "8-bit", "Description");
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
