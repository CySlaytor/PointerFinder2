#include "CloneStrideDialog.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    CloneStrideDialog::CloneStrideDialog(Emulators::IEmulatorManager* manager,
                                         uint32_t currentAddress,
                                         const QString& currentNoteText,
                                         QWidget* parent)
        : QDialog(parent),
          m_manager(manager),
          m_currentAddress(currentAddress),
          m_originalNoteText(currentNoteText) {
        setWindowTitle("Clone Structure Stride (Batch Note Generator)");
        setModal(true);
        resize(860, 560);
        setMinimumSize(780, 480);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        setupLayout();
        onParametersChanged();
    }

    CloneStrideDialog::~CloneStrideDialog() = default;

    void CloneStrideDialog::parseInitialNote(uint32_t& outStride, QString& outTemplate, QString& outFirstName) {
        outStride = 64; // Fallback default stride
        outFirstName = "";
        outTemplate = "Structure {INDEX:02}";

        QString trimmed = m_originalNoteText.trimmed();
        if (trimmed.isEmpty()) return;

        QStringList lines = trimmed.split(QRegularExpression("[\r\n]+"));
        QString header = lines.first().trimmed();

        // Separate header line from structure body offsets
        if (lines.size() > 1) {
            QStringList bodyLines = lines.mid(1);
            m_originalBody = "\n" + bodyLines.join("\n");
        } else {
            m_originalBody = "";
        }

        // 1. Auto-detect stride from [N bytes] tag in header (e.g. [144 bytes])
        QString sizeTag = "";
        static const QRegularExpression sizeTagRx(R"(^(\s*\[(?:(\d+)\s*x\s*(\d+)|(\d+))\s*bytes\]\s*))", QRegularExpression::CaseInsensitiveOption);
        auto match = sizeTagRx.match(header);
        if (match.hasMatch()) {
            sizeTag = match.captured(1).trimmed();
            if (!match.captured(2).isEmpty() && !match.captured(3).isEmpty()) {
                outStride = match.captured(2).toUInt() * match.captured(3).toUInt();
            } else if (!match.captured(4).isEmpty()) {
                outStride = match.captured(4).toUInt();
            }
        }

        // 2. Auto-detect numbering and parenthesized names in title
        // Isolate title without the size tag so the stride number (e.g. 144) is never mistaken for a sequence index
        QString titleCandidate = header;
        if (!sizeTag.isEmpty()) {
            titleCandidate = titleCandidate.mid(match.capturedLength()).trimmed();
        }

        // Extract custom name inside parentheses, e.g. "(Isolation)"
        static const QRegularExpression parenRx(R"(\(([^)]+)\))");
        auto parenMatch = parenRx.match(titleCandidate);
        if (parenMatch.hasMatch()) {
            outFirstName = parenMatch.captured(1).trimmed();
            titleCandidate.replace(parenMatch.capturedStart(), parenMatch.capturedLength(), "({NAME})");
        }

        // Detect sequential index e.g. "Level 01", "Level 1", "#01"
        static const QRegularExpression numRx(R"((\b(?:Level|Stage|Player|Char|Slot|Item|#)?\s*)(\d+))", QRegularExpression::CaseInsensitiveOption);
        auto numMatch = numRx.match(titleCandidate);
        if (numMatch.hasMatch()) {
            QString digits = numMatch.captured(2);
            QString prefix = numMatch.captured(1);
            QString token = (digits.length() >= 2) ? "{INDEX:02}" : "{INDEX}";
            titleCandidate.replace(numMatch.capturedStart(2), digits.length(), token);
        }

        // Re-attach size tag prefix to the template
        if (!sizeTag.isEmpty()) {
            outTemplate = sizeTag + " " + titleCandidate;
        } else {
            outTemplate = titleCandidate;
        }

        m_detectedFirstName = outFirstName;
    }

    void CloneStrideDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        uint32_t detectedStride = 64;
        QString detectedTemplate;
        QString detectedFirstName;
        parseInitialNote(detectedStride, detectedTemplate, detectedFirstName);

        // Parameters Form Group
        auto* paramGroup = new QGroupBox("Stride Parameters && Title Pattern", this);
        auto* formGrid = new QGridLayout(paramGroup);
        formGrid->setContentsMargins(10, 14, 10, 10);
        formGrid->setHorizontalSpacing(10);
        formGrid->setVerticalSpacing(6);

        // Row 0: Base Address & Stride
        formGrid->addWidget(new QLabel("Start Base Address (Hex):", this), 0, 0);
        m_baseAddressEdit = new QLineEdit(this);
        QString baseStr = m_manager ? m_manager->formatDisplayAddress(m_currentAddress) : QString::number(m_currentAddress, 16).toUpper();
        m_baseAddressEdit->setText(baseStr);
        m_baseAddressEdit->setFixedWidth(120);
        formGrid->addWidget(m_baseAddressEdit, 0, 1);

        auto* strideLayout = new QHBoxLayout();
        strideLayout->setSpacing(4);
        m_strideSpin = new QSpinBox(this);
        m_strideSpin->setRange(1, 0x100000);
        m_strideSpin->setValue(static_cast<int>(detectedStride));
        m_strideSpin->setFixedWidth(90);
        m_strideHexLabel = new QLabel(QString("(+0x%1)").arg(detectedStride, 2, 16, QChar('0')).toUpper(), this);
        m_strideHexLabel->setStyleSheet("color: #58A6FF; font-weight: bold; font-family: Consolas;");
        strideLayout->addWidget(m_strideSpin);
        strideLayout->addWidget(m_strideHexLabel);
        strideLayout->addStretch();

        formGrid->addWidget(new QLabel("Struct Stride (Bytes):", this), 0, 2);
        formGrid->addLayout(strideLayout, 0, 3);

        // Row 1: Instance Count
        formGrid->addWidget(new QLabel("Instance Count:", this), 1, 0);
        m_countSpin = new QSpinBox(this);
        m_countSpin->setRange(2, 256);
        m_countSpin->setValue(16);
        m_countSpin->setFixedWidth(120);
        formGrid->addWidget(m_countSpin, 1, 1);

        QLabel* tokensHelpLabel = new QLabel("Tokens: <b>{INDEX}</b>, <b>{INDEX:02}</b>, <b>{OFFSET}</b>, <b>{NAME}</b>", this);
        tokensHelpLabel->setStyleSheet("color: #8B949E; font-size: 11px;");
        formGrid->addWidget(tokensHelpLabel, 1, 2, 1, 2);

        // Row 2: Template String
        formGrid->addWidget(new QLabel("Header Template:", this), 2, 0);
        m_templateEdit = new QLineEdit(this);
        m_templateEdit->setText(detectedTemplate);
        formGrid->addWidget(m_templateEdit, 2, 1, 1, 3);

        mainLayout->addWidget(paramGroup);

        // Preview Table Section
        auto* tableHeaderLayout = new QHBoxLayout();
        auto* tableDescLayout = new QVBoxLayout();
        tableDescLayout->setSpacing(2);
        tableDescLayout->addWidget(new QLabel("<b>Generated Code Notes (Excel-Style In-Cell Editing):</b>", this));
        auto* hintLabel = new QLabel("Double-click any cell to customize names/titles. Staged in memory until saved.", this);
        hintLabel->setStyleSheet("color: #8B949E; font-size: 11px;");
        tableDescLayout->addWidget(hintLabel);
        tableHeaderLayout->addLayout(tableDescLayout);
        tableHeaderLayout->addStretch();

        m_pasteNamesBtn = new QPushButton("Paste Names from Clipboard", this);
        m_pasteNamesBtn->setFixedHeight(26);
        m_pasteNamesBtn->setToolTip("Paste multi-line text from clipboard to instantly populate custom names for each row");
        tableHeaderLayout->addWidget(m_pasteNamesBtn);
        mainLayout->addLayout(tableHeaderLayout);

        m_previewTable = new QTableWidget(this);
        m_previewTable->setColumnCount(5);
        m_previewTable->setHorizontalHeaderLabels({ "#", "Address", "Stride", "Level / Name (Optional)", "Generated Note Title (Editable Preview)" });
        m_previewTable->setAlternatingRowColors(true);
        m_previewTable->verticalHeader()->setVisible(false);
        m_previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_previewTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        m_previewTable->setFont(monoFont);

        m_previewTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_previewTable->setColumnWidth(0, 36);
        m_previewTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        m_previewTable->setColumnWidth(1, 105);
        m_previewTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        m_previewTable->setColumnWidth(2, 75);
        m_previewTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
        m_previewTable->setColumnWidth(3, 170);
        m_previewTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

        mainLayout->addWidget(m_previewTable, 1);

        // Bottom Action Row
        auto* bottomLayout = new QHBoxLayout();
        bottomLayout->setContentsMargins(0, 6, 0, 2);

        m_generateBtn = new QPushButton("Generate Notes", this);
        m_generateBtn->setObjectName("cloneGenerateBtn");
        m_generateBtn->setMinimumWidth(120);
        m_generateBtn->setFixedHeight(26);
        m_generateBtn->setDefault(true);

        m_cancelBtn = new QPushButton("Cancel", this);
        m_cancelBtn->setObjectName("cloneCancelBtn");
        m_cancelBtn->setMinimumWidth(80);
        m_cancelBtn->setFixedHeight(26);

        bottomLayout->addStretch();
        bottomLayout->addWidget(m_generateBtn);
        bottomLayout->addWidget(m_cancelBtn);
        mainLayout->addLayout(bottomLayout);

        // Connections
        connect(m_baseAddressEdit, &QLineEdit::textChanged, this, &CloneStrideDialog::onParametersChanged);
        connect(m_strideSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CloneStrideDialog::onParametersChanged);
        connect(m_countSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CloneStrideDialog::onParametersChanged);
        connect(m_templateEdit, &QLineEdit::textChanged, this, &CloneStrideDialog::onParametersChanged);
        connect(m_pasteNamesBtn, &QPushButton::clicked, this, &CloneStrideDialog::onPasteNamesFromClipboard);
        connect(m_previewTable, &QTableWidget::itemChanged, this, &CloneStrideDialog::onTableItemChanged);

        connect(m_generateBtn, &QPushButton::clicked, this, &CloneStrideDialog::onGenerateClicked);
        connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    }

    uint32_t CloneStrideDialog::getBaseAddress() const {
        QString clean = m_baseAddressEdit ? m_baseAddressEdit->text().trimmed().toUpper() : "";
        if (clean.startsWith("0X")) clean = clean.mid(2);

        bool ok = false;
        uint32_t parsed = clean.toUInt(&ok, 16);
        if (!ok) return m_currentAddress;
        return m_manager ? m_manager->unnormalizeAddress(QString::number(parsed, 16)) : parsed;
    }

    QString CloneStrideDialog::formatTitle(int index, uint32_t offsetBytes, const QString& customName) const {
        QString tmpl = m_templateEdit ? m_templateEdit->text() : "Structure {INDEX:02}";

        // Replace index placeholders
        QString idxStr = QString::number(index + 1);
        QString idx02Str = QString("%1").arg(index + 1, 2, 10, QChar('0'));
        QString idx03Str = QString("%1").arg(index + 1, 3, 10, QChar('0'));

        tmpl.replace("{INDEX:03}", idx03Str);
        tmpl.replace("{INDEX:02}", idx02Str);
        tmpl.replace("{INDEX}", idxStr);

        // Replace offset placeholder
        QString offHex = QString("+0x%1").arg(offsetBytes, 2, 16, QChar('0')).toUpper();
        tmpl.replace("{OFFSET}", offHex);

        // Replace name placeholder
        if (tmpl.contains("({NAME})")) {
            if (customName.trimmed().isEmpty()) {
                tmpl.replace("({NAME})", "");
            } else {
                tmpl.replace("({NAME})", "(" + customName.trimmed() + ")");
            }
        } else {
            tmpl.replace("{NAME}", customName.trimmed());
        }

        // Clean up redundant consecutive spaces
        return tmpl.simplified();
    }

    void CloneStrideDialog::onParametersChanged() {
        if (m_isUpdatingTable) return;

        uint32_t stride = static_cast<uint32_t>(m_strideSpin->value());
        m_strideHexLabel->setText(QString("(+0x%1)").arg(stride, 2, 16, QChar('0')).toUpper());

        rebuildTableRows();
    }

    void CloneStrideDialog::rebuildTableRows() {
        m_isUpdatingTable = true;

        uint32_t baseAddr = getBaseAddress();
        uint32_t stride = static_cast<uint32_t>(m_strideSpin->value());
        int count = m_countSpin->value();

        // Preserve existing names previously typed into the table
        std::vector<QString> existingNames;
        for (int i = 0; i < m_previewTable->rowCount(); ++i) {
            auto* item = m_previewTable->item(i, 3);
            existingNames.push_back(item ? item->text() : "");
        }

        m_previewTable->setRowCount(count);

        for (int i = 0; i < count; ++i) {
            uint32_t rowAddr = baseAddr + (i * stride);
            uint32_t offBytes = i * stride;

            QString addrStr = m_manager ? m_manager->formatDisplayAddress(rowAddr) : QString::number(rowAddr, 16).toUpper().rightJustified(8, '0');
            QString offStr = (i == 0) ? "+0x00" : QString("+0x%1").arg(offBytes, 2, 16, QChar('0')).toUpper();

            // Column 0: Index #
            auto* idxItem = new QTableWidgetItem(QString::number(i + 1));
            idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
            idxItem->setTextAlignment(Qt::AlignCenter);
            m_previewTable->setItem(i, 0, idxItem);

            // Column 1: Address
            auto* addrItem = new QTableWidgetItem("0x" + addrStr);
            addrItem->setFlags(addrItem->flags() & ~Qt::ItemIsEditable);
            addrItem->setData(Qt::UserRole, rowAddr);
            m_previewTable->setItem(i, 1, addrItem);

            // Column 2: Offset
            auto* offItem = new QTableWidgetItem(offStr);
            offItem->setFlags(offItem->flags() & ~Qt::ItemIsEditable);
            offItem->setTextAlignment(Qt::AlignCenter);
            m_previewTable->setItem(i, 2, offItem);

            // Column 3: Custom Name
            QString name = "";
            if (i < static_cast<int>(existingNames.size()) && !existingNames[i].isEmpty()) {
                name = existingNames[i];
            } else if (i == 0 && !m_detectedFirstName.isEmpty()) {
                name = m_detectedFirstName;
            }

            auto* nameItem = new QTableWidgetItem(name);
            m_previewTable->setItem(i, 3, nameItem);

            // Column 4: Full Title Preview
            QString fullTitle = formatTitle(i, offBytes, name);
            auto* titleItem = new QTableWidgetItem(fullTitle);
            m_previewTable->setItem(i, 4, titleItem);
        }

        m_isUpdatingTable = false;
    }

    void CloneStrideDialog::onTableItemChanged(QTableWidgetItem* item) {
        if (m_isUpdatingTable || !item) return;

        int row = item->row();
        int col = item->column();

        // When custom name (Column 3) is edited, automatically reformat the full title preview (Column 4)
        if (col == 3) {
            m_isUpdatingTable = true;
            uint32_t stride = static_cast<uint32_t>(m_strideSpin->value());
            uint32_t offBytes = row * stride;
            QString formatted = formatTitle(row, offBytes, item->text());
            m_previewTable->setItem(row, 4, new QTableWidgetItem(formatted));
            m_isUpdatingTable = false;
        }
    }

    void CloneStrideDialog::onPasteNamesFromClipboard() {
        QString text = QApplication::clipboard()->text();
        if (text.trimmed().isEmpty()) return;

        QStringList lines = text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        if (lines.isEmpty()) return;

        m_isUpdatingTable = true;

        int rowCount = m_previewTable->rowCount();
        uint32_t stride = static_cast<uint32_t>(m_strideSpin->value());

        for (int i = 0; i < std::min(static_cast<int>(lines.size()), rowCount); ++i) {
            QString name = lines[i].trimmed();
            m_previewTable->setItem(i, 3, new QTableWidgetItem(name));

            uint32_t offBytes = i * stride;
            QString formatted = formatTitle(i, offBytes, name);
            m_previewTable->setItem(i, 4, new QTableWidgetItem(formatted));
        }

        m_isUpdatingTable = false;
    }

    void CloneStrideDialog::onGenerateClicked() {
        m_generatedNotes.clear();

        for (int i = 0; i < m_previewTable->rowCount(); ++i) {
            auto* addrItem = m_previewTable->item(i, 1);
            auto* titleItem = m_previewTable->item(i, 4);

            if (!addrItem || !titleItem) continue;

            uint32_t rowAddr = addrItem->data(Qt::UserRole).toUInt();
            QString fullTitle = titleItem->text().trimmed();

            GeneratedStrideNote note;
            note.address = rowAddr;
            note.title = fullTitle;
            note.noteText = fullTitle + m_originalBody;

            m_generatedNotes.push_back(note);
        }

        if (m_generatedNotes.empty()) {
            QMessageBox::warning(this, "No Notes", "No code notes were generated.");
            return;
        }

        accept();
    }

    std::vector<GeneratedStrideNote> CloneStrideDialog::getGeneratedNotes() const {
        return m_generatedNotes;
    }

}
