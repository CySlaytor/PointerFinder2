#include "PpssppRamScanRangeFinderDialog.h"

#include "../../Core/SettingsManager.h"
#include "../../Models/EmulatorTarget.h"  
#include "../../Models/GlobalSettings.h"

#include <algorithm>
#include <functional>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QtConcurrent>
#include <QVBoxLayout>

namespace PointerFinder2::UI::StaticRangeFinders {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::Emulators;
    using namespace PointerFinder2::DataModels;

    QWidget* createPpssppRangeFinder(IEmulatorManager* manager, QWidget* parent) {
        return new PpssppRamScanRangeFinderDialog(manager, parent);
    }

    // Initializes the PSP range helper, prompting the user to check their active Game ID.
    PpssppRamScanRangeFinderDialog::PpssppRamScanRangeFinderDialog(IEmulatorManager* manager, QWidget* parent)
        : QDialog(parent), m_manager(manager) {
        setWindowTitle("PPSSPP Static Range Finder");
        setModal(true);
        resize(680, 440);

        setupLayout();

        log("--- Welcome to the PPSSPP Static Range Finder ---", Qt::cyan);
        log("This tool attempts to automatically detect the static memory range of your game.", Qt::white);
        log("\nInstructions:", Qt::yellow);
        log("1. Make sure you are fully in-game (e.g., controlling your character).", Qt::lightGray);
        log("2. Find your game's ID (e.g., 'UCUS98737'). This is usually in the PPSSPP window title.", Qt::lightGray);
        log("3. Enter the Game ID below, without dashes.", Qt::lightGray);
        log("4. Click 'Find Static Range' to begin.", Qt::lightGray);
        log("\nHow it works:", Qt::yellow);
        log("The finder searches for your Game ID, which is stored as a constant string at the end of the game's static data region. "
            "By finding the last occurrence, we can approximate the static memory boundaries.", Qt::lightGray);
    }

    PpssppRamScanRangeFinderDialog::~PpssppRamScanRangeFinderDialog() = default;

    void PpssppRamScanRangeFinderDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        auto* topLayout = new QHBoxLayout();
        m_gameIdEdit = new QLineEdit(this);
        m_gameIdEdit->setPlaceholderText("Game ID (e.g., UCUS98737)");

        m_findButton = new QPushButton("Find Static Range", this);
        QFont boldFont = m_findButton->font();
        boldFont.setBold(true);
        m_findButton->setFont(boldFont);
        m_findButton->setEnabled(false);

        topLayout->addWidget(m_gameIdEdit, 1);
        topLayout->addWidget(m_findButton);
        mainLayout->addLayout(topLayout);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setValue(0);
        mainLayout->addWidget(m_progressBar);

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

