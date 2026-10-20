#include "qml_material/carousel/carousel_item.hpp"
#include "qml_material/carousel/carousel_view.hpp"
#include "qml_material/util/qt.hpp"
#include <QKeyEvent>

namespace qml_material
{
CarouselItem::CarouselItem(QQuickItem* parent): CommandButton(parent) {
    inputHost().click().pointerStarted = [this](const PointerInput&) {
        if (m_carousel) m_carousel->claimInteractionFocus();
    };
    m_focusConnections.append(
        connect(this, &QQuickItem::activeFocusChanged, this, &CarouselItem::updateFocus));
    m_focusConnections.append(
        connect(this, &ButtonControl::focusVisibleChanged, this, &CarouselItem::updateFocus));
}
CarouselItem::~CarouselItem() { utils::disconnectAll(m_focusConnections); }
CarouselView* CarouselItem::carousel() const { return m_carousel; }
void          CarouselItem::setCarousel(CarouselView* owner) {
    if (m_carousel == owner) return;
    m_carousel = owner;
    m_retired  = ! owner;
    cancelInput();
}
bool CarouselItem::canActivate() const {
    return ! m_retired && CommandButton::canActivate() &&
           (! m_carousel || m_carousel->ownsDelegate(this));
}
void CarouselItem::updateFocus() {
    if (m_carousel) m_carousel->delegateFocusChanged(this);
}
void CarouselItem::keyPressEvent(QKeyEvent* event) {
    if (m_carousel && canActivate()) {
        const bool horizontal = m_carousel->orientation() == Qt::Horizontal;
        const bool mirrored   = horizontal && m_carousel->mirrored();
        const int  previous   = horizontal ? (mirrored ? Qt::Key_Right : Qt::Key_Left) : Qt::Key_Up;
        const int  next = horizontal ? (mirrored ? Qt::Key_Left : Qt::Key_Right) : Qt::Key_Down;
        if (event->key() == previous || event->key() == next) {
            const QPointer<CarouselView> owner = m_carousel;
            event->accept();
            if (event->key() == previous)
                owner->decrementCurrentIndexFromKeyboard();
            else
                owner->incrementCurrentIndexFromKeyboard();
            return;
        }
    }
    CommandButton::keyPressEvent(event);
}
} // namespace qml_material
