#include "qml_material/view/lazy_list.hpp"
#include "delegate_recycler.hpp"
#include "viewport_controller.hpp"
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSet>
#include <utility>

namespace qml_material
{
struct LazyList::Private {
    struct PositionRequest {
        QString      key;
        qreal        alignment;
        qreal        offset;
        PositionMode mode;
    };
    std::optional<PositionRequest>                 position;
    QPointer<ItemSource>                           source;
    QPointer<QQmlComponent>                        delegate;
    ItemSnapshotPtr                                pending = std::make_shared<ItemSnapshot>();
    ViewportController                             controller;
    std::unique_ptr<DelegateRecycler>              recycler;
    QHash<QString, QPointer<QQuickItem>>           live;
    QSet<QString>                                  dirtyMeasurements;
    QHash<QString, QList<QMetaObject::Connection>> connections;
    qreal                                          spacing    = 0;
    qreal                                          cache      = 160;
    qreal                                          estimate   = 48;
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
    std::optional<int>                             requestedCount;
};

LazyList::LazyList(QQuickItem* parent): Flickable(parent), d(std::make_shared<Private>()) {
    setClip(true);
    setFlickableDirection(VerticalFlick);
}
LazyList::~LazyList() {
    d->shuttingDown = true;
    if (d->source) disconnect(d->source, nullptr, this, nullptr);
    if (d->delegate) disconnect(d->delegate, nullptr, this, nullptr);
    for (const auto& connections : std::as_const(d->connections))
        for (const auto& connection : connections) disconnect(connection);
    d->live.clear();
    if (d->recycler) d->recycler->clear();
}
ItemSource*    LazyList::source() const { return d->source; }
QQmlComponent* LazyList::delegate() const { return d->delegate; }
qreal          LazyList::spacing() const { return d->spacing; }
qreal          LazyList::cacheExtent() const { return d->cache; }
qreal          LazyList::estimatedItemExtent() const { return d->estimate; }
int            LazyList::poolLimit() const { return d->pool; }
int            LazyList::count() const { return int(d->pending->rows.size()); }
int            LazyList::firstVisibleIndex() const { return d->first; }
int            LazyList::lastVisibleIndex() const { return d->last; }
int            LazyList::liveCount() const { return d->live.size(); }
bool           LazyList::estimatedContentHeight() const { return d->controller.estimated(); }
QString        LazyList::errorString() const { return d->error; }

void LazyList::setSource(ItemSource* value) {
    if (d->shuttingDown) return;
    if (d->source == value) return;
    QPointer<LazyList> guard(this);
    if (d->source) disconnect(d->source, nullptr, this, nullptr);
    d->source = value;
    d->requestedCount.reset();
    d->pending = value ? value->snapshot() : std::make_shared<ItemSnapshot>();
    d->reset = d->apply = true;
    ++d->generation;
    if (value) {
        connect(value, &ItemSource::fetchStateChanged, this, [this] {
            d->requestedCount.reset();
            scheduleLayout();
        });
        connect(value, &ItemSource::committed, this, [this](const ItemChangeSet& change) {
            d->pending = change.after;
            d->apply   = true;
            ++d->generation;
            scheduleLayout();
        });
        connect(value, &QObject::destroyed, this, [this] {
            d->source  = nullptr;
            d->pending = std::make_shared<ItemSnapshot>();
            d->reset = d->apply = true;
            ++d->generation;
            scheduleLayout();
            QPointer<LazyList> guard(this);
            finishPositioning(false);
            if (guard) emit sourceChanged();
        });
    }
    scheduleLayout();
    finishPositioning(false);
    if (guard) emit sourceChanged();
}
void LazyList::setDelegate(QQmlComponent* value) {
    if (d->shuttingDown) return;
    if (d->delegate == value) return;
    QPointer<LazyList> guard(this);
    if (d->delegate) disconnect(d->delegate, nullptr, this, nullptr);
    d->delegate = value;
    d->reset = d->apply = true;
    ++d->generation;
    if (value) {
        connect(value, &QQmlComponent::statusChanged, this, [this] {
            scheduleLayout();
        });
        connect(value, &QObject::destroyed, this, [this] {
            d->delegate = nullptr;
            d->reset = d->apply = true;
            ++d->generation;
            scheduleLayout();
            QPointer<LazyList> guard(this);
            finishPositioning(false);
            if (guard) emit delegateChanged();
        });
    }
    scheduleLayout();
    finishPositioning(false);
    if (guard) emit delegateChanged();
}
void LazyList::setSpacing(qreal value) {
    if (! std::isfinite(value) || value < 0 || d->spacing == value) return;
    d->spacing = value;
    d->apply   = true;
    ++d->generation;
    scheduleLayout();
    emit spacingChanged();
}
void LazyList::setCacheExtent(qreal value) {
    if (! std::isfinite(value) || value < 0 || d->cache == value) return;
    d->cache = value;
    ++d->generation;
    scheduleLayout();
    emit cacheExtentChanged();
}
void LazyList::setEstimatedItemExtent(qreal value) {
    if (! std::isfinite(value) || value <= 0 || d->estimate == value) return;
    d->estimate = value;
    d->apply    = true;
    ++d->generation;
    scheduleLayout();
    emit estimatedItemExtentChanged();
}
void LazyList::setPoolLimit(int value) {
    value = std::max(0, value);
    if (d->pool == value) return;
    d->pool = value;
    scheduleLayout();
    emit poolLimitChanged();
}
void LazyList::scheduleLayout() {
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
void LazyList::componentComplete() {
    Flickable::componentComplete();
    scheduleLayout();
}
void LazyList::geometryChange(const QRectF& geometry, const QRectF& old) {
    Flickable::geometryChange(geometry, old);
    if (! d) return;
    if (geometry.width() != old.width()) d->apply = true;
    ++d->generation;
    scheduleLayout();
}
void LazyList::viewportMoved(Qt::Orientations) {
    if (d) scheduleLayout();
}
bool LazyList::positionAtKey(const QString& key, qreal alignment, qreal offset, PositionMode mode) {
    if (d->shuttingDown || d->pending->indexOfKey(key) < 0 || ! std::isfinite(alignment) ||
        alignment < 0 || alignment > 1 || ! std::isfinite(offset) ||
        (mode != Immediate && mode != Smooth))
        return false;
    QPointer<LazyList> guard(this);
    finishPositioning(false);
    if (! guard || d->position) return false;
    d->position = Private::PositionRequest { key, alignment, offset, mode };
    ++d->generation;
    cancelFlick();
    if (! guard) return false;
    scheduleLayout();
    emit positioningChanged();
    return true;
}
bool LazyList::positionAtIndex(int index, qreal alignment, qreal offset, PositionMode mode) {
    if (index < 0 || index >= d->pending->rows.size()) return false;
    return positionAtKey(d->pending->rows[index].key, alignment, offset, mode);
}
bool LazyList::positioning() const { return d->position.has_value(); }
void LazyList::finishPositioning(bool success) {
    if (! d->position) return;
    const auto key        = d->position->key;
    const bool stopMotion = ! success && d->position->mode == Smooth;
    d->position.reset();
    ++d->generation;
    QPointer<LazyList> guard(this);
    if (stopMotion) cancelFlick();
    if (! guard) return;
    emit positioningChanged();
    if (guard) emit positioningFinished(key, success);
}
void LazyList::scrollInputStarted() { finishPositioning(false); }
void LazyList::scrollActivityCancelled() { finishPositioning(false); }
void LazyList::setContentY(qreal value) {
    QPointer<LazyList> guard(this);
    finishPositioning(false);
    if (guard) Flickable::setContentY(value);
}

void LazyList::updatePolish() {
    const auto         state = d;
    QPointer<LazyList> guard(this);
    Flickable::updatePolish();
    if (! guard) return;
    const auto generation = state->generation;
    const auto current    = [&] {
        return guard && state->generation == generation;
    };
    const bool limited =
        state->limitedGeneration == generation && state->limitedOffset == contentY();
    if (limited) {
        bool progress = false;
        for (const auto& key : std::as_const(state->dirtyMeasurements)) {
            const auto item = state->live.value(key);
            if (item && std::isfinite(item->height()) && item->height() > 0) {
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
        state->controller.clear();
        state->recycler->clear();
        if (! current()) return;
    }
    state->recycler->setPoolLimit(state->pool);
    if (! current()) return;
    qreal offset = contentY();
    if (state->apply) {
        state->apply      = false;
        const auto result = state->controller.apply(
            state->pending, width(), state->estimate, state->spacing, offset);
        if (! result.accepted) {
            state->error = QStringLiteral("Invalid layout extent");
            finishPositioning(false);
            if (! guard) return;
            emit layoutChanged();
            return;
        }
        offset = result.offset;
    }
    const auto snapshot = state->controller.snapshot();
    if (! snapshot) return;
    const auto visibleBeforeMeasurement =
        state->controller.layout().visibleRange(offset, offset + height());
    const bool preserveAnchor =
        ! limited || (visibleBeforeMeasurement.first >= 0 &&
                      state->live.contains(snapshot->rows[visibleBeforeMeasurement.first].key));
    const auto dirty = std::exchange(state->dirtyMeasurements, {});
    for (const auto& key : dirty) {
        const auto item  = state->live.value(key);
        const int  index = snapshot->indexOfKey(key);
        if (! item || index < 0 || item->width() != width()) continue;
        const auto row = state->recycler->row(item);
        if (! row || row->revision() != snapshot->revision) continue;
        const auto measured = state->controller.measure(index, item->height(), offset);
        // A budget-limited empty viewport has no measured anchor to preserve.
        if (measured.accepted && preserveAnchor) offset = measured.offset;
    }
    if (state->position) {
        const auto& request = *state->position;
        const auto  desired =
            state->controller.positionAt(request.key, height(), request.alignment, request.offset);
        if (! desired) {
            finishPositioning(false);
            if (guard) scheduleLayout();
            return;
        }
        if (request.mode == Immediate)
            offset = std::clamp(
                *desired,
                minYExtent(),
                std::max(minYExtent(),
                         state->controller.layout().totalExtent() + bottomMargin() - height()));
    }
    auto&         layout = state->controller.layout();
    const int     start  = layout.indexAt(std::max<qreal>(0, offset - state->cache));
    const qreal   end    = offset + height() + state->cache;
    QSet<QString> wanted;
    QVector<int>  indexes;
    if (width() > 0 && height() > 0 && state->delegate && state->delegate->isReady()) {
        for (int index = start, work = 0; index >= 0 && index < layout.count() && work < 256;
             ++index, ++work) {
            if (layout.offset(index) > end) break;
            wanted.insert(snapshot->rows[index].key);
            indexes.append(index);
        }
        if (state->position) {
            const int target = snapshot->indexOfKey(state->position->key);
            if (! wanted.contains(state->position->key)) {
                if (indexes.size() == 256) wanted.remove(snapshot->rows[indexes.takeLast()].key);
                indexes.prepend(target);
                wanted.insert(state->position->key);
            }
        }
    }
    for (const auto& key : state->live.keys()) {
        if (wanted.contains(key) && state->live[key]) continue;
        auto item = state->live.take(key);
        for (const auto& connection : state->connections.take(key)) disconnect(connection);
        if (item) state->recycler->release(item);
        if (! current()) return;
    }
    state->error = snapshot->error;
    if (state->delegate && state->delegate->isError())
        state->error = state->delegate->errorString();
    bool changedExtent = false;
    int  processed     = 0;
    for (const int index : std::as_const(indexes)) {
        const auto& key = snapshot->rows[index].key;
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
                        &QQuickItem::heightChanged,
                        this,
                        [this, key] {
                            d->dirtyMeasurements.insert(key);
                            scheduleLayout();
                        }),
                connect(item, &QObject::destroyed, this, &LazyList::scheduleLayout)
            };
            state->recycler->attach(item, contentItem());
            if (! current()) return;
        }
        state->recycler->layout(item, { 0, layout.offset(index), width(), layout.extent(index) });
        if (! current()) return;
        if (! item) {
            scheduleLayout();
            continue;
        }
        const qreal previousExtent = layout.extent(index);
        const auto  measured       = state->controller.measure(index, item->height(), offset);
        ++processed;
        if (measured.accepted) {
            changedExtent |= previousExtent != item->height();
            if (previousExtent > 0 && item->height() == 0) ++state->zeroWork;
            offset = measured.offset;
        } else {
            // Keep the last valid extent and retry only when layout inputs change.
            state->error = QStringLiteral("Invalid delegate height for key %1").arg(key);
        }
    }
    if (state->position && state->position->mode == Immediate) {
        const auto& request = *state->position;
        offset =
            *state->controller.positionAt(request.key, height(), request.alignment, request.offset);
    }
    updateContentGeometry({ width(), layout.totalExtent() }, { 0, offset - contentY() });
    if (! current()) return;
    if (state->position && state->position->mode == Smooth) {
        const auto request = *state->position;
        setVerticalScrollTarget(*state->controller.positionAt(
            request.key, height(), request.alignment, request.offset));
        if (! current()) return;
    }
    for (const auto& key : state->live.keys()) {
        const int index = snapshot->indexOfKey(key);
        if (index < 0) continue;
        state->recycler->layout(state->live.value(key),
                                { 0, layout.offset(index), width(), layout.extent(index) });
        if (! current()) return;
    }
    const auto visible = width() > 0 ? layout.visibleRange(contentY(), contentY() + height())
                                     : LinearLayout::Range {};
    state->first       = visible.first;
    state->last        = visible.last;
    if (state->zeroWork >= 256 || (processed == 256 && start + processed < layout.count() &&
                                   layout.offset(start + processed) < end)) {
        state->limitedGeneration = generation;
        state->limitedOffset     = contentY();
        state->error = QStringLiteral("Delegate work limit reached before filling viewport");
    } else if (changedExtent) {
        scheduleLayout();
    }
    if (state->source && state->error.isEmpty() && width() > 0 && height() > 0 && state->delegate &&
        state->delegate->isReady() &&
        contentY() + height() + state->cache >= layout.totalExtent() &&
        state->requestedCount != layout.count()) {
        state->requestedCount = layout.count();
        state->source->requestMore();
        if (! current()) return;
    }
    bool reached = false;
    if (state->position) {
        const auto& request = *state->position;
        const auto  target =
            state->controller.positionAt(request.key, height(), request.alignment, request.offset);
        reached = target && qAbs(contentY() - std::clamp(*target, minYExtent(), maxYExtent())) <=
                                (pixelAligned() ? 1.0 : 0.01);
    }
    if (state->position &&
        (! state->error.isEmpty() || (! changedExtent && reached && ! isMovingVertically() &&
                                      state->live.value(state->position->key)))) {
        finishPositioning(state->error.isEmpty());
        if (! guard) return;
    }
    emit layoutChanged();
}

} // namespace qml_material
