#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

namespace PointerFinder2::UI {

    // Compact modal dialog for naming newly added pointers without requiring manual format prefixes.
    class AddPointerDialog : public QDialog {
        Q_OBJECT
    public:
        explicit AddPointerDialog(const QString& defaultName = "Pointer",
                                  QWidget* parent = nullptr);
        ~AddPointerDialog() override = default;

        QString getPointerName() const;
        QString getFullDescription(const QString& platformPointerType = "[32-bit] Pointer") const;

    private:
        QLineEdit* m_nameEdit = nullptr;
        QPushButton* m_okButton = nullptr;
        QPushButton* m_cancelButton = nullptr;

        void setupLayout(const QString& defaultName);
    };

}
