#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace PointerFinder2::UI::Views {

    // Encapsulates the bottom scanning and progress interface:
    // progress bar, scan stopwatch, progress count, stop button,
    // and scan activation triggers.
    class ScanControlPanel : public QWidget {
        Q_OBJECT
    public:
        explicit ScanControlPanel(QWidget* parent = nullptr);
        ~ScanControlPanel() override = default;

        void setScanningActive(bool scanningActive);
        void setAttached(bool attached, bool hasResults);

        void setProgress(int currentValue, int maxValue, const QString& progressText);
        void setIndeterminate(bool indeterminate, const QString& labelText = QString());
        void resetProgress();
        void setTimeText(const QString& timeText);

    signals:
        void stateBasedScanClicked();
        void filterClicked();
        void stopScanClicked();

    private:
        QProgressBar* m_progressBar = nullptr;
        QPushButton* m_stopScanButton = nullptr;
        QLabel* m_progressLabel = nullptr;
        QLabel* m_timeLabel = nullptr;

        QWidget* m_scanButtonsWidget = nullptr;
        QPushButton* m_stateScanButton = nullptr;
        QPushButton* m_filterButton = nullptr;

        QPropertyAnimation* m_progressAnimation = nullptr;

        void setupLayout();
        void animateProgressBar(int targetValue);
    };

}
