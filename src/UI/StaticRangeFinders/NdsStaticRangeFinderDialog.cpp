#include "NdsStaticRangeFinderDialog.h"

#include "../../Core/SettingsManager.h"
#include "../../Models/EmulatorTarget.h"  
#include "../../Models/GlobalSettings.h"

#include <QDataStream>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QtConcurrent>
#include <QVBoxLayout>

namespace PointerFinder2::UI::StaticRangeFinders {

    using namespace PointerFinder2::Core;
    using namespace PointerFinder2::Emulators;
    using namespace PointerFinder2::DataModels;

    QWidget* createNdsRangeFinder(IEmulatorManager* manager, QWidget* parent) {
        return new NdsStaticRangeFinderDialog(manager, parent);
    }

    // Initializes the DS range helper, loading instructions explaining how NDS 
    // file headers are analyzed.
    NdsStaticRangeFinderDialog::NdsStaticRangeFinderDialog(IEmulatorManager* manager, QWidget* parent)
        : QDialog(parent), m_manager(manager) {
        setWindowTitle("NDS Static Range Finder");
        setModal(true);
        resize(580, 390);

        setupLayout();

        log("--- Welcome to the NDS Static Range Finder ---", Qt::cyan);
        log("This tool analyzes a .nds ROM file to automatically detect the static memory range of your game.", Qt::white);
        log("\nInstructions:", Qt::yellow);
        log("1. Click 'Browse...' to select the .nds ROM file for the game you are running.", Qt::lightGray);
        log("2. Click 'Analyze ROM' to begin the analysis.", Qt::lightGray);
        log("\nHow it works:", Qt::yellow);
        log("The finder reads the game's header to find where the ARM9 binary is loaded into RAM and its size. "
            "Based on this, it calculates a suggested search range that is most likely to contain static data.", Qt::lightGray);
    }

    NdsStaticRangeFinderDialog::~NdsStaticRangeFinderDialog() = default;

    void NdsStaticRangeFinderDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        auto* fileLayout = new QHBoxLayout();
        m_romPathEdit = new QLineEdit(this);
        m_browseButton = new QPushButton("Browse...", this);
        m_analyzeButton = new QPushButton("Analyze ROM", this);

        QFont boldFont = m_analyzeButton->font();
        boldFont.setBold(true);
        m_analyzeButton->setFont(boldFont);
        m_analyzeButton->setEnabled(false);

        fileLayout->addWidget(m_romPathEdit);
        fileLayout->addWidget(m_browseButton);
        fileLayout->addWidget(m_analyzeButton);
        mainLayout->addLayout(fileLayout);

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

