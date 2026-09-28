#include "qml_material/input/floating_toolbar_exit.hpp"
#include <QPointer>
#include <algorithm>
#include <cmath>
#include <utility>

namespace qml_material
{
FloatingToolbarExit::FloatingToolbarExit(QObject* parent): QObject(parent) {}
void FloatingToolbarExit::classBegin() { m_initializing = true; }
void FloatingToolbarExit::componentComplete() {
    m_initializing = false;
    if (const auto initial = std::exchange(m_initialOffset, std::nullopt)) setOffset(*initial);
}
void FloatingToolbarExit::update(qreal offset, bool active) {
    const bool moved   = m_offset != offset;
    const bool changed = m_active != active;
    m_offset           = offset;
    m_active           = active;
    QPointer<FloatingToolbarExit> guard(this);
    if (changed) emit activeChanged();
    if (guard && moved) emit offsetChanged();
}
void FloatingToolbarExit::setDistance(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_distance) return;
    const auto fraction = m_distance > 0 ? m_offset / m_distance : 0;
    m_distance          = value;
    m_offset            = fraction * value;
    QPointer<FloatingToolbarExit> guard(this);
    emit                          distanceChanged();
    if (guard) emit offsetChanged();
}
void FloatingToolbarExit::setOffset(qreal value) {
    if (! std::isfinite(value)) return;
    // QML may assign the initial offset before the distance binding is evaluated.
    if (m_initializing) m_initialOffset = value;
    update(m_enabled ? std::clamp(value, qreal(0), m_distance) : 0, m_active);
}
void FloatingToolbarExit::setEnabled(bool value) {
    if (m_enabled == value) return;
    m_enabled = value;
    QPointer<FloatingToolbarExit> guard(this);
    reset();
    if (guard) emit enabledChanged();
}
void FloatingToolbarExit::setReverseLayout(bool value) {
    if (m_reverseLayout == value) return;
    m_reverseLayout = value;
    emit reverseLayoutChanged();
}
void FloatingToolbarExit::begin() {
    if (! m_enabled) return;
    QPointer<FloatingToolbarExit> guard(this);
    update(m_offset, true);
    if (guard && m_enabled && m_active) emit inputStarted();
}
void FloatingToolbarExit::scrollBy(QPointF consumed) {
    if (! m_enabled || ! std::isfinite(consumed.y()) || consumed.y() == 0) return;
    QPointer<FloatingToolbarExit> guard(this);
    begin();
    if (guard && m_enabled && m_active)
        setOffset(m_offset + (m_reverseLayout ? -consumed.y() : consumed.y()));
}
void FloatingToolbarExit::settle() {
    update(m_enabled && m_offset >= m_distance / 2 ? m_distance : 0, false);
}
void FloatingToolbarExit::reset() { update(0, false); }
} // namespace qml_material
