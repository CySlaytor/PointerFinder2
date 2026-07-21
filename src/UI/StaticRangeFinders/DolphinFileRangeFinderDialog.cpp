#include "DolphinFileRangeFinderDialog.h"

#include "../../Core/SettingsManager.h"
#include "../../Models/EmulatorTarget.h"  
#include "../../Models/GlobalSettings.h"

#include <QDataStream>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QtConcurrent>
#include <QVBoxLayout>
#include <stdlib.h>

namespace PointerFinder2::UI::StaticRangeFinders {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::Emulators;
    using namespace PointerFinder2::DataModels;

    QWidget* createDolphinRangeFinder(IEmulatorManager* manager, QWidget* parent) {
        return new DolphinFileRangeFinderDialog(manager, parent);
    }

    // Initializes the Dolphin range helper, loading instructions on how to 
    // extract system files from your active game.
    DolphinFileRangeFinderDialog::DolphinFileRangeFinderDialog(IEmulatorManager* manager, QWidget* parent)
        : QDialog(parent), m_manager(manager) {
        setWindowTitle("Dolphin/GC Static Range Finder");
        setModal(true);
        resize(680, 440);

        setupLayout();

        log("--- Welcome to the Dolphin/GC Static Range Finder ---", Qt::cyan);
        log("This tool analyzes a main.dol file to automatically detect the static memory range of your game.", Qt::white);
        log("\nInstructions:", Qt::yellow);
        log("1. In Dolphin, right-click your game in the game list and select 'Properties'.", Qt::lightGray);
        log("2. Go to the 'Filesystem' tab.", Qt::lightGray);
        log("3. Right-click on the top-level entry and choose 'Extract System Data...'. Save the files to a folder.", Qt::lightGray);
        log("4. In this tool, click 'Browse...' and open the 'sys/main.dol' file.", Qt::lightGray);
        log("5. Adjust the 'Border Size' if needed (a larger border gives a wider search range).", Qt::lightGray);
        log("6. Click 'Analyze File' to begin.", Qt::lightGray);
    }

    DolphinFileRangeFinderDialog::~DolphinFileRangeFinderDialog() = default;

    void DolphinFileRangeFinderDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        auto* fileLayout = new QHBoxLayout();
        m_filePathEdit = new QLineEdit(this);
        m_browseButton = new QPushButton("Browse...", this);
        m_analyzeButton = new QPushButton("Analyze File", this);

        QFont boldFont = m_analyzeButton->font();
        boldFont.setBold(true);
        m_analyzeButton->setFont(boldFont);
        m_analyzeButton->setEnabled(false);

        fileLayout->addWidget(m_filePathEdit);
        fileLayout->addWidget(m_browseButton);
        fileLayout->addWidget(m_analyzeButton);
        mainLayout->addLayout(fileLayout);

        auto* sliderLayout = new QHBoxLayout();
        m_borderSlider = new QSlider(Qt::Horizontal, this);
        m_borderSlider->setRange(0, 2048);
        m_borderSlider->setSingleStep(64);
        m_borderSlider->setPageStep(256);
        m_borderSlider->setValue(1024);
        m_borderLabel = new QLabel("Border Size: 1024 KB", this);

        sliderLayout->addWidget(m_borderSlider, 1);
        sliderLayout->addWidget(m_borderLabel);
        mainLayout->addLayout(sliderLayout);

        m_logEdit = new QTextEdit(this);
        m_logEdit->setReadOnly(true);
        QFont monoFont("Consolas", 9);
        m_logEdit->setFont(monoFont);
        mainLayout->addWidget(m_logEdit);

        auto* resultsLayout = new QHBoxLayout();
        m_resultLabel = new QLabel("N/A", this);
        m_resultLabel->setFont(boldFont);
        resultsLayout->addWidget(new QLabel("Suggested Range (for scan):", this));
        resultsLayout->addWidget(m_resultLabel);
        resultsLayout->addStretch();
        mainLayout->addLayout(resultsLayout);

        auto* bottomLayout = new QHBoxLayout();
        m_applyButton = new QPushButton("Apply and Close", this);
        m_applyButton->setObjectName("applyButton");

        m_closeButton = new QPushButton("Close", this);
        m_closeButton->setObjectName("closeButton");

        m_applyButton->setEnabled(false);

        bottomLayout->addStretch();
        bottomLayout->addWidget(m_applyButton);
        bottomLayout->addWidget(m_closeButton);
        mainLayout->addLayout(bottomLayout);

