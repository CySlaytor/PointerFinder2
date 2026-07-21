#include "PointerResultsModel.h"

#include "../../Core/ResultsManager.h"
#include "../../Emulators/IEmulatorManager.h"
#include "../../Models/PointerPath.h"

#include <algorithm>
#include <QColor>
#include <QSize>

namespace PointerFinder2::UI::Controls {

    using namespace PointerFinder2::DataModels;

    PointerResultsModel::PointerResultsModel(Core::ResultsManager* resultsManager, QObject* parent)
        : QAbstractTableModel(parent), m_resultsManager(resultsManager) {
        if (m_resultsManager) {
            connect(m_resultsManager, &Core::ResultsManager::resultsChanged, this, &PointerResultsModel::refreshModel);
        }
    }

    PointerResultsModel::~PointerResultsModel() = default;

    void PointerResultsModel::setEmulatorManager(Emulators::IEmulatorManager* emuManager) {
        m_emuManager = emuManager;
        refreshModel(true);
    }

    void PointerResultsModel::updateMaxOffsetsCount() {
        if (!m_resultsManager) {
            m_maxOffsetsCount = 0;
            return;
        }

        const auto& results = m_resultsManager->getCurrentResults();
        size_t maxCount = 0;
        for (const auto& path : results) {
            maxCount = std::max(maxCount, path.offsets.size());
        }
        m_maxOffsetsCount = static_cast<int>(maxCount);
    }

    int PointerResultsModel::rowCount(const QModelIndex& parent) const {
        if (parent.isValid() || !m_resultsManager) return 0;
        return m_resultsManager->getResultsCount();
    }

    int PointerResultsModel::columnCount(const QModelIndex& parent) const {
        if (parent.isValid()) return 0;
        return 2 + m_maxOffsetsCount;
    }

    // This function tells the program exactly what text, color, or alignment
    // to display inside each individual cell of the main search results table.
    QVariant PointerResultsModel::data(const QModelIndex& index, int role) const {
        if (role != Qt::DisplayRole && role != Qt::ForegroundRole && role != Qt::TextAlignmentRole) {
            return QVariant();
        }

        if (!index.isValid() || !m_resultsManager) return QVariant();

        int row = index.row();
        int col = index.column();

        const auto& results = m_resultsManager->getCurrentResults();
        if (row < 0 || row >= static_cast<int>(results.size())) return QVariant();

        const PointerPath& path = results[row];

        if (role == Qt::DisplayRole) {
            if (col == 0) {
                auto it = m_addressStringCache.find(path.baseAddress);
                QString baseStr;
                if (it != m_addressStringCache.end()) {
                    baseStr = *it;
                }
                else {
                    baseStr = m_emuManager ? m_emuManager->formatDisplayAddress(path.baseAddress) : QString::number(path.baseAddress, 16).toUpper();
                    m_addressStringCache.insert(path.baseAddress, baseStr);
                }
                return path.isPartial ? QString("[Partial] %1").arg(baseStr) : baseStr;
            }

            if (col == 1 + m_maxOffsetsCount) {
                auto it = m_addressStringCache.find(path.finalAddress);
                if (it != m_addressStringCache.end()) {
                    return *it;
                }
                else {
                    QString finalStr = m_emuManager ? m_emuManager->formatDisplayAddress(path.finalAddress) : QString::number(path.finalAddress, 16).toUpper();
                    m_addressStringCache.insert(path.finalAddress, finalStr);
                    return finalStr;
                }
            }

            int offsetIndex = col - 1;
            if (offsetIndex >= 0 && offsetIndex < static_cast<int>(path.offsets.size())) {
                int32_t offset = path.offsets[offsetIndex];
                auto it = m_offsetStringCache.find(offset);
                if (it != m_offsetStringCache.end()) {
                    return *it;
                }

                if (offset == 0) {
                    m_offsetStringCache.insert(0, QStringLiteral("+0"));
                    return QStringLiteral("+0");
                }

                // Generates formatted hexadecimal strings using a stack-allocated buffer. 
                // This avoids standard heap-allocation patterns, minimizing rendering lag 
                // in the list view.
                char buf[16];
                int pos = 15;
                buf[pos] = '\0';
                uint32_t val = static_cast<uint32_t>(std::abs(offset));
                const char hex[] = "0123456789ABCDEF";
                do {
                    buf[--pos] = hex[val & 0xF];
                    val >>= 4;
                } while (val);
                buf[--pos] = (offset < 0) ? '-' : '+';
                QString formatted = QString::fromLatin1(&buf[pos]);
                m_offsetStringCache.insert(offset, formatted);
                return formatted;
            }
            return QVariant();
        }

        if (role == Qt::TextAlignmentRole) {
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }

        if (role == Qt::ForegroundRole && path.isPartial) {
            return QColor(230, 90, 90);
        }

        return QVariant();
    }

    QVariant PointerResultsModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return QVariant();

        if (section == 0) return QStringLiteral("Base Address");
        if (section == 1 + m_maxOffsetsCount) return QStringLiteral("Final Address");

        static std::vector<QString> headerCache;
        if (section >= static_cast<int>(headerCache.size())) {
            size_t oldSize = headerCache.size();
            headerCache.resize(section + 10);
            for (size_t i = oldSize; i < headerCache.size(); ++i) {
                headerCache[i] = QString("Offset %1").arg(i);
            }
        }
        return headerCache[section];
    }

    void PointerResultsModel::refreshModel(bool) {
        beginResetModel();
        m_offsetStringCache.clear();
        m_addressStringCache.clear();
        updateMaxOffsetsCount();
        endResetModel();
    }

}
