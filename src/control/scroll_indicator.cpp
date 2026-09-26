#include "qml_material/control/scroll_indicator.hpp"
#include "qml_material/util/qt.hpp"
#include "scroll_viewport.hpp"
#include <QQmlInfo>
#include <algorithm>
#include <cmath>

namespace qml_material
{

ScrollIndicator::ScrollIndicator(QQuickItem* parent): Control(parent) {
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptTouchEvents(false);
}

ScrollIndicatorAttached* ScrollIndicator::qmlAttachedProperties(QObject* object) {
    return new ScrollIndicatorAttached(object);
}

QPointF ScrollIndicator::visualArea() const {
    const auto extent = std::max(m_size, m_minimum_size);
    const auto offset =
        m_minimum_size > m_size ? m_position * (1 - m_minimum_size) / (1 - m_size) : m_position;
    const auto length =
        std::max(m_minimum_size,
                 std::min(extent + std::min<qreal>(0, offset), std::max<qreal>(0, 1 - offset)));
    return { std::clamp(offset, qreal(0), 1 - length), length };
}

qreal ScrollIndicator::visualSize() const { return visualArea().y(); }
qreal ScrollIndicator::visualPosition() const { return visualArea().x(); }

void ScrollIndicator::updateVisualArea(const QPointF& old) {
    QPointer<ScrollIndicator> guard(this);
    if (isComponentComplete()) layoutContentItem();
    if (! guard) return;
    const auto area = visualArea();
    if (! qFuzzyCompare(old.y(), area.y())) Q_EMIT visualSizeChanged();
    if (! guard) return;
    if (! qFuzzyCompare(old.x(), area.x())) Q_EMIT visualPositionChanged();
}

void ScrollIndicator::setSize(qreal value) {
    if (! std::isfinite(value)) return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (qFuzzyCompare(m_size, value)) return;
    const auto old   = visualArea();
    m_size           = value;
    const bool moved = m_position + m_size > 1;
    if (moved) m_position = 1 - m_size;
    QPointer<ScrollIndicator> guard(this);
    updateVisualArea(old);
    if (! guard) return;
    Q_EMIT sizeChanged();
    if (guard && moved) Q_EMIT positionChanged();
}

void ScrollIndicator::setPosition(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_position, value)) return;
    const auto old = visualArea();
    m_position     = value;
    QPointer<ScrollIndicator> guard(this);
    updateVisualArea(old);
    if (! guard) return;
    Q_EMIT positionChanged();
}

void ScrollIndicator::setMinimumSize(qreal value) {
    if (! std::isfinite(value)) return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (qFuzzyCompare(m_minimum_size, value)) return;
    const auto old = visualArea();
    m_minimum_size = value;
    QPointer<ScrollIndicator> guard(this);
    updateVisualArea(old);
    if (! guard) return;
    Q_EMIT minimumSizeChanged();
}

void ScrollIndicator::setActive(bool value) {
    if (m_active == value) return;
    m_active = value;
    Q_EMIT activeChanged();
}

void ScrollIndicator::setOrientation(Qt::Orientation value) {
    if (value != Qt::Horizontal && value != Qt::Vertical) return;
    if (m_orientation == value) return;
    m_orientation = value;
    if (isComponentComplete()) layoutContentItem();
    Q_EMIT orientationChanged();
}

QRectF ScrollIndicator::contentRect() const {
    const auto area = visualArea();
    if (horizontal())
        return { leftPadding() + area.x() * availableWidth(),
                 topPadding(),
                 area.y() * availableWidth(),
                 availableHeight() };
    return { leftPadding(),
             topPadding() + area.x() * availableHeight(),
             availableWidth(),
             area.y() * availableHeight() };
}

ScrollIndicatorAttached::ScrollIndicatorAttached(QObject* parent)
    : ScrollIndicatorAttached(parent, parent, false) {}

