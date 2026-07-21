#include "Pcsx2RamScanRangeFinderDialog.h"

#include "../../Core/SettingsManager.h"
#include "../../Models/EmulatorTarget.h"  
#include "../../Models/GlobalSettings.h"

#include <QFormLayout>
#include <QGroupBox>                           
#include <QHBoxLayout>
#include <QMessageBox>
#include <QtConcurrent>
#include <QVBoxLayout>

namespace PointerFinder2::UI::StaticRangeFinders {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::Emulators;
    using namespace PointerFinder2::DataModels;

    QWidget* createPcsx2RangeFinder(IEmulatorManager* manager, QWidget* parent) {
        return new Pcsx2RamScanRangeFinderDialog(manager, parent);
    }

    // Initializes the PS2 range helper, loading tips suggesting users are fully 
    // in-game before starting the search.
    Pcsx2RamScanRangeFinderDialog::Pcsx2RamScanRangeFinderDialog(IEmulatorManager* manager, QWidget* parent)
        : QDialog(parent), m_manager(manager) {
        setWindowTitle("PCSX2 Static Range Finder");
        setModal(true);
        resize(580, 440);

        setupLayout();

        log("--- Welcome to the PCSX2 Static Range Finder ---", Qt::cyan);
        log("This tool attempts to automatically detect the static memory range of your game.", Qt::white);
        log("\nFor best results:", Qt::yellow);
        log("1. Please make sure you are fully in-game (e.g., controlling your character).", Qt::lightGray);
        log("   Avoid running this tool from the main menu or during loading screens.", Qt::lightGray);
        log("\nHow it works:", Qt::yellow);
        log("The finder searches for memory card paths ('BASLUS', 'BESLES') located at the very end of the static data region. "
            "By finding the last occurrence, we can approximate the static memory boundaries.", Qt::lightGray);
    }

    Pcsx2RamScanRangeFinderDialog::~Pcsx2RamScanRangeFinderDialog() = default;

    void Pcsx2RamScanRangeFinderDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        auto* topLayout = new QHBoxLayout();
        m_findButton = new QPushButton("Find Static Range", this);
        QFont boldFont = m_findButton->font();
        boldFont.setBold(true);
        m_findButton->setFont(boldFont);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setValue(0);

        topLayout->addWidget(m_findButton);
        topLayout->addWidget(m_progressBar, 1);
        mainLayout->addLayout(topLayout);

        m_logEdit = new QTextEdit(this);
        m_logEdit->setReadOnly(true);
        QFont monoFont("Consolas", 9);
        m_logEdit->setFont(monoFont);
        mainLayout->addWidget(m_logEdit);

        auto* resultsGroup = new QGroupBox("Results", this);
        auto* formLayout = new QFormLayout(resultsGroup);
        m_absoluteLabel = new QLabel("N/A", this);
        m_approxLabel = new QLabel("N/A", this);
        m_absoluteLabel->setFont(boldFont);
        m_approxLabel->setFont(boldFont);

        formLayout->addRow("Absolute Static Range:", m_absoluteLabel);
        formLayout->addRow("Approximate 1MB Block (for scan):", m_approxLabel);
        mainLayout->addWidget(resultsGroup);

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

        connect(m_findButton, &QPushButton::clicked, this, &Pcsx2RamScanRangeFinderDialog::onFindClicked);
        connect(m_applyButton, &QPushButton::clicked, this, &Pcsx2RamScanRangeFinderDialog::onApplyClicked);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    // Reads active PS2 EE memory, searching for standard memory card marker codes.
    void Pcsx2RamScanRangeFinderDialog::onFindClicked() {
        m_findButton->setEnabled(false);
        m_applyButton->setEnabled(false);
        m_logEdit->clear();
        m_absoluteLabel->setText("N/A");
        m_approxLabel->setText("N/A");
        m_progressBar->setValue(0);

        QFuture<AnalysisResult> future = QtConcurrent::run([this]() {
            return performAnalysis();
            });

        auto* watcher = new QFutureWatcher<AnalysisResult>(this);
        connect(watcher, &QFutureWatcher<AnalysisResult>::finished, this, [=]() {
            AnalysisResult res = watcher->result();
            watcher->deleteLater();

            m_findButton->setEnabled(true);
            if (res.success) {
                m_foundStart = res.approxStart;
                m_foundEnd = res.approxEnd;

                m_absoluteLabel->setText(QString("0x100000 - %1")
                    .arg(m_manager->formatDisplayAddress(res.absoluteEnd)));
                m_approxLabel->setText(QString("%1 - %2")
                    .arg(m_manager->formatDisplayAddress(m_foundStart))
                    .arg(m_manager->formatDisplayAddress(m_foundEnd)));
                m_applyButton->setEnabled(true);
            }
            });
        watcher->setFuture(future);
    }

