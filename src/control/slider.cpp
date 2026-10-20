#include "qml_material/control/slider.hpp"
#include "qml_material/input/slider_key.hpp"
#include "qml_material/util/qt.hpp"
#include "input/control_accessibility_p.hpp"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QFocusEvent>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QTouchEvent>
#include <algorithm>
#include <cmath>

namespace qml_material
{

Slider::Slider(QQuickItem* parent)
    : Control(parent), m_input(this, [this] {
          Q_EMIT interactionSourceChanged();
      }) {
    connect(this, &Slider::positionChanged, this, &Slider::previewValueChanged);
    connect(this, &Slider::valueChanged, this, &Slider::previewValueChanged);
    connect(this, &Slider::valueChanged, this, [this] {
        detail::notifyAccessibleValue(this, value());
    });
    connect(this, &Slider::liveChanged, this, &Slider::previewValueChanged);
    m_input.setHoverEnabled(hoverEnabled());
    connect(this, &Control::hoverEnabledChanged, this, [this] {
        m_input.setHoverEnabled(hoverEnabled());
    });
    m_input.focusVisibleChanged = [this] {
        Q_EMIT focusVisibleChanged();
    };
    m_input.click().keyboardEnabled = false;
    m_input.acceptsInput            = [this] {
        return m_keys.isEmpty();
    };
    m_input.click().pointerStarted = [this](const PointerInput& input) {
        ++m_generation;
        m_pointerActive = true;
        m_point         = input.position;
    };
    m_input.click().pointerClicked = [this](QPointF point) {
        m_point = point;
        finishPointer(false);
    };
    m_input.click().pointerCancelled = [this] {
        finishPointer(true);
    };
    auto& drag     = m_input.drag();
    drag.axes      = m_orientation;
    drag.threshold = m_touch_threshold;
    drag.started   = [this](QPointF) {
        m_dragging = true;
        Q_EMIT previewValueChanged();
    };
    drag.moved = [this](QPointF point) {
        m_point = point;
        moveTo(point, false);
    };
    drag.finished = [this](QPointF point, bool cancelled) {
        m_point = point;
        finishPointer(cancelled);
    };
#ifdef Q_OS_MACOS
    setFocusPolicy(Qt::TabFocus);
#else
    setFocusPolicy(Qt::StrongFocus);
#endif
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    connect(this, &Control::mirroredChanged, this, &Slider::visualPositionChanged);
    observeWindow();
}
Slider::~Slider() {
    utils::disconnectAll(m_handle_connections);
    disconnect(m_window_connection);
}

void Slider::setFrom(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_from, value)) return;
    if (! prepareChange()) return;
    m_from = value;
    QPointer<Slider> guard(this);
    Q_EMIT fromChanged();
    if (guard && isComponentComplete()) {
        setValue(m_value);
        if (guard) updatePosition();
    }
}
void Slider::setTo(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_to, value)) return;
    if (! prepareChange()) return;
    m_to = value;
    QPointer<Slider> guard(this);
    Q_EMIT toChanged();
    if (guard && isComponentComplete()) {
        setValue(m_value);
        if (guard) updatePosition();
    }
}
void Slider::setValue(qreal value) {
    if (! std::isfinite(value)) return;
    if (isComponentComplete())
        value = std::clamp(value, std::min(m_from, m_to), std::max(m_from, m_to));
    if (qFuzzyIsNull(value)) value = 0;
    if (qFuzzyCompare(m_value, value)) return;
    m_value = value;
    QPointer<Slider> guard(this);
    updatePosition();
    if (guard) Q_EMIT valueChanged();
}
void Slider::setPosition(qreal position) {
    position = std::clamp(position, qreal(0), qreal(1));
    if (qFuzzyCompare(m_position, position)) return;
    m_position = position;
    QPointer<Slider> guard(this);
    Q_EMIT positionChanged();
    if (guard) Q_EMIT visualPositionChanged();
}
void Slider::updatePosition() {
    setPosition(qFuzzyCompare(m_from, m_to) ? 0 : (m_value - m_from) / (m_to - m_from));
}
qreal Slider::previewValue() const {
    if (m_live || ! m_dragging) return m_value;
    return std::clamp(valueAt(m_position), std::min(m_from, m_to), std::max(m_from, m_to));
}
void Slider::setStepSize(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_step, value)) return;
    if (! prepareChange()) return;
    m_step = value;
    Q_EMIT stepSizeChanged();
}
void Slider::setSnapMode(SnapMode value) {
    if (m_snap == value) return;
    if (! prepareChange()) return;
    m_snap = value;
    Q_EMIT snapModeChanged();
}
void Slider::setLive(bool value) {
    if (m_live == value) return;
    if (! prepareChange()) return;
    m_live = value;
    Q_EMIT liveChanged();
}
void Slider::setWheelEnabled(bool value) {
    if (m_wheel_enabled == value) return;
    m_wheel_enabled = value;
    Q_EMIT wheelEnabledChanged();
}
void Slider::setOrientation(Qt::Orientation value) {
    if (m_orientation == value) return;
    if (! prepareChange()) return;
    m_orientation       = value;
    m_input.drag().axes = value;
    QPointer<Slider> guard(this);
    Q_EMIT orientationChanged();
    if (guard) Q_EMIT visualPositionChanged();
}
qreal Slider::implicitHandleWidth() const { return m_handle ? m_handle->implicitWidth() : 0; }
qreal Slider::implicitHandleHeight() const { return m_handle ? m_handle->implicitHeight() : 0; }
void  Slider::setHandle(QQuickItem* item) {
    if (m_handle == item) return;
    const auto           revision = ++m_handleRevision;
    QPointer<Slider>     guard(this);
    QPointer<QQuickItem> requested(item);
    if (! prepareChange() || revision != m_handleRevision) return;
    const auto oldWidth = implicitHandleWidth(), oldHeight = implicitHandleHeight();
    utils::disconnectAll(m_handle_connections);
    auto old = m_handle;
    m_handle = requested;
    if (old && old->parentItem() == this) old->setParentItem(nullptr);
    if (! guard || revision != m_handleRevision) return;
    if (requested) {
        requested->setParentItem(this);
        if (! guard || revision != m_handleRevision || ! requested) return;
        m_handle_connections.append(connect(requested,
                                            &QQuickItem::implicitWidthChanged,
                                            this,
                                            &Slider::implicitHandleWidthChanged));
        m_handle_connections.append(connect(requested,
                                            &QQuickItem::implicitHeightChanged,
                                            this,
                                            &Slider::implicitHandleHeightChanged));
        m_handle_connections.append(connect(requested, &QObject::destroyed, this, [this] {
            m_handle                  = nullptr;
            const auto       revision = ++m_handleRevision;
            QPointer<Slider> guard(this);
            if (! prepareChange() || revision != m_handleRevision) return;
            Q_EMIT handleChanged();
            if (! guard || revision != m_handleRevision) return;
            Q_EMIT implicitHandleWidthChanged();
            if (guard && revision == m_handleRevision) Q_EMIT implicitHandleHeightChanged();
        }));
    }
    if (! qFuzzyCompare(oldWidth, implicitHandleWidth())) Q_EMIT implicitHandleWidthChanged();
    if (! guard || revision != m_handleRevision) return;
    if (! qFuzzyCompare(oldHeight, implicitHandleHeight())) Q_EMIT implicitHandleHeightChanged();
    if (guard && revision == m_handleRevision) Q_EMIT handleChanged();
}
void Slider::setTouchDragThreshold(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_touch_threshold, value)) return;
    if (! prepareChange()) return;
    m_touch_threshold        = value;
    m_input.drag().threshold = value;
    Q_EMIT touchDragThresholdChanged();
}
void  Slider::resetTouchDragThreshold() { setTouchDragThreshold(-1); }
qreal Slider::valueAt(qreal position) const {
    if (! std::isfinite(position)) return m_from;
    const qreal offset = (m_to - m_from) * position;
    return m_from + (qFuzzyIsNull(m_step) ? offset : std::round(offset / m_step) * m_step);
}
bool Slider::requestChange(qreal value) {
    if (! isEnabled() || ! isVisible() || ! std::isfinite(value)) return false;
    value = std::clamp(value, std::min(m_from, m_to), std::max(m_from, m_to));
    if (qFuzzyIsNull(value)) value = 0;
    if (qFuzzyCompare(value, m_value)) return true;
    const auto       sequence = m_generation;
    QPointer<Slider> guard(this);
    Q_EMIT valueChangeRequested(value);
    return guard && sequence == m_generation;
}
void  Slider::requestValue(qreal value) { requestChange(value); }
void  Slider::increase() { requestValue(m_value + (qFuzzyIsNull(m_step) ? 0.1 : m_step)); }
void  Slider::decrease() { requestValue(m_value - (qFuzzyIsNull(m_step) ? 0.1 : m_step)); }
qreal Slider::snapPosition(qreal position) const {
    if (qFuzzyCompare(m_from, m_to) || qFuzzyIsNull(m_step)) return position;
    return std::clamp((valueAt(position) - m_from) / (m_to - m_from), qreal(0), qreal(1));
}
qreal Slider::positionAt(const QPointF& point) const {
    const qreal size   = m_handle ? (horizontal() ? m_handle->width() : m_handle->height()) : 0;
    const qreal extent = (horizontal() ? availableWidth() : availableHeight()) - size;
    if (extent <= 0) return 0;
    const qreal distance = horizontal() ? (mirrored() ? width() - rightPadding() - point.x()
                                                      : point.x() - leftPadding())
                                        : height() - bottomPadding() - point.y();
    return std::clamp((distance - size / 2) / extent, qreal(0), qreal(1));
}
void Slider::moveTo(const QPointF& point, bool release) {
    const auto       sequence = m_generation;
    QPointer<Slider> guard(this);
    qreal            position = positionAt(point);
    if (m_snap == SnapAlways || (release && m_snap == SnapOnRelease))
        position = snapPosition(position);
    if (m_live || release) {
        if (! requestChange(valueAt(position))) return;
    } else {
        setPosition(position);
    }
    if (! guard || sequence != m_generation) return;
    if (! release) return;
    updatePosition();
    if (! guard || sequence != m_generation) return;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
}
void Slider::finishPointer(bool cancelled) {
    if (! m_pointerActive) return;
    const auto       generation = m_generation;
    QPointer<Slider> guard(this);
    if (! cancelled) moveTo(m_point, true);
    if (! guard || generation != m_generation) return;
    m_pointerActive = m_dragging = false;
    ++m_generation;
    updatePosition();
    if (! guard) return;
    Q_EMIT previewValueChanged();
    if (guard) Q_EMIT interactionFinished(cancelled);
}
bool Slider::prepareChange() {
    const auto       generation = m_generation + (m_pointerActive || ! m_keys.isEmpty() ? 1 : 0);
    QPointer<Slider> guard(this);
    cancel();
    return guard && m_generation == generation;
}
void Slider::cancel() {
    QPointer<Slider> guard(this);
    const auto       generation = m_generation;
    m_input.cancel();
    if (! guard || m_generation != generation) return;
    if (m_pointerActive) finishPointer(true);
    if (! guard || m_keys.isEmpty()) return;
    m_keys.clear();
    ++m_generation;
    updatePosition();
    if (guard) Q_EMIT interactionFinished(true);
}
bool Slider::event(QEvent* event) {
    const QPointer<Slider> guard(this);
    const bool             handled = m_input.observeEvent(event);
    if (! guard || handled) return true;
    return Control::event(event);
}
void Slider::mousePressEvent(QMouseEvent* event) { m_input.event(event); }
void Slider::mouseMoveEvent(QMouseEvent* event) { m_input.event(event); }
void Slider::mouseReleaseEvent(QMouseEvent* event) { m_input.event(event); }
void Slider::mouseDoubleClickEvent(QMouseEvent* event) { m_input.event(event); }
void Slider::touchEvent(QTouchEvent* event) { m_input.event(event); }
void Slider::mouseUngrabEvent() { m_input.mouseUngrabbed(); }
void Slider::touchUngrabEvent() { m_input.touchUngrabbed(); }
void Slider::focusInEvent(QFocusEvent* event) { QQuickItem::focusInEvent(event); }
void Slider::hoverEnterEvent(QHoverEvent* event) { QQuickItem::hoverEnterEvent(event); }
void Slider::hoverMoveEvent(QHoverEvent* event) { QQuickItem::hoverMoveEvent(event); }
void Slider::hoverLeaveEvent(QHoverEvent* event) { QQuickItem::hoverLeaveEvent(event); }
void Slider::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && (m_pointerActive || ! m_keys.isEmpty())) {
        event->accept();
        cancel();
        return;
    }
    if (! isEnabled() || m_pointerActive) {
        event->ignore();
        return;
    }
    const qreal length = std::abs(m_to - m_from);
    const auto  next   = sliderKeyPosition(event->key(),
                                           m_position,
                                           length > 0 ? std::abs(m_step) / length : 0,
                                           m_orientation,
                                           vertical() || mirrored());
    if (! next) {
        event->ignore();
        return;
    }
    event->accept();
    m_keys.insert(event->key());
    ++m_generation;
    requestValue(valueAt(*next));
}
void Slider::keyReleaseEvent(QKeyEvent* event) {
    const bool accepted = m_keys.contains(event->key());
    event->setAccepted(accepted);
    if (! accepted || event->isAutoRepeat()) return;
    m_keys.remove(event->key());
    if (m_keys.isEmpty()) {
        ++m_generation;
    }
    if (m_keys.isEmpty()) Q_EMIT interactionFinished(false);
}
void Slider::wheelEvent(QWheelEvent* event) {
    if (! m_wheel_enabled || ! isEnabled() || ! isVisible() ||
        (m_pointerActive || ! m_keys.isEmpty())) {
        event->ignore();
        return;
    }
    const auto  angle = event->angleDelta();
    const qreal delta =
        (std::abs(angle.y()) < std::abs(angle.x()) ? angle.x()
                                                   : (event->inverted() ? -angle.y() : angle.y())) /
        120.0;
    event->accept();
    QPointer<Slider> guard(this);
    if (requestChange(m_value + delta * (qFuzzyIsNull(m_step) ? 0.1 : m_step)) && guard)
        Q_EMIT interactionFinished(false);
}
void Slider::focusOutEvent(QFocusEvent* event) {
    QPointer<Slider> guard(this);
    QQuickItem::focusOutEvent(event);
    if (guard && ! m_keys.isEmpty()) cancel();
}
void Slider::observeWindow() {
    disconnect(m_window_connection);
    if (auto win = window())
        m_window_connection = connect(win, &QWindow::activeChanged, this, [this, win]() {
            if (! win->isActive()) cancel();
        });
}
void Slider::itemChange(ItemChange change, const ItemChangeData& data) {
    QPointer<Slider> guard(this);
    Control::itemChange(change, data);
    if (! guard) return;
    if (change == ItemSceneChange) observeWindow();
    if (change == ItemSceneChange || change == ItemParentHasChanged ||
        (change == ItemEnabledHasChanged && ! data.boolValue) ||
        (change == ItemVisibleHasChanged && ! data.boolValue))
        cancel();
}
void Slider::componentComplete() {
    QPointer<Slider> guard(this);
    Control::componentComplete();
    if (! guard) return;
    setValue(m_value);
    if (guard) updatePosition();
}

} // namespace qml_material
