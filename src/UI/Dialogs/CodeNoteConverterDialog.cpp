#include "CodeNoteConverterDialog.h"

#include "../../Core/CodeNoteHelper.h"
#include "../../Models/CodeNoteSettings.h"

#include <QApplication>
#include <QClipboard>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::DataModels;

    QString CodeNoteConverterDialog::s_lastTriggerInput = "";
    QString CodeNoteConverterDialog::s_lastCodeNoteInput = "";
    QString CodeNoteConverterDialog::s_lastBaseAddress = "";
    int CodeNoteConverterDialog::s_lastPrefixIndex = 0;
    bool CodeNoteConverterDialog::s_lastUseMask = false;
    QString CodeNoteConverterDialog::s_lastDescription = "";

    CodeNoteConverterDialog::CodeNoteConverterDialog(Emulators::IEmulatorManager* manager, QWidget* parent)
        : QDialog(parent), m_manager(manager) {
        setWindowTitle("Code Note Converter");
        resize(600, 420);
        setMinimumSize(520, 420);

        setupLayout();

        m_memorySizeCombo->addItems({
            "8-bit", "16-bit", "32-bit", "24-bit", "Lower4", "Upper4",
            "16-bit BE", "32-bit BE", "24-bit BE",
            "Bit0", "Bit1", "Bit2", "Bit3", "Bit4", "Bit5", "Bit6", "Bit7",
            "BitCount", "Float", "Float BE", "Double32", "Double32 BE",
            "MBF32", "MBF32 LE", "ASCII Text"
            });

        m_pointerPrefixCombo->addItems({
            "X (32-bit)",
            "G (32-bit BE)",
            "W (24-bit)"
            });

        m_triggerInput->setText(s_lastTriggerInput);
        m_codeNoteInput->setPlainText(s_lastCodeNoteInput);
        m_baseAddressEdit->setText(s_lastBaseAddress);
        m_pointerPrefixCombo->setCurrentIndex(s_lastPrefixIndex);
        m_useMaskCheck->setChecked(s_lastUseMask);
        m_descriptionEdit->setText(s_lastDescription);

        if (m_manager && m_manager->isAttached()) {
            applyEmulatorDefaults();
        }

        if (!m_triggerInput->text().isEmpty()) {
            onConvertClicked();
        }
        if (!m_codeNoteInput->toPlainText().isEmpty()) {
            onReconvertClicked();
        }

        loadGeometrySettings();
    }

    CodeNoteConverterDialog::~CodeNoteConverterDialog() {
        saveGeometrySettings();
        s_lastTriggerInput = m_triggerInput->text();
        s_lastCodeNoteInput = m_codeNoteInput->toPlainText();
        s_lastBaseAddress = m_baseAddressEdit->text();
        s_lastPrefixIndex = m_pointerPrefixCombo->currentIndex();
        s_lastUseMask = m_useMaskCheck->isChecked();
        s_lastDescription = m_descriptionEdit->text();
    }

    void CodeNoteConverterDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        m_tabWidget = new QTabWidget(this);

        m_toNoteTab = new QWidget(this);
        auto* toNoteLayout = new QVBoxLayout(m_toNoteTab);

        toNoteLayout->addWidget(new QLabel("Input RetroAchievements Trigger:", this));
        auto* triggerInputLayout = new QHBoxLayout();
        m_triggerInput = new QLineEdit(this);
        m_convertButton = new QPushButton("Convert", this);
        triggerInputLayout->addWidget(m_triggerInput);
        triggerInputLayout->addWidget(m_convertButton);
        toNoteLayout->addLayout(triggerInputLayout);

        auto* noteMetaLayout = new QHBoxLayout();
        m_memorySizeCombo = new QComboBox(this);
        m_descriptionEdit = new QLineEdit(this);

        auto* sizeForm = new QFormLayout();
        sizeForm->addRow("Memory Size (final):", m_memorySizeCombo);
        auto* descForm = new QFormLayout();
        descForm->addRow("Description (final):", m_descriptionEdit);

        noteMetaLayout->addLayout(sizeForm, 1);
        noteMetaLayout->addLayout(descForm, 3);
        toNoteLayout->addLayout(noteMetaLayout);

        toNoteLayout->addWidget(new QLabel("Formatted Code Note:", this));
        m_codeNoteOutput = new QPlainTextEdit(this);
        m_codeNoteOutput->setReadOnly(true);
        QFont monoFont("Consolas", 9);
        m_codeNoteOutput->setFont(monoFont);
        toNoteLayout->addWidget(m_codeNoteOutput);

        m_copyToClipboardButton = new QPushButton("Copy to Clipboard", this);
        toNoteLayout->addWidget(m_copyToClipboardButton, 0, Qt::AlignRight);

        m_tabWidget->addTab(m_toNoteTab, "RA Trigger to Code Note");

        m_toTriggerTab = new QWidget(this);
        auto* toTriggerLayout = new QVBoxLayout(m_toTriggerTab);

        toTriggerLayout->addWidget(new QLabel("Input Code Note:", this));
        m_codeNoteInput = new QPlainTextEdit(this);
        m_codeNoteInput->setFont(monoFont);
        toTriggerLayout->addWidget(m_codeNoteInput);

        auto* triggerMetaLayout = new QHBoxLayout();
        m_baseAddressEdit = new QLineEdit(this);
        m_pointerPrefixCombo = new QComboBox(this);
        m_useMaskCheck = new QCheckBox("Use Mask", this);

        auto* baseForm = new QFormLayout();
        baseForm->addRow("Base Address (Hex):", m_baseAddressEdit);
        auto* prefixForm = new QFormLayout();
        prefixForm->addRow("Pointer Prefix:", m_pointerPrefixCombo);

        triggerMetaLayout->addLayout(baseForm);
        triggerMetaLayout->addLayout(prefixForm);
        triggerMetaLayout->addWidget(m_useMaskCheck);

        m_reconvertButton = new QPushButton("Re-Convert", this);
        triggerMetaLayout->addWidget(m_reconvertButton);
        toTriggerLayout->addLayout(triggerMetaLayout);

        toTriggerLayout->addWidget(new QLabel("Output RA Trigger String:", this));
        m_triggerOutput = new QLineEdit(this);
        m_triggerOutput->setReadOnly(true);
        toTriggerLayout->addWidget(m_triggerOutput);

        m_copyTriggerButton = new QPushButton("Copy to Clipboard", this);
        toTriggerLayout->addWidget(m_copyTriggerButton, 0, Qt::AlignRight);

        m_tabWidget->addTab(m_toTriggerTab, "Code Note to RA Trigger");

        mainLayout->addWidget(m_tabWidget);

        m_closeButton = new QPushButton("Close", this);
        m_closeButton->setObjectName("closeButton");
        mainLayout->addWidget(m_closeButton, 0, Qt::AlignRight);

        connect(m_convertButton, &QPushButton::clicked, this, &CodeNoteConverterDialog::onConvertClicked);
        connect(m_reconvertButton, &QPushButton::clicked, this, &CodeNoteConverterDialog::onReconvertClicked);
        connect(m_copyToClipboardButton, &QPushButton::clicked, this, &CodeNoteConverterDialog::onCopyToClipboardClicked);
        connect(m_copyTriggerButton, &QPushButton::clicked, this, &CodeNoteConverterDialog::onCopyTriggerClicked);
        connect(m_pointerPrefixCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CodeNoteConverterDialog::onPointerPrefixChanged);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

        connect(m_descriptionEdit, &QLineEdit::textChanged, this, &CodeNoteConverterDialog::updateNotePreview);
        connect(m_memorySizeCombo, &QComboBox::currentTextChanged, this, &CodeNoteConverterDialog::updateNotePreview);
    }

    void CodeNoteConverterDialog::processTrigger(const QString& trigger) {
        if (m_tabWidget->currentWidget() != m_toNoteTab) {
            m_tabWidget->setCurrentWidget(m_toNoteTab);
        }
        m_triggerInput->setText(trigger);
        onConvertClicked();
    }

    void CodeNoteConverterDialog::applyEmulatorDefaults() {
        QString emu = m_manager->getEmulatorName();

        if (emu.contains("Dolphin", Qt::CaseInsensitive)) {
            m_pointerPrefixCombo->setCurrentIndex(1); // G (32-bit BE)
            m_useMaskCheck->setChecked(true);
            m_useMaskCheck->setEnabled(false);
            m_pointerPrefixCombo->setEnabled(false);
        }
        else if (emu.contains("DuckStation", Qt::CaseInsensitive) || emu.contains("RALibretro", Qt::CaseInsensitive)) {
            m_pointerPrefixCombo->setCurrentIndex(2); // W (24-bit)
            m_useMaskCheck->setChecked(false);
            m_useMaskCheck->setEnabled(false);
            m_pointerPrefixCombo->setEnabled(false);
        }
        else if (emu.contains("PCSX2", Qt::CaseInsensitive)) {
            m_pointerPrefixCombo->setCurrentIndex(0); // X (32-bit)
            m_useMaskCheck->setChecked(false);
            m_useMaskCheck->setEnabled(true);
            m_pointerPrefixCombo->setEnabled(true);
        }
    }

    void CodeNoteConverterDialog::onConvertClicked() {
        QString trigger = m_triggerInput->text().trimmed();
        if (trigger.isEmpty()) {
            m_codeNoteOutput->setPlainText("Please enter a trigger string to convert.");
            m_lastOffsets.clear();
            return;
        }

        auto [offsets, parsedSize] = CodeNoteHelper::parseTrigger(trigger);
        if (offsets.empty()) {
            m_codeNoteOutput->setPlainText("Could not find any valid offsets in the trigger string.");
            m_lastOffsets.clear();
            m_memorySizeCombo->setCurrentText("N/A");
            return;
        }

        m_lastOffsets = offsets;
        m_memorySizeCombo->setCurrentText(parsedSize.isEmpty() ? "N/A" : parsedSize);
        updateNotePreview();
    }

    void CodeNoteConverterDialog::updateNotePreview() {
        if (m_lastOffsets.empty()) return;

        CodeNoteSettings settings = CodeNoteSettings::getFromGlobalSettings();
        QString note = CodeNoteHelper::buildCodeNote(
            m_lastOffsets,
            settings,
            m_memorySizeCombo->currentText(),
            m_descriptionEdit->text()
        );
        m_codeNoteOutput->setPlainText(note);
    }

    // Reads your formatted text note and converts it back into a packed, 
    // compressed RetroAchievements cheat code trigger.
    void CodeNoteConverterDialog::onReconvertClicked() {
        QString pointerPrefix = m_pointerPrefixCombo->currentText().split(' ').first();
        m_triggerOutput->setText(CodeNoteHelper::generateTriggerFromCodeNote(
            m_codeNoteInput->toPlainText(),
            m_baseAddressEdit->text(),
            pointerPrefix,
            m_useMaskCheck->isChecked()
        ));
    }

    // Copies the converted text notes directly to your system's clipboard.
    void CodeNoteConverterDialog::onCopyToClipboardClicked() {
        QString text = m_codeNoteOutput->toPlainText();
        if (text.isEmpty()) return;

        QApplication::clipboard()->setText(text);

        QString original = m_copyToClipboardButton->text();
        m_copyToClipboardButton->setText("✓ Copied!");
        m_copyToClipboardButton->setEnabled(false);

        QTimer::singleShot(1500, this, [this, original]() {
            m_copyToClipboardButton->setText(original);
            m_copyToClipboardButton->setEnabled(true);
            });
    }

    // Copies the generated cheat trigger directly to your system's clipboard.
    void CodeNoteConverterDialog::onCopyTriggerClicked() {
        QString text = m_triggerOutput->text();
        if (text.isEmpty()) return;

        QApplication::clipboard()->setText(text);

        QString original = m_copyTriggerButton->text();
        m_copyTriggerButton->setText("✓ Copied!");
        m_copyTriggerButton->setEnabled(false);

        QTimer::singleShot(1500, this, [this, original]() {
            m_copyTriggerButton->setText(original);
            m_copyTriggerButton->setEnabled(true);
            });
    }

    void CodeNoteConverterDialog::onPointerPrefixChanged(int index) {
        if (index == 2) { // W (24-bit)
            m_useMaskCheck->setChecked(false);
            m_useMaskCheck->setEnabled(false);
        }
        else if (!m_manager || !m_manager->isAttached()) {
            m_useMaskCheck->setEnabled(true);
        }
    }

    void CodeNoteConverterDialog::loadGeometrySettings() {
        QSettings settings;
        settings.beginGroup("CodeNoteConverter");
        if (settings.contains("geometry")) {
            restoreGeometry(settings.value("geometry").toByteArray());
        }
        settings.endGroup();
    }

    void CodeNoteConverterDialog::saveGeometrySettings() {
        QSettings settings;
        settings.beginGroup("CodeNoteConverter");
        settings.setValue("geometry", saveGeometry());
        settings.endGroup();
    }

}
