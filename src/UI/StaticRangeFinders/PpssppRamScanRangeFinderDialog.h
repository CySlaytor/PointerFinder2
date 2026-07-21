#pragma once

#include "../../Emulators/IEmulatorManager.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>

namespace PointerFinder2::UI::StaticRangeFinders {

    // This dialog scans active PSP memory to locate your game's ID marker,
    // which indicates where critical game data is loaded.
    class PpssppRamScanRangeFinderDialog : public QDialog {
        Q_OBJECT
    public:
        explicit PpssppRamScanRangeFinderDialog(Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        ~PpssppRamScanRangeFinderDialog() override;

    private slots:
        void onFindClicked();
        void onApplyClicked();

    private:
        Emulators::IEmulatorManager* m_manager = nullptr;
        uint32_t m_foundStart = 0;
        uint32_t m_foundEnd = 0;

        QLineEdit* m_gameIdEdit = nullptr;
        QPushButton* m_findButton = nullptr;
        QProgressBar* m_progressBar = nullptr;
        QTextEdit* m_logEdit = nullptr;
        QLabel* m_resultLabel = nullptr;

        QPushButton* m_applyButton = nullptr;
        QPushButton* m_closeButton = nullptr;

        void setupLayout();
        void log(const QString& message, const QColor& color);

        struct AnalysisResult {
            bool success = false;
            uint32_t approxStart = 0;
            uint32_t approxEnd = 0;
        };
        AnalysisResult performAnalysis(const QString& gameId);
        std::vector<int> findAllOccurrences(const std::vector<uint8_t>& buffer, const std::vector<uint8_t>& pattern);
    };

    QWidget* createPpssppRangeFinder(Emulators::IEmulatorManager* manager, QWidget* parent);

}
