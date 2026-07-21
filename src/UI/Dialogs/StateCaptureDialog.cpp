#include "StateCaptureDialog.h"

#include "../../Common/Memory.h"

#include <windows.h>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QtConcurrent>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::DataModels;

    StateCaptureDialog::StateCaptureDialog(Emulators::IEmulatorManager* manager,
        const AppSettings& settings,
        Core::MultiScanState* multiScanState,
        QWidget* parent)
        : QDialog(parent), m_manager(manager), m_multiScanState(multiScanState) {

        m_defaultSettings = m_manager->getDefaultSettings();
        setWindowTitle(QString("%1 State-Based Scan").arg(m_manager->getEmulatorName()));

        setFixedSize(450, 490);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMaximizeButtonHint & ~Qt::WindowMinimizeButtonHint);

        setupLayout();

        m_maxOffsetSpin->setValue(settings.maxOffset);
        m_useLastOffsetHintCheck->setChecked(settings.useLastOffsetHint);
        m_lastOffsetHintSpin->setEnabled(settings.useLastOffsetHint);

        if (settings.lastOffsetHint.has_value()) {
            m_lastOffsetHintSpin->setValue(settings.lastOffsetHint.value());
        }
        else {
            m_lastOffsetHintSpin->setValue(0);
        }

        m_maxLevelSpin->setValue(settings.maxLevel);
        m_maxCandidatesSpin->setValue(settings.maxCandidates);
        m_candidatesPerLevelSpin->setValue(settings.candidatesPerLevel);
        m_staticStartEdit->setText(settings.staticAddressStart);
        m_staticEndEdit->setText(settings.staticAddressEnd);
        m_stopOnFirstCheck->setChecked(settings.stopOnFirstPathFound);
        m_findAllLevelsCheck->setChecked(settings.findAllPathLevels);
        m_fastScanModeCheck->setChecked(settings.fastScanMode);
        m_printPartialPathsCheck->setChecked(settings.printPartialPaths);
        m_dynamicStaticDetectionCheck->setChecked(settings.dynamicStaticDetection);
        m_rangeGroup->setEnabled(!settings.dynamicStaticDetection);

        m_detectArraysCheck->setChecked(false);
        m_arrayRangeSpin->setValue(0x80);
        m_arrayRangeSpin->setEnabled(false);

        m_tableWidget->blockSignals(true);
        for (int i = 0; i < 4; ++i) {
            auto* state = m_multiScanState->getState(i);
            if (state) {
                auto* addrItem = new QTableWidgetItem(QString::number(state->targetAddress, 16).toUpper());
                m_tableWidget->setItem(i, 1, addrItem);

                auto* statusItem = new QTableWidgetItem("Captured");
                statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
                statusItem->setForeground(QColor(100, 200, 100));
                m_tableWidget->setItem(i, 2, statusItem);

                QWidget* cellWidget = m_tableWidget->cellWidget(i, 3);
                QPushButton* actionBtn = cellWidget ? cellWidget->findChild<QPushButton*>("captureBtn") : nullptr;
                if (actionBtn) {
                    actionBtn->setText("Release");
                }
            }
            else {
                m_tableWidget->setItem(i, 1, new QTableWidgetItem(""));

                auto* statusItem = new QTableWidgetItem("Empty");
                statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
                m_tableWidget->setItem(i, 2, statusItem);

                QWidget* cellWidget = m_tableWidget->cellWidget(i, 3);
                QPushButton* actionBtn = cellWidget ? cellWidget->findChild<QPushButton*>("captureBtn") : nullptr;
                if (actionBtn) {
                    actionBtn->setText("Capture");
                }
            }
        }
        m_tableWidget->blockSignals(false);

        updateScanButtonState();
    }

    StateCaptureDialog::~StateCaptureDialog() = default;

    QString StateCaptureDialog::getPlatformString() const {
        if (!m_manager) return "Hex";
        QString emu = m_manager->getEmulatorName();
        if (emu.contains("PCSX2", Qt::CaseInsensitive)) return "PS2";
        if (emu.contains("DuckStation", Qt::CaseInsensitive)) return "PS1";
        if (emu.contains("GBA", Qt::CaseInsensitive)) return "GBA";
        if (emu.contains("RALibretro", Qt::CaseInsensitive) || emu.contains("NDS", Qt::CaseInsensitive)) return "NDS";
        if (emu.contains("Dolphin", Qt::CaseInsensitive)) {
            if (emu.contains("Wii", Qt::CaseInsensitive)) return "Wii";
            return "GC";
        }
        if (emu.contains("PPSSPP", Qt::CaseInsensitive)) return "PSP";
        return "Hex";
    }

    void StateCaptureDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(8, 8, 8, 8);
        mainLayout->setSpacing(4);

        m_tableWidget = new QTableWidget(4, 4, this);
        m_tableWidget->setHorizontalHeaderLabels({ "State", QString("Target Address (%1, Hex)").arg(getPlatformString()), "Status", "Action" });
        m_tableWidget->verticalHeader()->setVisible(false);

        m_tableWidget->setFixedHeight(120);
        m_tableWidget->setFocusPolicy(Qt::NoFocus);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableWidget->verticalHeader()->setDefaultSectionSize(24);
        m_tableWidget->horizontalHeader()->setFixedHeight(22);
        m_tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_tableWidget->setColumnWidth(0, 58);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        m_tableWidget->setColumnWidth(2, 75);
        m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        m_tableWidget->setColumnWidth(3, 85);

        for (int i = 0; i < 4; ++i) {
            m_tableWidget->setRowHeight(i, 24);

            auto* slotItem = new QTableWidgetItem(QString("State %1").arg(i + 1));
            slotItem->setFlags(slotItem->flags() & ~Qt::ItemIsEditable);
            m_tableWidget->setItem(i, 0, slotItem);

            auto* statusItem = new QTableWidgetItem("Empty");
            statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
            m_tableWidget->setItem(i, 2, statusItem);

            auto* btn = new QPushButton("Capture", this);
            btn->setObjectName("captureBtn");
            btn->setFixedSize(72, 21);

            auto* cellContainer = new QWidget(this);
            auto* cellLayout = new QHBoxLayout(cellContainer);
            cellLayout->addWidget(btn);
            cellLayout->setContentsMargins(0, 0, 0, 0);
            cellLayout->setAlignment(Qt::AlignCenter);
            m_tableWidget->setCellWidget(i, 3, cellContainer);

            connect(btn, &QPushButton::clicked, this, [this, i]() {
                onCellWidgetClicked(i, 3);
                });
        }
        mainLayout->addWidget(m_tableWidget);

        m_rangeGroup = new QGroupBox(QString("Static Base Address Range (%1, Hex)").arg(getPlatformString()), this);
        m_rangeGroup->setFixedHeight(52);
        m_rangeGroup->setToolTip("<html><div style='width: 500px;'>Defines the memory range boundaries where the base pointer of the path must reside. Only pointers located inside this static block will be shown.</div></html>");

        auto* rangeLayout = new QHBoxLayout(m_rangeGroup);
        rangeLayout->setContentsMargins(6, 12, 6, 6);
        rangeLayout->setSpacing(4);
        m_staticStartEdit = new QLineEdit(this);
        m_staticEndEdit = new QLineEdit(this);
        m_resetRangeButton = new QPushButton("Reset", this);
        m_resetRangeButton->setFixedSize(50, 20);
        rangeLayout->addWidget(m_staticStartEdit);
        rangeLayout->addWidget(new QLabel("-"));
        rangeLayout->addWidget(m_staticEndEdit);
        rangeLayout->addWidget(m_resetRangeButton);
        mainLayout->addWidget(m_rangeGroup);

        m_parametersGroup = new QGroupBox("Search Parameters", this);
        auto* paramGrid = new QGridLayout(m_parametersGroup);
        paramGrid->setContentsMargins(8, 12, 8, 8);
        paramGrid->setHorizontalSpacing(10);
        paramGrid->setVerticalSpacing(4);

        m_maxLevelSpin = new CustomSpinBox(this, 5);
        m_maxLevelSpin->setRange(1, 50);
        m_maxLevelSpin->setToolTip("<html><div style='width: 500px;'>The maximum depth (number of pointer jumps) to search. Deeper levels scan deeper but increase complexity exponentially.</div></html>");

        m_candidatesPerLevelSpin = new CustomSpinBox(this, 5);
        m_candidatesPerLevelSpin->setRange(1, 1000);
        m_candidatesPerLevelSpin->setToolTip("<html><div style='width: 500px;'>Limits how many parent candidates are evaluated at each level. Low values (e.g., 3 to 7) dramatically speed up scans and favor logically sound paths.</div></html>");

        m_maxCandidatesSpin = new ThousandsSpinBox(this, 1000000);
        m_maxCandidatesSpin->setRange(1, 100000000);
        m_maxCandidatesSpin->setSingleStep(100000);
        m_maxCandidatesSpin->setToolTip("<html><div style='width: 500px;'>Clamps the maximum number of intermediate nodes generated during BFS search to protect RAM resources.</div></html>");

        m_maxOffsetSpin = new HexSpinBox(this, 0x1000);
        m_maxOffsetSpin->setRange(0, 0x7FFFFFFF);
        m_maxOffsetSpin->setSingleStep(1);
        m_maxOffsetSpin->setToolTip("<html><div style='width: 500px;'>The maximum distance allowed between pointer addresses. Match this with system limits (e.g., FFF for 4KB boundaries).</div></html>");

        m_useLastOffsetHintCheck = new QCheckBox("Last Offset Hint (Hex)", this);
        m_useLastOffsetHintCheck->setToolTip("<html><div style='width: 500px;'>Accelerates the search by forcing the first jump (Level 1) to use this exact offset value.</div></html>");

        m_lastOffsetHintSpin = new HexSpinBox(this, 0x1000);
        m_lastOffsetHintSpin->setRange(0, 0x7FFFFFFF);
        m_lastOffsetHintSpin->setSingleStep(1);
        m_lastOffsetHintSpin->setToolTip("<html><div style='width: 500px;'>Accelerates the search by forcing the first jump (Level 1) to use this exact offset value.</div></html>");

        m_detectArraysCheck = new QCheckBox("Detect Array Lists (Hex)", this);
        m_detectArraysCheck->setToolTip("<html><div style='width: 500px;'><b>Experimental:</b> Post-processes the static base addresses to check if they belong to contiguous structures or array sets. Excessively high ranges can cause instability.</div></html>");

        m_arrayRangeSpin = new HexSpinBox(this, 0x80);
        m_arrayRangeSpin->setRange(4, 0xFFFF);
        m_arrayRangeSpin->setToolTip("<html><div style='width: 500px;'><b>Experimental:</b> Post-processes the static base addresses to check if they belong to contiguous structures or array sets. Excessively high ranges can cause instability.</div></html>");

        paramGrid->addWidget(new QLabel("Max Level", this), 0, 0);
        paramGrid->addWidget(new QLabel("Candidates per Level", this), 0, 1);
        paramGrid->addWidget(new QLabel("Max Candidate Paths", this), 0, 2);

        paramGrid->addWidget(m_maxLevelSpin, 1, 0);
        paramGrid->addWidget(m_candidatesPerLevelSpin, 1, 1);
        paramGrid->addWidget(m_maxCandidatesSpin, 1, 2);

        paramGrid->addWidget(new QLabel(QString("Max Offset (%1, Hex)").arg(getPlatformString()), this), 2, 0);
        paramGrid->addWidget(m_useLastOffsetHintCheck, 2, 1);
        paramGrid->addWidget(m_detectArraysCheck, 2, 2);

        paramGrid->addWidget(m_maxOffsetSpin, 3, 0);
        paramGrid->addWidget(m_lastOffsetHintSpin, 3, 1);
        paramGrid->addWidget(m_arrayRangeSpin, 3, 2);

        mainLayout->addWidget(m_parametersGroup);

        m_optionsGroup = new QGroupBox("Scan Options", this);
        auto* optionsLayout = new QVBoxLayout(m_optionsGroup);
        optionsLayout->setContentsMargins(8, 14, 8, 6);
        optionsLayout->setSpacing(3);

        m_stopOnFirstCheck = new QCheckBox("Stop when first path found", this);
        m_stopOnFirstCheck->setToolTip("<html><div style='width: 500px;'>Stops search threads immediately once a single complete pointer chain matching all captured states is resolved.</div></html>");

        m_findAllLevelsCheck = new QCheckBox("Find all path levels (slower)", this);
        m_findAllLevelsCheck->setToolTip("<html><div style='width: 500px;'>Forces the search to scan deeper levels even if a complete static path was already found at an earlier level.</div></html>");

        m_fastScanModeCheck = new QCheckBox("Fast Mode (Aggressive Pruning)", this);
        m_fastScanModeCheck->setToolTip("<html><div style='width: 500px;'><b>Recommended:</b> Keeps track of visited nodes to prevent cyclic loops and redundant branch processing, preventing application memory exhaustion.</div></html>");

        m_printPartialPathsCheck = new QCheckBox("Output partial/broken paths", this);
        m_printPartialPathsCheck->setToolTip("<html><div style='width: 500px;'>Outputs paths that broke during search. Shows the exact offset step where the target connection became invalid.</div></html>");

        m_dynamicStaticDetectionCheck = new QCheckBox("Multi-Step Explore (Dynamic Base Detection)", this);
        m_dynamicStaticDetectionCheck->setToolTip("<html><div style='width: 500px;'><b>Experimental:</b> Disables the strict static range requirement. Instead, the search monitors each chain and terminates once a stable, consistent base address is resolved across all scans. Recommended for GBA.</div></html>");

        optionsLayout->addWidget(m_stopOnFirstCheck);
        optionsLayout->addWidget(m_findAllLevelsCheck);
        optionsLayout->addWidget(m_fastScanModeCheck);
        optionsLayout->addWidget(m_printPartialPathsCheck);
        optionsLayout->addWidget(m_dynamicStaticDetectionCheck);

        mainLayout->addWidget(m_optionsGroup);

        auto* bottomLayout = new QHBoxLayout();
        bottomLayout->setSpacing(6);
        m_helpButton = new QPushButton("?", this);
        m_helpButton->setFixedSize(24, 23);
        m_clearAllButton = new QPushButton("Clear All", this);
        m_clearAllButton->setFixedSize(75, 23);

        m_scanButton = new QPushButton("Scan", this);
        m_scanButton->setObjectName("okButton");
        m_scanButton->setFixedSize(75, 23);

        m_cancelButton = new QPushButton("Cancel", this);
        m_cancelButton->setObjectName("cancelButton");
        m_cancelButton->setFixedSize(75, 23);

        bottomLayout->addWidget(m_helpButton);
        bottomLayout->addWidget(m_clearAllButton);
        bottomLayout->addStretch();
        bottomLayout->addWidget(m_scanButton);
        bottomLayout->addWidget(m_cancelButton);
        mainLayout->addLayout(bottomLayout);

        m_scanButton->setDefault(true);

        connect(m_useLastOffsetHintCheck, &QCheckBox::checkStateChanged, this, &StateCaptureDialog::onUseLastOffsetHintChanged);
        connect(m_detectArraysCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
            m_arrayRangeSpin->setEnabled(state == Qt::Checked);
            });
        connect(m_dynamicStaticDetectionCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
            m_rangeGroup->setEnabled(state != Qt::Checked);
            });
        connect(m_clearAllButton, &QPushButton::clicked, this, &StateCaptureDialog::onClearAllClicked);
        connect(m_resetRangeButton, &QPushButton::clicked, this, &StateCaptureDialog::onResetRangeClicked);
        connect(m_helpButton, &QPushButton::clicked, this, &StateCaptureDialog::onHelpClicked);
        connect(m_scanButton, &QPushButton::clicked, this, &StateCaptureDialog::onScanClicked);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        connect(m_staticStartEdit, &QLineEdit::editingFinished, this, &StateCaptureDialog::onFieldsSanitizationFinished);
        connect(m_staticEndEdit, &QLineEdit::editingFinished, this, &StateCaptureDialog::onFieldsSanitizationFinished);
        connect(m_tableWidget, &QTableWidget::itemChanged, this, &StateCaptureDialog::onTableItemChanged);
    }

    void StateCaptureDialog::onFieldsSanitizationFinished() {
        QLineEdit* edit = qobject_cast<QLineEdit*>(sender());
        if (!edit) return;
        edit->setText(sanitizeHexInput(edit->text()));
    }

    void StateCaptureDialog::onTableItemChanged(QTableWidgetItem* item) {
        if (item && item->column() == 1) {
            m_tableWidget->blockSignals(true);
            item->setText(sanitizeHexInput(item->text()));
            m_tableWidget->blockSignals(false);
        }
    }

    void StateCaptureDialog::onUseLastOffsetHintChanged() {
        m_lastOffsetHintSpin->setEnabled(m_useLastOffsetHintCheck->isChecked());
    }

    // Handles slot capture/release button clicks. When clicked, it copies a snapshot
    // of the emulator's current RAM and saves it in the selected slot.
    void StateCaptureDialog::onCellWidgetClicked(int row, int col) {
        QWidget* cellWidget = m_tableWidget->cellWidget(row, col);
        QPushButton* btn = cellWidget ? cellWidget->findChild<QPushButton*>("captureBtn") : nullptr;
        if (!btn) return;

        if (btn->text() == "Release") {
            m_multiScanState->releaseState(row);
            m_tableWidget->blockSignals(true);
            m_tableWidget->setItem(row, 1, new QTableWidgetItem(""));
            auto* statusItem = new QTableWidgetItem("Empty");
            statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
            m_tableWidget->setItem(row, 2, statusItem);
            m_tableWidget->blockSignals(false);
            btn->setText("Capture");

            updateScanButtonState();
            Core::Memory::forceGarbageCollection();
            return;
        }

        auto* addressItem = m_tableWidget->item(row, 1);
        QString addressText = addressItem ? addressItem->text().trimmed() : "";
        addressText = sanitizeHexInput(addressText);

        if (addressText.isEmpty() || addressText == "0") {
            QMessageBox::warning(this, "Input Required", QString("Please enter a target address for Slot %1.").arg(row + 1));
            return;
        }

        uint32_t targetAddress = m_manager->unnormalizeAddress(addressText);

        if (targetAddress % 4 != 0) {
            uint32_t corrected = targetAddress & 0xFFFFFFFC;
            QString correctionMsg = QString(
                "The target address 0x%1 is not 4-byte aligned. Pointer scans operate on 32-bit (4-byte) values, "
                "so the target address must be a multiple of 4.\n\n"
                "This can happen on systems like Wii/GameCube when reading an 8-bit value from a Big-Endian address.\n\n"
                "The address will be automatically corrected to: 0x%2\n\n"
                "Click OK to apply this correction and continue."
            ).arg(QString::number(targetAddress, 16).toUpper())
                .arg(QString::number(corrected, 16).toUpper());

            QMessageBox::information(this, "Address Alignment Correction", correctionMsg);
            targetAddress = corrected;
            m_tableWidget->blockSignals(true);
            m_tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(targetAddress, 16).toUpper()));
            m_tableWidget->blockSignals(false);
        }

        if (!m_manager->verifyAttachment()) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_manager->getProcessId());
            bool running = false;
            if (hProcess) {
                DWORD exitCode = 0;
                if (GetExitCodeProcess(hProcess, &exitCode)) {
                    running = (exitCode == STILL_ACTIVE);
                }
                CloseHandle(hProcess);
            }

            if (running) {
                auto reply = QMessageBox::question(
                    this,
                    "Memory Layout Changed",
                    "The emulator's memory layout has changed (possibly due to a game reboot or ASLR shift).\n\n"
                    "Would you like to reattach to the current memory banks?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes
                );

                if (reply == QMessageBox::Yes) {
                    if (m_manager->attach(m_manager->getProcessId())) {
                        QMessageBox::information(this, "Success", "Successfully reattached to the new memory layout.");
                    }
                    else {
                        QMessageBox::critical(this, "Reattach Failed", "Could not locate the emulator's memory banks. Please check if the game is running.");
                        return;
                    }
                }
                else {
                    return;
                }
            }
            else {
                QMessageBox::critical(this, "Process Terminated", "The emulator process has terminated. Please close this scan and attach to a new process.");
                return;
            }
        }

        auto* statusItem = new QTableWidgetItem("Capturing...");
        statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
        statusItem->setForeground(QColor(255, 165, 0));
        m_tableWidget->setItem(row, 2, statusItem);

        m_tableWidget->setEnabled(false);
        setCursor(Qt::WaitCursor);

        Core::Memory::forceGarbageCollection();

        QFuture<std::vector<uint8_t>> future = QtConcurrent::run([this]() {
            return m_manager->readMemory(m_manager->getMainMemoryStart(), m_manager->getMainMemorySize());
            });

        auto* watcher = new QFutureWatcher<std::vector<uint8_t>>(this);
        connect(watcher, &QFutureWatcher<std::vector<uint8_t>>::finished, this, [=]() {
            std::vector<uint8_t> dump = watcher->result();
            watcher->deleteLater();

            m_tableWidget->setEnabled(true);
            unsetCursor();

            if (dump.empty()) {
                QMessageBox::critical(this, "Capture Failed", "Failed to read emulator memory.");
                auto* statusItem = new QTableWidgetItem("Failed");
                statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
                statusItem->setForeground(Qt::red);
                m_tableWidget->setItem(row, 2, statusItem);
                return;
            }

            ScanState state;
            state.memoryDump = std::move(dump);
            state.targetAddress = targetAddress;

            m_multiScanState->captureState(row, state);

            m_tableWidget->blockSignals(true);
            m_tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(targetAddress, 16).toUpper()));
            auto* statusItem = new QTableWidgetItem("Captured");
            statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
            statusItem->setForeground(QColor(100, 200, 100));
            m_tableWidget->setItem(row, 2, statusItem);
            m_tableWidget->blockSignals(false);
            btn->setText("Release");

            updateScanButtonState();
            });
        watcher->setFuture(future);
    }

    // Releases and wipes all currently captured RAM snapshots to free up computer memory.
    void StateCaptureDialog::onClearAllClicked() {
        m_multiScanState->clearAll();
        m_tableWidget->blockSignals(true);
        for (int i = 0; i < 4; ++i) {
            m_tableWidget->setItem(i, 1, new QTableWidgetItem(""));
            auto* statusItem = new QTableWidgetItem("Empty");
            statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
            m_tableWidget->setItem(i, 2, statusItem);
            QWidget* cellWidget = m_tableWidget->cellWidget(i, 3);
            QPushButton* btn = cellWidget ? cellWidget->findChild<QPushButton*>("captureBtn") : nullptr;
            if (btn) btn->setText("Capture");
        }
        m_tableWidget->blockSignals(false);
        updateScanButtonState();
        Core::Memory::forceGarbageCollection();
    }

    void StateCaptureDialog::onResetRangeClicked() {
        m_staticStartEdit->setText(m_defaultSettings.staticAddressStart);
        m_staticEndEdit->setText(m_defaultSettings.staticAddressEnd);
    }

    void StateCaptureDialog::onHelpClicked() {
        QString helpText =
            "--- State-Based Pointer Scan Help ---\n\n"
            "How it works:\n"
            "This tool uses a Breadth-First Search (BFS) algorithm to work backwards from your target address. By verifying candidates across multiple memory dumps (states), it quickly filters out temporary addresses and isolates highly reliable, static pointer chains.\n\n"
            "Crucial Settings for Beginners:\n\n"
            "• Candidates per Level:\n"
            "Keep this LOW (e.g., 2 to 5). This forces the scanner to prefer smaller, tighter offsets. A low number usually results in a much more accurate and stable pointer chain that makes logical sense.\n\n"
            "• Last Offset Hint:\n"
            "If you used a debugger and know the exact final offset the game uses for your value (e.g., +4C0), enter it here. This speeds up the scan by skipping thousands of incorrect branches right at Level 1.\n\n"
            "• Fast Mode (Aggressive Pruning):\n"
            "Leaving this ON prevents the scanner from going in circles or duplicating work, protecting your system memory during deep scans.";

        QMessageBox::information(this, "Scanner Information & Tips", helpText);
    }

    void StateCaptureDialog::onScanClicked() {
        accept();
    }

    void StateCaptureDialog::updateScanButtonState() {
        m_scanButton->setEnabled(m_multiScanState->getCapturedCount() >= 2);
    }

    QString StateCaptureDialog::sanitizeHexInput(const QString& input) {
        QString clean = input.trimmed().toUpper();
        if (clean.startsWith("0X")) clean = clean.mid(2);

        QString result;
        for (QChar c : clean) {
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
                result.append(c);
            }
        }

        while (result.startsWith('0') && result.length() > 1) {
            result.remove(0, 1);
        }
        return result.isEmpty() ? "0" : result;
    }

    // Gathers your configured ranges, offsets, and snapshot objects into an 
    // execution package so the scanning engine can run.
    ScanParameters StateCaptureDialog::getScanParameters() const {
        ScanParameters params;

        params.maxOffset = m_maxOffsetSpin->value();
        if (m_useLastOffsetHintCheck->isChecked()) {
            params.lastOffsetHint = m_lastOffsetHintSpin->value();
        }
        else {
            params.lastOffsetHint = std::nullopt;
        }

        params.staticBaseStart = m_manager->unnormalizeAddress(m_staticStartEdit->text());
        params.staticBaseEnd = m_manager->unnormalizeAddress(m_staticEndEdit->text());
        params.maxLevel = m_maxLevelSpin->value();
        params.maxCandidates = m_maxCandidatesSpin->value();
        params.candidatesPerLevel = m_candidatesPerLevelSpin->value();
        params.stopOnFirstPathFound = m_stopOnFirstCheck->isChecked();
        params.findAllPathLevels = m_findAllLevelsCheck->isChecked();
        params.fastScanMode = m_fastScanModeCheck->isChecked();
        params.printPartialPaths = m_printPartialPathsCheck->isChecked();
        params.dynamicStaticDetection = m_dynamicStaticDetectionCheck->isChecked();

        params.detectArrays = m_detectArraysCheck->isChecked();
        params.arraySearchRange = m_arrayRangeSpin->value();

        int lastIdx = m_multiScanState->getLastCapturedSlotIndex();
        if (lastIdx != -1) {
            params.finalAddressTarget = m_multiScanState->getState(lastIdx)->targetAddress;
        }
        else {
            auto active = m_multiScanState->getCapturedStates();
            params.finalAddressTarget = active.empty() ? 0 : active.front().targetAddress;
        }

        params.capturedStates = m_multiScanState->getCapturedStates();
        return params;
    }

    void StateCaptureDialog::updateSettings(AppSettings& settings) {
        settings.maxOffset = m_maxOffsetSpin->value();
        settings.useLastOffsetHint = m_useLastOffsetHintCheck->isChecked();
        if (settings.useLastOffsetHint) {
            settings.lastOffsetHint = m_lastOffsetHintSpin->value();
        }
        else {
            settings.lastOffsetHint = std::nullopt;
        }
        settings.staticAddressStart = m_staticStartEdit->text().trimmed().toUpper();
        settings.staticAddressEnd = m_staticEndEdit->text().trimmed().toUpper();
        settings.maxLevel = m_maxLevelSpin->value();
        settings.maxCandidates = m_maxCandidatesSpin->value();
        settings.candidatesPerLevel = m_candidatesPerLevelSpin->value();
        settings.stopOnFirstPathFound = m_stopOnFirstCheck->isChecked();
        settings.findAllPathLevels = m_findAllLevelsCheck->isChecked();
        settings.fastScanMode = m_fastScanModeCheck->isChecked();
        settings.printPartialPaths = m_printPartialPathsCheck->isChecked();
        settings.dynamicStaticDetection = m_dynamicStaticDetectionCheck->isChecked();
    }

}
