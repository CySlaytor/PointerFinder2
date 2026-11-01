#include "CodeNoteHierarchyEngine.h"

#include <algorithm>
#include <map>
#include <QRegularExpression>

namespace PointerFinder2::Core {

    std::vector<std::shared_ptr<CodeNoteNode>>
    CodeNoteHierarchyEngine::parseLinesToTree(const QStringList& lines, QString& outHeaderComment) {
        auto rootNodes = std::make_shared<std::vector<std::shared_ptr<CodeNoteNode>>>();

        struct StackFrame {
            int indent;
            std::vector<std::shared_ptr<CodeNoteNode>>* parentList;
        };

        std::vector<StackFrame> stack;
        stack.push_back({ -1, rootNodes.get() });
        std::shared_ptr<CodeNoteNode> lastNode = nullptr;
        int lineIndex = 0;
        bool hasEncounteredOffset = false;

        // Support standard offset jumps as well as array element offsets with pipe symbol
        QRegularExpression offsetSearch("(?:\\|)?[\\+\\-]0x[0-9a-fA-F]+", QRegularExpression::CaseInsensitiveOption);

        for (const auto& line : lines) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.isEmpty()) {
                if (!hasEncounteredOffset) {
                    if (!outHeaderComment.isEmpty()) outHeaderComment.append("\n");
                }
                lineIndex++;
                continue;
            }

            int indent = 0;
            QString content;
            QString untrimmedLine = line;

            auto offsetMatch = offsetSearch.match(untrimmedLine);
            bool isOffsetLine = offsetMatch.hasMatch();

            if (isOffsetLine) {
                hasEncounteredOffset = true;
                int offsetPos = offsetMatch.capturedStart();
                QString prefix = untrimmedLine.left(offsetPos);

                bool hasBoxChar = false;
                for (QChar c : prefix) {
                    if (c == L'│' || c == ' ' || c == L'├' || c == L'└' || c == L'┬' || c == L'─') {
                        hasBoxChar = true;
                        break;
                    }
                }

                if (hasBoxChar) {
                    indent = (prefix.length() > 0) ? (prefix.length() - 1) : 0;
                }
                else {
                    indent = prefix.length();
                }

                content = untrimmedLine.mid(offsetPos).trimmed();
            }
            else {
                indent = 0;
                content = trimmedLine;
            }

            if (indent < 0) indent = 0;

            if (!hasEncounteredOffset && !isOffsetLine) {
                if (!outHeaderComment.isEmpty()) outHeaderComment.append("\n");
                outHeaderComment.append(trimmedLine);
                lineIndex++;
                continue;
            }

            bool isAttachedNote = !isOffsetLine && (lastNode != nullptr);

            if (isAttachedNote) {
                // Strip all leading prefix dots, pipes, or spaces for clean data representation
                QString cleanNote = trimmedLine;
                int prefixChars = 0;
                while (prefixChars < cleanNote.length() && (cleanNote[prefixChars] == '.' || cleanNote[prefixChars] == ' ' || cleanNote[prefixChars] == L'│')) {
                    prefixChars++;
                }
                cleanNote = cleanNote.mid(prefixChars).trimmed();

                if (!cleanNote.isEmpty()) {
                    auto noteNode = std::make_shared<CodeNoteNode>();
                    noteNode->content = cleanNote;
                    noteNode->originalIndex = lineIndex;
                    lastNode->children.push_back(noteNode);
                }
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

    std::vector<std::shared_ptr<CodeNoteNode>>
    CodeNoteHierarchyEngine::mergeTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
        std::vector<std::shared_ptr<CodeNoteNode>> finalNodes;
        std::map<QString, std::vector<std::shared_ptr<CodeNoteNode>>> mergeableGroups;
        std::vector<std::shared_ptr<CodeNoteNode>> nonMergeableNodes;

        QRegularExpression offsetRegex("^(\\|?[\\+\\-]0x[0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);

        for (const auto& node : nodes) {
            auto match = offsetRegex.match(node->content);
            if (match.hasMatch()) {
                QString rawKey = match.captured(1);
                QString pipePrefix = rawKey.startsWith('|') ? "|" : "";
                QString offsetPart = rawKey.startsWith('|') ? rawKey.mid(1) : rawKey;
                QString sign = offsetPart.left(1);
                bool ok;
                qlonglong offsetVal = offsetPart.mid(3).toLongLong(&ok, 16);
                QString key = QString("%1%20x%3").arg(pipePrefix).arg(sign).arg(QString("%1").arg(offsetVal, 2, 16, QChar('0')).toLower());

                mergeableGroups[key].push_back(node);
            }
            else {
                nonMergeableNodes.push_back(node);
            }
        }

        QRegularExpression contentRegex("^(\\|?[\\+\\-]0x)([0-9a-fA-F]+)(.*)", QRegularExpression::CaseInsensitiveOption);

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
                    .arg(QString("%1").arg(offsetVal, 2, 16, QChar('0')).toLower())
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

    std::vector<std::shared_ptr<CodeNoteNode>>
    CodeNoteHierarchyEngine::sortNodes(std::vector<std::shared_ptr<CodeNoteNode>>& nodes) {
        QRegularExpression offsetRegex("^\\|?[\\+\\-]0x([0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);

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

    QString CodeNoteHierarchyEngine::rebuildTextFromTree(
        const std::vector<std::shared_ptr<CodeNoteNode>>& nodes,
        int indentLevel,
        const QString& style,
        bool indentDescriptions,
        const QString& headerComment,
        std::vector<bool> ancestorIsLast
    ) {
        QString sb = "";

        if (indentLevel == 0 && !headerComment.trimmed().isEmpty()) {
            sb.append(headerComment.trimmed() + "\n\n");
        }

        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            bool isLastChild = (i == nodes.size() - 1);

            bool isPointerOffset = node->content.startsWith("+0x", Qt::CaseInsensitive) || 
                                   node->content.startsWith("-0x", Qt::CaseInsensitive) ||
                                   node->content.startsWith("|+0x", Qt::CaseInsensitive) ||
                                   node->content.startsWith("|-0x", Qt::CaseInsensitive);
            bool isAttachedNote = !isPointerOffset;

            bool hasPointerChildren = false;
            for (const auto& child : node->children) {
                if (child->content.startsWith("+0x", Qt::CaseInsensitive) || 
                    child->content.startsWith("-0x", Qt::CaseInsensitive) ||
                    child->content.startsWith("|+0x", Qt::CaseInsensitive) ||
                    child->content.startsWith("|-0x", Qt::CaseInsensitive)) {
                    hasPointerChildren = true;
                    break;
                }
            }

            QString prefix = "";

            if (style.startsWith("Unicode Tree")) {
                if (isAttachedNote) {
                    if (indentDescriptions && indentLevel > 1) {
                        for (int a = 0; a < indentLevel - 2; ++a) {
                            prefix.append(ancestorIsLast[a] ? " " : "│");
                        }
                        prefix.append(ancestorIsLast[indentLevel - 2] ? "  " : "│ ");
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
                sb.append(rebuildTextFromTree(node->children, indentLevel + 1, style, indentDescriptions, "", nextAncestors));
            }
        }
        return sb;
    }

}
