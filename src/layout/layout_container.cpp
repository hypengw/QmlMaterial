#include "qml_material/layout/layout_container.hpp"

#include "layout_item_adapter_p.hpp"
#include "qml_material/layout/layout.hpp"

#include <QMetaObject>
#include <QPointer>
#include <QQmlContext>
#include <QQmlInfo>
#include <QQmlProperty>
#include <algorithm>
#include <utility>
#include <vector>

namespace qml_material
{

namespace
{

bool equalReal(qreal first, qreal second) {
    return qFuzzyCompare(first, second) || (qFuzzyIsNull(first) && qFuzzyIsNull(second));
}

qreal horizontalOffset(qreal available, qreal used, Qt::Alignment alignment,
                       Qt::LayoutDirection direction) {
    if (alignment.testFlag(Qt::AlignHCenter)) {
        return (available - used) / 2.0;
    }
    if (alignment.testFlag(Qt::AlignRight)) {
        return available - used;
    }
    if (alignment.testFlag(Qt::AlignLeft)) {
        return 0;
    }
    return direction == Qt::RightToLeft ? available - used : 0;
}

qreal verticalOffset(qreal available, qreal used, Qt::Alignment alignment) {
    if (alignment.testFlag(Qt::AlignVCenter)) {
        return (available - used) / 2.0;
    }
    if (alignment.testFlag(Qt::AlignBottom)) {
        return available - used;
    }
    return 0;
}

struct LayoutEntry {
    QQuickItem*     item     = nullptr;
    LayoutAttached* attached = nullptr;
};

struct ChildWatch {
    QPointer<QQuickItem>                 item;
    QPointer<LayoutAttached>             attached;
    std::vector<QMetaObject::Connection> connections;
    std::vector<QMetaObject::Connection> source_connections;
    QPointer<QQuickItem>                 visibility_source;
    bool                                 invalid_source_warned = false;
};

} // namespace

class LayoutMirroringObserver final : public QObject {
    Q_OBJECT

public:
    std::function<void()> callback;

private Q_SLOTS:
    void changed() {
        if (callback) {
            callback();
        }
    }
};

class LayoutContainerPrivate {
public:
    LayoutContainerPrivate(LayoutContainer* owner, LayoutContainer::Mode layoutMode)
        : q(owner), mode(layoutMode), adapter([this](QQuickItem*) {
              markGeometryDirty();
          }) {
        mirroring_observer.callback = [this]() {
            updateEffectiveDirection();
        };
    }

    ~LayoutContainerPrivate() {
        while (! watches.empty()) {
            removeWatch(watches.back().item, true);
        }
    }

    void schedulePolish() {
        if (q->isComponentComplete()) {
            q->polish();
        }
    }

    void markHintDirty() {
        hint_dirty = true;
        schedulePolish();
    }

    void markGeometryDirty() {
        geometry_dirty = true;
        schedulePolish();
    }

    void markAllDirty() {
        hint_dirty     = true;
        geometry_dirty = true;
        schedulePolish();
    }

    auto findWatch(QQuickItem* item) {
        return std::find_if(watches.begin(), watches.end(), [item](const ChildWatch& watch) {
            return watch.item == item;
        });
    }

    void refreshVisibilitySource(QQuickItem* item) {
        const auto existing = findWatch(item);
        if (existing == watches.end()) {
            return;
        }

        for (const auto& connection : existing->source_connections) {
            QObject::disconnect(connection);
        }
        existing->source_connections.clear();

        QQuickItem* source = nullptr;
        if (existing->attached && existing->attached->isVisibilitySourceSet()) {
            source = existing->attached->visibilitySource();
        }
        if (existing->visibility_source != source) {
            existing->invalid_source_warned = false;
        }
        existing->visibility_source = source;

        if (! source) {
            return;
        }

        existing->source_connections.push_back(
            QObject::connect(source, &QQuickItem::visibleChanged, q, [this]() {
                markAllDirty();
            }));
        existing->source_connections.push_back(
            QObject::connect(source, &QQuickItem::parentChanged, q, [this, item](QQuickItem*) {
                refreshVisibilitySource(item);
                markAllDirty();
            }));

        if (source->parentItem() != item && ! existing->invalid_source_warned) {
            qmlWarning(q)
                << "Layout.visibilitySource must be a direct visual child of its target item";
            existing->invalid_source_warned = true;
        }
    }

