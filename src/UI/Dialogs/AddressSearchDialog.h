#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

namespace PointerFinder2::UI {

    // This small popup lets users search for a specific hexadecimal base address 
    // to quickly jump to it within the main list of results.
    class AddressSearchDialog : public QDialog {
        Q_OBJECT
    public:
        explicit AddressSearchDialog(const QString& currentSearch, QWidget* parent = nullptr);
        ~AddressSearchDialog() override;

        QString getSearchAddress() const;

    private slots:
        void onFindClicked();

    private:
        QLineEdit* m_addressEdit = nullptr;
        QPushButton* m_findButton = nullptr;
        QPushButton* m_cancelButton = nullptr;
        QString m_searchAddress = "";

        void setupLayout();
    };

}
