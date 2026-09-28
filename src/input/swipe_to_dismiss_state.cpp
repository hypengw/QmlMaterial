#include "qml_material/input/swipe_to_dismiss_state.hpp"

#include <QPointer>
#include <algorithm>
#include <array>
#include <cmath>

namespace qml_material
{
SwipeToDismissState::SwipeToDismissState(QObject* parent): QObject(parent) {}

qreal SwipeToDismissState::progress() const {
    return m_distance > 0 ? std::abs(m_offset) / m_distance : 0;
}

SwipeToDismissState::Value SwipeToDismissState::dismissDirection() const {
    return m_offset > 0 ? StartToEnd : m_offset < 0 ? EndToStart : Settled;
}

bool SwipeToDismissState::allowed(Value value) const {
    return value == Settled || (value == StartToEnd && m_startToEnd) ||
           (value == EndToStart && m_endToStart);
}

qreal SwipeToDismissState::position(Value value) const {
    return value == StartToEnd ? m_distance : value == EndToStart ? -m_distance : 0;
}

qreal SwipeToDismissState::bounded(qreal offset) const {
    return std::clamp(offset, m_endToStart ? -m_distance : 0, m_startToEnd ? m_distance : 0);
}

void SwipeToDismissState::setDistance(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_distance) return;
    m_distance = value;
    anchorsChanged();
}

void SwipeToDismissState::setStartToEndEnabled(bool value) {
    if (value == m_startToEnd) return;
    m_startToEnd = value;
    anchorsChanged();
}

void SwipeToDismissState::setEndToStartEnabled(bool value) {
    if (value == m_endToStart) return;
    m_endToStart = value;
    anchorsChanged();
}

void SwipeToDismissState::setPositionalThreshold(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_positionalThreshold) return;
    m_positionalThreshold = value;
    emit configurationChanged();
}

void SwipeToDismissState::setVelocityThreshold(qreal value) {
    if (! std::isfinite(value) || value <= 0 || value == m_velocityThreshold) return;
    m_velocityThreshold = value;
    emit configurationChanged();
}

void SwipeToDismissState::anchorsChanged() {
    QPointer<SwipeToDismissState> guard(this);
    const auto                    revision = ++m_revision;
    emit                          configurationChanged();
    if (! guard || revision != m_revision) return;
    const auto target = allowed(m_target) && m_distance > 0 ? m_target : Settled;
    request(target, m_settling || m_dragging);
}

SwipeToDismissState::Value SwipeToDismissState::releaseTarget(qreal velocity) const {
    std::array<Value, 3> values { EndToStart, Settled, StartToEnd };
    Value                left          = Settled;
    Value                right         = Settled;
    qreal                leftPosition  = -m_distance;
    qreal                rightPosition = m_distance;
    bool                 hasLeft       = false;
    bool                 hasRight      = false;
    for (const auto value : values) {
        if (! allowed(value)) continue;
        const auto anchor = position(value);
        if (anchor <= m_offset && (! hasLeft || anchor > leftPosition)) {
            left         = value;
            leftPosition = anchor;
            hasLeft      = true;
        }
        if (anchor >= m_offset && (! hasRight || anchor < rightPosition)) {
            right         = value;
            rightPosition = anchor;
            hasRight      = true;
        }
    }
    if (! hasLeft) return right;
    if (! hasRight) return left;
    if (velocity == 0) return m_offset - leftPosition <= rightPosition - m_offset ? left : right;
    if (std::abs(velocity) >= m_velocityThreshold) return velocity > 0 ? right : left;
    if (velocity > 0) return m_offset - leftPosition >= m_positionalThreshold ? right : left;
    return rightPosition - m_offset >= m_positionalThreshold ? left : right;
}

void SwipeToDismissState::request(Value value, bool animate) {
    if (! allowed(value)) return;
    if (m_distance <= 0) value = Settled;
    const auto revision = ++m_revision;
    m_dragging          = false;
    m_target            = value;
    m_offset            = position(value);
    m_settling          = animate;
    if (! animate) m_settled = value;
    QPointer<SwipeToDismissState> guard(this);
    emit                          motionChanged();
    if (! guard || revision != m_revision) return;
    emit transitionRequested(revision, m_offset, animate);
}

void SwipeToDismissState::dismiss(Value value) {
    if (value == Settled || ! allowed(value) || m_distance <= 0) return;
    if (! m_dragging && ! m_settling && m_settled == value) return;
    request(value, true);
}

void SwipeToDismissState::reset() { request(Settled, true); }
void SwipeToDismissState::snapTo(Value value) { request(value, false); }

bool SwipeToDismissState::begin(qreal presentedOffset) {
    if (! std::isfinite(presentedOffset) || m_distance <= 0 || m_settled != Settled ||
        (! m_startToEnd && ! m_endToStart))
        return false;
    const auto revision = ++m_revision;
    m_offset            = bounded(presentedOffset);
    m_dragging          = true;
    m_settling          = false;
    m_target            = releaseTarget(0);
    QPointer<SwipeToDismissState> guard(this);
    emit                          motionChanged();
    return guard && revision == m_revision && m_dragging;
}

void SwipeToDismissState::dragBy(qreal delta) {
    if (! m_dragging || ! std::isfinite(delta)) return;
    const auto offset = bounded(m_offset + delta);
    if (offset == m_offset) return;
    ++m_revision;
    m_offset = offset;
    m_target = releaseTarget(0);
    emit motionChanged();
}

void SwipeToDismissState::release(qreal velocity) {
    if (! m_dragging) return;
    request(releaseTarget(std::isfinite(velocity) ? velocity : 0), true);
}

void SwipeToDismissState::cancel() {
    if (! m_dragging) return;
    request(allowed(m_settled) ? m_settled : Settled, true);
}

void SwipeToDismissState::complete(quint32 revision) {
    if (revision != m_revision || ! m_settling) return;
    const auto value  = m_target;
    const bool notify = value != Settled && value != m_settled;
    m_settled         = value;
    m_settling        = false;
    QPointer<SwipeToDismissState> guard(this);
    emit                          motionChanged();
    if (! guard || revision != m_revision) return;
    if (notify) emit dismissed(value);
}
} // namespace qml_material
