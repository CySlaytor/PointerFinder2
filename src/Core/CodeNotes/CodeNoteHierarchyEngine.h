#pragma once

#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

namespace PointerFinder2::Core {

struct CodeNoteNode {
    QString content;
    std::vector<std::shared_ptr<CodeNoteNode>> children;
    int originalIndex = 0;
};

// Dedicated engine that parses, merges, sorts, and reformats multiline
// hierarchical code notes independently of UI presentation controls.
class CodeNoteHierarchyEngine {
  public:
    static std::vector<std::shared_ptr<CodeNoteNode>> parseLinesToTree(const QStringList& lines, QString& outHeaderComment);

    static std::vector<std::shared_ptr<CodeNoteNode>> mergeTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes);

    static std::vector<std::shared_ptr<CodeNoteNode>> sortNodes(std::vector<std::shared_ptr<CodeNoteNode>>& nodes);

    static QString rebuildTextFromTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes, int indentLevel, bool useUnicodeTree,
                                       bool indentDescriptions, const QString& headerComment = "", std::vector<bool> ancestorIsLast = {},
                                       int parentNestingLevel = 0, const QString& delimiterSuffix = "", bool autoPopulateTemplate = false,
                                       const QString& pointerTypeTemplate = "", const QString& indentPrefix = "");

    static QString rebuildTextFromTree(const std::vector<std::shared_ptr<CodeNoteNode>>& nodes, int indentLevel, const QString& style,
                                       bool indentDescriptions, const QString& headerComment = "", std::vector<bool> ancestorIsLast = {},
                                       int parentNestingLevel = 0, const QString& delimiterSuffix = "", bool autoPopulateTemplate = false,
                                       const QString& pointerTypeTemplate = "", const QString& indentPrefix = "");
};

} // namespace PointerFinder2::Core
