#include "qml_material/input/app_bar_scroll.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace qml_material
{
AppBarScroll::AppBarScroll(QObject* parent): NestedScrollConnection(parent) {}
void AppBarScroll::classBegin() { m_initializing = true; }
void AppBarScroll::componentComplete() {
    m_initializing = false;
    if (const auto value = std::exchange(m_initialOffset, std::nullopt)) setHeightOffset(*value);
}
void AppBarScroll::update(qreal offset, bool active) {
    const bool moved     = m_offset != offset;
    const bool activated = m_active != active;
    const bool overlap   = overlapped();
    m_offset             = offset;
    m_active             = active;
    QPointer<AppBarScroll> guard(this);
    if (activated) emit activeChanged();
    if (guard && moved) emit heightOffsetChanged();
    if (guard && overlap != overlapped()) emit overlappedChanged();
}
void AppBarScroll::setHeightOffset(qreal value) {
    if (! std::isfinite(value)) return;
    if (m_initializing) m_initialOffset = value;
    update(m_enabled && m_mode != Pinned ? std::clamp(value, -m_distance, qreal(0)) : 0, m_active);
}
void AppBarScroll::setCollapseDistance(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_distance) return;
    const auto fraction       = collapsedFraction();
    const auto previousOffset = m_offset;
    m_distance                = value;
    QPointer<AppBarScroll> guard(this);
    update(-fraction * value, false);
    if (guard) emit invalidated();
    if (guard) emit collapseDistanceChanged();
    if (guard && previousOffset == m_offset) emit heightOffsetChanged();
}
void AppBarScroll::setMode(Mode value) {
    if (value < Pinned || value > ExitUntilCollapsed || m_mode == value) return;
    m_mode = value;
    QPointer<AppBarScroll> guard(this);
    update(value == Pinned ? 0 : m_offset, false);
    if (guard) emit invalidated();
    if (guard) emit modeChanged();
}
void AppBarScroll::setEnabled(bool value) {
    if (value == m_enabled) return;
    m_enabled = value;
    QPointer<AppBarScroll> guard(this);
    update(0, false);
    if (guard) emit invalidated();
    if (guard) emit enabledChanged();
    if (guard) emit overlappedChanged();
}
void AppBarScroll::setReverseLayout(bool value) {
    if (value == m_reverseLayout) return;
    m_reverseLayout = value;
    QPointer<AppBarScroll> guard(this);
    update(m_offset, false);
    if (guard) emit invalidated();
    if (guard) emit reverseLayoutChanged();
}
void AppBarScroll::setContentAtStart(bool value) {
    if (value == m_contentAtStart) return;
    const bool overlap = overlapped();
    m_contentAtStart   = value;
    QPointer<AppBarScroll> guard(this);
    emit                   contentAtStartChanged();
    if (guard && overlap != overlapped()) emit overlappedChanged();
}
bool AppBarScroll::canConsume(QPointF delta, Source) const {
    if (! m_enabled || m_mode == Pinned || ! std::isfinite(delta.y())) return false;
    const auto amount = m_reverseLayout ? -delta.y() : delta.y();
    return amount > 0 ? m_offset > -m_distance : amount < 0 && m_offset < 0;
}
QPointF AppBarScroll::consume(QPointF delta) {
    if (! canConsume(delta, Drag)) return {};
    const auto direction = m_reverseLayout ? -1 : 1;
    const auto offset    = std::clamp(m_offset - delta.y() * direction, -m_distance, qreal(0));
    const auto used      = (m_offset - offset) * direction;
    setHeightOffset(offset);
    return { 0, used };
}
QPointF AppBarScroll::preScroll(QPointF available, Source) {
    const auto amount = m_reverseLayout ? -available.y() : available.y();
    return m_mode == EnterAlways || amount > 0 ? consume(available) : QPointF();
}
QPointF AppBarScroll::postScroll(QPointF, QPointF available, Source) {
    const auto amount = m_reverseLayout ? -available.y() : available.y();
    return m_mode == ExitUntilCollapsed && amount < 0 ? consume(available) : QPointF();
}
void AppBarScroll::begin(Source) {
    if (! m_enabled) return;
    QPointer<AppBarScroll> guard(this);
    update(m_offset, true);
    if (guard && m_active && m_enabled) emit inputStarted();
}
void AppBarScroll::end(bool cancelled) {
    update(cancelled ? m_offset : collapsedFraction() >= 0.5 ? -m_distance : 0, false);
}
void AppBarScroll::reset() {
    QPointer<AppBarScroll> guard(this);
    update(0, false);
    if (guard) emit invalidated();
}
} // namespace qml_material
