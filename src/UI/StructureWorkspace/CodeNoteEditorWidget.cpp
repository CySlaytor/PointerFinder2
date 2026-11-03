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
        fixerBottomRow->addWidget(new QLabel("Style:", this));

        m_styleCombo = new QComboBox(this);
        m_styleCombo->addItems({
            "Plusses (+)",
            "Dots (.)",
            "Unicode Tree (├─)"
        });
        m_styleCombo->setFixedHeight(26);
        fixerBottomRow->addWidget(m_styleCombo);

        m_indentDescCheck = new QCheckBox("Indent Descriptions", this);
        m_indentDescCheck->setChecked(true);
        fixerBottomRow->addWidget(m_indentDescCheck);

        m_fixHierarchyBtn = new QPushButton("Fix Hierarchy", this);
        m_fixHierarchyBtn->setIcon(getThemeIcon(":/icons/hierarchy_fixer.svg"));
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

        connect(m_styleCombo, &QComboBox::currentTextChanged, this, &CodeNoteEditorWidget::hierarchyStyleChanged);
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

    QString CodeNoteEditorWidget::getStyle() const {
        return m_styleCombo ? m_styleCombo->currentText() : "Plusses (+)";
    }

    void CodeNoteEditorWidget::setStyle(const QString& style) {
        if (m_styleCombo) {
            m_styleCombo->blockSignals(true);
            m_styleCombo->setCurrentText(style);
            m_styleCombo->blockSignals(false);
        }
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

        QTimer::singleShot(1500, this, [this, origText]() {
            m_copyCodeNoteBtn->setText(origText);
            m_copyCodeNoteBtn->setEnabled(true);
        });
    }

}
