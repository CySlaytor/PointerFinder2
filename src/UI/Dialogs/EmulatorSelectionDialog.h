#pragma once

#include "../../Core/ProcessScanner.h"

#include <QDialog>
#include <QListWidget>
#include <QPushButton>

namespace PointerFinder2::UI {

    // This dialog displays a choice window when multiple running instances 
    // of an emulator are detected, letting you choose which one to link into.
    class EmulatorSelectionDialog : public QDialog {
        Q_OBJECT
    public:
        explicit EmulatorSelectionDialog(const std::vector<Core::DetectedEmulatorInstance>& instances, QWidget* parent = nullptr);
        ~EmulatorSelectionDialog() override;

        Core::DetectedEmulatorInstance getSelectedInstance() const;

    private slots:
        void onOkClicked();

    private:
        QListWidget* m_listWidget = nullptr;
        QPushButton* m_okButton = nullptr;
        QPushButton* m_cancelButton = nullptr;

        std::vector<Core::DetectedEmulatorInstance> m_instances;
        Core::DetectedEmulatorInstance m_selectedInstance;

        void setupLayout();
    };

}