#pragma once

#include <QHeaderView>
#include <QPainter>
#include <QStyleOptionHeader>

namespace PointerFinder2::UI::Controls {

    // This helper class draws the header row above the search results list,
    // displaying the column titles and small arrows indicating the current sorting order.
    class PointerResultsHeaderView : public QHeaderView {
        Q_OBJECT
    public:
        explicit PointerResultsHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr)
            : QHeaderView(orientation, parent) {
            setDefaultAlignment(Qt::AlignCenter);
            setSectionsClickable(true);
        }

    protected:
        void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override {
            if (!rect.isValid()) return;

            painter->save();

            QStyleOptionHeader opt;
            initStyleOption(&opt);
            opt.rect = rect;
            opt.section = logicalIndex;

            if (model()) {
                opt.text = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
            }

            bool isSorted = isSortIndicatorShown() && (sortIndicatorSection() == logicalIndex);

            opt.textAlignment = Qt::AlignHCenter | Qt::AlignBottom;
            opt.sortIndicator = QStyleOptionHeader::None;
            style()->drawControl(QStyle::CE_Header, &opt, painter, this);

            if (isSorted) {
                QStyleOptionHeader arrowOpt = opt;
                arrowOpt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
                    ? QStyleOptionHeader::SortDown
                    : QStyleOptionHeader::SortUp;

                int arrowWidth = 10;
                int arrowHeight = 8;
                int arrowX = rect.left() + (rect.width() - arrowWidth) / 2;
                int arrowY = rect.top() + 3;
                arrowOpt.rect = QRect(arrowX, arrowY, arrowWidth, arrowHeight);

                style()->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &arrowOpt, painter, this);
            }

            painter->restore();
        }
    };

}
