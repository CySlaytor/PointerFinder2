#include "AddressSearchDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    // Sets up the search popup, setting default search texts and focus.
    AddressSearchDialog::AddressSearchDialog(const QString& currentSearch, QWidget* parent)
        : QDialog(parent) {
        setWindowTitle("Find Address");
        setModal(true);
        setFixedSize(300, 120);

        setupLayout();
        m_addressEdit->setText(currentSearch);
        m_addressEdit->setFocus();
    }

    AddressSearchDialog::~AddressSearchDialog() = default;

    void AddressSearchDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        auto* label = new QLabel("Enter Base Address (Hex):", this);
        mainLayout->addWidget(label);

        m_addressEdit = new QLineEdit(this);
        mainLayout->addWidget(m_addressEdit);

        auto* buttonLayout = new QHBoxLayout();
        m_findButton = new QPushButton("Find", this);
        m_findButton->setObjectName("okButton");

        m_cancelButton = new QPushButton("Cancel", this);
        m_cancelButton->setObjectName("cancelButton");

        buttonLayout->addStretch();
        buttonLayout->addWidget(m_findButton);
        buttonLayout->addWidget(m_cancelButton);
        mainLayout->addLayout(buttonLayout);

        m_findButton->setDefault(true);
        connect(m_findButton, &QPushButton::clicked, this, &AddressSearchDialog::onFindClicked);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    // Verifies your entered text to ensure it is a valid hexadecimal number 
    // before executing the search operation.
    void AddressSearchDialog::onFindClicked() {
        QString text = m_addressEdit->text().trimmed();
        if (text.isEmpty()) {
            QMessageBox::warning(this, "Input Required", "Please enter an address to find.");
            return;
        }

        QString hexCheck = text;
        if (hexCheck.startsWith("0x", Qt::CaseInsensitive)) {
            hexCheck = hexCheck.mid(2);
        }

        QRegularExpression hexRegex("^[0-9a-fA-F]+$");
        if (!hexRegex.match(hexCheck).hasMatch()) {
            QMessageBox::critical(this, "Invalid Input", "Invalid address format. Please use hexadecimal characters (0-9, A-F).");
            return;
        }

        while (hexCheck.startsWith('0') && hexCheck.length() > 1) {
            hexCheck.remove(0, 1);
        }

        m_searchAddress = hexCheck.isEmpty() ? "0" : hexCheck;
        accept();
    }

    QString AddressSearchDialog::getSearchAddress() const {
        return m_searchAddress;
    }

}
