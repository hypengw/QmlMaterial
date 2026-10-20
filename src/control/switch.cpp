#include "qml_material/control/switch.hpp"
#include "qml_material/util/qt.hpp"
#include <algorithm>

namespace qml_material
{
Switch::Switch(QQuickItem* parent): ToggleButton(parent) {
    auto& drag    = inputHost().drag();
    drag.axes     = Qt::Horizontal;
    drag.canStart = [this](QPointF origin, QPointF point) {
        const auto track = indicator();
        if (! track || track->width() <= 0) return false;
        const auto inside = [this, track](QPointF point) {
            const auto x = track->mapFromItem(this, point).x();
            return x >= 0 && x <= track->width();
        };
        return inside(origin) || inside(point);
    };
    drag.started = [this](QPointF) {
        m_dragPosition = position();
        ++m_dragRevision;
        Q_EMIT draggingChanged();
    };
    drag.moved = [this](QPointF point) {
        setDragPosition(positionAt(point));
    };
    drag.finished = [this](QPointF point, bool cancelled) {
        finishDrag(point, cancelled);
    };
    connect(this, &ButtonPresentation::indicatorChanged, this, &Switch::observeIndicator);
    connect(this, &Control::mirroredChanged, this, [this] {
        const QPointer<Switch> guard(this);
        cancelActivation();
        if (guard) Q_EMIT visualPositionChanged();
    });
}
Switch::~Switch() { utils::disconnectAll(m_indicatorConnections); }
void Switch::observeIndicator() {
    const QPointer<Switch> guard(this);
    cancelActivation();
    if (! guard) return;
    utils::disconnectAll(m_indicatorConnections);
    if (auto* track = indicator()) {
        const auto cancel = [this] {
            cancelActivation();
        };
        m_indicatorConnections.append(connect(track, &QQuickItem::widthChanged, this, cancel));
        m_indicatorConnections.append(connect(track, &QQuickItem::heightChanged, this, cancel));
        m_indicatorConnections.append(connect(track, &QQuickItem::xChanged, this, cancel));
    }
}
qreal Switch::positionAt(QPointF point) const {
    const auto track = indicator();
    if (! track || track->width() <= 0) return position();
    const auto physical = track->mapFromItem(this, point).x() / track->width();
    return std::clamp(mirrored() ? 1 - physical : physical, qreal(0), qreal(1));
}
void Switch::notifyPosition() {
    const QPointer<Switch> guard(this);
    Q_EMIT positionChanged();
    if (guard) Q_EMIT visualPositionChanged();
}
void Switch::setDragPosition(qreal value) {
    if (! m_dragPosition || qFuzzyCompare(*m_dragPosition, value)) return;
    m_dragPosition = value;
    notifyPosition();
}
void Switch::finishDrag(QPointF point, bool cancelled) {
    if (! m_dragPosition) return;
    const auto             revision      = m_dragRevision;
    const auto             inputRevision = inputHost().activationRevision();
    const QPointer<Switch> guard(this);
    if (! cancelled) {
        setDragPosition(positionAt(point));
        if (! guard || revision != m_dragRevision) return;
        if (inputRevision == inputHost().activationRevision()) requestChecked(position() > .5);
        if (! guard || revision != m_dragRevision) return;
    }
    const auto previous = position();
    m_dragPosition.reset();
    const auto finished = ++m_dragRevision;
    Q_EMIT draggingChanged();
    if (guard && finished == m_dragRevision && ! qFuzzyCompare(previous, position()))
        notifyPosition();
}
void Switch::checkedChange() {
    QPointer<Switch> guard(this);
    ToggleButton::checkedChange();
    if (guard && ! isDragging()) notifyPosition();
}
} // namespace qml_material
