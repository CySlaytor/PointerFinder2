#include "EditValueDescriptionsDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

EditValueDescriptionsDialog::EditValueDescriptionsDialog(const QString& variableName,
                                                         const QStringList& initialDescriptions,
                                                         QWidget* parent)
    : QDialog(parent), m_variableName(variableName) {
    setWindowTitle(QString("Value Descriptions - %1").arg(variableName.isEmpty() ? "Variable" : variableName));
    setModal(true);
    resize(480, 360);
    setMinimumSize(400, 260);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupLayout(initialDescriptions);
}

void EditValueDescriptionsDialog::setupLayout(const QStringList& initialDescriptions) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    auto* headerLabel = new QLabel(
        "<b>Value Descriptions:</b> (one per line, e.g. 0x01=Running)<br>"
        "<span style='color: #8B949E; font-size: 11px;'>Indentation dots are managed automatically. Paste your list freely.</span>",
        this);
    headerLabel->setWordWrap(true);
    mainLayout->addWidget(headerLabel);

    m_descriptionsTextEdit = new QPlainTextEdit(this);
    QFont monoFont("Consolas", 10);
    monoFont.setStyleHint(QFont::Monospace);
    m_descriptionsTextEdit->setFont(monoFont);
    m_descriptionsTextEdit->setPlaceholderText("0x00=Idle\n0x01=Walking\n0x02=Jumping");

    // Clean any leading dots from initial descriptions so the user edits pure value-description lines
    QStringList cleanLines;
    for (const auto& line : initialDescriptions) {
        QString trimmed = line.trimmed();
        int idx = 0;
        while (idx < trimmed.length() && (trimmed[idx] == '.' || trimmed[idx] == ' ')) {
            idx++;
        }
        cleanLines.append(trimmed.mid(idx).trimmed());
    }
    m_descriptionsTextEdit->setPlainText(cleanLines.join("\n"));
    mainLayout->addWidget(m_descriptionsTextEdit, 1);

    // Bottom Action Buttons with unconstrained custom styling to prevent theme clipping
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 4, 0, 0);
    btnLayout->setSpacing(8);

    m_saveBtn = new QPushButton("Save", this);
    m_saveBtn->setObjectName("saveDescriptionsBtn");
    m_saveBtn->setMinimumWidth(80);
    m_saveBtn->setFixedHeight(24);
    m_saveBtn->setDefault(true);

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setObjectName("cancelDescriptionsBtn");
    m_cancelBtn->setMinimumWidth(75);
    m_cancelBtn->setFixedHeight(24);

    btnLayout->addStretch();
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_saveBtn, &QPushButton::clicked, this, &EditValueDescriptionsDialog::onSaveClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_descriptionsTextEdit->setFocus();
}

void EditValueDescriptionsDialog::onSaveClicked() {
    accept();
}

QStringList EditValueDescriptionsDialog::getDescriptions() const {
    QString text = m_descriptionsTextEdit->toPlainText().trimmed();
    if (text.isEmpty())
        return {};

    QStringList rawLines = text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    QStringList result;
    for (const auto& line : rawLines) {
        QString trimmed = line.trimmed();
        int idx = 0;
        while (idx < trimmed.length() && (trimmed[idx] == '.' || trimmed[idx] == ' ')) {
            idx++;
        }
        QString clean = trimmed.mid(idx).trimmed();
        if (!clean.isEmpty()) {
            result.append(clean);
        }
    }
    return result;
}

} // namespace PointerFinder2::UI
