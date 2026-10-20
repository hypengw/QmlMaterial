#include "qml_material/input/interactive_item.hpp"

#include <QMouseEvent>
#include <QTouchEvent>
#include <QKeyEvent>

namespace qml_material
{
InteractiveItem::InteractiveItem(QQuickItem* parent)
    : QQuickItem(parent), m_input(this, [this] {
          Q_EMIT interactionSourceChanged();
      }) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setAcceptHoverEvents(true);
    setActiveFocusOnTab(true);
    m_input.click().clicked = [this] {
        activate();
    };
    m_input.focusVisibleChanged = [this] {
        Q_EMIT focusVisibleChanged();
    };
}
bool InteractiveItem::event(QEvent* event) {
    QPointer<InteractiveItem> guard(this);
    const bool                handled = m_input.observeEvent(event);
    if (! guard || handled) return true;
    return QQuickItem::event(event);
}
void InteractiveItem::touchUngrabEvent() { m_input.touchUngrabbed(); }
void InteractiveItem::mousePressEvent(QMouseEvent* event) { m_input.event(event); }
void InteractiveItem::mouseMoveEvent(QMouseEvent* event) { m_input.event(event); }
void InteractiveItem::mouseReleaseEvent(QMouseEvent* event) { m_input.event(event); }
void InteractiveItem::mouseDoubleClickEvent(QMouseEvent* event) { m_input.event(event); }
void InteractiveItem::touchEvent(QTouchEvent* event) { m_input.event(event); }
void InteractiveItem::keyPressEvent(QKeyEvent* event) { m_input.event(event); }
void InteractiveItem::keyReleaseEvent(QKeyEvent* event) { m_input.event(event); }
void InteractiveItem::mouseUngrabEvent() { m_input.mouseUngrabbed(); }
} // namespace qml_material
