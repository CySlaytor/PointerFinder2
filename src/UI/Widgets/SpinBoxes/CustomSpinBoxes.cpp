#include "CustomSpinBoxes.h"

namespace PointerFinder2::UI::Widgets {

    // CustomSpinBox Implementation
    CustomSpinBox::CustomSpinBox(QWidget* parent, int largeStep)
        : QSpinBox(parent), m_largeStep(largeStep) {
    }

    void CustomSpinBox::stepBy(int steps) {
        if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
            int newValue = value() + (m_largeStep * steps);
            setValue(qBound(minimum(), newValue, maximum()));
        }
        else {
            QSpinBox::stepBy(steps);
        }
    }

    // HexSpinBox Implementation
    HexSpinBox::HexSpinBox(QWidget* parent, int largeStep)
        : QSpinBox(parent), m_largeStep(largeStep) {
        setRange(0, 0x7FFFFFFF);
    }

    void HexSpinBox::stepBy(int steps) {
        if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
            int newValue = value() + (m_largeStep * steps);
            setValue(qBound(minimum(), newValue, maximum()));
        }
        else {
            QSpinBox::stepBy(steps);
        }
    }

    QString HexSpinBox::textFromValue(int value) const {
        return QString::number(value, 16).toUpper();
    }

    int HexSpinBox::valueFromText(const QString& text) const {
        QString cleanText = text.trimmed();
        if (cleanText.startsWith("0x", Qt::CaseInsensitive)) {
            cleanText = cleanText.mid(2);
        }
        bool ok;
        int val = cleanText.toInt(&ok, 16);
        if (ok) return val;
        return QSpinBox::valueFromText(text);
    }

    QValidator::State HexSpinBox::validate(QString& input, int&) const {
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

    // ThousandsSpinBox Implementation
    ThousandsSpinBox::ThousandsSpinBox(QWidget* parent, int largeStep)
        : CustomSpinBox(parent, largeStep) {
    }

    QString ThousandsSpinBox::textFromValue(int value) const {
        return locale().toString(value);
    }

    int ThousandsSpinBox::valueFromText(const QString& text) const {
        bool ok;
        int val = locale().toInt(text, &ok);
        if (ok) return val;
        return CustomSpinBox::valueFromText(text);
    }

}
