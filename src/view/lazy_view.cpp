#include "qml_material/view/lazy_view.hpp"
#include "delegate_recycler.hpp"
#include "lazy_view_layout_p.hpp"
#include <QKeyEvent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSet>
#include <utility>

namespace qml_material
{
struct LazyView::Private {
    struct PositionRequest {
        QString      key;
        qreal        alignment;
        qreal        offset;
        PositionMode mode;
    };
    Qt::Orientation                                orientation = Qt::Vertical;
    std::optional<PositionRequest>                 position;
    QPointer<ItemSource>                           source;
    QPointer<QQmlComponent>                        delegate;
    ItemSnapshotPtr                                pending = std::make_shared<ItemSnapshot>();
    std::unique_ptr<LazyViewLayout>                layout;
    QString                                        currentKey;
    std::unique_ptr<DelegateRecycler>              recycler;
    QHash<QString, QPointer<QQuickItem>>           live;
    QSet<QString>                                  dirtyMeasurements;
    QHash<QString, QList<QMetaObject::Connection>> connections;
    qreal                                          cache      = 160;
    int                                            pool       = 16;
    int                                            first      = -1;
    int                                            last       = -1;
    quint64                                        generation = 0;
    bool                                           reset      = true;
    bool                                           apply      = true;
    bool                                           queued     = false;
    QString                                        error;
    std::optional<quint64>                         limitedGeneration;
    qreal                                          limitedOffset  = 0;
    quint64                                        zeroGeneration = 0;
    int                                            zeroWork       = 0;
    bool                                           shuttingDown   = false;
    ItemDemand                                     demand;
    bool                                           demandDirty = false;
    std::shared_ptr<const bool>                    demandLifetime;
};

LazyView::LazyView(std::unique_ptr<LazyViewLayout> layout, QQuickItem* parent)
    : Flickable(parent), d(std::make_shared<Private>()) {
    d->layout = std::move(layout);
    setActiveFocusOnTab(true);
    setClip(true);
    setFlickableDirection(VerticalFlick);
    const auto marginsChanged = [this] {
        invalidateDemand();
        d->apply = true;
        ++d->generation;
        scheduleLayout();
    };
    connect(this, &Flickable::leftMarginChanged, this, marginsChanged);
    connect(this, &Flickable::rightMarginChanged, this, marginsChanged);
    connect(this, &Flickable::topMarginChanged, this, marginsChanged);
    connect(this, &Flickable::bottomMarginChanged, this, marginsChanged);
}
LazyView::~LazyView() {
    d->shuttingDown = true;
    d->demandLifetime.reset();
    if (d->source) disconnect(d->source, nullptr, this, nullptr);
    if (d->delegate) disconnect(d->delegate, nullptr, this, nullptr);
    for (const auto& connections : std::as_const(d->connections))
        for (const auto& connection : connections) disconnect(connection);
    d->live.clear();
    if (d->recycler) d->recycler->clear();
}
ItemSource*    LazyView::source() const { return d->source; }
QQmlComponent* LazyView::delegate() const { return d->delegate; }
qreal          LazyView::cacheExtent() const { return d->cache; }
int            LazyView::poolLimit() const { return d->pool; }
int            LazyView::count() const { return d->pending->count(); }
int            LazyView::firstVisibleIndex() const { return d->first; }
int            LazyView::lastVisibleIndex() const { return d->last; }
int            LazyView::liveCount() const { return d->live.size(); }
bool           LazyView::estimatedContentHeight() const {
    return d->orientation == Qt::Vertical && d->layout->estimated();
}
bool LazyView::estimatedContentWidth() const {
    return d->orientation == Qt::Horizontal && d->layout->estimated();
}
Qt::Orientation LazyView::orientation() const { return d->orientation; }
void            LazyView::setOrientation(Qt::Orientation value) {
    if ((value != Qt::Horizontal && value != Qt::Vertical) || d->orientation == value) return;
    QPointer<LazyView> guard(this);
    d->orientation = value;
    d->layout->setOrientation(value);
    invalidateDemand();
    d->reset = d->apply = true;
    ++d->generation;
    scheduleLayout();
    finishPositioning(false);
    if (! guard || d->orientation != value) return;
    cancelFlick();
    if (! guard || d->orientation != value) return;
    setFlickableDirection(value == Qt::Horizontal ? HorizontalFlick : VerticalFlick);
    if (guard && d->orientation == value) emit orientationChanged();
}
QString LazyView::errorString() const { return d->error; }

void LazyView::setSource(ItemSource* value) {
    if (d->shuttingDown) return;
    if (d->source == value) return;
    QPointer<LazyView> guard(this);
    if (d->source) disconnect(d->source, nullptr, this, nullptr);
    d->source = value;
    invalidateDemand();
    d->pending = value ? value->snapshot() : std::make_shared<ItemSnapshot>();
    if (currentIndex() < 0) d->currentKey.clear();
    d->reset = d->apply = true;
    ++d->generation;
    if (value) {
        connect(value, &ItemSource::stateChanged, this, &LazyView::scheduleLayout);
        connect(value, &ItemSource::committed, this, [this](const ItemChangeSet& change) {
            invalidateDemand();
            const int oldCurrent = currentIndex();
            d->pending           = change.after;
            d->apply             = true;
            ++d->generation;
            scheduleLayout();
            if (currentIndex() < 0) d->currentKey.clear();
            if (currentIndex() != oldCurrent) emit currentChanged();
        });
        connect(value, &QObject::destroyed, this, [this] {
            invalidateDemand();
            d->source  = nullptr;
            d->pending = std::make_shared<ItemSnapshot>();
            d->currentKey.clear();
            d->reset = d->apply = true;
            ++d->generation;
            scheduleLayout();
            QPointer<LazyView> guard(this);
            finishPositioning(false);
            if (guard) emit sourceChanged();
            if (guard) emit currentChanged();
        });
    }
    scheduleLayout();
    finishPositioning(false);
    if (guard) emit sourceChanged();
    if (guard) emit currentChanged();
}
void LazyView::setDelegate(QQmlComponent* value) {
    if (d->shuttingDown) return;
    if (d->delegate == value) return;
    QPointer<LazyView> guard(this);
    if (d->delegate) disconnect(d->delegate, nullptr, this, nullptr);
    invalidateDemand();
    d->delegate = value;
    d->reset = d->apply = true;
    ++d->generation;
    if (value) {
        connect(value, &QQmlComponent::statusChanged, this, [this] {
            scheduleLayout();
        });
        connect(value, &QObject::destroyed, this, [this] {
            invalidateDemand();
            d->delegate = nullptr;
            d->reset = d->apply = true;
            ++d->generation;
            scheduleLayout();
            QPointer<LazyView> guard(this);
            finishPositioning(false);
            if (guard) emit delegateChanged();
        });
    }
    scheduleLayout();
    finishPositioning(false);
    if (guard) emit delegateChanged();
}
void LazyView::setCacheExtent(qreal value) {
    if (! std::isfinite(value) || value < 0 || d->cache == value) return;
    d->cache = value;
    ++d->generation;
    scheduleLayout();
    emit cacheExtentChanged();
}
LazyViewLayout& LazyView::layoutEngine() const { return *d->layout; }
void            LazyView::invalidateLayout() {
    invalidateDemand();
    d->apply = true;
    ++d->generation;
    scheduleLayout();
}
QString LazyView::currentKey() const { return d->currentKey; }
int     LazyView::currentIndex() const { return d->pending->indexOfKey(d->currentKey); }
void    LazyView::setCurrentKey(const QString& key) {
    if (key == d->currentKey || (! key.isEmpty() && d->pending->indexOfKey(key) < 0)) return;
    d->currentKey = key;
    emit currentChanged();
}
void    LazyView::setCurrentIndex(int index) { setCurrentKey(keyAtIndex(index)); }
QString LazyView::keyAtIndex(int index) const {
    return index >= 0 && index < d->pending->count() ? d->pending->descriptorAt(index).key
                                                     : QString();
}
QRectF LazyView::itemRect(int index) const {
    if (d->layout->snapshot() != d->pending || index < 0 || index >= d->layout->count()) return {};
    auto rect = d->layout->rect(index);
    if (d->orientation == Qt::Horizontal)
        rect = { rect.y(), rect.x(), rect.height(), rect.width() };
    return rect.translated(-contentX(), -contentY());
}
void LazyView::keyPressEvent(QKeyEvent* event) {
    const bool horizontal = d->orientation == Qt::Horizontal;
    int        lineDelta = 0, slotDelta = 0, target = currentIndex();
    switch (event->key()) {
    case Qt::Key_Left: (horizontal ? lineDelta : slotDelta) = -1; break;
    case Qt::Key_Right: (horizontal ? lineDelta : slotDelta) = 1; break;
    case Qt::Key_Up: (horizontal ? slotDelta : lineDelta) = -1; break;
    case Qt::Key_Down: (horizontal ? slotDelta : lineDelta) = 1; break;
    case Qt::Key_Home: target = 0; break;
    case Qt::Key_End: target = count() - 1; break;
    default: Flickable::keyPressEvent(event); return;
    }
    if (lineDelta || slotDelta) target = d->layout->neighbor(target, lineDelta, slotDelta);
    if (target < 0 || target >= count()) {
        event->ignore();
        return;
    }
    QPointer<LazyView> guard(this);
    setCurrentIndex(target);
    if (! guard) return;
    const auto  rect        = itemRect(currentIndex());
    const qreal begin       = horizontal ? rect.left() : rect.top();
    const qreal end         = horizontal ? rect.right() : rect.bottom();
    const qreal startMargin = horizontal ? leftMargin() : topMargin();
    const qreal viewportEnd = horizontal ? width() - rightMargin() : height() - bottomMargin();
    if (begin < startMargin)
        positionAtKey(currentKey(), 0);
    else if (end > viewportEnd)
        positionAtKey(currentKey(), 1);
    event->accept();
}
void LazyView::setPoolLimit(int value) {
    value = std::max(0, value);
    if (d->pool == value) return;
    d->pool = value;
    scheduleLayout();
    emit poolLimitChanged();
}
ItemDemand LazyView::currentDemand() const { return d->demand; }
void       LazyView::invalidateDemand() {
    d->demandLifetime.reset();
    d->demandDirty |= bool(d->demand.snapshot());
    d->demand = {};
}
void LazyView::scheduleLayout() {
    if (d->shuttingDown) return;
    if (d->queued) return;
    d->queued = true;
    QMetaObject::invokeMethod(
        this,
        [this] {
            d->queued = false;
            polish();
            if (window()) window()->update();
        },
        Qt::QueuedConnection);
}
void LazyView::componentComplete() {
    Flickable::componentComplete();
    scheduleLayout();
}
void LazyView::geometryChange(const QRectF& geometry, const QRectF& old) {
    Flickable::geometryChange(geometry, old);
    if (! d) return;
    invalidateDemand();
    if ((d->orientation == Qt::Vertical && geometry.width() != old.width()) ||
        (d->orientation == Qt::Horizontal && geometry.height() != old.height()))
        d->apply = true;
    ++d->generation;
    scheduleLayout();
}
void LazyView::viewportMoved(Qt::Orientations) {
    if (d) {
        invalidateDemand();
        scheduleLayout();
    }
}
bool LazyView::positionAtKey(const QString& key, qreal alignment, qreal offset, PositionMode mode) {
    if (d->shuttingDown || d->pending->indexOfKey(key) < 0 || ! std::isfinite(alignment) ||
        alignment < 0 || alignment > 1 || ! std::isfinite(offset) ||
        (mode != Immediate && mode != Smooth))
        return false;
    QPointer<LazyView> guard(this);
    finishPositioning(false);
    if (! guard || d->position) return false;
    cancelScrollActivity();
    if (! guard || d->position) return false;
    d->position = Private::PositionRequest { key, alignment, offset, mode };
    ++d->generation;
    scheduleLayout();
    emit positioningChanged();
    return true;
}
bool LazyView::positionAtIndex(int index, qreal alignment, qreal offset, PositionMode mode) {
    if (index < 0 || index >= d->pending->count()) return false;
    return positionAtKey(d->pending->descriptorAt(index).key, alignment, offset, mode);
}
bool LazyView::positioning() const { return d->position.has_value(); }
void LazyView::finishPositioning(bool success) {
    if (! d->position) return;
    const auto key        = d->position->key;
    const bool stopMotion = ! success && d->position->mode == Smooth;
    d->position.reset();
    ++d->generation;
    QPointer<LazyView> guard(this);
    if (stopMotion) cancelFlick();
    if (! guard) return;
    emit positioningChanged();
    if (guard) emit positioningFinished(key, success);
}
void LazyView::scrollInputStarted() { finishPositioning(false); }
void LazyView::scrollActivityCancelled() { finishPositioning(false); }
void LazyView::setContentY(qreal value) {
    QPointer<LazyView> guard(this);
    if (d->orientation == Qt::Vertical) finishPositioning(false);
    if (guard) Flickable::setContentY(value);
}
void LazyView::setContentX(qreal value) {
    QPointer<LazyView> guard(this);
    if (d->orientation == Qt::Horizontal) finishPositioning(false);
    if (guard) Flickable::setContentX(value);
}

void LazyView::updatePolish() {
    const auto         state = d;
    QPointer<LazyView> guard(this);
    Flickable::updatePolish();
    if (! guard) return;
    const auto generation = state->generation;
    const auto current    = [&] {
        return guard && state->generation == generation;
    };
    const bool horizontal    = state->orientation == Qt::Horizontal;
    const auto primaryOffset = [&] {
        return horizontal ? contentX() : contentY();
    };
    const auto primarySize = [&] {
        return horizontal ? width() : height();
    };
    const auto crossSize = [&] {
        return std::max<qreal>(0,
                               horizontal ? height() - topMargin() - bottomMargin()
                                          : width() - leftMargin() - rightMargin());
    };
    const auto minimumOffset = [&] {
        return horizontal ? minXExtent() : minYExtent();
    };
    const auto maximumOffset = [&] {
        return horizontal ? maxXExtent() : maxYExtent();
    };
    const auto startMargin = [&] {
        return horizontal ? leftMargin() : topMargin();
    };
    const auto endMargin = [&] {
        return horizontal ? rightMargin() : bottomMargin();
    };
    const auto itemExtent = [horizontal](QQuickItem* item) {
        return horizontal ? item->width() : item->height();
    };
    const auto itemCrossSize = [horizontal](QQuickItem* item) {
        return horizontal ? item->height() : item->width();
    };
    const auto itemGeometry = [&](int index) {
        const auto rect = state->layout->rect(index);
        return horizontal ? QRectF(rect.y(), rect.x(), rect.height(), rect.width()) : rect;
    };
    const auto positionFor = [&](const Private::PositionRequest& request) {
        return state->layout->positionAt(
            request.key,
            std::max<qreal>(0, primarySize() - startMargin() - endMargin()),
            request.alignment,
            request.offset - startMargin());
    };
    const bool limited =
        state->limitedGeneration == generation && state->limitedOffset == primaryOffset();
    if (limited) {
        bool progress = false;
        for (const auto& key : std::as_const(state->dirtyMeasurements)) {
            const auto item = state->live.value(key);
            if (item && std::isfinite(itemExtent(item)) && itemExtent(item) > 0) {
                progress = true;
                break;
            }
        }
        if (! progress) return;
        state->limitedGeneration.reset();
        state->zeroWork = 0;
    }
    if (state->zeroGeneration != generation) {
        state->zeroGeneration = generation;
        state->zeroWork       = 0;
    }
    if (! state->recycler) {
        auto* engine = qmlEngine(this);
        if (! engine) return;
        state->recycler = std::make_unique<DelegateRecycler>(engine);
    }
    if (state->reset) {
        state->reset = false;
        for (const auto& connections : std::as_const(state->connections))
            for (const auto& connection : connections) disconnect(connection);
        state->connections.clear();
        state->live.clear();
        state->dirtyMeasurements.clear();
        state->layout->clear();
        state->recycler->clear();
        if (! current()) return;
    }
    state->recycler->setPoolLimit(state->pool);
    if (! current()) return;
    qreal offset = primaryOffset();
    if (state->apply) {
        state->apply      = false;
        const auto result = state->layout->apply(
            state->pending, crossSize(), offset, primarySize() - startMargin() - endMargin());
        if (! result.accepted) {
            state->error = QStringLiteral("Invalid layout extent");
            finishPositioning(false);
            if (! guard) return;
            emit layoutChanged();
            return;
        }
        offset = result.offset;
    }
    const auto snapshot = state->layout->snapshot();
    if (! snapshot) return;
    const auto visibleBeforeMeasurement =
        (*state->layout).visibleRange(offset, offset + primarySize());
    const bool preserveAnchor =
        ! limited ||
        (visibleBeforeMeasurement.first >= 0 &&
         state->live.contains(snapshot->descriptorAt(visibleBeforeMeasurement.first).key));
    const auto dirty = std::exchange(state->dirtyMeasurements, {});
    for (const auto& key : dirty) {
        const auto item  = state->live.value(key);
        const int  index = snapshot->indexOfKey(key);
        if (! item || index < 0 || itemCrossSize(item) != crossSize()) continue;
        const auto row = state->recycler->row(item);
        if (! row || row->revision() != snapshot->revision) continue;
        const auto measured = state->layout->measure(index, itemExtent(item), offset);
        // A budget-limited empty viewport has no measured anchor to preserve.
        if (measured.accepted && preserveAnchor) offset = measured.offset;
    }
    if (state->position) {
        const auto& request = *state->position;
        const auto  desired = positionFor(request);
        if (! desired) {
            finishPositioning(false);
            if (guard) scheduleLayout();
            return;
        }
        if (request.mode == Immediate)
            offset =
                std::clamp(*desired,
                           minimumOffset(),
                           std::max(minimumOffset(),
                                    (*state->layout).totalExtent() + endMargin() - primarySize()));
    }
    auto&       layout    = (*state->layout);
    const auto  requested = layout.requestedRange(offset, primarySize(), state->cache);
    const int   start     = requested.first;
    const qreal end       = offset + primarySize() + state->cache;
    const int   budget =
        layout.fixedItemExtent() ? std::max(256, requested.last - requested.first + 1) : 256;
    QSet<QString> wanted;
    QVector<int>  indexes;
    if (crossSize() > 0 && primarySize() > 0 && state->delegate && state->delegate->isReady()) {
        for (int index = start, work = 0;
             index >= 0 && index < layout.count() && index <= requested.last && work < budget;
             ++index, ++work) {
            if (layout.offset(index) > end) break;
            wanted.insert(snapshot->descriptorAt(index).key);
            indexes.append(index);
        }
        if (state->position) {
            const int target = snapshot->indexOfKey(state->position->key);
            if (! wanted.contains(state->position->key)) {
                if (! layout.fixedItemExtent() && indexes.size() == 256)
                    wanted.remove(snapshot->descriptorAt(indexes.takeLast()).key);
                indexes.prepend(target);
                wanted.insert(state->position->key);
            }
        }
    }
    if (layout.fixedItemExtent()) {
        const auto visible = layout.visibleRange(offset, offset + primarySize());
        std::stable_partition(indexes.begin(), indexes.end(), [&](int index) {
            return visible.first >= 0 && index >= visible.first && index <= visible.last;
        });
    }
    for (const auto& key : state->live.keys()) {
        if (wanted.contains(key) && state->live[key]) continue;
        auto item = state->live.take(key);
        for (const auto& connection : state->connections.take(key)) disconnect(connection);
        if (item) state->recycler->release(item);
        if (! current()) return;
    }
    state->error = (state->source ? state->source->errorString() : QString());
    if (state->delegate && state->delegate->isError())
        state->error = state->delegate->errorString();
    bool changedExtent = false;
    int  processed     = 0;
    for (const int index : std::as_const(indexes)) {
        const auto& key = snapshot->descriptorAt(index).key;
        if (! wanted.contains(key)) break;
        QPointer<QQuickItem> item = state->live.value(key);
        if (item) {
            auto row = state->recycler->row(item);
            if (! row || row->revision() != snapshot->revision || row->index() != index) {
                if (! state->recycler->rebind(item, snapshot, index)) {
                    for (const auto& connection : state->connections.take(key))
                        disconnect(connection);
                    state->live.remove(key);
                    if (item) state->recycler->release(item);
                    item = nullptr;
                }
                if (! current()) return;
            }
        }
        if (! item) {
            item = state->recycler->acquire(state->delegate, snapshot, index);
            if (! current()) {
                if (item) state->recycler->release(item);
                return;
            }
            if (! item) {
                state->error = state->recycler->errorString();
                break;
            }
            state->live.insert(key, item);
            state->connections[key] = {
                connect(item,
                        horizontal ? &QQuickItem::widthChanged : &QQuickItem::heightChanged,
                        this,
                        [this, key] {
                            d->dirtyMeasurements.insert(key);
                            scheduleLayout();
                        }),
                connect(item, &QObject::destroyed, this, &LazyView::scheduleLayout)
            };
            state->recycler->attach(item, contentItem());
            if (! current()) return;
        }
        state->recycler->layout(
            item, itemGeometry(index), state->orientation, layout.fixedItemExtent());
        if (! current()) return;
        if (! item) {
            scheduleLayout();
            continue;
        }
        const qreal previousExtent = layout.extent(index);
        const auto  measured       = state->layout->measure(index, itemExtent(item), offset);
        ++processed;
        if (measured.accepted) {
            if (! layout.fixedItemExtent()) {
                changedExtent |= previousExtent != itemExtent(item);
                if (previousExtent > 0 && itemExtent(item) == 0) ++state->zeroWork;
            }
            offset = measured.offset;
        } else {
            // Keep the last valid extent and retry only when layout inputs change.
            state->error = QStringLiteral("Invalid delegate extent for key %1").arg(key);
        }
    }
    if (state->position && state->position->mode == Immediate) {
        const auto& request = *state->position;
        offset              = *positionFor(request);
    }
    updateContentGeometry(horizontal ? QSizeF(layout.totalExtent(), crossSize())
                                     : QSizeF(crossSize(), layout.totalExtent()),
                          horizontal ? QPointF(offset - primaryOffset(), 0)
                                     : QPointF(0, offset - primaryOffset()));
    if (! current()) return;
    if (state->position && state->position->mode == Smooth) {
        const auto request = *state->position;
        const auto target  = *positionFor(request);
        if (horizontal)
            setHorizontalScrollTarget(target);
        else
            setVerticalScrollTarget(target);
        if (! current()) return;
    }
    for (const auto& key : state->live.keys()) {
        const int index = snapshot->indexOfKey(key);
        if (index < 0) continue;
        state->recycler->layout(state->live.value(key),
                                itemGeometry(index),
                                state->orientation,
                                layout.fixedItemExtent());
        if (! current()) return;
    }
    const auto visible = crossSize() > 0
                             ? layout.visibleRange(primaryOffset(), primaryOffset() + primarySize())
                             : ItemRange {};
    state->first       = visible.first;
    state->last        = visible.last;
    for (const auto& key : state->live.keys()) {
        auto item = state->live.value(key);
        auto row  = state->recycler->row(item);
        if (! item || ! row) continue;
        const auto area = item->mapRectToItem(this, item->boundingRect());
        row->setUsage(! area.isEmpty() && area.intersects(boundingRect()) ? LazyRow::Visible
                                                                          : LazyRow::Cached,
                      this);
        if (! current()) return;
    }
    if (! layout.fixedItemExtent() &&
        (state->zeroWork >= 256 || (processed == 256 && start + processed < layout.count() &&
                                    layout.offset(start + processed) < end))) {
        state->limitedGeneration = generation;
        state->limitedOffset     = primaryOffset();
        state->error = QStringLiteral("Delegate work limit reached before filling viewport");
    } else if (changedExtent) {
        scheduleLayout();
    }
    const auto range =
        crossSize() > 0 && primarySize() > 0 && state->delegate && state->delegate->isReady()
            ? layout.demandRange(primaryOffset(), primarySize(), state->cache)
            : ItemRange {};
    const int  first  = range.first >= 0 ? range.first : 0;
    const int  length = range.first >= 0 ? range.last - range.first + 1 : 0;
    const bool atEnd = layout.fixedItemExtent()
                           ? range.last >= 0 && range.last == layout.count() - 1
                           : primaryOffset() + primarySize() + state->cache >= layout.totalExtent();
    if ((length > 0 || state->demand.snapshot() || state->demandDirty) &&
        (state->demand.snapshot() != snapshot || state->demand.first() != first ||
         state->demand.count() != length || state->demand.reachesEnd() != atEnd ||
         ! state->demand.valid())) {
        invalidateDemand();
        if (length > 0) {
            state->demandLifetime    = std::make_shared<const bool>(true);
            state->demand            = ItemDemand(snapshot, first, length, atEnd);
            state->demand.m_tracked  = true;
            state->demand.m_lifetime = state->demandLifetime;
        }
        state->demandDirty = false;
        const auto demand  = state->demand;
        emit       itemsNeeded(demand);
        if (! current()) return;
    }
    bool reached = false;
    if (state->position) {
        const auto& request = *state->position;
        const auto  target  = positionFor(request);
        reached = target &&
                  qAbs(primaryOffset() - std::clamp(*target, minimumOffset(), maximumOffset())) <=
                      (pixelAligned() ? 1.0 : 0.01);
    }
    if (state->position && (! state->error.isEmpty() ||
                            (! changedExtent && reached &&
                             ! (horizontal ? isMovingHorizontally() : isMovingVertically()) &&
                             state->live.value(state->position->key)))) {
        finishPositioning(state->error.isEmpty());
        if (! guard) return;
    }
    emit layoutChanged();
}

} // namespace qml_material