    void addWatch(QQuickItem* item) {
        if (! item || findWatch(item) != watches.end()) {
            return;
        }

        ChildWatch watch;
        watch.item = item;
        watch.attached =
            static_cast<LayoutAttached*>(qmlAttachedPropertiesObject<Layout>(item, true));
        watch.connections.push_back(
            QObject::connect(item, &QQuickItem::implicitWidthChanged, q, [this]() {
                markHintDirty();
            }));
        watch.connections.push_back(
            QObject::connect(item, &QQuickItem::implicitHeightChanged, q, [this]() {
                markHintDirty();
            }));
        watch.connections.push_back(QObject::connect(item, &QQuickItem::widthChanged, q, [this]() {
            if (! laying_out) {
                markGeometryDirty();
            }
        }));
        watch.connections.push_back(QObject::connect(item, &QQuickItem::heightChanged, q, [this]() {
            if (! laying_out) {
                markGeometryDirty();
            }
        }));
        watch.connections.push_back(
            QObject::connect(item, &QQuickItem::visibleChanged, q, [this]() {
                markAllDirty();
            }));
        watch.connections.push_back(QObject::connect(item, &QObject::destroyed, q, [this, item]() {
            removeWatch(item, false);
            markAllDirty();
        }));
        if (watch.attached) {
            watch.connections.push_back(
                QObject::connect(watch.attached, &LayoutAttached::fillWidthChanged, q, [this]() {
                    markGeometryDirty();
                }));
            watch.connections.push_back(
                QObject::connect(watch.attached, &LayoutAttached::fillHeightChanged, q, [this]() {
                    markGeometryDirty();
                }));
            watch.connections.push_back(QObject::connect(
                watch.attached, &LayoutAttached::visibilitySourceChanged, q, [this, item]() {
                    refreshVisibilitySource(item);
                    markAllDirty();
                }));
        }

        watches.push_back(std::move(watch));
        adapter.watch(item);
        refreshVisibilitySource(item);
    }

    void removeWatch(QQuickItem* item, bool removePrivateListener) {
        const auto existing = findWatch(item);
        if (existing == watches.end()) {
            return;
        }
        for (const auto& connection : existing->connections) {
            QObject::disconnect(connection);
        }
        for (const auto& connection : existing->source_connections) {
            QObject::disconnect(connection);
        }
        if (removePrivateListener) {
            adapter.unwatch(item);
        }
        watches.erase(existing);
    }

    void syncChildren() {
        const auto children = q->childItems();
        for (auto* child : children) {
            addWatch(child);
        }
        while (true) {
            const auto stale =
                std::find_if(watches.begin(), watches.end(), [this](const auto& watch) {
                    return ! watch.item || watch.item->parentItem() != q;
                });
            if (stale == watches.end()) {
                break;
            }
            QQuickItem* item = stale->item;
            removeWatch(item, item != nullptr);
        }
    }

    std::vector<LayoutEntry> participatingItems() {
        syncChildren();
        std::vector<LayoutEntry> result;
        const auto               children = q->childItems();
        result.reserve(children.size());
        for (auto* child : children) {
            if (! adapter.participates(child)) {
                continue;
            }
            const auto watch = findWatch(child);
            if (watch != watches.end() && watch->attached &&
                watch->attached->isVisibilitySourceSet()) {
                auto* source = watch->attached->visibilitySource();
                if (! source || source->parentItem() != child ||
                    ! adapter.isExplicitlyVisible(source)) {
                    continue;
                }
            }
            result.push_back({ child, watch != watches.end() ? watch->attached.data() : nullptr });
        }
        return result;
    }

    void initializeMirroring() {
        mirroring_property =
            QQmlProperty(q, QStringLiteral("LayoutMirroring.enabled"), qmlContext(q));
        if (! mirroring_property.isValid() || ! mirroring_property.hasNotifySignal() ||
            ! mirroring_property.connectNotifySignal(&mirroring_observer, SLOT(changed()))) {
            qmlWarning(q) << "Qcm.Material.Layouts could not observe LayoutMirroring.enabled";
            mirroring_property = {};
        }
        updateEffectiveDirection();
    }

    void updateEffectiveDirection() {
        const bool mirrored = mirroring_property.isValid() && mirroring_property.read().toBool();
        const auto effective =
            mirrored ? (layout_direction == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft)
                     : layout_direction;
        if (effective_direction == effective) {
            return;
        }
        effective_direction = effective;
        Q_EMIT q->effectiveLayoutDirectionChanged();
        markGeometryDirty();
    }

