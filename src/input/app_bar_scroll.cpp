#include "qml_material/input/app_bar_scroll.hpp"
#include "qml_material/anim/interpolator.hpp"
#include "qml_material/token/duration.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace qml_material
{
AppBarScroll::AppBarScroll(QObject* parent)
    : NestedScrollConnection(parent), m_snap(new QVariantAnimation(this)) {
    m_snap->setDuration(int(token::Duration().short4));
    m_snap->setEasingCurve(anim::emphasized());
    connect(m_snap, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        if (settling()) update(std::clamp(value.toReal(), -m_distance, qreal(0)), false);
    });
    connect(m_snap, &QVariantAnimation::stateChanged, this, [this] {
        emit settlingChanged();
    });
}
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
    ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard) return;
    update(m_enabled && m_mode != Pinned ? std::clamp(value, -m_distance, qreal(0)) : 0, m_active);
}
void AppBarScroll::setCollapseDistance(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_distance) return;
    const auto fraction       = collapsedFraction();
    const auto previousOffset = m_offset;
    const auto overlap        = overlappedFraction();
    ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard) return;
    m_distance = value;
    update(-fraction * value, false);
    if (guard) emit invalidated();
    if (guard) emit collapseDistanceChanged();
    if (guard && previousOffset == m_offset) emit heightOffsetChanged();
    if (guard && overlap != overlappedFraction()) emit overlappedFractionChanged();
}
void AppBarScroll::setMode(Mode value) {
    if (value < Pinned || value > ExitUntilCollapsed || m_mode == value) return;
    ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard) return;
    m_mode = value;
    update(value == Pinned ? 0 : m_offset, false);
    if (guard) emit invalidated();
    if (guard) emit modeChanged();
}
void AppBarScroll::setEnabled(bool value) {
    if (value == m_enabled) return;
    ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard) return;
    m_enabled = value;
    update(0, false);
    if (guard) emit invalidated();
    if (guard) emit enabledChanged();
    if (guard) emit overlappedChanged();
    if (guard) emit overlappedFractionChanged();
}
void AppBarScroll::setReverseLayout(bool value) {
    if (value == m_reverseLayout) return;
    ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard) return;
    m_reverseLayout = value;
    setContentOffset(0);
    if (! guard) return;
    update(m_offset, false);
    if (guard) emit invalidated();
    if (guard) emit reverseLayoutChanged();
}
void AppBarScroll::setContentAtStart(bool value) {
    if (value == m_contentAtStart) return;
    const bool overlap      = overlapped();
    const auto fraction     = overlappedFraction();
    const bool resetContent = value && m_contentOffset != 0;
    m_contentAtStart        = value;
    if (value) m_contentOffset = 0;
    QPointer<AppBarScroll> guard(this);
    emit                   contentAtStartChanged();
    if (guard && resetContent) emit contentOffsetChanged();
    if (guard && fraction != overlappedFraction()) emit overlappedFractionChanged();
    if (guard && overlap != overlapped()) emit overlappedChanged();
}
qreal AppBarScroll::overlappedFraction() const {
    if (! m_enabled) return 0;
    if (! m_contentAtStart && m_contentOffset == 0) return 1;
    return m_distance > 0 ? std::clamp(std::abs(m_contentOffset) / m_distance, qreal(0), qreal(1))
           : m_contentOffset != 0 ? 1
                                  : 0;
}
void AppBarScroll::setContentOffset(qreal value) {
    if (! std::isfinite(value) || value == m_contentOffset) return;
    const auto fraction = overlappedFraction();
    m_contentOffset     = value;
    QPointer<AppBarScroll> guard(this);
    emit                   contentOffsetChanged();
    if (guard && fraction != overlappedFraction()) emit overlappedFractionChanged();
}
void AppBarScroll::setAnimationsEnabled(bool value) {
    if (value == m_animationsEnabled) return;
    const auto target               = m_snap->endValue().toReal();
    const bool finish               = settling() && ! value;
    m_animationsEnabled             = value;
    const auto             revision = ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard) return;
    if (finish && revision == m_revision) update(std::clamp(target, -m_distance, qreal(0)), false);
    if (guard) emit animationsEnabledChanged();
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
QPointF AppBarScroll::postScroll(QPointF consumed, QPointF available, Source) {
    if (! m_enabled) return {};
    const auto             content = m_reverseLayout ? -consumed.y() : consumed.y();
    QPointer<AppBarScroll> guard(this);
    // The view may reset the offset at its start before this post-scroll arrives.
    setContentOffset(
        m_contentAtStart && m_contentOffset == 0 && content < 0 ? 0 : m_contentOffset + content);
    if (! guard) return {};
    const auto amount = m_reverseLayout ? -available.y() : available.y();
    return m_mode == ExitUntilCollapsed && amount < 0 ? consume(available) : QPointF();
}
void AppBarScroll::begin(Source) {
    if (! m_enabled) return;
    const auto             revision = ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard || revision != m_revision || ! m_enabled) return;
    update(m_offset, true);
    if (guard && m_active && m_enabled) emit inputStarted();
}
void AppBarScroll::end(bool cancelled) {
    const auto             revision = ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard || revision != m_revision) return;
    update(m_offset, false);
    if (! guard || revision != m_revision || cancelled || ! m_enabled || m_mode == Pinned) return;
    const auto fraction = collapsedFraction();
    if (fraction < 0.01 || fraction == 1) return;
    const auto target = fraction >= 0.5 ? -m_distance : 0;
    if (! m_animationsEnabled) {
        update(target, false);
        return;
    }
    m_snap->setStartValue(m_offset);
    m_snap->setEndValue(target);
    m_snap->start();
}
void AppBarScroll::reset() {
    ++m_revision;
    QPointer<AppBarScroll> guard(this);
    m_snap->stop();
    if (! guard) return;
    update(0, false);
    if (guard) setContentOffset(0);
    if (guard) emit invalidated();
}
} // namespace qml_material