ScrollIndicatorAttached::ScrollIndicatorAttached(QObject* target, QObject* parent,
                                                 bool bidirectional)
    : QObject(parent),
      m_viewport(std::make_unique<ScrollViewport>(target)),
      m_flickable(m_viewport->item()),
      m_bidirectional(bidirectional) {
    if (! m_flickable) {
        qmlWarning(target) << "Scroll indicator/bar must be attached to a Flickable";
        return;
    }
    connect(m_flickable, &QQuickItem::widthChanged, this, [this]() {
        layout(m_horizontal, true);
        layout(m_vertical, false);
    });
    connect(m_flickable, &QQuickItem::heightChanged, this, [this]() {
        layout(m_horizontal, true);
        layout(m_vertical, false);
    });
    m_viewport->observeMoving(this, [this](bool horizontal) {
        activate(horizontal ? m_horizontal : m_vertical, horizontal);
    });
}

ScrollIndicatorAttached::~ScrollIndicatorAttached() {
    detach(m_horizontal);
    detach(m_vertical);
}

void ScrollIndicatorAttached::detach(Axis& axis) {
    utils::disconnectAll(axis.connections);
    auto item = axis.item;
    axis.item = nullptr;
    if (item) {
        item->m_attachment = nullptr;
        item->disconnectFlickable();
    }
}

void ScrollIndicatorAttached::activate(Axis& axis, bool horizontal) {
    if (axis.item && m_flickable) axis.item->setMoving(m_viewport->moving(horizontal));
}

void ScrollIndicatorAttached::syncHorizontal() { sync(m_horizontal, true); }
void ScrollIndicatorAttached::syncVertical() { sync(m_vertical, false); }

void ScrollIndicatorAttached::sync(Axis& axis, bool horizontal) {
    QPointer<ScrollIndicatorAttached> guard(this);
    if (! axis.item || ! m_flickable) return;
    if (axis.syncing) {
        axis.syncPending = true;
        return;
    }
    axis.syncing = true;
    do {
        axis.syncPending    = false;
        auto       item     = axis.item;
        auto       area     = m_viewport->visibleArea();
        const auto size     = area->property(horizontal ? "widthRatio" : "heightRatio").toReal();
        const auto position = area->property(horizontal ? "xPosition" : "yPosition").toReal();
        item->setSize(size);
        if (! guard) return;
        if (item && axis.item == item && ! axis.syncPending) item->setPosition(position);
        if (! guard) return;
    } while (axis.syncPending && axis.item && m_flickable);
    axis.syncing = false;
}

void ScrollIndicatorAttached::scroll(Axis& axis, bool horizontal) {
    QPointer<ScrollIndicatorAttached> guard(this);
    if (! axis.item || ! m_flickable || axis.syncing) return;
    auto       area     = m_viewport->visibleArea();
    const auto ratio    = area->property(horizontal ? "widthRatio" : "heightRatio").toReal();
    const auto position = area->property(horizontal ? "xPosition" : "yPosition").toReal();
    const auto viewport = horizontal ? m_flickable->width() : m_flickable->height();
    if (ratio <= 0 || viewport <= 0) return;
    const auto content = m_viewport->offset(horizontal);
    const auto aligned = m_viewport->pixelAligned() ? std::round(content) : content;
    const auto target  = aligned + (axis.item->position() - position) * viewport / ratio;
    if (! std::isfinite(target) || qFuzzyCompare(target, content)) return;
    axis.syncing = true;
    m_viewport->setOffset(horizontal, target);
    if (! guard) return;
    axis.syncing = false;
    sync(axis, horizontal);
}

