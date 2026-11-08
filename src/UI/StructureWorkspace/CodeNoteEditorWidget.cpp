#include "CodeNoteEditorWidget.h"

#include "Models/GlobalSettings.h"
#include "UI/Common/ThemeManager.h"

#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    using namespace PointerFinder2::DataModels;

    CodeNoteEditorWidget::CodeNoteEditorWidget(QWidget* parent)
        : QWidget(parent) {
        setupLayout();
    }

    CodeNoteEditorWidget::~CodeNoteEditorWidget() = default;

    QIcon CodeNoteEditorWidget::getThemeIcon(const QString& resourcePath) const {
        return ThemeManager::getIcon(resourcePath, GlobalSettings::activeTheme);
    }

    void CodeNoteEditorWidget::updateThemeIcons() {
        m_fixHierarchyBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
        m_copyCodeNoteBtn->setIcon(getThemeIcon(":/icons/copy.svg"));
    }

    void CodeNoteEditorWidget::setupLayout() {
        // --- Panel 3: Code Note Hierarchy Fixer ---
        auto* rightLayout = new QVBoxLayout(this);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(6);

        m_codeNoteTextEdit = new QPlainTextEdit(this);
        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        m_codeNoteTextEdit->setFont(monoFont);
        m_codeNoteTextEdit->setPlaceholderText("Code Note displays here. You can paste raw pointer chains or edit lines directly, then click 'Fix Hierarchy' to merge...");
        rightLayout->addWidget(m_codeNoteTextEdit, 1);

        // Hierarchy Controls Bar (positioned at bottom)
        auto* fixerBottomRow = new QHBoxLayout();

        m_unicodeTreeCheck = new QCheckBox("Unicode Tree", this);
        m_unicodeTreeCheck->setToolTip("Experimental feature: Use Unicode box-drawing characters (├─, └─, │) for tree hierarchy indentation instead of the configured prefix character.");
        m_unicodeTreeCheck->setFixedHeight(26);
        fixerBottomRow->addWidget(m_unicodeTreeCheck);

        m_indentDescCheck = new QCheckBox("Indent Descriptions", this);
        m_indentDescCheck->setToolTip("Indent attached value descriptions and enumerations (using dots) to match the parent pointer's level.");
        m_indentDescCheck->setChecked(true);
        m_indentDescCheck->setFixedHeight(26);
        fixerBottomRow->addWidget(m_indentDescCheck);

        m_fixHierarchyBtn = new QPushButton("Fix Hierarchy", this);
        m_fixHierarchyBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
        m_fixHierarchyBtn->setToolTip("Merge duplicate offsets, standardize delimiters, format pointer templates, and rebuild hierarchy.");
        m_fixHierarchyBtn->setFixedHeight(26);
        m_fixHierarchyBtn->setAutoDefault(false);
        m_fixHierarchyBtn->setDefault(false);
        fixerBottomRow->addWidget(m_fixHierarchyBtn);

        fixerBottomRow->addStretch();

        m_copyCodeNoteBtn = new QPushButton("Copy", this);
        m_copyCodeNoteBtn->setIcon(getThemeIcon(":/icons/copy.svg"));
        m_copyCodeNoteBtn->setFixedHeight(26);
        m_copyCodeNoteBtn->setAutoDefault(false);
        m_copyCodeNoteBtn->setDefault(false);
        fixerBottomRow->addWidget(m_copyCodeNoteBtn);

        rightLayout->addLayout(fixerBottomRow);

        connect(m_unicodeTreeCheck, &QCheckBox::checkStateChanged, this, &CodeNoteEditorWidget::hierarchyStyleChanged);
        connect(m_indentDescCheck, &QCheckBox::checkStateChanged, this, &CodeNoteEditorWidget::hierarchyStyleChanged);
        connect(m_fixHierarchyBtn, &QPushButton::clicked, this, &CodeNoteEditorWidget::fixHierarchyClicked);
        connect(m_copyCodeNoteBtn, &QPushButton::clicked, this, &CodeNoteEditorWidget::onCopyCodeNoteClicked);
    }

    QString CodeNoteEditorWidget::toPlainText() const {
        return m_codeNoteTextEdit ? m_codeNoteTextEdit->toPlainText() : QString();
    }

    void CodeNoteEditorWidget::setPlainText(const QString& text) {
        if (m_codeNoteTextEdit) {
            m_codeNoteTextEdit->setPlainText(text);
        }
    }

    void CodeNoteEditorWidget::clear() {
        if (m_codeNoteTextEdit) {
            m_codeNoteTextEdit->clear();
        }
    }

    bool CodeNoteEditorWidget::isUnicodeTree() const {
        return m_unicodeTreeCheck ? m_unicodeTreeCheck->isChecked() : false;
    }

    void CodeNoteEditorWidget::setUnicodeTree(bool enabled) {
        if (m_unicodeTreeCheck) {
            m_unicodeTreeCheck->blockSignals(true);
            m_unicodeTreeCheck->setChecked(enabled);
            m_unicodeTreeCheck->blockSignals(false);
        }
    }

    QString CodeNoteEditorWidget::getStyle() const {
        return isUnicodeTree() ? "Unicode Tree (├─)" : "Plusses (+)";
    }

    void CodeNoteEditorWidget::setStyle(const QString& style) {
        setUnicodeTree(style.startsWith("Unicode Tree", Qt::CaseInsensitive));
    }

    bool CodeNoteEditorWidget::isIndentDescriptions() const {
        return m_indentDescCheck ? m_indentDescCheck->isChecked() : true;
    }

    void CodeNoteEditorWidget::setIndentDescriptions(bool indent) {
        if (m_indentDescCheck) {
            m_indentDescCheck->blockSignals(true);
            m_indentDescCheck->setChecked(indent);
            m_indentDescCheck->blockSignals(false);
        }
    }

    int CodeNoteEditorWidget::getCursorPosition() const {
        return m_codeNoteTextEdit ? m_codeNoteTextEdit->textCursor().position() : 0;
    }

    void CodeNoteEditorWidget::setCursorPosition(int pos) {
        if (m_codeNoteTextEdit) {
            QTextCursor cur = m_codeNoteTextEdit->textCursor();
            cur.setPosition(std::min(pos, static_cast<int>(m_codeNoteTextEdit->toPlainText().length())));
            m_codeNoteTextEdit->setTextCursor(cur);
        }
    }

    void CodeNoteEditorWidget::onCopyCodeNoteClicked() {
        QString text = toPlainText();
        if (text.isEmpty()) return;

        QApplication::clipboard()->setText(text);

        QString origText = m_copyCodeNoteBtn->text();
        m_copyCodeNoteBtn->setText("✓ Copied!");
        m_copyCodeNoteBtn->setEnabled(false);

        emit statusMessageRequested("Copied code note text to clipboard.");

        QTimer::singleShot(1500, this, [this, origText]() {
            m_copyCodeNoteBtn->setText(origText);
            m_copyCodeNoteBtn->setEnabled(true);
        });
    }

}
