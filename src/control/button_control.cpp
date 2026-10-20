#include "qml_material/control/button_control.hpp"
#include <QFocusEvent>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTouchEvent>

namespace qml_material
{
ButtonControl::ButtonControl(QQuickItem* parent)
    : ButtonPresentation(parent), m_input(this, [this] {
          const QPointer<ButtonControl> guard(this);
          inputSourceChange();
          if (guard) Q_EMIT interactionSourceChanged();
      }) {
    setFocusPolicy(Qt::TabFocus);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    m_input.setHoverEnabled(hoverEnabled());
    connect(this, &Control::hoverEnabledChanged, this, [this] {
        m_input.setHoverEnabled(hoverEnabled());
    });
    m_input.acceptsInput = [this] {
        return canActivate();
    };
    m_input.click().clicked = [this] {
        activate();
    };
    m_input.focusVisibleChanged = [this] {
        Q_EMIT focusVisibleChanged();
    };
}
bool ButtonControl::canActivate() const { return isEnabled() && isVisible(); }
void ButtonControl::click() { m_input.click().activate(); }
bool ButtonControl::event(QEvent* event) {
    const QPointer<ButtonControl> guard(this);
    const bool                    handled = m_input.observeEvent(event);
    if (! guard || handled) return true;
    return ButtonPresentation::event(event);
}
void ButtonControl::mousePressEvent(QMouseEvent* event) { m_input.event(event); }
void ButtonControl::mouseMoveEvent(QMouseEvent* event) { m_input.event(event); }
void ButtonControl::mouseReleaseEvent(QMouseEvent* event) { m_input.event(event); }
void ButtonControl::mouseDoubleClickEvent(QMouseEvent* event) { m_input.event(event); }
void ButtonControl::touchEvent(QTouchEvent* event) { m_input.event(event); }
void ButtonControl::keyPressEvent(QKeyEvent* event) { m_input.event(event); }
void ButtonControl::keyReleaseEvent(QKeyEvent* event) { m_input.event(event); }
void ButtonControl::mouseUngrabEvent() { m_input.mouseUngrabbed(); }
void ButtonControl::touchUngrabEvent() { m_input.touchUngrabbed(); }
void ButtonControl::focusInEvent(QFocusEvent* event) { QQuickItem::focusInEvent(event); }
void ButtonControl::focusOutEvent(QFocusEvent* event) { QQuickItem::focusOutEvent(event); }
void ButtonControl::hoverEnterEvent(QHoverEvent* event) { QQuickItem::hoverEnterEvent(event); }
void ButtonControl::hoverMoveEvent(QHoverEvent* event) { QQuickItem::hoverMoveEvent(event); }
void ButtonControl::hoverLeaveEvent(QHoverEvent* event) { QQuickItem::hoverLeaveEvent(event); }
} // namespace qml_material
