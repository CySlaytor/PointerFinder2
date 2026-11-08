#include "AddPointerDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

AddPointerDialog::AddPointerDialog(const QString& defaultName, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Add as Pointer");
    setModal(true);
    setFixedSize(330, 104);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupLayout(defaultName);
}

void AddPointerDialog::setupLayout(const QString& defaultName) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    auto* fieldLayout = new QVBoxLayout();
    fieldLayout->setSpacing(3);

    auto* label = new QLabel("Pointer Name:", this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(defaultName);
    m_nameEdit->setPlaceholderText("e.g. PlayerObject, Coordinates");

    fieldLayout->addWidget(label);
    fieldLayout->addWidget(m_nameEdit);
    mainLayout->addLayout(fieldLayout);

    auto* buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("OK", this);
    m_okButton->setObjectName("okButton");
    m_okButton->setDefault(true);

    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setObjectName("cancelButton");

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &QDialog::accept);

    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

QString AddPointerDialog::getPointerName() const { return m_nameEdit ? m_nameEdit->text().trimmed() : QString(); }

QString AddPointerDialog::getFullDescription(const QString& platformPointerType) const {
    QString name = getPointerName();

    if (name.isEmpty() || name.compare("Pointer", Qt::CaseInsensitive) == 0) {
        return platformPointerType;
    }

    if (name.startsWith("[")) {
        return name;
    }

    if (name.startsWith("Pointer to ", Qt::CaseInsensitive)) {
        QString tag = platformPointerType;
        int bracketEnd = tag.indexOf(']');
        if (bracketEnd != -1) {
            tag = tag.left(bracketEnd + 1);
        } else {
            int spaceIdx = tag.indexOf(' ');
            if (spaceIdx != -1) {
                tag = tag.left(spaceIdx);
            }
        }
        return QString("%1 %2").arg(tag, name);
    }

    return QString("%1 to %2").arg(platformPointerType, name);
}

} // namespace PointerFinder2::UI