        connect(m_browseButton, &QPushButton::clicked, this, &NdsStaticRangeFinderDialog::onBrowseClicked);
        connect(m_analyzeButton, &QPushButton::clicked, this, &NdsStaticRangeFinderDialog::onAnalyzeClicked);
        connect(m_applyButton, &QPushButton::clicked, this, &NdsStaticRangeFinderDialog::onApplyClicked);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_romPathEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_analyzeButton->setEnabled(!text.trimmed().isEmpty());
            });
    }

    // Opens a file browser so you can select the active DS ROM (.nds) file on your computer.
    void NdsStaticRangeFinderDialog::onBrowseClicked() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Select NDS ROM File",
            "",
            "NDS ROM Files (*.nds);;All files (*.*)"
        );

        if (!filePath.isEmpty()) {
            m_romPathEdit->setText(filePath);
        }
    }

    // Triggers a background thread to read file segment layouts directly from the DS ROM.
    void NdsStaticRangeFinderDialog::onAnalyzeClicked() {
        QString romPath = m_romPathEdit->text().trimmed();
        if (romPath.isEmpty() || !QFile::exists(romPath)) {
            QMessageBox::warning(this, "File Not Found", "Please select a valid .nds ROM file.");
            return;
        }

        m_analyzeButton->setEnabled(false);
        m_applyButton->setEnabled(false);
        m_logEdit->clear();
        m_resultLabel->setText("N/A");

        QFuture<AnalysisResult> future = QtConcurrent::run([this, romPath]() {
            return performAnalysis(romPath);
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

    // Inspects DS binary headers to extract standard loading properties and execution boundaries.
    NdsStaticRangeFinderDialog::AnalysisResult NdsStaticRangeFinderDialog::performAnalysis(const QString& romPath) {
        AnalysisResult res;
        QFile file(romPath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] Could not open ROM for reading.", Qt::red); });
            return res;
        }

        if (file.size() < 0x80) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] ROM file is too small to be a valid NDS header.", Qt::red); });
            return res;
        }

        QByteArray header = file.read(0x80);
        QString gameTitle = QString::fromLatin1(header.left(12)).trimmed();

        QMetaObject::invokeMethod(this, [=]() { log(QString("--- Analyzing Header for: %1 ---").arg(gameTitle), Qt::cyan); });

        file.seek(0x28);
        QDataStream in(&file);
        in.setByteOrder(QDataStream::LittleEndian);

        uint32_t arm9LoadAddr;
        uint32_t arm9Size;
        in >> arm9LoadAddr;
        in >> arm9Size;

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("[INFO] ARM9 Load Address: 0x%1").arg(QString::number(arm9LoadAddr, 16).toUpper().rightJustified(8, '0')), Qt::white);
            log(QString("[INFO] ARM9 Size:         0x%1 (%2 bytes)").arg(QString::number(arm9Size, 16).toUpper()).arg(arm9Size), Qt::white);
            });

        constexpr uint32_t NDS_RAM_START = 0x02000000;
        constexpr uint32_t NDS_RAM_END = 0x02400000;

        if (arm9LoadAddr < NDS_RAM_START || arm9LoadAddr >= NDS_RAM_END) {
            QMetaObject::invokeMethod(this, [=]() { log("[FAIL] The 'ARM9 Load Address' is outside of main RAM.", Qt::red); });
            return res;
        }

        uint32_t calculatedEnd = arm9LoadAddr + arm9Size;

        QMetaObject::invokeMethod(this, [=]() {
            log("\n--- [Calculations] ---", Qt::yellow);
            log("1. Base Static Range (from header):", Qt::lightGray);
            log(QString("   Start: 0x%1").arg(QString::number(arm9LoadAddr, 16).toUpper().rightJustified(8, '0')), Qt::white);
            log(QString("   End:   0x%1 (Load Address + ARM9 Size)").arg(QString::number(calculatedEnd, 16).toUpper().rightJustified(8, '0')), Qt::white);
            log("\n2. Applying Asymmetrical Search Buffer:", Qt::lightGray);
            });

        constexpr uint32_t paddingStart = 0x5000;
        constexpr uint32_t paddingEnd = 0x10000;

        uint32_t suggestedStart = calculatedEnd - paddingStart;
        uint32_t suggestedEnd = calculatedEnd + paddingEnd;

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("   Buffer Before: 20 KB (0x5000), Buffer After: 64 KB (0x10000)"), Qt::white);
            log(QString("   New Start = 0x%1 - 0x5000 = 0x%2").arg(QString::number(calculatedEnd, 16).toUpper()).arg(QString::number(suggestedStart, 16).toUpper()), Qt::white);
            log(QString("   New End   = 0x%1 + 0x10000 = 0x%2").arg(QString::number(calculatedEnd, 16).toUpper()).arg(QString::number(suggestedEnd, 16).toUpper()), Qt::white);
            log("\n3. Clamping to Valid Memory Bounds:", Qt::lightGray);
            });

        uint32_t finalStart = std::max(arm9LoadAddr, suggestedStart);
        uint32_t finalEnd = std::min(suggestedEnd, NDS_RAM_END);

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("   Clamped Start: 0x%1").arg(QString::number(finalStart, 16).toUpper().rightJustified(8, '0')), Qt::white);
            log(QString("   Clamped End:   0x%1").arg(QString::number(finalEnd, 16).toUpper().rightJustified(8, '0')), Qt::white);
            log("\n4. Suggested Search Range (4KB Aligned):", Qt::lightGray);
            });

        uint32_t alignedStart = finalStart & 0xFFFFF000U;
        uint32_t alignedEnd = finalEnd & 0xFFFFF000U;

        QMetaObject::invokeMethod(this, [=]() {
            log(QString("   Start: 0x%1 (from 0x%2)").arg(QString::number(alignedStart, 16).toUpper()).arg(QString::number(finalStart, 16).toUpper()), Qt::white);
            log(QString("   End:   0x%1 (from 0x%2)").arg(QString::number(alignedEnd, 16).toUpper()).arg(QString::number(finalEnd, 16).toUpper()), Qt::white);

            log("\n--- [RESULT] ---", Qt::cyan);
            log(QString("  Start: %1").arg(m_manager->formatDisplayAddress(alignedStart)), Qt::green);
            log(QString("  End:   %1").arg(m_manager->formatDisplayAddress(alignedEnd)), Qt::green);
            });

        res.success = true;
        res.start = alignedStart;
        res.end = alignedEnd;
        return res;
    }

    // Saves the calculated NDS memory scanning boundaries to your local configuration settings.
    void NdsStaticRangeFinderDialog::onApplyClicked() {
        if (m_foundStart == 0 || m_foundEnd == 0) return;

        try {
            log("\n[APPLY] Saving approximate range to settings.ini...", Qt::yellow);
            auto settings = SettingsManager::load(EmulatorTarget::RALibretroNDS, m_manager->getDefaultSettings());
            settings.staticAddressStart = m_manager->formatDisplayAddress(m_foundStart);
            settings.staticAddressEnd = m_manager->formatDisplayAddress(m_foundEnd);
            SettingsManager::save(EmulatorTarget::RALibretroNDS, settings);

            QMessageBox::information(this, "Settings Applied",
                "Static range applied successfully!\nThe new range will be used the next time you open a scan window.");
            accept();
        }
        catch (...) {
            QMessageBox::critical(this, "Error", "Failed to save settings.");
        }
    }

    void NdsStaticRangeFinderDialog::log(const QString& message, const QColor& color) {
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
