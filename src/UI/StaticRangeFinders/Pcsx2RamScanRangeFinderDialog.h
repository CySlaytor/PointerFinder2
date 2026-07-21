#pragma once

#include "../../Emulators/IEmulatorManager.h"

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>

namespace PointerFinder2::UI::StaticRangeFinders {

    // This dialog scans active PS2 emulator RAM to locate memory card markers,
    // helping you find where static game data ends.
    class Pcsx2RamScanRangeFinderDialog : public QDialog {
        Q_OBJECT
    public:
        explicit Pcsx2RamScanRangeFinderDialog(Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        ~Pcsx2RamScanRangeFinderDialog() override;

    private slots:
        void onFindClicked();
        void onApplyClicked();

    private:
        Emulators::IEmulatorManager* m_manager = nullptr;
        uint32_t m_foundStart = 0;
        uint32_t m_foundEnd = 0;

        QPushButton* m_findButton = nullptr;
        QProgressBar* m_progressBar = nullptr;
        QTextEdit* m_logEdit = nullptr;
        QLabel* m_absoluteLabel = nullptr;
        QLabel* m_approxLabel = nullptr;

        QPushButton* m_applyButton = nullptr;
        QPushButton* m_closeButton = nullptr;

        void setupLayout();
        void log(const QString& message, const QColor& color);

        struct AnalysisResult {
            bool success = false;
            uint32_t absoluteEnd = 0;
            uint32_t approxStart = 0;
            uint32_t approxEnd = 0;
        };
        AnalysisResult performAnalysis();
        std::vector<int> findAllOccurrences(const std::vector<uint8_t>& buffer, const std::vector<std::vector<uint8_t>>& patterns);
    };

    QWidget* createPcsx2RangeFinder(Emulators::IEmulatorManager* manager, QWidget* parent);

}
