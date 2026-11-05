#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

namespace PointerFinder2::UI {

    // Modal dialog providing separate data type selection and variable name entry
    // when adding documented variables to a memory structure.
    class AddVariableDialog : public QDialog {
        Q_OBJECT
    public:
        explicit AddVariableDialog(const QString& defaultType = "32-bit",
                                   const QString& defaultName = "Variable",
                                   QWidget* parent = nullptr);
        ~AddVariableDialog() override = default;

        QString getDataType() const;
        QString getVariableName() const;
        QString getFullDescription() const;

    private:
        QComboBox* m_typeCombo = nullptr;
        QLineEdit* m_nameEdit = nullptr;
        QPushButton* m_okButton = nullptr;
        QPushButton* m_cancelButton = nullptr;

        void setupLayout(const QString& defaultType, const QString& defaultName);
    };

}
