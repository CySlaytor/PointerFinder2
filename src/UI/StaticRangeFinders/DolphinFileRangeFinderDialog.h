#pragma once

#include "../../Emulators/IEmulatorManager.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QTextEdit>

namespace PointerFinder2::UI::StaticRangeFinders {

    // This dialog reads a GameCube/Wii game file (main.dol) to locate 
    // the safest memory addresses to scan when searching for pointers.
    class DolphinFileRangeFinderDialog : public QDialog {
        Q_OBJECT
    public:
        explicit DolphinFileRangeFinderDialog(Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        ~DolphinFileRangeFinderDialog() override;

    private slots:
        void onBrowseClicked();
        void onAnalyzeClicked();
        void onApplyClicked();
        void onSliderMoved(int value);

    private:
        Emulators::IEmulatorManager* m_manager = nullptr;
        uint32_t m_foundStart = 0;
        uint32_t m_foundEnd = 0;

        QLineEdit* m_filePathEdit = nullptr;
        QPushButton* m_browseButton = nullptr;
        QPushButton* m_analyzeButton = nullptr;
        QSlider* m_borderSlider = nullptr;
        QLabel* m_borderLabel = nullptr;
        QTextEdit* m_logEdit = nullptr;
        QLabel* m_resultLabel = nullptr;

        QPushButton* m_applyButton = nullptr;
        QPushButton* m_closeButton = nullptr;

        void setupLayout();
        void log(const QString& message, const QColor& color);

        struct AnalysisResult {
            bool success = false;
            uint32_t start = 0;
            uint32_t end = 0;
        };
        AnalysisResult performAnalysis(const QString& filePath, uint32_t borderSize);
    };

    QWidget* createDolphinRangeFinder(Emulators::IEmulatorManager* manager, QWidget* parent);

}
