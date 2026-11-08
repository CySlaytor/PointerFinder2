#pragma once

#include "Emulators/IEmulatorManager.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

#include <cstdint>
#include <vector>

namespace PointerFinder2::UI {

struct GeneratedStrideNote {
    uint32_t address = 0;
    QString title;
    QString noteText;
};

// Dialog providing an Excel-style sequential note cloner for array structures
// and repeating memory tables (e.g. Level 1..N stats, Player 1..N structs).
class CloneStrideDialog : public QDialog {
    Q_OBJECT
  public:
    explicit CloneStrideDialog(Emulators::IEmulatorManager* manager, uint32_t currentAddress, const QString& currentNoteText,
                               QWidget* parent = nullptr);
    ~CloneStrideDialog() override;

    std::vector<GeneratedStrideNote> getGeneratedNotes() const;

  private slots:
    void onParametersChanged();
    void onPasteNamesFromClipboard();
    void onTableItemChanged(QTableWidgetItem* item);
    void onGenerateClicked();

  private:
    Emulators::IEmulatorManager* m_manager = nullptr;
    uint32_t m_currentAddress = 0;
    QString m_originalNoteText;
    QString m_originalBody;
    QString m_detectedFirstName;
    bool m_isUpdatingTable = false;

    QLineEdit* m_baseAddressEdit = nullptr;
    QSpinBox* m_strideSpin = nullptr;
    QLabel* m_strideHexLabel = nullptr;
    QSpinBox* m_countSpin = nullptr;
    QLineEdit* m_templateEdit = nullptr;
    QPushButton* m_pasteNamesBtn = nullptr;

    QTableWidget* m_previewTable = nullptr;
    QPushButton* m_generateBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    std::vector<GeneratedStrideNote> m_generatedNotes;

    void setupLayout();
    void parseInitialNote(uint32_t& outStride, QString& outTemplate, QString& outFirstName);
    void rebuildTableRows();
    QString formatTitle(int index, uint32_t offsetBytes, const QString& customName) const;
    uint32_t getBaseAddress() const;
};

} // namespace PointerFinder2::UI
