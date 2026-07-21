#include "EmulatorSelectionDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::Core;

    // Fills the list window with details like window titles and Process IDs (PIDs)
    // of the detected emulator processes.
    EmulatorSelectionDialog::EmulatorSelectionDialog(const std::vector<DetectedEmulatorInstance>& instances, QWidget* parent)
        : QDialog(parent), m_instances(instances) {
        setWindowTitle("Select Emulator Instance");
        setModal(true);
        resize(450, 220);

        setupLayout();

        for (const auto& inst : m_instances) {
            QString titleText = inst.windowTitle.isEmpty() ? "" : QString(" - %1").arg(inst.windowTitle);
            QString itemText = QString("%1 (PID: %2%3)").arg(inst.profile.name).arg(inst.pid).arg(titleText);
            m_listWidget->addItem(itemText);
        }

        if (m_listWidget->count() > 0) {
            m_listWidget->setCurrentRow(0);
        }
    }

    EmulatorSelectionDialog::~EmulatorSelectionDialog() = default;

    void EmulatorSelectionDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        auto* label = new QLabel("Multiple emulator instances found. Please select one to attach to:", this);
        mainLayout->addWidget(label);

        m_listWidget = new QListWidget(this);
        mainLayout->addWidget(m_listWidget);

        auto* buttonLayout = new QHBoxLayout();
        m_okButton = new QPushButton("OK", this);
        m_okButton->setObjectName("okButton");

        m_cancelButton = new QPushButton("Cancel", this);
        m_cancelButton->setObjectName("cancelButton");

        buttonLayout->addStretch();
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);
        mainLayout->addLayout(buttonLayout);

        m_okButton->setDefault(true);
        connect(m_okButton, &QPushButton::clicked, this, &EmulatorSelectionDialog::onOkClicked);
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    // Saves your selection and closes the selection popup when you click OK.
    void EmulatorSelectionDialog::onOkClicked() {
        int currentRow = m_listWidget->currentRow();
        if (currentRow >= 0 && currentRow < static_cast<int>(m_instances.size())) {
            m_selectedInstance = m_instances[currentRow];
            accept();
        }
        else {
            QMessageBox::warning(this, "Selection Required", "Please select an emulator instance.");
        }
    }

    DetectedEmulatorInstance EmulatorSelectionDialog::getSelectedInstance() const {
        return m_selectedInstance;
    }

}