        connect(m_findButton, &QPushButton::clicked, this, &PpssppRamScanRangeFinderDialog::onFindClicked);
        connect(m_applyButton, &QPushButton::clicked, this, &PpssppRamScanRangeFinderDialog::onApplyClicked);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_gameIdEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_findButton->setEnabled(!text.trimmed().isEmpty());
            });
    }

    // Reads active PSP memory, searching for the location of the entered Game ID string.
    void PpssppRamScanRangeFinderDialog::onFindClicked() {
        QString gameId = m_gameIdEdit->text().trimmed().replace("-", "");
        if (gameId.isEmpty()) {
            QMessageBox::warning(this, "Input Required", "Please enter the game's ID (e.g., 'UCUS98737').");
            return;
        }

        m_findButton->setEnabled(false);
        m_applyButton->setEnabled(false);
        m_logEdit->clear();
        m_resultLabel->setText("N/A");
        m_progressBar->setValue(0);

        QFuture<AnalysisResult> future = QtConcurrent::run([this, gameId]() {
            return performAnalysis(gameId);
            });

        auto* watcher = new QFutureWatcher<AnalysisResult>(this);
        connect(watcher, &QFutureWatcher<AnalysisResult>::finished, this, [=]() {
            AnalysisResult res = watcher->result();
            watcher->deleteLater();

            m_findButton->setEnabled(true);
            if (res.success) {
                m_foundStart = res.approxStart;
                m_foundEnd = res.approxEnd;

                m_resultLabel->setText(QString("%1 - %2")
                    .arg(m_manager->formatDisplayAddress(m_foundStart))
                    .arg(m_manager->formatDisplayAddress(m_foundEnd)));
                m_applyButton->setEnabled(true);
            }
            });
        watcher->setFuture(future);
    }

    PpssppRamScanRangeFinderDialog::AnalysisResult PpssppRamScanRangeFinderDialog::performAnalysis(const QString& gameId) {
        AnalysisResult res;
        QMetaObject::invokeMethod(this, [=]() { log("[INFO] Starting static range analysis for PPSSPP...", Qt::cyan); });

        constexpr uint32_t PSP_MEM_START = 0x08000000;
        constexpr uint32_t PSP_MEM_END = 0x0a000000;
        constexpr uint32_t searchSize = PSP_MEM_END - PSP_MEM_START;

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("[INFO] Reading %1 KB of memory from PSP address range 0x%2 - 0x%3...")
                .arg(searchSize / 1024)
                .arg(QString::number(PSP_MEM_START, 16).toUpper())
                .arg(QString::number(PSP_MEM_END, 16).toUpper()), Qt::white);
            });

        std::vector<uint8_t> searchBuffer = m_manager->readMemory(PSP_MEM_START, searchSize);
        if (searchBuffer.empty()) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] Could not read emulator memory. Is the game running?", Qt::red); });
            return res;
        }

        QMetaObject::invokeMethod(this, [=]() { log("[SUCCESS] Memory block read successfully.", Qt::green); });

        QByteArray patternBytes = gameId.toLatin1();
        std::vector<uint8_t> pattern(patternBytes.begin(), patternBytes.end());

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("[INFO] Searching for game ID marker: '%1'...").arg(gameId), Qt::white);
            m_progressBar->setMaximum(1);
            });

        auto matches = findAllOccurrences(searchBuffer, pattern);
        if (matches.empty()) {
            QMetaObject::invokeMethod(this, [=]() {
                log("[FAIL] Could not find the game ID string in the specified memory range.", Qt::red);
                log("Please ensure you have entered the correct ID and are fully in-game.", Qt::yellow);
                });
            return res;
        }

        std::sort(matches.begin(), matches.end());
        int lastIndex = matches.back();
        uint32_t absoluteEnd = PSP_MEM_START + static_cast<uint32_t>(lastIndex);

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("  -> Last occurrence found at PSP address: 0x%1").arg(QString::number(absoluteEnd, 16).toUpper()), Qt::green);
            m_progressBar->setValue(1);
            });

        constexpr uint32_t MB_BLOCK_SIZE = 0x100000;
        uint32_t approxStart = (absoluteEnd / MB_BLOCK_SIZE) * MB_BLOCK_SIZE;
        uint32_t approxEnd = approxStart + MB_BLOCK_SIZE - 1;

        QMetaObject::invokeMethod(this, [=]() {
            log("\n--- [RESULTS] ---", Qt::cyan);
            log(QString("Approximate 1MB Block (for scan): 0x%1 - 0x%2").arg(QString::number(approxStart, 16).toUpper()).arg(QString::number(approxEnd, 16).toUpper()), Qt::white);
            });

        res.success = true;
        res.approxStart = approxStart;
        res.approxEnd = approxEnd;
        return res;
    }

    std::vector<int> PpssppRamScanRangeFinderDialog::findAllOccurrences(const std::vector<uint8_t>& buffer, const std::vector<uint8_t>& pattern) {
        std::vector<int> indices;
        if (pattern.empty() || buffer.size() < pattern.size()) return indices;

        auto it = std::search(
            buffer.begin(), buffer.end(),
            std::boyer_moore_searcher(pattern.begin(), pattern.end())
        );

        while (it != buffer.end()) {
            int index = static_cast<int>(std::distance(buffer.begin(), it));
            indices.push_back(index);

            it = std::search(
                it + 1, buffer.end(),
                std::boyer_moore_searcher(pattern.begin(), pattern.end())
            );
        }
        return indices;
    }

    // Saves the detected PSP memory boundaries to your local configuration settings.
    void PpssppRamScanRangeFinderDialog::onApplyClicked() {
        if (m_foundStart == 0 || m_foundEnd == 0) return;

        try {
            log("\n[APPLY] Saving approximate range to settings.ini...", Qt::yellow);
            auto settings = SettingsManager::load(EmulatorTarget::PPSSPP, m_manager->getDefaultSettings());
            settings.staticAddressStart = m_manager->formatDisplayAddress(m_foundStart);
            settings.staticAddressEnd = m_manager->formatDisplayAddress(m_foundEnd);
            SettingsManager::save(EmulatorTarget::PPSSPP, settings);

            QMessageBox::information(this, "Settings Applied",
                "Static range applied successfully!\nThe new range will be used the next time you open a scan window.");
            accept();
        }
        catch (...) {
            QMessageBox::critical(this, "Error", "Failed to save settings.");
        }
    }

    void PpssppRamScanRangeFinderDialog::log(const QString& message, const QColor& color) {
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
