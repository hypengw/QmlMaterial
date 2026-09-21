#include "qml_material/control/switch.hpp"
#include <QGuiApplication>
#include <QStyleHints>
#include <algorithm>
#include <cmath>

namespace qml_material
{
Switch::Switch(QQuickItem* parent): AbstractButton(parent) {
    setCheckable(true);
    connect(this, &AbstractButton::checkedChanged, this, [this] {
        setPosition(isChecked() ? 1 : 0);
    });
    connect(this, &Control::mirroredChanged, this, &Switch::visualPositionChanged);
    connect(this, &AbstractButton::indicatorChanged, this, &Switch::interactionEnded);
}
void Switch::setPosition(qreal value) {
    value = std::clamp(value, qreal(0), qreal(1));
    if (qFuzzyCompare(m_position, value)) return;
    m_position = value;
    QPointer<Switch> guard(this);
    Q_EMIT positionChanged();
    if (guard) Q_EMIT visualPositionChanged();
}
qreal Switch::positionAt(const QPointF& point) const {
    if (! indicator() || indicator()->width() <= 0) return -1;
    const qreal value = indicator()->mapFromItem(this, point).x() / indicator()->width();
    return mirrored() ? 1 - value : value;
}
void Switch::pointerStarted(const QPointF& point) {
    m_origin   = point;
    m_dragging = false;
}
void Switch::pointerMoved(const QPointF& point) {
    if (! indicator() || indicator()->width() <= 0) return;
    const auto start   = positionAt(m_origin);
    const auto current = positionAt(point);
    if (! m_dragging &&
        std::abs(point.x() - m_origin.x()) > QGuiApplication::styleHints()->startDragDistance() &&
        ((start >= 0 && start <= 1) || (current >= 0 && current <= 1))) {
        m_dragging = true;
        setKeepMouseGrab(true);
        setKeepTouchGrab(true);
    }
    if (m_dragging) setPosition(current);
}
void Switch::nextCheckState() {
    if (m_dragging)
        setCheckedByInteraction(m_position > 0.5);
    else
        AbstractButton::nextCheckState();
}
void Switch::interactionEnded() {
    m_dragging = false;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    setPosition(isChecked() ? 1 : 0);
}
} // namespace qml_material
