#include "PointerPath.h"

#include "../Emulators/IEmulatorManager.h"

#include <QStringList>

namespace PointerFinder2::DataModels {

    QString PointerPath::getOffsetsString() const {
        QStringList parts;
        for (int32_t offset : offsets) {
            if (offset < 0) {
                parts << QString("-%1").arg(std::abs(offset), 0, 16).toUpper();
            }
            else {
                parts << QString("+%1").arg(offset, 0, 16).toUpper();
            }
        }
        return parts.join(", ");
    }

    QString PointerPath::toRetroAchievementsString(Emulators::IEmulatorManager* manager) const {
        if (!manager) return "";

        QString sb;
        QString sizePrefix = "X";
        QString pointerMask = "";

        QString raPrefix = manager->getRetroAchievementsPrefix();
        QString emuName = manager->getEmulatorName();

        // Map system formats and masking constants.
        if (raPrefix == "W" || raPrefix == "D" || raPrefix == "O") {
            sizePrefix = "W";
        }
        else if (raPrefix == "G") {
            sizePrefix = "G";
            pointerMask = "&536870911"; // 0x1FFFFFFF mask
        }
        else if (emuName.contains("PPSSPP", Qt::CaseInsensitive)) {
            sizePrefix = "X";
            pointerMask = "&33554431";  // 0x01FFFFFF mask
        }

        // Format base address (indirect reference).
        QString normalizedBase = manager->formatDisplayAddress(baseAddress);

        bool isGba = (raPrefix == "O" || emuName.contains("GBA", Qt::CaseInsensitive));
        QString baseModifier = "";
        std::vector<QString> offsetModifiers;

        if (isGba && manager->isAttached()) {
            // Unnormalize the base address to ensure we always read from the native layout address
            uint32_t nativeBase = manager->unnormalizeAddress(manager->formatDisplayAddress(baseAddress));
            auto currentPtrOpt = manager->readUInt32(nativeBase);
            if (currentPtrOpt.has_value()) {
                uint32_t currentPtr = currentPtrOpt.value();
                if ((currentPtr >> 24) == 0x02) {
                    baseModifier = "+32768";
                }

                for (size_t i = 0; i < offsets.size() - 1; ++i) {
                    uint32_t nextAddress = currentPtr + static_cast<uint32_t>(offsets[i]);
                    auto nextPtrOpt = manager->readUInt32(nextAddress);
                    if (nextPtrOpt.has_value()) {
                        currentPtr = nextPtrOpt.value();
                        if ((currentPtr >> 24) == 0x02) {
                            offsetModifiers.push_back("+32768");
                        }
                        else {
                            offsetModifiers.push_back("");
                        }
                    }
                    else {
                        offsetModifiers.push_back("");
                    }
                }
            }
        }

        while (offsetModifiers.size() < (offsets.size() > 1 ? offsets.size() - 1 : 0)) {
            offsetModifiers.push_back("");
        }

        sb.append(QString("I:0x%1%2%3%4")
            .arg(sizePrefix)
            .arg(normalizedBase.rightJustified(8, '0').toLower())
            .arg(pointerMask)
            .arg(baseModifier));

        // Append intermediate offsets.
        if (offsets.size() > 1) {
            for (size_t i = 0; i < offsets.size() - 1; ++i) {
                int32_t offset = offsets[i];
                uint32_t rawOffset = static_cast<uint32_t>(offset);
                QString modifier = offsetModifiers[i];
                sb.append(QString("_I:0x%1%2%3%4")
                    .arg(sizePrefix)
                    .arg(QString("%1").arg(rawOffset, 8, 16, QChar('0')).toLower())
                    .arg(pointerMask)
                    .arg(modifier));
            }
        }

        // Append the final direct memory access offset.
        if (!offsets.empty()) {
            int32_t lastOffset = offsets.back();
            uint32_t rawOffset = static_cast<uint32_t>(lastOffset);
            sb.append(QString("_0x%1%2")
                .arg(sizePrefix)
                .arg(QString("%1").arg(rawOffset, 8, 16, QChar('0')).toLower()));
        }

        sb.append("=1");
        return sb;
    }

    bool PointerPath::operator==(const PointerPath& other) const {
        return baseAddress == other.baseAddress && offsets == other.offsets;
    }

}