    void updateHint() {
        const auto items  = participatingItems();
        qreal      width  = left_padding + right_padding;
        qreal      height = top_padding + bottom_padding;

        if (mode == LayoutContainer::Mode::Row) {
            qreal children_width = 0;
            qreal max_height     = 0;
            for (const auto& entry : items) {
                children_width += entry.item->implicitWidth();
                max_height = qMax(max_height, entry.item->implicitHeight());
            }
            width += children_width + spacing * qMax(qsizetype(0), qsizetype(items.size()) - 1);
            height += max_height;
        } else if (mode == LayoutContainer::Mode::Column) {
            qreal max_width       = 0;
            qreal children_height = 0;
            for (const auto& entry : items) {
                max_width = qMax(max_width, entry.item->implicitWidth());
                children_height += entry.item->implicitHeight();
            }
            width += max_width;
            height += children_height + spacing * qMax(qsizetype(0), qsizetype(items.size()) - 1);
        } else {
            qreal max_width  = 0;
            qreal max_height = 0;
            for (const auto& entry : items) {
                max_width  = qMax(max_width, entry.item->implicitWidth());
                max_height = qMax(max_height, entry.item->implicitHeight());
            }
            width += max_width;
            height += max_height;
        }

        q->setImplicitSize(width, height);
    }

    void setItemWidth(QQuickItem* item, qreal width) {
        if (! equalReal(item->width(), width)) {
            item->setWidth(width);
        }
    }

    void setItemHeight(QQuickItem* item, qreal height) {
        if (! equalReal(item->height(), height)) {
            item->setHeight(height);
        }
    }

    void setItemPosition(QQuickItem* item, qreal x, qreal y) {
        if (! equalReal(item->x(), x)) {
            item->setX(x);
        }
        if (! equalReal(item->y(), y)) {
            item->setY(y);
        }
    }

    void updateContentSize(qreal width, qreal height) {
        if (! equalReal(content_width, width)) {
            content_width = width;
            Q_EMIT q->contentWidthChanged();
        }
        if (! equalReal(content_height, height)) {
            content_height = height;
            Q_EMIT q->contentHeightChanged();
        }
    }

    void updateBox(const std::vector<LayoutEntry>& items, qreal availableWidth,
                   qreal availableHeight) {
        for (const auto& entry : items) {
            if (entry.attached && entry.attached->fillWidth()) {
                setItemWidth(entry.item, availableWidth);
            }
            if (entry.attached && entry.attached->fillHeight()) {
                setItemHeight(entry.item, availableHeight);
            }
        }

        qreal max_width  = 0;
        qreal max_height = 0;
        for (const auto& entry : items) {
            max_width     = qMax(max_width, entry.item->width());
            max_height    = qMax(max_height, entry.item->height());
            const qreal x = left_padding + horizontalOffset(availableWidth,
                                                            entry.item->width(),
                                                            alignment,
                                                            effective_direction);
            const qreal y =
                top_padding + verticalOffset(availableHeight, entry.item->height(), alignment);
            setItemPosition(entry.item, x, y);
        }
        updateContentSize(left_padding + right_padding + max_width,
                          top_padding + bottom_padding + max_height);
    }

    void updateRow(const std::vector<LayoutEntry>& items, qreal availableWidth,
                   qreal availableHeight) {
        qsizetype fill_index = -1;
        int       fill_count = 0;
        for (qsizetype index = 0; index < qsizetype(items.size()); ++index) {
            if (items[index].attached && items[index].attached->fillWidth()) {
                if (fill_index < 0) {
                    fill_index = index;
                }
                ++fill_count;
            }
        }

        const qreal gap_size = spacing * qMax(qsizetype(0), qsizetype(items.size()) - 1);
        if (fill_index >= 0) {
            qreal fixed_width = 0;
            for (qsizetype index = 0; index < qsizetype(items.size()); ++index) {
                if (index != fill_index) {
                    fixed_width += items[index].item->width();
                }
            }
            setItemWidth(items[fill_index].item,
                         qMax(qreal(0), availableWidth - gap_size - fixed_width));
        }
        for (const auto& entry : items) {
            if (entry.attached && entry.attached->fillHeight()) {
                setItemHeight(entry.item, availableHeight);
            }
        }

        if (fill_count > 1 && ! main_fill_conflict) {
            qmlWarning(q) << "LiteRow supports only one fillWidth child";
        }
        main_fill_conflict = fill_count > 1;

        qreal used_width = gap_size;
        qreal max_height = 0;
        for (const auto& entry : items) {
            used_width += entry.item->width();
            max_height = qMax(max_height, entry.item->height());
        }

        const qreal group_left =
            left_padding +
            horizontalOffset(availableWidth, used_width, alignment, effective_direction);
        qreal cursor =
            effective_direction == Qt::LeftToRight ? group_left : group_left + used_width;
        for (const auto& entry : items) {
            qreal x = cursor;
            if (effective_direction == Qt::RightToLeft) {
                x      = cursor - entry.item->width();
                cursor = x - spacing;
            } else {
                cursor += entry.item->width() + spacing;
            }
            const qreal y =
                top_padding + verticalOffset(availableHeight, entry.item->height(), alignment);
            setItemPosition(entry.item, x, y);
        }

        updateContentSize(left_padding + right_padding + used_width,
                          top_padding + bottom_padding + max_height);
    }

