#include "ScanControlPanel.h"

#include <QEasingCurve>

namespace PointerFinder2::UI::Views {

    ScanControlPanel::ScanControlPanel(QWidget* parent)
        : QWidget(parent) {
        setupLayout();
    }

    void ScanControlPanel::setupLayout() {
        auto* bottomLayout = new QVBoxLayout(this);
        bottomLayout->setContentsMargins(5, 5, 5, 5);

        auto* progressRow = new QHBoxLayout();
        m_timeLabel = new QLabel("Time: 00:00.0", this);
        m_progressBar = new QProgressBar(this);
        m_progressBar->setValue(0);
        m_progressLabel = new QLabel("0 / 0", this);
        m_stopScanButton = new QPushButton("Stop", this);

        progressRow->addWidget(m_timeLabel);
        progressRow->addWidget(m_progressBar, 1);
        progressRow->addWidget(m_progressLabel);
        progressRow->addWidget(m_stopScanButton);
        bottomLayout->addLayout(progressRow);

        m_scanButtonsWidget = new QWidget(this);
        auto* scanButtonsLayout = new QHBoxLayout(m_scanButtonsWidget);
        scanButtonsLayout->setContentsMargins(0, 0, 0, 0);

        m_stateScanButton = new QPushButton("State-Based Scan", m_scanButtonsWidget);
        m_filterButton = new QPushButton("Filter Dynamic Paths", m_scanButtonsWidget);
        m_stateScanButton->setFixedHeight(35);
        m_filterButton->setFixedHeight(35);

        scanButtonsLayout->addWidget(m_stateScanButton);
        scanButtonsLayout->addWidget(m_filterButton);
        bottomLayout->addWidget(m_scanButtonsWidget);

        connect(m_stateScanButton, &QPushButton::clicked, this, &ScanControlPanel::stateBasedScanClicked);
        connect(m_filterButton, &QPushButton::clicked, this, &ScanControlPanel::filterClicked);
        connect(m_stopScanButton, &QPushButton::clicked, this, &ScanControlPanel::stopScanClicked);
    }

    void ScanControlPanel::setScanningActive(bool scanningActive) {
        m_scanButtonsWidget->setVisible(!scanningActive);
        m_progressBar->setVisible(scanningActive);
        m_stopScanButton->setVisible(scanningActive);
        m_progressLabel->setVisible(scanningActive);
        m_timeLabel->setVisible(scanningActive);
    }

    void ScanControlPanel::setAttached(bool attached, bool hasResults) {
        m_stateScanButton->setEnabled(attached);
        m_filterButton->setEnabled(attached && hasResults);
    }

    void ScanControlPanel::resetProgress() {
        if (m_progressAnimation) {
            m_progressAnimation->stop();
        }
        m_progressBar->setMaximum(100);
        m_progressBar->setValue(0);
        m_progressLabel->setText("0 / 0");
    }

    void ScanControlPanel::setTimeText(const QString& timeText) {
        m_timeLabel->setText(timeText);
    }

    void ScanControlPanel::setProgress(int currentValue, int maxValue, const QString& progressText) {
        m_progressBar->setMaximum(maxValue);
        animateProgressBar(currentValue);
        m_progressLabel->setText(progressText);
    }

    void ScanControlPanel::setIndeterminate(bool indeterminate, const QString& labelText) {
        if (m_progressAnimation) {
            m_progressAnimation->stop();
        }
        m_progressBar->setMaximum(indeterminate ? 0 : 100);
        m_progressBar->setValue(0);
        if (!labelText.isEmpty()) {
            m_progressLabel->setText(labelText);
        }
    }

    void ScanControlPanel::animateProgressBar(int targetValue) {
        if (!m_progressAnimation) {
            m_progressAnimation = new QPropertyAnimation(m_progressBar, "value", this);
        }
        m_progressAnimation->stop();
        m_progressAnimation->setDuration(120);
        m_progressAnimation->setStartValue(m_progressBar->value());
        m_progressAnimation->setEndValue(targetValue);
        m_progressAnimation->setEasingCurve(QEasingCurve::OutQuad);
        m_progressAnimation->start();
    }

}
