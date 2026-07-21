#pragma once

#include "../../Core/MultiScanState.h"
#include "../../Emulators/IEmulatorManager.h"
#include "../../Models/AppSettings.h"       
#include "../../Models/ScanParameters.h"     

#include <QCheckBox>
#include <QDialog>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTableWidget>

namespace PointerFinder2::UI {

    class CustomSpinBox : public QSpinBox {
        Q_OBJECT
    public:
        explicit CustomSpinBox(QWidget* parent = nullptr, int largeStep = 5)
            : QSpinBox(parent), m_largeStep(largeStep) {
        }

        void setLargeStep(int step) { m_largeStep = step; }

    protected:
        void stepBy(int steps) override {
            if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
                int newValue = value() + (m_largeStep * steps);
                setValue(qBound(minimum(), newValue, maximum()));
            }
            else {
                QSpinBox::stepBy(steps);
            }
        }
    private:
        int m_largeStep;
    };

    class HexSpinBox : public QSpinBox {
        Q_OBJECT
    public:
        explicit HexSpinBox(QWidget* parent = nullptr, int largeStep = 0x1000)
            : QSpinBox(parent), m_largeStep(largeStep) {
            setRange(0, 0x7FFFFFFF);
        }

        void setLargeStep(int step) { m_largeStep = step; }

    protected:
        void stepBy(int steps) override {
            if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
                int newValue = value() + (m_largeStep * steps);
                setValue(qBound(minimum(), newValue, maximum()));
            }
            else {
                QSpinBox::stepBy(steps);
            }
        }

        QString textFromValue(int value) const override {
            return QString::number(value, 16).toUpper();
        }

        int valueFromText(const QString& text) const override {
            QString cleanText = text.trimmed();
            if (cleanText.startsWith("0x", Qt::CaseInsensitive)) {
                cleanText = cleanText.mid(2);
            }
            bool ok;
            int val = cleanText.toInt(&ok, 16);
            if (ok) return val;
            return QSpinBox::valueFromText(text);
        }

        QValidator::State validate(QString& input, int&) const override {
            QString cleanText = input.trimmed();
            if (cleanText.startsWith("0x", Qt::CaseInsensitive)) {
                cleanText = cleanText.mid(2);
            }
            if (cleanText.isEmpty()) {
                return QValidator::Intermediate;
            }
            QRegularExpression hexRegex("^[0-9a-fA-F]*$");
            if (hexRegex.match(cleanText).hasMatch()) {
                return QValidator::Acceptable;
            }
            return QValidator::Invalid;
        }

    private:
        int m_largeStep;
    };

    class ThousandsSpinBox : public CustomSpinBox {
        Q_OBJECT
    public:
        explicit ThousandsSpinBox(QWidget* parent = nullptr, int largeStep = 1000000)
            : CustomSpinBox(parent, largeStep) {
        }
    protected:
        QString textFromValue(int value) const override {
            return locale().toString(value);
        }
        int valueFromText(const QString& text) const override {
            bool ok;
            int val = locale().toInt(text, &ok);
            if (ok) return val;
            return CustomSpinBox::valueFromText(text);
        }
    };

    // This configuration menu sets up snapshot capture slots, search ranges, 
    // maximum search depth levels, and optimization parameters for the scanner.
    class StateCaptureDialog : public QDialog {
        Q_OBJECT
    public:
        explicit StateCaptureDialog(Emulators::IEmulatorManager* manager,
            const DataModels::AppSettings& settings,
            Core::MultiScanState* multiScanState,
            QWidget* parent = nullptr);
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
        HexSpinBox* m_maxOffsetSpin = nullptr;
        QCheckBox* m_useLastOffsetHintCheck = nullptr;
        HexSpinBox* m_lastOffsetHintSpin = nullptr;
        CustomSpinBox* m_maxLevelSpin = nullptr;
        ThousandsSpinBox* m_maxCandidatesSpin = nullptr;
        CustomSpinBox* m_candidatesPerLevelSpin = nullptr;

        QCheckBox* m_detectArraysCheck = nullptr;
        HexSpinBox* m_arrayRangeSpin = nullptr;

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

}