    void updateColumn(const std::vector<LayoutEntry>& items, qreal availableWidth,
                      qreal availableHeight) {
        qsizetype fill_index = -1;
        int       fill_count = 0;
        for (qsizetype index = 0; index < qsizetype(items.size()); ++index) {
            if (items[index].attached && items[index].attached->fillHeight()) {
                if (fill_index < 0) {
                    fill_index = index;
                }
                ++fill_count;
            }
        }

        const qreal gap_size = spacing * qMax(qsizetype(0), qsizetype(items.size()) - 1);
        if (fill_index >= 0) {
            qreal fixed_height = 0;
            for (qsizetype index = 0; index < qsizetype(items.size()); ++index) {
                if (index != fill_index) {
                    fixed_height += items[index].item->height();
                }
            }
            setItemHeight(items[fill_index].item,
                          qMax(qreal(0), availableHeight - gap_size - fixed_height));
        }
        for (const auto& entry : items) {
            if (entry.attached && entry.attached->fillWidth()) {
                setItemWidth(entry.item, availableWidth);
            }
        }

        if (fill_count > 1 && ! main_fill_conflict) {
            qmlWarning(q) << "LiteColumn supports only one fillHeight child";
        }
        main_fill_conflict = fill_count > 1;

        qreal used_height = gap_size;
        qreal max_width   = 0;
        for (const auto& entry : items) {
            used_height += entry.item->height();
            max_width = qMax(max_width, entry.item->width());
        }

        qreal cursor = top_padding + verticalOffset(availableHeight, used_height, alignment);
        for (const auto& entry : items) {
            const qreal x = left_padding + horizontalOffset(availableWidth,
                                                            entry.item->width(),
                                                            alignment,
                                                            effective_direction);
            setItemPosition(entry.item, x, cursor);
            cursor += entry.item->height() + spacing;
        }

        updateContentSize(left_padding + right_padding + max_width,
                          top_padding + bottom_padding + used_height);
    }

    void updateGeometry() {
        const auto  items            = participatingItems();
        const qreal available_width  = qMax(qreal(0), q->width() - left_padding - right_padding);
        const qreal available_height = qMax(qreal(0), q->height() - top_padding - bottom_padding);

        laying_out = true;
        if (mode == LayoutContainer::Mode::Row) {
            updateRow(items, available_width, available_height);
        } else if (mode == LayoutContainer::Mode::Column) {
            updateColumn(items, available_width, available_height);
        } else {
            updateBox(items, available_width, available_height);
        }
        laying_out = false;
    }

