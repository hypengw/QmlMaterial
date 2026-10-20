#include "qml_material/control/floating_toolbar.hpp"
#include "qml_material/util/qt.hpp"
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace qml_material
{
FloatingToolbar::FloatingToolbar(QQuickItem* parent): Control(parent), m_input(this) {
    m_fling.setParent(this);
    m_input.acceptsInput = [this] {
        return m_dragToHide && m_exit && m_exit->enabled();
    };
    m_input.click().keyboardEnabled = false;
    auto& drag                      = m_input.drag();
    drag.axes                       = Qt::Vertical;
    drag.started                    = [this](QPointF point) {
        m_lastScene                           = mapToScene(point);
        m_dragging                            = true;
        const auto                      owner = m_exit;
        const QPointer<FloatingToolbar> guard(this);
        if (owner) owner->begin();
        if (guard && (! owner || m_exit != owner || ! owner->active())) m_input.cancelActivation();
    };
    drag.moved = [this](QPointF point) {
        movePointer(point);
    };
    drag.finished = [this](QPointF point, bool cancelled) {
        if (! std::exchange(m_dragging, false)) return;
        if (cancelled) {
            if (m_exit) m_exit->reset();
            return;
        }
        const auto                      revision = m_revision;
        const auto                      velocity = axis(m_input.drag().velocity);
        const QPointer<FloatingToolbar> guard(this);
        movePointer(point);
        if (guard && revision == m_revision) releaseExit(velocity);
    };
    m_input.observeChildPointerInput();
    m_fling.setEasingCurve(QEasingCurve::OutQuad);
    connect(&m_fling, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        const auto                      owner = m_exit;
        const QPointer<FloatingToolbar> guard(this);
        if (owner) owner->setOffset(value.toReal());
        if (! guard || ! owner || owner != m_exit) return;
        if (owner->offset() <= 0 || owner->offset() >= owner->distance()) {
            m_fling.stop();
            owner->settle();
        }
    });
    connect(&m_fling, &QVariantAnimation::finished, this, [this] {
        if (m_exit) m_exit->settle();
    });
    connect(&m_fling, &QVariantAnimation::stateChanged, this, &FloatingToolbar::exitFlingChanged);
}
FloatingToolbar::~FloatingToolbar() { utils::disconnectAll(m_connections); }
qreal FloatingToolbar::axis(QPointF vector) const {
    return m_edge == Qt::LeftEdge    ? -vector.x()
           : m_edge == Qt::RightEdge ? vector.x()
           : m_edge == Qt::TopEdge   ? -vector.y()
                                     : vector.y();
}
void FloatingToolbar::movePointer(QPointF point) {
    const auto scene = mapToScene(point);
    const auto delta = scene - std::exchange(m_lastScene, scene);
    const auto local = mapFromScene(delta) - mapFromScene(QPointF());
    if (m_exit) m_exit->setOffset(m_exit->offset() + axis(local));
}
void FloatingToolbar::cancelExit() {
    const QPointer<FloatingToolbar> guard(this);
    m_input.cancelActivation();
    if (! guard) return;
    m_dragging = false;
    m_fling.stop();
    if (guard && m_exit && m_exit->active()) m_exit->reset();
}
void FloatingToolbar::setExitBehavior(FloatingToolbarExit* value) {
    if (m_exit == value && (value || m_connections.isEmpty())) return;
    const auto                          revision = ++m_revision;
    const QPointer<FloatingToolbar>     guard(this);
    const QPointer<FloatingToolbarExit> next(value);
    cancelExit();
    if (! guard || revision != m_revision) return;
    utils::disconnectAll(m_connections);
    m_exit = next;
    if (next) {
        m_connections.append(connect(next, &QObject::destroyed, this, [this] {
            setExitBehavior(nullptr);
        }));
        m_connections.append(connect(next, &FloatingToolbarExit::enabledChanged, this, [this] {
            if (! m_exit || ! m_exit->enabled()) cancelExit();
        }));
        m_connections.append(connect(next, &FloatingToolbarExit::distanceChanged, this, [this] {
            const QPointer<FloatingToolbar> guard(this);
            m_input.cancelActivation();
            if (! guard) return;
            m_fling.stop();
            if (guard && m_exit) m_exit->settle();
        }));
        m_connections.append(connect(next, &FloatingToolbarExit::inputStarted, this, [this] {
            const auto                      revision = m_revision;
            const QPointer<FloatingToolbar> guard(this);
            m_fling.stop();
            if (! guard || revision != m_revision) return;
            Q_EMIT exitInputStarted();
            if (guard && revision == m_revision && m_exit) m_exit->setOffset(m_presented);
        }));
    }
    Q_EMIT exitBehaviorChanged();
}
void FloatingToolbar::setExitEdge(Qt::Edge edge) {
    if (edge != Qt::LeftEdge && edge != Qt::RightEdge && edge != Qt::TopEdge &&
        edge != Qt::BottomEdge)
        return;
    if (m_edge == edge) return;
    m_edge = edge;
    ++m_revision;
    const QPointer<FloatingToolbar> guard(this);
    cancelExit();
    if (! guard) return;
    m_input.drag().axes =
        edge == Qt::LeftEdge || edge == Qt::RightEdge ? Qt::Horizontal : Qt::Vertical;
    Q_EMIT exitConfigurationChanged();
}
void FloatingToolbar::setDragToHide(bool value) {
    if (m_dragToHide == value) return;
    m_dragToHide = value;
    const QPointer<FloatingToolbar> guard(this);
    if (! value) cancelExit();
    if (guard) Q_EMIT exitConfigurationChanged();
}
void FloatingToolbar::setAnimationsEnabled(bool value) {
    if (m_animationsEnabled == value) return;
    m_animationsEnabled = value;
    const QPointer<FloatingToolbar> guard(this);
    if (! value && exitFlingRunning()) {
        m_fling.stop();
        if (guard && m_exit) m_exit->settle();
    }
    if (guard) Q_EMIT animationsEnabledChanged();
}
void FloatingToolbar::setExitFlickDeceleration(qreal value) {
    if (! std::isfinite(value) || value <= 0 || value == m_deceleration) return;
    m_deceleration = value;
    Q_EMIT exitConfigurationChanged();
}
void FloatingToolbar::setExitMaximumFlickVelocity(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_maximumVelocity) return;
    m_maximumVelocity = value;
    Q_EMIT exitConfigurationChanged();
}
void FloatingToolbar::setPresentedExitOffset(qreal value) {
    if (! std::isfinite(value) || value == m_presented) return;
    m_presented = value;
    Q_EMIT presentedExitOffsetChanged();
}
void FloatingToolbar::releaseExit(qreal velocity) {
    if (! m_exit) return;
    const auto speed =
        std::isfinite(velocity) ? std::clamp(velocity, -m_maximumVelocity, m_maximumVelocity) : 0;
    if (! m_animationsEnabled || std::abs(speed) <= 1 || m_exit->offset() <= 0 ||
        m_exit->offset() >= m_exit->distance()) {
        m_exit->settle();
        return;
    }
    {
        const QSignalBlocker block(&m_fling);
        m_fling.setStartValue(m_exit->offset());
        const auto distance = speed * (std::abs(speed) / (2 * m_deceleration));
        const auto end      = m_exit->offset() + distance;
        m_fling.setEndValue(
            std::isfinite(end) ? end : std::copysign(std::numeric_limits<qreal>::max(), speed));
        m_fling.setDuration(int(std::clamp(std::round(1000 * std::abs(speed) / m_deceleration),
                                           qreal(1),
                                           qreal(std::numeric_limits<int>::max()))));
    }
    m_fling.start();
}
} // namespace qml_material