    Pcsx2RamScanRangeFinderDialog::AnalysisResult Pcsx2RamScanRangeFinderDialog::performAnalysis() {
        AnalysisResult res;
        QMetaObject::invokeMethod(this, [=]() { log("[INFO] Starting static range analysis for PCSX2...", Qt::cyan); });

        constexpr uint32_t PS2_SEARCH_START = 0x20300000;
        constexpr uint32_t PS2_SEARCH_END = 0x207FFFFF;
        constexpr uint32_t searchSize = PS2_SEARCH_END - PS2_SEARCH_START;

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("[INFO] Reading %1 KB of memory from PS2 address range 0x%2 - 0x%3...")
                .arg(searchSize / 1024)
                .arg(QString::number(PS2_SEARCH_START, 16).toUpper())
                .arg(QString::number(PS2_SEARCH_END, 16).toUpper()), Qt::white);
            });

        std::vector<uint8_t> searchBuffer = m_manager->readMemory(PS2_SEARCH_START, searchSize);
        if (searchBuffer.empty()) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] Could not read emulator memory. Is the game running?", Qt::red); });
            return res;
        }

        QMetaObject::invokeMethod(this, [=]() { log("[SUCCESS] Memory block read successfully.", Qt::green); });

        std::vector<std::vector<uint8_t>> searchPatterns = {
          {'B', 'A', 'S', 'L', 'U', 'S'},
          {'B', 'E', 'S', 'L', 'E', 'S'}
        };

        auto matches = findAllOccurrences(searchBuffer, searchPatterns);
        if (matches.empty()) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] Could not find any target strings in the specified range.", Qt::red); });
            return res;
        }

        std::sort(matches.begin(), matches.end());
        QMetaObject::invokeMethod(this, [=, size = matches.size()]() {
            log(QString("\n[INFO] Found %1 total candidates. Validating in reverse order...").arg(size), Qt::yellow);
            m_progressBar->setMaximum(static_cast<int>(size));
            });

        int processed = 0;
        for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
            int index = *it;
            uint32_t ps2StringAddr = PS2_SEARCH_START + static_cast<uint32_t>(index);

            QMetaObject::invokeMethod(this, [=]() {
                log(QString("  -> Testing candidate at PS2 address: 0x%1")
                    .arg(QString::number(ps2StringAddr, 16).toUpper()), Qt::white);
                });

            uint32_t validationAddr = ps2StringAddr + 0x30;
            std::vector<uint8_t> validationBuffer = m_manager->readMemory(validationAddr, 64);

            bool allZero = !validationBuffer.empty();
            for (uint8_t b : validationBuffer) {
                if (b != 0) {
                    allZero = false;
                    break;
                }
            }

            if (allZero) {
                QMetaObject::invokeMethod(this, [=]() { log("     [SUCCESS] Validation passed! A solid block of zeros was found.", Qt::green); });

                uint32_t absoluteEnd = ps2StringAddr;
                constexpr uint32_t MB_BLOCK_SIZE = 0x100000;
                constexpr uint32_t HALF_MB_THRESHOLD = MB_BLOCK_SIZE / 2;

                uint32_t currentBlockStart = (absoluteEnd / MB_BLOCK_SIZE) * MB_BLOCK_SIZE;
                uint32_t offsetInBlock = absoluteEnd - currentBlockStart;

                uint32_t approxStart = (offsetInBlock >= HALF_MB_THRESHOLD)
                    ? currentBlockStart
                    : currentBlockStart - MB_BLOCK_SIZE;

                uint32_t approxEnd = approxStart + MB_BLOCK_SIZE - 1;

                QMetaObject::invokeMethod(this, [=]() {
                    log("\n--- [RESULTS] ---", Qt::cyan);
                    log(QString("Absolute Static End (PS2):   0x%1").arg(QString::number(absoluteEnd, 16).toUpper()), Qt::white);
                    log(QString("Approximate 1MB Block (PS2): 0x%1 - 0x%2").arg(QString::number(approxStart, 16).toUpper()).arg(QString::number(approxEnd, 16).toUpper()), Qt::white);
                    m_progressBar->setValue(m_progressBar->maximum());
                    });

                res.success = true;
                res.absoluteEnd = absoluteEnd;
                res.approxStart = approxStart;
                res.approxEnd = approxEnd;
                return res;
            }
            else {
                QMetaObject::invokeMethod(this, [=]() { log("     [FAIL] Validation failed: The memory block was not all zeros.", Qt::red); });
            }

            processed++;
            QMetaObject::invokeMethod(this, [=]() { m_progressBar->setValue(processed); });
        }

        QMetaObject::invokeMethod(this, [=]() { log("\n[FINAL RESULT] Scanned all candidates, but none passed the validation.", Qt::red); });
        return res;
    }

    std::vector<int> Pcsx2RamScanRangeFinderDialog::findAllOccurrences(const std::vector<uint8_t>& buffer, const std::vector<std::vector<uint8_t>>& patterns) {
        std::vector<int> indices;
        if (buffer.size() < 6) return indices;

        for (size_t i = 0; i <= buffer.size() - 6; ++i) {
            for (const auto& pattern : patterns) {
                bool match = true;
                for (size_t j = 0; j < pattern.size(); ++j) {
                    if (buffer[i + j] != pattern[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    indices.push_back(static_cast<int>(i));
                    break;
                }
            }
        }
        return indices;
    }

    // Saves the calculated PS2 boundaries to your local configuration settings.
    void Pcsx2RamScanRangeFinderDialog::onApplyClicked() {
        if (m_foundStart == 0 || m_foundEnd == 0) return;

        try {
            log("\n[APPLY] Saving approximate range to settings.ini...", Qt::yellow);
            auto settings = SettingsManager::load(EmulatorTarget::PCSX2, m_manager->getDefaultSettings());
            settings.staticAddressStart = m_manager->formatDisplayAddress(m_foundStart);
            settings.staticAddressEnd = m_manager->formatDisplayAddress(m_foundEnd);
            SettingsManager::save(EmulatorTarget::PCSX2, settings);

            QMessageBox::information(this, "Settings Applied",
                "Static range applied successfully!\nThe new range will be used the next time you open a scan window.");
            accept();
        }
        catch (...) {
            QMessageBox::critical(this, "Error", "Failed to save settings.");
        }
    }

    void Pcsx2RamScanRangeFinderDialog::log(const QString& message, const QColor& color) {
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
