#include "TooltipAnimationFilter.h"

#include <QEasingCurve>
#include <QPropertyAnimation>
#include <QWidget>

namespace PointerFinder2::UI {

TooltipAnimationFilter::TooltipAnimationFilter(QObject* parent) : QObject(parent) {}

bool TooltipAnimationFilter::eventFilter(QObject* obj, QEvent* event) {
#ifdef Q_OS_WIN
    // Hardware-accelerated symmetric fade-in and fade-out for all application tooltips
    if (obj && obj->isWidgetType()) {
        auto* w = static_cast<QWidget*>(obj);
        if ((w->windowFlags() & Qt::WindowType_Mask) == Qt::ToolTip) {
            if (event->type() == QEvent::Show) {
                auto* existingAnim = w->findChild<QPropertyAnimation*>("tooltip_anim");
                if (existingAnim) {
                    existingAnim->stop();
                    delete existingAnim;
                }

                w->setProperty("pf_closing", false);
                w->setProperty("pf_allow_close", false);
                w->setWindowOpacity(0.0);

                auto* anim = new QPropertyAnimation(w, "windowOpacity", w);
                anim->setObjectName("tooltip_anim");
                anim->setDuration(120);
                anim->setStartValue(0.0);
                anim->setEndValue(1.0);
                anim->setEasingCurve(QEasingCurve::OutQuad);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            } else if (event->type() == QEvent::Close) {
                if (w->property("pf_allow_close").toBool()) {
                    return false;
                }

                event->ignore();

                if (!w->property("pf_closing").toBool()) {
                    w->setProperty("pf_closing", true);

                    auto* existingAnim = w->findChild<QPropertyAnimation*>("tooltip_anim");
                    if (existingAnim) {
                        existingAnim->stop();
                        delete existingAnim;
                    }

                    if (w->windowOpacity() <= 0.05) {
                        w->setProperty("pf_allow_close", true);
                        w->close();
                        w->deleteLater();
                        return true;
                    }

                    auto* anim = new QPropertyAnimation(w, "windowOpacity", w);
                    anim->setObjectName("tooltip_anim");
                    anim->setDuration(140);
                    anim->setStartValue(w->windowOpacity());
                    anim->setEndValue(0.0);
                    anim->setEasingCurve(QEasingCurve::OutQuad);
                    connect(anim, &QPropertyAnimation::finished, w, [w]() {
                        w->setProperty("pf_allow_close", true);
                        w->close();
                        w->deleteLater();
                    });
                    anim->start(QAbstractAnimation::DeleteWhenStopped);
                }
                return true;
            } else if (event->type() == QEvent::DeferredDelete) {
                // Prevent Qt from destroying the tooltip widget immediately before the fade-out animation completes
                if (!w->property("pf_allow_close").toBool()) {
                    return true;
                }
            }
        }
    }
#endif
    return QObject::eventFilter(obj, event);
}

} // namespace PointerFinder2::UI