void ScrollIndicatorAttached::layout(Axis& axis, bool horizontal, bool force) {
    QPointer<ScrollIndicatorAttached> guard(this);
    auto                              item = axis.item;
    if (! item || ! m_flickable || axis.layingOut || item->parentItem() != m_flickable) return;
    axis.layingOut      = true;
    const auto position = horizontal ? item->y() : item->x();
    const bool move     = force || qFuzzyIsNull(position) || qFuzzyCompare(position, axis.edge);
    if (horizontal)
        item->setWidth(m_flickable->width());
    else
        item->setHeight(m_flickable->height());
    if (! guard) return;
    if (item && axis.item == item && m_flickable) {
        axis.edge = horizontal         ? m_flickable->height() - item->height()
                    : item->mirrored() ? 0
                                       : m_flickable->width() - item->width();
        if (move) {
            if (horizontal)
                item->setY(axis.edge);
            else
                item->setX(axis.edge);
        }
    }
    if (guard) axis.layingOut = false;
}

void ScrollIndicatorAttached::attach(Axis& axis, ScrollIndicator* indicator, bool horizontal) {
    QPointer<ScrollIndicatorAttached> guard(this);
    if (axis.item == indicator || ! m_flickable) return;
    QPointer<ScrollIndicator> item = indicator;
    if (item && item->m_attachment) {
        auto       owner    = item->m_attachment;
        const bool reparent = owner != this && item->parentItem() == owner->m_flickable;
        if (owner->horizontal() == item)
            owner->setHorizontal(nullptr);
        else
            owner->setVertical(nullptr);
        if (! guard) return;
        if (item && reparent) item->setParentItem(nullptr);
        if (! guard) return;
    }
    detach(axis);
    if (! guard) return;
    axis.item = item;
    axis.edge = 0;
    if (item) {
        item->m_attachment = this;
        // visibleArea owns margin/origin/overshoot normalization; do not reconstruct it here.
        auto area = m_viewport->visibleArea();
        if (horizontal) {
            axis.connections.append(
                connect(area, SIGNAL(widthRatioChanged(qreal)), this, SLOT(syncHorizontal())));
            axis.connections.append(
                connect(area, SIGNAL(xPositionChanged(qreal)), this, SLOT(syncHorizontal())));
        } else {
            axis.connections.append(
                connect(area, SIGNAL(heightRatioChanged(qreal)), this, SLOT(syncVertical())));
            axis.connections.append(
                connect(area, SIGNAL(yPositionChanged(qreal)), this, SLOT(syncVertical())));
        }
        if (m_bidirectional)
            axis.connections.append(
                connect(item, &ScrollIndicator::positionChanged, this, [this, &axis, horizontal]() {
                    scroll(axis, horizontal);
                }));
        axis.connections.append(
            connect(item,
                    horizontal ? &QQuickItem::heightChanged : &QQuickItem::widthChanged,
                    this,
                    [this, &axis, horizontal]() {
                        layout(axis, horizontal);
                    }));
        axis.connections.append(
            connect(item, &QQuickItem::parentChanged, this, [this, &axis, horizontal]() {
                layout(axis, horizontal, true);
            }));
        axis.connections.append(
            connect(item, &Control::mirroredChanged, this, [this, &axis, horizontal]() {
                layout(axis, horizontal);
            }));
        axis.connections.append(
            connect(item, &QObject::destroyed, this, [this, &axis, horizontal]() {
                detach(axis);
                if (horizontal)
                    Q_EMIT horizontalChanged();
                else
                    Q_EMIT verticalChanged();
            }));
        const auto current = [&]() {
            return guard && item && axis.item == item && item->m_attachment == this;
        };
        if (! item->parentItem()) item->setParentItem(m_flickable);
        if (! current()) return;
        item->setOrientation(horizontal ? Qt::Horizontal : Qt::Vertical);
        if (! current()) return;
        sync(axis, horizontal);
        if (! current()) return;
        activate(axis, horizontal);
        if (! current()) return;
        layout(axis, horizontal, true);
        if (! current()) return;
    }
    if (horizontal)
        Q_EMIT horizontalChanged();
    else
        Q_EMIT verticalChanged();
}

void ScrollIndicatorAttached::setHorizontal(ScrollIndicator* item) {
    attach(m_horizontal, item, true);
}
void ScrollIndicatorAttached::setVertical(ScrollIndicator* item) {
    attach(m_vertical, item, false);
}

} // namespace qml_material