        connect(m_browseButton, &QPushButton::clicked, this, &DolphinFileRangeFinderDialog::onBrowseClicked);
        connect(m_analyzeButton, &QPushButton::clicked, this, &DolphinFileRangeFinderDialog::onAnalyzeClicked);
        connect(m_borderSlider, &QSlider::valueChanged, this, &DolphinFileRangeFinderDialog::onSliderMoved);
        connect(m_applyButton, &QPushButton::clicked, this, &DolphinFileRangeFinderDialog::onApplyClicked);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_filePathEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_analyzeButton->setEnabled(!text.trimmed().isEmpty());
            });
    }

    // Opens a file browser window so you can select the active game's executable file (main.dol).
    void DolphinFileRangeFinderDialog::onBrowseClicked() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Select main.dol File",
            "",
            "DOL Executable (*.dol);;All files (*.*)"
        );

        if (!filePath.isEmpty()) {
            m_filePathEdit->setText(filePath);
        }
    }

    void DolphinFileRangeFinderDialog::onSliderMoved(int value) {
        int snapped = static_cast<int>(qRound(value / 64.0) * 64);
        if (snapped != value) {
            m_borderSlider->blockSignals(true);
            m_borderSlider->setValue(snapped);
            m_borderSlider->blockSignals(false);
            return;
        }
        m_borderLabel->setText(QString("Border Size: %1 KB").arg(snapped));
    }

    // Triggers the background scanning thread to parse the game's executable file.
    void DolphinFileRangeFinderDialog::onAnalyzeClicked() {
        QString filePath = m_filePathEdit->text().trimmed();
        if (filePath.isEmpty() || !QFile::exists(filePath)) {
            QMessageBox::warning(this, "File Not Found", "Please select a valid main.dol file.");
            return;
        }

        uint32_t borderSize = static_cast<uint32_t>(m_borderSlider->value()) * 1024;

        m_analyzeButton->setEnabled(false);
        m_applyButton->setEnabled(false);
        m_logEdit->clear();
        m_resultLabel->setText("N/A");

        QFuture<AnalysisResult> future = QtConcurrent::run([this, filePath, borderSize]() {
            return performAnalysis(filePath, borderSize);
            });

        auto* watcher = new QFutureWatcher<AnalysisResult>(this);
        connect(watcher, &QFutureWatcher<AnalysisResult>::finished, this, [=]() {
            AnalysisResult res = watcher->result();
            watcher->deleteLater();

            m_analyzeButton->setEnabled(true);
            if (res.success) {
                m_foundStart = res.start;
                m_foundEnd = res.end;
                m_resultLabel->setText(QString("%1 - %2")
                    .arg(m_manager->formatDisplayAddress(m_foundStart))
                    .arg(m_manager->formatDisplayAddress(m_foundEnd)));
                m_applyButton->setEnabled(true);
            }
            });
        watcher->setFuture(future);
    }

    // Parses GameCube/Wii .dol file header segments to identify exact system module layouts.
    DolphinFileRangeFinderDialog::AnalysisResult DolphinFileRangeFinderDialog::performAnalysis(const QString& filePath, uint32_t borderSize) {
        AnalysisResult res;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] Could not open file for reading.", Qt::red); });
            return res;
        }

        if (file.size() < 0x100) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] File is too small to be a valid DOL header.", Qt::red); });
            return res;
        }

        QMetaObject::invokeMethod(this, [=]() { log(QString("--- Analyzing '%1' ---").arg(QFileInfo(filePath).fileName()), Qt::cyan); });

        QDataStream in(&file);
        in.setByteOrder(QDataStream::BigEndian);

        uint32_t textOffsets[7];
        uint32_t dataOffsets[11];
        uint32_t textAddrs[7];
        uint32_t dataAddrs[11];
        uint32_t textSizes[7];
        uint32_t dataSizes[11];
        uint32_t bssAddr;
        uint32_t bssSize;

        for (int i = 0; i < 7; ++i) in >> textOffsets[i];
        for (int i = 0; i < 11; ++i) in >> dataOffsets[i];
        for (int i = 0; i < 7; ++i) in >> textAddrs[i];
        for (int i = 0; i < 11; ++i) in >> dataAddrs[i];
        for (int i = 0; i < 7; ++i) in >> textSizes[i];
        for (int i = 0; i < 11; ++i) in >> dataSizes[i];
        in >> bssAddr;
        in >> bssSize;

        uint32_t minAddr = 0xFFFFFFFF;
        uint32_t maxAddr = 0;

        for (int i = 0; i < 7; ++i) {
            if (textSizes[i] > 0 && textAddrs[i] > 0) {
                minAddr = std::min(minAddr, textAddrs[i]);
                maxAddr = std::max(maxAddr, textAddrs[i] + textSizes[i]);
            }
        }

        for (int i = 0; i < 11; ++i) {
            if (dataSizes[i] > 0 && dataAddrs[i] > 0) {
                minAddr = std::min(minAddr, dataAddrs[i]);
                maxAddr = std::max(maxAddr, dataAddrs[i] + dataSizes[i]);
            }
        }

        if (bssSize > 0 && bssAddr > 0) {
            minAddr = std::min(minAddr, bssAddr);
            maxAddr = std::max(maxAddr, bssAddr + bssSize);
        }

        if (maxAddr == 0) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] No active sections found in the DOL file.", Qt::red); });
            return res;
        }

        QMetaObject::invokeMethod(this, [=]() {
            log("\n--- [Calculations] ---", Qt::yellow);
            log(QString("1. Base Static Endpoint: Highest memory address is 0x%1.").arg(QString::number(maxAddr, 16).toUpper()), Qt::white);
            log(QString("\n2. Applying Border:"), Qt::lightGray);
            });

        uint32_t halfBorder = borderSize / 2;
        uint32_t suggestedStart = (maxAddr > halfBorder) ? maxAddr - halfBorder : 0;
        uint32_t suggestedEnd = maxAddr + halfBorder;

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("   New Start = 0x%1 - 0x%2 = 0x%3")
                .arg(QString::number(maxAddr, 16).toUpper())
                .arg(QString::number(halfBorder, 16).toUpper())
                .arg(QString::number(suggestedStart, 16).toUpper()), Qt::white);
            log(QString("   New End   = 0x%1 + 0x%2 = 0x%3")
                .arg(QString::number(maxAddr, 16).toUpper())
                .arg(QString::number(halfBorder, 16).toUpper())
                .arg(QString::number(suggestedEnd, 16).toUpper()), Qt::white);
            log("\n3. Clamping to Valid Memory Bounds:", Qt::lightGray);
            });

        uint32_t finalStart = std::max(minAddr, suggestedStart);
        uint32_t finalEnd = std::min(suggestedEnd, 0x81800000U);

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("   Clamped Start: max(0x%1, 0x%2) = 0x%3")
                .arg(QString::number(minAddr, 16).toUpper())
                .arg(QString::number(suggestedStart, 16).toUpper())
                .arg(QString::number(finalStart, 16).toUpper()), Qt::white);
            log(QString("   Clamped End:   min(0x%1, 0x81800000) = 0x%2")
                .arg(QString::number(suggestedEnd, 16).toUpper())
                .arg(QString::number(finalEnd, 16).toUpper()), Qt::white);
            log("\n4. Suggested Search Range (4KB Aligned):", Qt::lightGray);
            });

        uint32_t alignedStart = finalStart & 0xFFFFF000U;
        uint32_t alignedEnd = finalEnd & 0xFFFFF000U;

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("   Start: 0x%1").arg(QString::number(alignedStart, 16).toUpper()), Qt::white);
            log(QString("   End:   0x%1").arg(QString::number(alignedEnd, 16).toUpper()), Qt::white);

            log("\n--- [RESULT] ---", Qt::cyan);
            log(QString("  Start: %1").arg(m_manager->formatDisplayAddress(alignedStart)), Qt::green);
            log(QString("  End:   %1").arg(m_manager->formatDisplayAddress(alignedEnd)), Qt::green);
            });

        res.success = true;
        res.start = alignedStart;
        res.end = alignedEnd;
        return res;
    }

    // Saves the identified Wii/GC boundaries to your local configurations so they load next scan.
    void DolphinFileRangeFinderDialog::onApplyClicked() {
        if (m_foundStart == 0 || m_foundEnd == 0) return;

        try {
            log("\n[APPLY] Saving approximate range to settings.ini...", Qt::yellow);
            auto settings = SettingsManager::load(EmulatorTarget::Dolphin, m_manager->getDefaultSettings());
            settings.staticAddressStart = m_manager->formatDisplayAddress(m_foundStart);
            settings.staticAddressEnd = m_manager->formatDisplayAddress(m_foundEnd);
            SettingsManager::save(EmulatorTarget::Dolphin, settings);

            QMessageBox::information(this, "Settings Applied",
                "Static range applied successfully!\nThe new range will be used the next time you open a scan window.");
            accept();
        }
        catch (...) {
            QMessageBox::critical(this, "Error", "Failed to save settings.");
        }
    }

    void DolphinFileRangeFinderDialog::log(const QString& message, const QColor& color) {
        m_logEdit->moveCursor(QTextCursor::End);

        QColor contrastColor = color;
        if (GlobalSettings::activeTheme.compare("Light", Qt::CaseInsensitive) == 0) {
            if (color == Qt::white) contrastColor = QColor("#2b2b2b");
            else if (color == Qt::lightGray) contrastColor = QColor("#555555");
            else if (color == Qt::cyan) contrastColor = QColor("#0066cc");
            else if (color == Qt::green) contrastColor = QColor("#008000");
            else if (color == Qt::yellow) contrastColor = QColor("#b58900");
            else if (color == Qt::red) contrastColor = QColor("#cc0000");
        }

        m_logEdit->setTextColor(contrastColor);
        m_logEdit->insertPlainText(message + "\n");
        m_logEdit->ensureCursorVisible();
    }

}
