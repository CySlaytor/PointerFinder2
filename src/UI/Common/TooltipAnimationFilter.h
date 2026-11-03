#pragma once

#include <QEvent>
#include <QObject>

namespace PointerFinder2::UI {

    // Event filter providing hardware-accelerated symmetric fade-in and fade-out
    // animations for all tooltips across the application on Windows.
    class TooltipAnimationFilter : public QObject {
        Q_OBJECT
    public:
        explicit TooltipAnimationFilter(QObject* parent = nullptr);
        ~TooltipAnimationFilter() override = default;

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;
    };

}
