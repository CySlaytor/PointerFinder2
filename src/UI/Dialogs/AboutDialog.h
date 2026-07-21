#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace PointerFinder2::UI {

    // This popup shows general information about the program, its version number, 
    // and lists the developers who contributed to the project.
    class AboutDialog : public QDialog {
        Q_OBJECT
    public:
        explicit AboutDialog(QWidget* parent = nullptr);
        ~AboutDialog() override;

    private:
        QLabel* m_iconLabel = nullptr;
        QLabel* m_titleLabel = nullptr;
        QLabel* m_descLabel = nullptr;
        QLabel* m_creditsLabel = nullptr;
        QPushButton* m_closeButton = nullptr;

        void setupLayout();
    };

}
