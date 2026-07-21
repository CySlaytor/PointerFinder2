#pragma once

#include <memory>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <vector>

namespace PointerFinder2::UI {

    // This dialog reformats messy, copy-pasted developer notes into organized, 
    // correctly aligned tree diagrams using plusses, dots, or box characters.
    class CodeNoteHierarchyFixerDialog : public QDialog {
        Q_OBJECT
    public:
        explicit CodeNoteHierarchyFixerDialog(QWidget* parent = nullptr);
        ~CodeNoteHierarchyFixerDialog() override;

    protected:
        void changeEvent(QEvent* event) override;

    private slots:
        void onFixClicked();
        void onCopyClicked();

    private:
        struct CodeNoteNode {
            QString content;
            std::vector<std::shared_ptr<CodeNoteNode>> children;
            int originalIndex = 0;
        };

        QPlainTextEdit* m_inputEdit = nullptr;
        QPlainTextEdit* m_outputEdit = nullptr;
        QComboBox* m_styleCombo = nullptr;
        QCheckBox* m_indentDescCheck = nullptr;
        QPushButton* m_fixButton = nullptr;
        QPushButton* m_copyButton = nullptr;
        QPushButton* m_closeButton = nullptr;

        void setupLayout();
        void saveGeometrySettings();
        void loadGeometrySettings();

        QChar detectIndentStyle(const QStringList& lines);
        std::vector<std::shared_ptr<CodeNoteNode>> parseLinesToTree(const QStringList& lines);
        std::vector<std::shared_ptr<CodeNoteNode>> mergeTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes);
        std::vector<std::shared_ptr<CodeNoteNode>> sortNodes(std::vector<std::shared_ptr<CodeNoteNode>>& nodes);
        QString rebuildTextFromTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes,
            int indentLevel,
            const QString& style,
            bool indentDescriptions,
            std::vector<bool> ancestorIsLast = {});
    };

}
