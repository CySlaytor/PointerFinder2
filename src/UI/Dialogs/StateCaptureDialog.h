#pragma once

#include "Core/Scanning/MultiScanState.h"
#include "Emulators/IEmulatorManager.h"
#include "Models/AppSettings.h"
#include "Models/ScanParameters.h"
#include "UI/Widgets/SpinBoxes/CustomSpinBoxes.h"

#include <QCheckBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

namespace PointerFinder2::UI {

// This configuration menu sets up snapshot capture slots, search ranges,
// maximum search depth levels, and optimization parameters for the scanner.
class StateCaptureDialog : public QDialog {
    Q_OBJECT
  public:
    explicit StateCaptureDialog(Emulators::IEmulatorManager* manager, const DataModels::AppSettings& settings,
                                Core::MultiScanState* multiScanState, QWidget* parent = nullptr);
    ~StateCaptureDialog() override;

    DataModels::ScanParameters getScanParameters() const;
    void updateSettings(DataModels::AppSettings& settings);

  private slots:
    void onCellWidgetClicked(int row, int col);
    void onClearAllClicked();
    void onResetRangeClicked();
    void onHelpClicked();
    void onScanClicked();
    void onUseLastOffsetHintChanged();
    void onFieldsSanitizationFinished();
    void onTableItemChanged(QTableWidgetItem* item);

  private:
    Emulators::IEmulatorManager* m_manager = nullptr;
    Core::MultiScanState* m_multiScanState = nullptr;
    DataModels::AppSettings m_defaultSettings;

    QTableWidget* m_tableWidget = nullptr;
    Widgets::HexSpinBox* m_maxOffsetSpin = nullptr;
    QCheckBox* m_useLastOffsetHintCheck = nullptr;
    Widgets::HexSpinBox* m_lastOffsetHintSpin = nullptr;
    Widgets::CustomSpinBox* m_maxLevelSpin = nullptr;
    Widgets::ThousandsSpinBox* m_maxCandidatesSpin = nullptr;
    Widgets::CustomSpinBox* m_candidatesPerLevelSpin = nullptr;

    QCheckBox* m_detectArraysCheck = nullptr;
    Widgets::HexSpinBox* m_arrayRangeSpin = nullptr;

    QGroupBox* m_rangeGroup = nullptr;
    QLineEdit* m_staticStartEdit = nullptr;
    QLineEdit* m_staticEndEdit = nullptr;
    QPushButton* m_resetRangeButton = nullptr;

    QGroupBox* m_parametersGroup = nullptr;
    QGroupBox* m_optionsGroup = nullptr;
    QCheckBox* m_stopOnFirstCheck = nullptr;
    QCheckBox* m_findAllLevelsCheck = nullptr;
    QCheckBox* m_fastScanModeCheck = nullptr;
    QCheckBox* m_printPartialPathsCheck = nullptr;
    QCheckBox* m_dynamicStaticDetectionCheck = nullptr;

    QPushButton* m_scanButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_clearAllButton = nullptr;
    QPushButton* m_helpButton = nullptr;

    void setupLayout();
    void updateScanButtonState();
    QString sanitizeHexInput(const QString& input);
    QString getPlatformString() const;
};

} // namespace PointerFinder2::UI
