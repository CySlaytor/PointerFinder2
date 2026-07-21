#include "CodeNoteHierarchyFixerDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

    CodeNoteHierarchyFixerDialog::CodeNoteHierarchyFixerDialog(QWidget* parent)
        : QDialog(parent) {
        setWindowTitle("Code Note Hierarchy Fixer");
        resize(800, 500);
        setMinimumSize(600, 400);

        setupLayout();
        loadGeometrySettings();
    }

    CodeNoteHierarchyFixerDialog::~CodeNoteHierarchyFixerDialog() {
        saveGeometrySettings();
    }

    void CodeNoteHierarchyFixerDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);

        auto* splitter = new QSplitter(Qt::Horizontal, this);
        m_inputEdit = new QPlainTextEdit(this);
        m_inputEdit->setPlaceholderText("Paste your raw code note here...");

        m_outputEdit = new QPlainTextEdit(this);
        m_outputEdit->setReadOnly(true);
        m_outputEdit->setPlaceholderText("Fixed output will appear here...");

        QFont monoFont("Consolas", 9);
        monoFont.setStyleHint(QFont::Monospace);
        m_inputEdit->setFont(monoFont);
        m_outputEdit->setFont(monoFont);

        splitter->addWidget(m_inputEdit);
        splitter->addWidget(m_outputEdit);
        mainLayout->addWidget(splitter, 1);

        auto* controlsLayout = new QHBoxLayout();
        m_indentDescCheck = new QCheckBox("Indent Descriptions", this);
        m_indentDescCheck->setChecked(true);
        controlsLayout->addWidget(m_indentDescCheck);

        m_styleCombo = new QComboBox(this);
        m_styleCombo->addItems({
            "Unicode Tree (├─)",
            "Auto Detect (. or +)",
            "Dots (.)",
            "Plusses (+)"
            });
        controlsLayout->addWidget(m_styleCombo);

        m_fixButton = new QPushButton("Fix Hierarchy >>", this);
        QFont boldFont = m_fixButton->font();
        boldFont.setBold(true);
        m_fixButton->setFont(boldFont);
        controlsLayout->addWidget(m_fixButton);

        controlsLayout->addStretch();

        m_copyButton = new QPushButton("Copy Output", this);
        m_closeButton = new QPushButton("Close", this);
        m_closeButton->setObjectName("closeButton");

        controlsLayout->addWidget(m_copyButton);
        controlsLayout->addWidget(m_closeButton);

        mainLayout->addLayout(controlsLayout);

        connect(m_fixButton, &QPushButton::clicked, this, &CodeNoteHierarchyFixerDialog::onFixClicked);
        connect(m_copyButton, &QPushButton::clicked, this, &CodeNoteHierarchyFixerDialog::onCopyClicked);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    // Takes your text input, groups the indentation steps, joins duplicates,
    // and prints out a neatly organized outline tree.
    void CodeNoteHierarchyFixerDialog::onFixClicked() {
        QString inputText = m_inputEdit->toPlainText().trimmed();
        if (inputText.isEmpty()) {
            m_outputEdit->clear();
            return;
        }

        QStringList lines = inputText.split(QRegularExpression("[\r\n]+"));
        QString selectedStyle = m_styleCombo->currentText();
        QString style = selectedStyle;

        if (selectedStyle.startsWith("Auto Detect")) {
            QChar detected = detectIndentStyle(lines);
            style = (detected == '+') ? "Plusses (+)" : "Dots (.)";
        }

        auto parsedTree = parseLinesToTree(lines);
        auto mergedTree = mergeTree(parsedTree);
        QString finalResult = rebuildTextFromTree(mergedTree, 0, style, m_indentDescCheck->isChecked());

        m_outputEdit->setPlainText(finalResult.trimmed());
    }

    // Copies your cleaned, newly aligned outline to the system's clipboard.
    void CodeNoteHierarchyFixerDialog::onCopyClicked() {
        QString text = m_outputEdit->toPlainText();
        if (text.isEmpty()) return;

        QApplication::clipboard()->setText(text);

        QString originalText = m_copyButton->text();
        m_copyButton->setText("✓ Copied!");
        m_copyButton->setEnabled(false);

        QTimer::singleShot(1500, this, [this, originalText]() {
            m_copyButton->setText(originalText);
            m_copyButton->setEnabled(true);
            });
    }

    QChar CodeNoteHierarchyFixerDialog::detectIndentStyle(const QStringList& lines) {
        for (const auto& line : lines) {
            if (line.trimmed().startsWith("++")) {
                return '+';
            }
        }
        return '.';
    }

    // Parses a list of lines representing dynamic pointer paths into an organized, 
    // structured N-ary tree, grouping comments and properties.
    std::vector<std::shared_ptr<CodeNoteHierarchyFixerDialog::CodeNoteNode>>
        CodeNoteHierarchyFixerDialog::parseLinesToTree(const QStringList& lines) {
        auto rootNodes = std::make_shared<std::vector<std::shared_ptr<CodeNoteNode>>>();

        struct StackFrame {
            int indent;
            std::vector<std::shared_ptr<CodeNoteNode>>* parentList;
        };

        std::vector<StackFrame> stack;
        stack.push_back({ -1, rootNodes.get() });
        std::shared_ptr<CodeNoteNode> lastNode = nullptr;
        int lineIndex = 0;

        for (const auto& line : lines) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.isEmpty() || trimmedLine.startsWith("---")) {
                lineIndex++;
                continue;
            }

            int indent = 0;
            QString content;
            QString untrimmedLine = line;

            int classicPrefixLength = 0;
            for (int i = 0; i < untrimmedLine.length(); ++i) {
                QChar c = untrimmedLine[i];
                if (c == '.' || c == '+') {
                    classicPrefixLength++;
                }
                else {
                    break;
                }
            }

            if (classicPrefixLength > 0) {
                if (untrimmedLine[classicPrefixLength - 1] == '+' &&
                    untrimmedLine.length() > classicPrefixLength &&
                    untrimmedLine[classicPrefixLength] != ' ') {
                    indent = classicPrefixLength - 1;
                    content = untrimmedLine.mid(indent).trimmed();
                }
                else {
                    indent = classicPrefixLength;
                    content = untrimmedLine.mid(indent).trimmed();
                }
            }
            else {
                int unicodePrefixLength = 0;
                bool hasBoxChar = false;
                for (int i = 0; i < untrimmedLine.length(); ++i) {
                    QChar c = untrimmedLine[i];
                    if (c == L'│' || c == ' ' || c == L'├' || c == L'└' || c == L'┬' || c == L'─') {
                        unicodePrefixLength++;
                        if (c != ' ') hasBoxChar = true;
                    }
                    else {
                        break;
                    }
                }

                if (unicodePrefixLength > 0 && hasBoxChar) {
                    QChar lastChar = untrimmedLine[unicodePrefixLength - 1];
                    if (lastChar == L'┬' || lastChar == L'─') {
                        indent = unicodePrefixLength - 1;
                    }
                    else {
                        indent = 0;
                    }
                    content = untrimmedLine.mid(unicodePrefixLength).trimmed();
                }
                else {
                    indent = 0;
                    content = untrimmedLine.trimmed();
                }
            }

            if (indent < 0) indent = 0;

            bool isAttachedNote = (indent == 0) && (lastNode != nullptr) &&
                !(content.startsWith("+0x", Qt::CaseInsensitive) ||
                    content.startsWith("-0x", Qt::CaseInsensitive) ||
                    content.startsWith("["));

            if (isAttachedNote) {
                auto noteNode = std::make_shared<CodeNoteNode>();
                noteNode->content = content;
                noteNode->originalIndex = lineIndex;
                lastNode->children.push_back(noteNode);
            }
            else {
                while (stack.back().indent >= indent) {
                    stack.pop_back();
                }

                auto node = std::make_shared<CodeNoteNode>();
                node->content = content;
                node->originalIndex = lineIndex;

                stack.back().parentList->push_back(node);
                stack.push_back({ indent, &(node->children) });
                lastNode = node;
            }
            lineIndex++;
        }
        return *rootNodes;
    }

    // Resolves and merges identical offset branches in the tree to consolidate 
    // redundant paths into single descriptive nodes.
    std::vector<std::shared_ptr<CodeNoteHierarchyFixerDialog::CodeNoteNode>>
        CodeNoteHierarchyFixerDialog::mergeTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
        std::vector<std::shared_ptr<CodeNoteNode>> finalNodes;
        std::map<QString, std::vector<std::shared_ptr<CodeNoteNode>>> mergeableGroups;
        std::vector<std::shared_ptr<CodeNoteNode>> nonMergeableNodes;

        QRegularExpression offsetRegex("^([\\+\\-]0x[0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);

        for (const auto& node : nodes) {
            auto match = offsetRegex.match(node->content);
            if (match.hasMatch()) {
                QString rawKey = match.captured(1);
                QString sign = rawKey.left(1);
                bool ok;
                qlonglong offsetVal = rawKey.mid(3).toLongLong(&ok, 16);
                QString key = QString("%10x%2").arg(sign).arg(QString::number(offsetVal, 16));

                mergeableGroups[key].push_back(node);
            }
            else {
                nonMergeableNodes.push_back(node);
            }
        }

        QRegularExpression contentRegex("^([\\+\\-]0x)([0-9a-fA-F]+)(.*)", QRegularExpression::CaseInsensitiveOption);

        for (const auto& [key, group] : mergeableGroups) {
            auto representative = group.front();
            for (const auto& n : group) {
                if (n->originalIndex < representative->originalIndex) {
                    representative = n;
                }
            }

            QString bestContent = representative->content;
            for (const auto& n : group) {
                if (n->content.length() > bestContent.length()) {
                    bestContent = n->content;
                }
            }

            auto contentMatch = contentRegex.match(bestContent);
            if (contentMatch.hasMatch()) {
                QString signAndPrefix = contentMatch.captured(1);
                qlonglong offsetVal = contentMatch.captured(2).toLongLong(nullptr, 16);
                QString restOfLine = contentMatch.captured(3);
                bestContent = QString("%1%2%3")
                    .arg(signAndPrefix.toLower())
                    .arg(QString::number(offsetVal, 16))
                    .arg(restOfLine);
            }

            auto newNode = std::make_shared<CodeNoteNode>();
            newNode->content = bestContent;
            newNode->originalIndex = representative->originalIndex;

            std::vector<std::shared_ptr<CodeNoteNode>> allChildren;
            for (const auto& n : group) {
                allChildren.insert(allChildren.end(), n->children.begin(), n->children.end());
            }

            if (!allChildren.empty()) {
                newNode->children = mergeTree(allChildren);
            }
            finalNodes.push_back(newNode);
        }

        for (auto node : nonMergeableNodes) {
            if (!node->children.empty()) {
                node->children = mergeTree(node->children);
            }
            finalNodes.push_back(node);
        }

        return sortNodes(finalNodes);
    }

    std::vector<std::shared_ptr<CodeNoteHierarchyFixerDialog::CodeNoteNode>>
        CodeNoteHierarchyFixerDialog::sortNodes(std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
        QRegularExpression offsetRegex("^[\\+\\-]0x([0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);

        std::sort(nodes.begin(), nodes.end(), [&](const std::shared_ptr<CodeNoteNode>& a, const std::shared_ptr<CodeNoteNode>& b) {
            bool aIsOffset = offsetRegex.match(a->content).hasMatch();
            bool bIsOffset = offsetRegex.match(b->content).hasMatch();

            if (aIsOffset != bIsOffset) {
                return !aIsOffset;
            }

            if (aIsOffset) {
                qlonglong aVal = offsetRegex.match(a->content).captured(1).toLongLong(nullptr, 16);
                qlonglong bVal = offsetRegex.match(b->content).captured(1).toLongLong(nullptr, 16);
                return aVal < bVal;
            }

            return a->originalIndex < b->originalIndex;
            });

        return nodes;
    }

    QString CodeNoteHierarchyFixerDialog::rebuildTextFromTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes,
        int indentLevel,
        const QString& style,
        bool indentDescriptions,
        std::vector<bool> ancestorIsLast) {
        QString sb = "";

        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            bool isLastChild = (i == nodes.size() - 1);

            bool hasPointerChildren = false;
            for (const auto& c : node->children) {
                if (c->content.startsWith("+0x", Qt::CaseInsensitive) || c->content.startsWith("-0x", Qt::CaseInsensitive)) {
                    hasPointerChildren = true;
                    break;
                }
            }

            if (indentLevel == 0 && !sb.isEmpty()) {
                sb.append("\n");
            }

            bool isPointerOffset = node->content.startsWith("+0x", Qt::CaseInsensitive) || node->content.startsWith("-0x", Qt::CaseInsensitive);
            bool isAttachedNote = !isPointerOffset;

            QString prefix = "";

            if (style.startsWith("Unicode Tree")) {
                if (isAttachedNote) {
                    if (indentDescriptions && indentLevel > 0) {
                        for (int a = 0; a < indentLevel - 1; ++a) {
                            prefix.append(ancestorIsLast[a] ? " " : "│");
                        }
                        prefix.append(" ");
                    }
                }
                else {
                    if (indentLevel > 0) {
                        for (int a = 0; a < indentLevel - 1; ++a) {
                            prefix.append(ancestorIsLast[a] ? " " : "│");
                        }
                        prefix.append(isLastChild ? "└" : "├");
                        prefix.append(hasPointerChildren ? "┬" : "─");
                    }
                }
            }
            else {
                QChar indentChar = style.startsWith("Plusses") ? '+' : '.';

                if (isAttachedNote) {
                    if (indentDescriptions) {
                        prefix = indentLevel > 0 ? QString(indentLevel, '.') : "";
                    }
                }
                else {
                    prefix = indentLevel > 0 ? QString(indentLevel, indentChar) : "";
                }
            }

            sb.append(QString("%1%2\n").arg(prefix).arg(node->content));

            if (!node->children.empty()) {
                std::vector<bool> nextAncestors = ancestorIsLast;
                if (indentLevel > 0) {
                    nextAncestors.push_back(isLastChild);
                }
                sb.append(rebuildTextFromTree(node->children, indentLevel + 1, style, indentDescriptions, nextAncestors));
            }
        }
        return sb;
    }

    void CodeNoteHierarchyFixerDialog::changeEvent(QEvent* event) {
        QDialog::changeEvent(event);
    }

    void CodeNoteHierarchyFixerDialog::loadGeometrySettings() {
        QSettings settings;
        settings.beginGroup("CodeNoteHierarchyFixer");
        if (settings.contains("geometry")) {
            restoreGeometry(settings.value("geometry").toByteArray());
        }
        settings.endGroup();
    }

    void CodeNoteHierarchyFixerDialog::saveGeometrySettings() {
        QSettings settings;
        settings.beginGroup("CodeNoteHierarchyFixer");
        settings.setValue("geometry", saveGeometry());
        settings.endGroup();
    }

}
