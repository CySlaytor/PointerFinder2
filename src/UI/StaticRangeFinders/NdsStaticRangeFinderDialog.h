#pragma once

#include "../../Emulators/IEmulatorManager.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

namespace PointerFinder2::UI::StaticRangeFinders {

    // This dialog reads Nintendo DS ROM files (.nds) to determine exactly where 
    // core game programming is loaded into the system RAM.
    class NdsStaticRangeFinderDialog : public QDialog {
        Q_OBJECT
    public:
        explicit NdsStaticRangeFinderDialog(Emulators::IEmulatorManager* manager, QWidget* parent = nullptr);
        ~NdsStaticRangeFinderDialog() override;

    private slots:
        void onBrowseClicked();
        void onAnalyzeClicked();
        void onApplyClicked();

    private:
        Emulators::IEmulatorManager* m_manager = nullptr;
        uint32_t m_foundStart = 0;
        uint32_t m_foundEnd = 0;

        QLineEdit* m_romPathEdit = nullptr;
        QPushButton* m_browseButton = nullptr;
        QPushButton* m_analyzeButton = nullptr;
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
        AnalysisResult performAnalysis(const QString& romPath);
    };

    QWidget* createNdsRangeFinder(Emulators::IEmulatorManager* manager, QWidget* parent);

}
