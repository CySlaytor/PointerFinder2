#include "CodeNoteHelper.h"

#include <QMap>
#include <QRegularExpression>

namespace PointerFinder2::Core {

    using namespace PointerFinder2::DataModels;

    static const QMap<QChar, QString> SizeMap = {
        {' ', "16-bit"}, {'H', "8-bit"}, {'X', "32-bit"}, {'W', "24-bit"},
        {'M', "Bit0"}, {'N', "Bit1"}, {'O', "Bit2"}, {'P', "Bit3"},
        {'Q', "Bit4"}, {'R', "Bit5"}, {'S', "Bit6"}, {'T', "Bit7"},
        {'L', "Lower4"}, {'U', "Upper4"}, {'K', "BitCount"},
        {'G', "32-bit BE"}, {'I', "16-bit BE"}, {'J', "24-bit BE"}
    };

    static QMap<QString, QChar> getReverseSizeMap() {
        QMap<QString, QChar> rev;
        for (auto it = SizeMap.begin(); it != SizeMap.end(); ++it) {
            rev[it.value()] = it.key();
        }
        return rev;
    }

    std::pair<std::vector<int32_t>, QString> CodeNoteHelper::parseTrigger(const QString& trigger) {
        std::vector<int32_t> offsets;
        QString finalSize = "";

        if (trigger.trimmed().isEmpty()) return { offsets, finalSize };

        QStringList parts = trigger.split('_');
        QRegularExpression pointerRegex("^I:0x[A-Z]?([0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression finalOffsetRegex("(?:[A-Z]:)?[dpb~]?0x([A-Z])([0-9a-fA-F]+)", QRegularExpression::CaseInsensitiveOption);

        for (const QString& part : parts) {
            auto pMatch = pointerRegex.match(part);
            if (pMatch.hasMatch()) {
                if (!offsets.empty() || part != parts.first()) {
                    bool ok;
                    offsets.push_back(static_cast<int32_t>(pMatch.captured(1).toLong(&ok, 16)));
                }
                continue;
            }

            auto fMatch = finalOffsetRegex.match(part);
            if (fMatch.hasMatch()) {
                bool ok;
                offsets.push_back(static_cast<int32_t>(fMatch.captured(2).toLong(&ok, 16)));
                QChar sizeChar = fMatch.captured(1).at(0).toUpper();
                finalSize = SizeMap.value(sizeChar, "Unknown Size");
                break;
            }
        }
        return { offsets, finalSize };
    }

    QString CodeNoteHelper::generateTriggerFromCodeNote(const QString& codeNote, const QString& baseAddress, const QString& pointerPrefix, bool useMask) {
        if (codeNote.trimmed().isEmpty() || baseAddress.trimmed().isEmpty()) {
            return "Code note and base address cannot be empty.";
        }

        try {
            std::vector<int32_t> offsets;
            QString finalMemorySize = "8-bit";

            QStringList lines = codeNote.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
            QRegularExpression offsetRegex("(?:\\+|-)?0x([0-9a-fA-F]+)");

            for (int i = 0; i < lines.size(); ++i) {
                auto match = offsetRegex.match(lines[i]);
                if (!match.hasMatch()) continue;

                bool ok;
                offsets.push_back(static_cast<int32_t>(match.captured(1).toLong(&ok, 16)));

                if (i == lines.size() - 1) {
                    QRegularExpression sizeRegex("\\[(.*?)\\]");
                    auto sMatch = sizeRegex.match(lines[i]);
                    if (sMatch.hasMatch()) {
                        finalMemorySize = sMatch.captured(1);
                    }
                }
            }

            if (offsets.empty()) {
                return "Could not parse any offsets from the code note.";
            }

            QString sb;
            QString maskString = useMask ? "&536870911" : "";
            sb.append(QString("I:0x%1%2%3").arg(pointerPrefix).arg(baseAddress.trimmed().remove("0x"), 8, '0').arg(maskString));

            static const auto reverseSizeMap = getReverseSizeMap();
            QChar sizeChar = reverseSizeMap.value(finalMemorySize, 'H');

            for (size_t i = 0; i < offsets.size() - 1; ++i) {
                sb.append(QString("_I:0x%1%2%3").arg(pointerPrefix).arg(QString::number(offsets[i], 16)).arg(maskString));
            }

            sb.append(QString("_0x%1%2").arg(sizeChar).arg(QString::number(offsets.back(), 16)));
            sb.append(">=1");

            return sb;
        }
        catch (...) {
            return "Error generating trigger.";
        }
    }

    QString CodeNoteHelper::buildCodeNote(const std::vector<int32_t>& offsets, const CodeNoteSettings& settings, const QString& finalMemorySize, const QString& finalDescription) {
        if (offsets.empty()) return "";

        QStringList lines;
        for (size_t i = 0; i < offsets.size(); ++i) {
            int32_t offset = offsets[i];
            QString prefix = (i == 0) ? "" : QString(i, settings.Prefix.isEmpty() ? '.' : settings.Prefix.at(0));
            QString sign = (offset < 0) ? "-" : "+";
            QString offsetHex = QString::number(std::abs(offset), 16).toUpper();

            // Separate formatting arguments into four parameters. This prevents the Qt parser 
            // from misinterpreting a trailing "0x" and dynamic numerical placeholder "%2" 
            // as the unified token "%20".
            lines << QString("%1%2%3%4").arg(prefix).arg(sign).arg("0x").arg(offsetHex);
        }

        int maxLength = 0;
        if (settings.AlignSuffixes) {
            for (const auto& l : lines) {
                maxLength = std::max(maxLength, static_cast<int>(l.length()));
            }
        }

        QStringList finalLines;
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i];
            QString currentSuffix = "";
            if (!settings.SuffixOnLastLineOnly || (i == lines.size() - 1)) {
                currentSuffix = settings.Suffix;
            }

            QString padding = settings.AlignSuffixes ? QString(maxLength - line.length(), ' ') : "";
            finalLines << QString("%1%2%3").arg(line).arg(padding).arg(currentSuffix);
        }

        if (!finalLines.empty() && (!finalMemorySize.isEmpty() || !finalDescription.isEmpty())) {
            int lastIndex = finalLines.size() - 1;
            QString lastLine = finalLines[lastIndex];

            if (!finalMemorySize.isEmpty() && finalMemorySize != "N/A") {
                lastLine.append(QString("[%1] ").arg(finalMemorySize));
            }
            if (!finalDescription.isEmpty()) {
                lastLine.append(finalDescription);
            }
            finalLines[lastIndex] = lastLine.trimmed();
        }

        return finalLines.join("\n");
    }

}
