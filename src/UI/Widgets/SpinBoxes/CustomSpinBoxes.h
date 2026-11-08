#pragma once

#include <QGuiApplication>
#include <QRegularExpression>
#include <QSpinBox>

namespace PointerFinder2::UI::Widgets {

class CustomSpinBox : public QSpinBox {
    Q_OBJECT
  public:
    explicit CustomSpinBox(QWidget* parent = nullptr, int largeStep = 5);

    void setLargeStep(int step) { m_largeStep = step; }

  protected:
    void stepBy(int steps) override;

  private:
    int m_largeStep;
};

class HexSpinBox : public QSpinBox {
    Q_OBJECT
  public:
    explicit HexSpinBox(QWidget* parent = nullptr, int largeStep = 0x1000);

    void setLargeStep(int step) { m_largeStep = step; }

  protected:
    void stepBy(int steps) override;
    QString textFromValue(int value) const override;
    int valueFromText(const QString& text) const override;
    QValidator::State validate(QString& input, int&) const override;

  private:
    int m_largeStep;
};

class ThousandsSpinBox : public CustomSpinBox {
    Q_OBJECT
  public:
    explicit ThousandsSpinBox(QWidget* parent = nullptr, int largeStep = 1000000);

  protected:
    QString textFromValue(int value) const override;
    int valueFromText(const QString& text) const override;
};

} // namespace PointerFinder2::UI::Widgets
