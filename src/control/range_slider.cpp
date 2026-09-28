#include "qml_material/control/range_slider.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace qml_material
{
RangeSliderNode::RangeSliderNode(RangeSlider* owner, int index)
    : QObject(owner), m_owner(owner), m_index(index) {
    connect(this, &RangeSliderNode::positionChanged, this, &RangeSliderNode::visualPositionChanged);
    connect(owner, &RangeSlider::orientationChanged, this, &RangeSliderNode::visualPositionChanged);
    connect(owner, &Control::mirroredChanged, this, &RangeSliderNode::visualPositionChanged);
    connect(owner, &Control::visualFocusChanged, this, &RangeSliderNode::focusedChanged);
}
qreal RangeSliderNode::value() const { return m_owner->nodeValue(m_index); }
qreal RangeSliderNode::position() const { return m_owner->nodePosition(m_index); }
void  RangeSliderNode::setValue(qreal value) { m_owner->setNodeValue(m_index, value); }
void  RangeSliderNode::increase() {
    setValue(value() + (m_owner->stepSize() > 0 ? m_owner->stepSize() : .1));
}
void RangeSliderNode::decrease() {
    setValue(value() - (m_owner->stepSize() > 0 ? m_owner->stepSize() : .1));
}

RangeSlider::RangeSlider(QQuickItem* parent): Control(parent), m_first(this, 0), m_second(this, 1) {
#ifdef Q_OS_MACOS
    setFocusPolicy(Qt::TabFocus);
#else
    setFocusPolicy(Qt::StrongFocus);
#endif
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    connect(this, &Control::mirroredChanged, this, [this] {
        if (prepareChange()) Q_EMIT trackGeometryChanged();
    });
    connect(this, &Control::availableWidthChanged, this, &RangeSlider::trackGeometryChanged);
    connect(this, &Control::availableHeightChanged, this, &RangeSlider::trackGeometryChanged);
    connect(this, &Control::leftPaddingChanged, this, &RangeSlider::trackGeometryChanged);
    connect(this, &Control::topPaddingChanged, this, &RangeSlider::trackGeometryChanged);
    connect(this, &RangeSlider::orientationChanged, this, &RangeSlider::trackGeometryChanged);
}
qreal RangeSlider::from() const { return m_state.value().from; }
qreal RangeSlider::to() const { return m_state.value().to; }
qreal RangeSlider::stepSize() const { return m_state.value().step; }
qreal RangeSlider::minimumRange() const { return m_state.value().minimum; }
qreal RangeSlider::effectiveMinimumRange() const { return m_state.value().effective; }
qreal RangeSlider::nodeValue(int index) const { return m_state.value().values[index]; }
qreal RangeSlider::nodePosition(int index) const { return m_state.value().positions[index]; }
qreal RangeSlider::span(const State& state) { return std::abs(state.to - state.from); }
qreal RangeSlider::distance(const State& state, qreal v) {
    // Subtract after clamping to avoid overflow for finite, opposite-sign inputs.
    v = std::clamp(v, std::min(state.from, state.to), std::max(state.from, state.to));
    return state.to >= state.from ? v - state.from : state.from - v;
}
qreal RangeSlider::value(const State& state, qreal offset) {
    return offset == span(state) ? state.to
                                 : state.from + (state.to >= state.from ? offset : -offset);
}

qreal RangeSlider::nearest(const State& state, qreal requested, qreal lower, qreal upper) {
    requested = std::clamp(requested, lower, upper);
    if (state.step == 0) return requested;
    if (! std::isfinite(span(state) / state.step)) return requested;
    // Include the non-grid range endpoint without snapping a constrained result off-grid.
    const qreal length    = span(state);
    const qreal tolerance = 16 * std::numeric_limits<qreal>::epsilon();
    const qreal firstTick = std::ceil(lower / state.step - tolerance);
    const qreal lastTick  = std::floor(upper / state.step + tolerance);
    qreal       best      = length;
    if (firstTick <= lastTick) {
        const qreal tick = std::clamp(std::round(requested / state.step), firstTick, lastTick);
        best             = std::clamp(tick * state.step, lower, upper);
    }
    if (upper == length && std::abs(length - requested) < std::abs(best - requested)) best = length;
    return best;
}

void RangeSlider::normalize(State& state) {
    const qreal length = span(state);
    state.effective    = std::min(state.minimum, length);
    if (state.step > 0 && state.effective > 0 && std::isfinite(length / state.step)) {
        const qreal steps =
            std::ceil(state.effective / state.step - 16 * std::numeric_limits<qreal>::epsilon());
        state.effective = std::min(length, std::max(qreal(1), steps) * state.step);
    }
    qreal a = distance(state, state.values[0]);
    qreal b = distance(state, state.values[1]);
    if (a > b) std::swap(a, b);
    a                  = nearest(state, a, 0, length - state.effective);
    b                  = nearest(state, b, a + state.effective, length);
    state.values[0]    = value(state, a);
    state.values[1]    = value(state, b);
    state.positions[0] = length > 0 ? a / length : 0;
    state.positions[1] = length > 0 ? b / length : 0;
}

void RangeSlider::publish(State next) {
    const State old = m_state.value();
    if (old == next) return;
    if (old.from != next.from) m_pending |= 1 << 0;
    if (old.to != next.to) m_pending |= 1 << 1;
    if (old.step != next.step) m_pending |= 1 << 2;
    if (old.minimum != next.minimum) m_pending |= 1 << 3;
    if (old.effective != next.effective) m_pending |= 1 << 4;
    if (old.values[0] != next.values[0]) m_pending |= 1 << 5;
    if (old.values[1] != next.values[1]) m_pending |= 1 << 6;
    if (old.positions[0] != next.positions[0]) m_pending |= 1 << 7;
    if (old.positions[1] != next.positions[1]) m_pending |= 1 << 8;
    if (old.orientation != next.orientation) m_pending |= 1 << 9;
    if (old.live != next.live) m_pending |= 1 << 10;
    if (old.snap != next.snap) m_pending |= 1 << 11;
    if (old.wheel != next.wheel) m_pending |= 1 << 12;
    if (old.active != next.active) m_pending |= 1 << 13;
    if (old.focused != next.focused) m_pending |= (1 << 14) | (1 << 17) | (1 << 18);
    if ((old.active == 0) != (next.active == 0)) m_pending |= 1 << 15;
    if ((old.active == 1) != (next.active == 1)) m_pending |= 1 << 16;
    if ((old.active >= 0) != (next.active >= 0)) m_pending |= 1 << 19;
    const bool notifying = m_notifying;
    m_notifying          = true;
    QPointer<RangeSlider> guard(this);
    // All getters observe the same snapshot, including inside binding reevaluation.
    m_state = next;
    if (! guard || notifying) return;
    while (m_pending) {
        unsigned bit = 0;
        while (! (m_pending & (1 << bit))) ++bit;
        m_pending &= ~(1 << bit);
        switch (bit) {
        case 0: Q_EMIT fromChanged(); break;
        case 1: Q_EMIT toChanged(); break;
        case 2: Q_EMIT stepSizeChanged(); break;
        case 3: Q_EMIT minimumRangeChanged(); break;
        case 4: Q_EMIT effectiveMinimumRangeChanged(); break;
        case 5: Q_EMIT m_first.valueChanged(); break;
        case 6: Q_EMIT m_second.valueChanged(); break;
        case 7: Q_EMIT m_first.positionChanged(); break;
        case 8: Q_EMIT m_second.positionChanged(); break;
        case 9: Q_EMIT orientationChanged(); break;
        case 10: Q_EMIT liveChanged(); break;
        case 11: Q_EMIT snapModeChanged(); break;
        case 12: Q_EMIT wheelEnabledChanged(); break;
        case 13: Q_EMIT activeHandleChanged(); break;
        case 14: Q_EMIT focusedHandleChanged(); break;
        case 15: Q_EMIT m_first.pressedChanged(); break;
        case 16: Q_EMIT m_second.pressedChanged(); break;
        case 17: Q_EMIT m_first.focusedChanged(); break;
        case 18: Q_EMIT m_second.focusedChanged(); break;
        case 19: Q_EMIT pressedChanged(); break;
        }
        if (! guard) return;
    }
    m_notifying = false;
}

void RangeSlider::setFrom(qreal v) {
    if (! std::isfinite(v) || v == from() || ! std::isfinite(to() - v)) return;
    if (! prepareChange()) return;
    State next = m_state.value();
    if (! std::isfinite(v) || ! std::isfinite(next.to - v)) return;
    next.from = v;
    if (! m_initializing) normalize(next);
    publish(next);
}
void RangeSlider::setTo(qreal v) {
    if (! std::isfinite(v) || v == to() || ! std::isfinite(v - from())) return;
    if (! prepareChange()) return;
    State next = m_state.value();
    if (! std::isfinite(v) || ! std::isfinite(v - next.from)) return;
    next.to = v;
    if (! m_initializing) normalize(next);
    publish(next);
}
void RangeSlider::setStepSize(qreal v) {
    if (! std::isfinite(v) || v < 0) return;
    if (v == stepSize() || ! prepareChange()) return;
    State next = m_state.value();
    next.step  = v;
    if (! m_initializing) normalize(next);
    publish(next);
}
void RangeSlider::setMinimumRange(qreal v) {
    if (! std::isfinite(v) || v < 0) return;
    if (v == minimumRange() || ! prepareChange()) return;
    State next   = m_state.value();
    next.minimum = v;
    if (! m_initializing) normalize(next);
    publish(next);
}
void RangeSlider::setValues(qreal first, qreal second) {
    if (! std::isfinite(first) || ! std::isfinite(second)) return;
    if (! prepareChange()) return;
    State next     = m_state.value();
    next.values[0] = first;
    next.values[1] = second;
    if (! m_initializing) normalize(next);
    publish(next);
}
void RangeSlider::setNodeValue(int index, qreal v) {
    if (! std::isfinite(v)) return;
    if (! prepareChange()) return;
    State next = m_state.value();
    if (m_initializing) {
        next.values[index] = v;
    } else {
        const qreal other     = distance(next, next.values[1 - index]);
        const qreal lower     = index == 0 ? 0 : other + next.effective;
        const qreal upper     = index == 0 ? other - next.effective : span(next);
        const qreal offset    = nearest(next, distance(next, v), lower, upper);
        next.values[index]    = value(next, offset);
        next.positions[index] = span(next) > 0 ? offset / span(next) : 0;
    }
    publish(next);
}
qreal RangeSlider::valueAt(qreal position) const {
    const State state = m_state.value();
    if (! std::isfinite(position)) return state.from;
    return value(
        state,
        nearest(state, std::clamp(position, qreal(0), qreal(1)) * span(state), 0, span(state)));
}
void RangeSlider::classBegin() {
    m_initializing = true;
    Control::classBegin();
}
QList<qreal> RangeSlider::tickPositions(int maximumCount) const {
    const auto  state  = m_state.value();
    const qreal length = span(state);
    if (maximumCount <= 0 || state.step <= 0 || length <= 0) return {};
    const qreal steps = length / state.step;
    if (! std::isfinite(steps) || steps > 9007199254740991.) return {};
    const qreal  rounded = std::round(steps);
    const qreal  last    = std::abs(steps - rounded) <= 16 * std::numeric_limits<qreal>::epsilon() *
                                                            std::max(qreal(1), steps)
                               ? std::max(qreal(1), rounded)
                               : std::ceil(steps);
    const int    count   = int(std::min(qreal(maximumCount), last + 1));
    QList<qreal> positions;
    positions.reserve(count);
    for (int i = 0; i < count; ++i) {
        const qreal index = count > 1 ? std::round(i * last / (count - 1)) : 0;
        positions.append(index == last ? 1 : index * state.step / length);
    }
    return positions;
}
void RangeSlider::componentComplete() {
    Control::componentComplete();
    m_initializing = false;
    State next     = m_state.value();
    normalize(next);
    publish(next);
}
} // namespace qml_material
