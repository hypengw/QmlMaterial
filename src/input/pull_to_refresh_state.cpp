#include "qml_material/input/pull_to_refresh_state.hpp"
#include <algorithm>
#include <cmath>

namespace qml_material
{
PullToRefreshState::PullToRefreshState(QObject* parent): NestedScrollConnection(parent) {}
void PullToRefreshState::update(qreal rawDistance, bool dragging) {
    const auto previousOffset   = m_offset;
    const auto previousArmed    = armed();
    const auto previousDragging = m_dragging;
    m_rawDistance               = rawDistance;
    m_dragging                  = dragging;
    if (m_refreshing)
        m_offset = m_threshold;
    else if (! dragging)
        m_offset = 0;
    else {
        const auto adjusted = rawDistance * 0.5;
        const auto tension  = std::clamp(adjusted / m_threshold - 1, qreal(0), qreal(2));
        m_offset = adjusted <= m_threshold ? adjusted
                                           : m_threshold * (1 + tension - tension * tension / 4);
    }
    ++m_revision;
    QPointer<PullToRefreshState> guard(this);
    if (previousDragging != dragging) emit draggingChanged();
    if (guard && (previousOffset != m_offset || previousArmed != armed())) emit distanceChanged();
}
void PullToRefreshState::setThreshold(qreal value) {
    if (! std::isfinite(value) || value <= 0 || value == m_threshold) return;
    m_threshold = value;
    QPointer<PullToRefreshState> guard(this);
    update(0, false);
    if (guard) emit invalidated();
    if (guard) emit thresholdChanged();
    if (guard) emit distanceChanged();
}
void PullToRefreshState::setEnabled(bool value) {
    if (value == m_enabled) return;
    m_enabled = value;
    QPointer<PullToRefreshState> guard(this);
    update(0, false);
    if (guard) emit invalidated();
    if (guard) emit enabledChanged();
}
void PullToRefreshState::setRefreshing(bool value) {
    if (value == m_refreshing) return;
    m_refreshing = value;
    QPointer<PullToRefreshState> guard(this);
    update(0, false);
    if (guard) emit refreshingChanged();
}
void PullToRefreshState::setSettling(bool value) {
    if (value == m_settling) return;
    m_settling = value;
    emit settlingChanged();
}
bool PullToRefreshState::canConsume(QPointF delta, Source source) const {
    return source == Drag && m_enabled && ! m_refreshing && ! m_settling &&
           std::isfinite(delta.y()) && (delta.y() < 0 || (delta.y() > 0 && m_rawDistance > 0));
}
QPointF PullToRefreshState::consume(QPointF available) {
    if (! m_dragging) return {};
    const auto next = std::max(qreal(0), m_rawDistance - available.y());
    if (! std::isfinite(next)) return {};
    const auto used = m_rawDistance - next;
    update(next, true);
    return { 0, used };
}
QPointF PullToRefreshState::preScroll(QPointF available, Source source) {
    return canConsume(available, source) && available.y() > 0 ? consume(available) : QPointF();
}
QPointF PullToRefreshState::postScroll(QPointF, QPointF available, Source source) {
    return canConsume(available, source) ? consume(available) : QPointF();
}
void PullToRefreshState::begin(Source source) {
    if (source == Drag && m_enabled && ! m_refreshing && ! m_settling) update(0, true);
}
QPointF PullToRefreshState::release(QPointF velocity) {
    if (! m_dragging) return {};
    const auto consumed =
        m_rawDistance > 0 && std::isfinite(velocity.y()) ? std::min(qreal(0), velocity.y()) : 0;
    const bool                   request = armed() && m_enabled && ! m_refreshing;
    QPointer<PullToRefreshState> guard(this);
    const auto                   revision = m_revision + 1;
    update(0, false);
    if (guard && m_revision == revision && request && m_enabled && ! m_refreshing)
        emit refreshRequested();
    return { 0, consumed };
}
void PullToRefreshState::end(bool) {
    if (m_dragging) update(0, false);
}
} // namespace qml_material
