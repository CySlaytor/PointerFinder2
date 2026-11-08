#pragma once

#include "../../Emulators/IEmulatorManager.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include <cstdint>

namespace PointerFinder2::UI {

// Dialog allowing users to manually create first-class address entries
// for either standard pointer structures or static root array structures.
class NewAddressDialog : public QDialog {
    Q_OBJECT
  public:
    explicit NewAddressDialog(Emulators::IEmulatorManager* manager, uint32_t defaultAddress = 0, QWidget* parent = nullptr);
    ~NewAddressDialog() override;

    uint32_t getAddress() const;
    QString getInitialCodeNote() const;
    bool isArrayStructure() const;

  private slots:
    void onStructureTypeChanged();
    void onCreateClicked();

  private:
    Emulators::IEmulatorManager* m_manager = nullptr;

    QLineEdit* m_addressEdit = nullptr;
    QRadioButton* m_typeArrayRadio = nullptr;
    QRadioButton* m_typePointerRadio = nullptr;
    QLineEdit* m_descEdit = nullptr;

    QLabel* m_previewLabel = nullptr;
    QPushButton* m_createBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    void setupLayout(uint32_t defaultAddress);
    void updatePreview();
    QString sanitizeHexInput(const QString& input) const;
};

} // namespace PointerFinder2::UI
