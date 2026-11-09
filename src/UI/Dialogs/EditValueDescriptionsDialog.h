#pragma once

#include <QDialog>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStringList>

#include <cstdint>
#include <functional>

namespace PointerFinder2::UI {

// Clean multi-line dialog for adding and editing value descriptions
// and enumerations attached to a structure variable.
class EditValueDescriptionsDialog : public QDialog {
    Q_OBJECT
  public:
    explicit EditValueDescriptionsDialog(const QString& variableName,
                                         const QStringList& initialDescriptions,
                                         QWidget* parent = nullptr);

    // Overload for backward-compatibility with existing call sites
    EditValueDescriptionsDialog(const QString& variableName, const QString& /*sizeTag*/,
                                const QStringList& initialDescriptions, uint32_t /*currentRamValue*/ = 0,
                                std::function<uint32_t()> /*liveRamReader*/ = nullptr,
                                QWidget* parent = nullptr)
        : EditValueDescriptionsDialog(variableName, initialDescriptions, parent) {}

    ~EditValueDescriptionsDialog() override = default;

    QStringList getDescriptions() const;

  private slots:
    void onSaveClicked();

  private:
    QString m_variableName;
    QPlainTextEdit* m_descriptionsTextEdit = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    void setupLayout(const QStringList& initialDescriptions);
};

} // namespace PointerFinder2::UI
