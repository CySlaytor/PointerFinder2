#include "NewAddressDialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

NewAddressDialog::NewAddressDialog(Emulators::IEmulatorManager* manager, uint32_t defaultAddress, QWidget* parent)
    : QDialog(parent), m_manager(manager) {
    setWindowTitle("Create New Address / Structure");
    setModal(true);
    resize(420, 260);
    setFixedSize(420, 260);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupLayout(defaultAddress);
    updatePreview();
}

NewAddressDialog::~NewAddressDialog() = default;

void NewAddressDialog::setupLayout(uint32_t defaultAddress) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // Address Form Row
    auto* addrForm = new QFormLayout();
    m_addressEdit = new QLineEdit(this);

    QString defaultAddrStr = "";
    if (defaultAddress != 0 && m_manager) {
        defaultAddrStr = m_manager->formatDisplayAddress(defaultAddress);
    } else if (defaultAddress != 0) {
        defaultAddrStr = QString::number(defaultAddress, 16).toUpper();
    }
    m_addressEdit->setText(defaultAddrStr);
    m_addressEdit->setPlaceholderText("e.g. 00492ED0");
    addrForm->addRow("Base Address (Hex):", m_addressEdit);
    mainLayout->addLayout(addrForm);

    // Structure Type Group
    auto* typeGroup = new QGroupBox("Structure Type", this);
    auto* typeLayout = new QVBoxLayout(typeGroup);

    m_typeArrayRadio = new QRadioButton("Array Structure (Static or In-Memory Array)", typeGroup);
    m_typePointerRadio = new QRadioButton("Standard Structure / Pointer", typeGroup);
    m_typeArrayRadio->setChecked(true);

    typeLayout->addWidget(m_typeArrayRadio);
    typeLayout->addWidget(m_typePointerRadio);
    mainLayout->addWidget(typeGroup);

    // Structure Name / Description Form
    auto* descForm = new QFormLayout();
    m_descEdit = new QLineEdit(this);
    m_descEdit->setText("Array");
    m_descEdit->setPlaceholderText("e.g. Array, Enemy array");
    descForm->addRow("Description:", m_descEdit);
    mainLayout->addLayout(descForm);

    // Live Generated Header Preview
    m_previewLabel = new QLabel(this);
    m_previewLabel->setStyleSheet("font-weight: bold; color: #D2A8FF; padding: 4px; border: 1px solid #30363D; border-radius: 4px;");
    mainLayout->addWidget(m_previewLabel);

    // Bottom Action Buttons
    auto* btnLayout = new QHBoxLayout();
    m_createBtn = new QPushButton("Create", this);
    m_createBtn->setObjectName("okButton");
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setObjectName("cancelButton");

    btnLayout->addStretch();
    btnLayout->addWidget(m_createBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    // Connections
    connect(m_typeArrayRadio, &QRadioButton::toggled, this, &NewAddressDialog::onStructureTypeChanged);
    connect(m_typePointerRadio, &QRadioButton::toggled, this, &NewAddressDialog::onStructureTypeChanged);
    connect(m_descEdit, &QLineEdit::textChanged, this, [this]() { updatePreview(); });

    connect(m_createBtn, &QPushButton::clicked, this, &NewAddressDialog::onCreateClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void NewAddressDialog::onStructureTypeChanged() {
    if (m_typeArrayRadio->isChecked()) {
        if (m_descEdit->text().trimmed().isEmpty() || m_descEdit->text() == "New Structure") {
            m_descEdit->setText("Array");
        }
        m_descEdit->setPlaceholderText("e.g. Array, Enemy array");
    } else {
        if (m_descEdit->text().trimmed().isEmpty() || m_descEdit->text() == "Array") {
            m_descEdit->setText("New Structure");
        }
        m_descEdit->setPlaceholderText("e.g. PlayerObject, GameState");
    }
    updatePreview();
}

void NewAddressDialog::updatePreview() {
    QString desc = m_descEdit->text().trimmed();

    if (m_typeArrayRadio->isChecked()) {
        if (desc.isEmpty())
            desc = "Array";

        if (desc.startsWith("[")) {
            m_previewLabel->setText(QString("Generated Header: %1").arg(desc));
        } else {
            m_previewLabel->setText(QString("Generated Header: [64 bytes] %1").arg(desc));
        }
    } else {
        if (desc.isEmpty())
            desc = "New Structure";
        uint32_t addr = getAddress();
        uint32_t dwordVal = 0;
        if (m_manager && m_manager->isAttached() && addr != 0) {
            auto valOpt = m_manager->readUInt32(addr);
            if (valOpt.has_value())
                dwordVal = valOpt.value();
        }
        QString ptrType = m_manager ? m_manager->getPlatformPointerTypeDescription(dwordVal) : "[32-bit] Pointer";
        m_previewLabel->setText(QString("Generated Header: %1 to %2").arg(ptrType, desc));
    }
}

QString NewAddressDialog::sanitizeHexInput(const QString& input) const {
    QString clean = input.trimmed().toUpper();
    if (clean.startsWith("0X"))
        clean = clean.mid(2);

    QString result;
    for (QChar c : clean) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
            result.append(c);
        }
    }
    return result;
}

uint32_t NewAddressDialog::getAddress() const {
    QString clean = sanitizeHexInput(m_addressEdit->text());
    bool ok = false;
    uint32_t parsed = clean.toUInt(&ok, 16);
    return ok ? parsed : 0;
}

QString NewAddressDialog::getInitialCodeNote() const {
    QString desc = m_descEdit->text().trimmed();

    if (m_typeArrayRadio->isChecked()) {
        if (desc.isEmpty())
            desc = "Array";
        if (desc.startsWith("[")) {
            return desc;
        }
        return QString("[64 bytes] %1").arg(desc);
    } else {
        if (desc.isEmpty())
            desc = "New Structure";
        uint32_t addr = getAddress();
        uint32_t dwordVal = 0;
        if (m_manager && m_manager->isAttached() && addr != 0) {
            auto valOpt = m_manager->readUInt32(addr);
            if (valOpt.has_value())
                dwordVal = valOpt.value();
        }
        QString ptrType = m_manager ? m_manager->getPlatformPointerTypeDescription(dwordVal) : "[32-bit] Pointer";
        return QString("%1 to %2").arg(ptrType, desc);
    }
}

bool NewAddressDialog::isArrayStructure() const { return m_typeArrayRadio->isChecked(); }

void NewAddressDialog::onCreateClicked() {
    QString clean = sanitizeHexInput(m_addressEdit->text());
    if (clean.isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please enter a valid hexadecimal memory address.");
        return;
    }

    bool ok = false;
    clean.toUInt(&ok, 16);
    if (!ok) {
        QMessageBox::warning(this, "Invalid Address", "The entered memory address is not valid hexadecimal.");
        return;
    }

    accept();
}

} // namespace PointerFinder2::UI
