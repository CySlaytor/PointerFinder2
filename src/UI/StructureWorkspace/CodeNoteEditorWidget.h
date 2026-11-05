#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QWidget>

namespace PointerFinder2::UI {

    // Encapsulates the Code Note Hierarchy Fixer panel (Panel 3)
    // with its editor, style selector, indentation toggle, and clipboard actions.
    class CodeNoteEditorWidget : public QWidget {
        Q_OBJECT
    public:
        explicit CodeNoteEditorWidget(QWidget* parent = nullptr);
        ~CodeNoteEditorWidget() override;

        QString toPlainText() const;
        void setPlainText(const QString& text);
        void clear();

        QString getStyle() const;
        void setStyle(const QString& style);

        bool isIndentDescriptions() const;
        void setIndentDescriptions(bool indent);

        int getCursorPosition() const;
        void setCursorPosition(int pos);

        void updateThemeIcons();

    signals:
        void fixHierarchyClicked();
        void hierarchyStyleChanged();
        void statusMessageRequested(const QString& message);

    private slots:
        void onCopyCodeNoteClicked();

    private:
        QPlainTextEdit* m_codeNoteTextEdit = nullptr;
        QComboBox* m_styleCombo = nullptr;
        QCheckBox* m_indentDescCheck = nullptr;
        QPushButton* m_fixHierarchyBtn = nullptr;
        QPushButton* m_copyCodeNoteBtn = nullptr;

        void setupLayout();
        QIcon getThemeIcon(const QString& resourcePath) const;
    };

}