    LayoutContainer*        q;
    LayoutContainer::Mode   mode;
    LayoutItemAdapter       adapter;
    std::vector<ChildWatch> watches;
    QQmlProperty            mirroring_property;
    LayoutMirroringObserver mirroring_observer;
    qreal                   left_padding   = 0;
    qreal                   right_padding  = 0;
    qreal                   top_padding    = 0;
    qreal                   bottom_padding = 0;
    qreal                   spacing        = 0;
    qreal                   content_width  = 0;
    qreal                   content_height = 0;
    Qt::Alignment           alignment;
    Qt::LayoutDirection     layout_direction    = Qt::LeftToRight;
    Qt::LayoutDirection     effective_direction = Qt::LeftToRight;
    bool                    hint_dirty          = true;
    bool                    geometry_dirty      = true;
    bool                    laying_out          = false;
    bool                    main_fill_conflict  = false;
};

LayoutContainer::LayoutContainer(Mode mode, QQuickItem* parent)
    : QQuickItem(parent), d(std::make_unique<LayoutContainerPrivate>(this, mode)) {
    connect(this, &QQuickItem::visibleChanged, this, [this]() {
        d->markAllDirty();
    });
}

LayoutContainer::~LayoutContainer() = default;

qreal LayoutContainer::leftPadding() const { return d->left_padding; }

void LayoutContainer::setLeftPadding(qreal padding) {
    if (equalReal(d->left_padding, padding)) {
        return;
    }
    d->left_padding = padding;
    Q_EMIT leftPaddingChanged();
    d->markAllDirty();
}

qreal LayoutContainer::rightPadding() const { return d->right_padding; }

void LayoutContainer::setRightPadding(qreal padding) {
    if (equalReal(d->right_padding, padding)) {
        return;
    }
    d->right_padding = padding;
    Q_EMIT rightPaddingChanged();
    d->markAllDirty();
}

qreal LayoutContainer::topPadding() const { return d->top_padding; }

void LayoutContainer::setTopPadding(qreal padding) {
    if (equalReal(d->top_padding, padding)) {
        return;
    }
    d->top_padding = padding;
    Q_EMIT topPaddingChanged();
    d->markAllDirty();
}

qreal LayoutContainer::bottomPadding() const { return d->bottom_padding; }

void LayoutContainer::setBottomPadding(qreal padding) {
    if (equalReal(d->bottom_padding, padding)) {
        return;
    }
    d->bottom_padding = padding;
    Q_EMIT bottomPaddingChanged();
    d->markAllDirty();
}

Qt::Alignment LayoutContainer::alignment() const { return d->alignment; }

void LayoutContainer::setAlignment(Qt::Alignment alignment) {
    if (d->alignment == alignment) {
        return;
    }
    d->alignment = alignment;
    Q_EMIT alignmentChanged();
    d->markGeometryDirty();
}

Qt::LayoutDirection LayoutContainer::layoutDirection() const { return d->layout_direction; }

void LayoutContainer::setLayoutDirection(Qt::LayoutDirection direction) {
    if (d->layout_direction == direction) {
        return;
    }
    d->layout_direction = direction;
    Q_EMIT layoutDirectionChanged();
    d->updateEffectiveDirection();
}

Qt::LayoutDirection LayoutContainer::effectiveLayoutDirection() const {
    return d->effective_direction;
}

qreal LayoutContainer::contentWidth() const { return d->content_width; }

qreal LayoutContainer::contentHeight() const { return d->content_height; }

qreal LayoutContainer::linearSpacing() const { return d->spacing; }

bool LayoutContainer::setLinearSpacing(qreal spacing) {
    if (equalReal(d->spacing, spacing)) {
        return false;
    }
    d->spacing = spacing;
    d->markAllDirty();
    return true;
}

void LayoutContainer::componentComplete() {
    QQuickItem::componentComplete();
    d->syncChildren();
    d->initializeMirroring();
    d->markAllDirty();
}

void LayoutContainer::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (! equalReal(newGeometry.width(), oldGeometry.width()) ||
        ! equalReal(newGeometry.height(), oldGeometry.height())) {
        d->markGeometryDirty();
    }
}

void LayoutContainer::itemChange(ItemChange change, const ItemChangeData& data) {
    if (change == ItemChildAddedChange) {
        d->addWatch(data.item);
        d->markAllDirty();
    } else if (change == ItemChildRemovedChange) {
        d->removeWatch(data.item, true);
        d->markAllDirty();
    } else if (change == ItemVisibleHasChanged) {
        d->markAllDirty();
    }
    QQuickItem::itemChange(change, data);
}

void LayoutContainer::updatePolish() {
    const bool update_hint     = std::exchange(d->hint_dirty, false);
    const bool update_geometry = std::exchange(d->geometry_dirty, false);

    if (update_hint) {
        d->updateHint();
    }
    if (update_geometry) {
        d->updateGeometry();
    }
    if (d->hint_dirty || d->geometry_dirty) {
        polish();
    }
}

LinearLayoutContainer::LinearLayoutContainer(Mode mode, QQuickItem* parent)
    : LayoutContainer(mode, parent) {}

qreal LinearLayoutContainer::spacing() const { return linearSpacing(); }

void LinearLayoutContainer::setSpacing(qreal spacing) {
    if (setLinearSpacing(spacing)) {
        Q_EMIT spacingChanged();
    }
}

Box::Box(QQuickItem* parent): LayoutContainer(Mode::Box, parent) {}

Row::Row(QQuickItem* parent): LinearLayoutContainer(Mode::Row, parent) {}

Column::Column(QQuickItem* parent): LinearLayoutContainer(Mode::Column, parent) {}

} // namespace qml_material

#include "layout_container.moc"
