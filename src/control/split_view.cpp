#include "qml_material/control/split_view.hpp"
#include "qml_material/util/qt.hpp"
#include "split_layout_p.hpp"
#include <algorithm>
#include <cmath>
#include <QQmlContext>
#include <QQmlInfo>
#include <QQmlEngine>
#include <utility>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QTouchEvent>
#include <QCborArray>
#include <QCborMap>
#include <QCborStreamReader>
#include <QSet>
#include <functional>
#include <QQuickWindow>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktransitionmanager_p_p.h>

namespace qml_material
{
class SplitTransitionTarget : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
public:
    qreal progress() const { return m_progress; }
    void  setProgress(qreal value) {
        if (! std::isfinite(value) || m_progress == value) return;
        m_progress = value;
        if (changed) changed();
    }
    std::function<void()> changed;

private:
    qreal m_progress = 1;
};

class SplitPaneHost : public QQuickItem {
public:
    QRectF clipRect() const override { return m_bounds; }
    bool   contains(const QPointF& point) const override { return m_bounds.contains(point); }
    void   setBounds(const QRectF& bounds) {
        if (m_bounds == bounds) return;
        m_bounds = bounds;
        QQuickItemPrivate::get(this)->dirty(QQuickItemPrivate::Size);
    }

private:
    QRectF m_bounds;
};

class SplitMotion : public QQuickTransitionManager {
public:
    std::function<void()> completed;

protected:
    void finished() override {
        if (completed) completed();
    }
};

struct SplitPaneState {
    QPointer<QQuickItem>        item;
    QPointer<SplitViewAttached> info;
    QPointer<SplitPaneHost>     host;
    SplitTransitionTarget       target;
    SplitMotion                 motion;
    quint64                     generation = 0;
    bool                        pending    = false;
    std::optional<qreal>        expandedSize;
};

namespace
{
SplitViewAttached* attached(QQuickItem* item) {
    return qobject_cast<SplitViewAttached*>(qmlAttachedPropertiesObject<SplitView>(item, true));
}
SplitHandleAttached* handleState(QQuickItem* item) {
    return qobject_cast<SplitHandleAttached*>(
        qmlAttachedPropertiesObject<SplitHandleAttached>(item, true));
}
} // namespace

struct SplitResizeState {
    QList<split_layout::Pane> panes;
    QPointer<QQuickItem>      handle;
    QPointer<QQuickItem>      item;
    int                       handleIndex   = -1;
    qreal                     available     = 0;
    qreal                     pressPosition = 0;
};

class SplitHandleInput : public QQuickItem {
public:
    SplitHandleInput(SplitView* view, QQuickItem* handle)
        : QQuickItem(handle), m_view(view), m_handle(handle) {
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptTouchEvents(true);
        setAcceptHoverEvents(true);
        setKeepMouseGrab(true);
        setKeepTouchGrab(true);
        setZ(1000);
        const auto size = [this] {
            if (m_handle) setSize(m_handle->size());
        };
        connect(handle, &QQuickItem::widthChanged, this, size);
        connect(handle, &QQuickItem::heightChanged, this, size);
        connect(this, &QQuickItem::visibleChanged, this, [this] {
            if (! isVisible()) cancel();
        });
        connect(this, &QQuickItem::enabledChanged, this, [this] {
            if (! isEnabled()) cancel();
        });
        auto* state = handleState(handle);
        connect(state, &SplitHandleAttached::resizableChanged, this, [this, state] {
            setEnabled(state->isResizable());
        });
        setEnabled(state->isResizable());
        size();
    }
    bool contains(const QPointF& point) const override {
        return m_handle && m_handle->contains(mapToItem(m_handle, point));
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        QPointer<SplitHandleInput> guard(this);
        const bool                 accepted =
            m_touch_id == -1 && m_view && m_view->beginResize(m_handle, event->scenePosition());
        event->setAccepted(accepted);
        if (! guard) return;
    }
    void mouseMoveEvent(QMouseEvent* event) override {
        if (ownsResize()) m_view->moveResize(event->scenePosition());
        event->accept();
    }
    void mouseReleaseEvent(QMouseEvent* event) override {
        QPointer<SplitHandleInput> guard(this);
        if (ownsResize()) m_view->moveResize(event->scenePosition());
        if (guard && ownsResize()) m_view->endResize();
        event->accept();
    }
    void mouseUngrabEvent() override { cancel(); }
    void touchUngrabEvent() override { cancel(); }
    void touchEvent(QTouchEvent* event) override {
        if (event->type() == QEvent::TouchCancel) {
            cancel();
            event->accept();
            return;
        }
        QPointer<SplitHandleInput> guard(this);
        for (const auto& point : event->points()) {
            if (m_touch_id == -1 && point.state() == QEventPoint::Pressed && m_view) {
                if (m_view->beginResize(m_handle, point.scenePosition())) {
                    if (! guard) return;
                    m_touch_id = point.id();
                    event->setExclusiveGrabber(point, this);
                }
            } else if (point.id() == m_touch_id) {
                if (ownsResize()) m_view->moveResize(point.scenePosition());
                if (! guard) return;
                if (point.state() == QEventPoint::Released) {
                    m_touch_id = -1;
                    if (ownsResize()) m_view->endResize();
                }
            }
            if (! guard) return;
        }
        event->accept();
    }
    void hoverEnterEvent(QHoverEvent*) override {
        if (m_handle) handleState(m_handle)->setHovered(true);
    }
    void hoverLeaveEvent(QHoverEvent*) override {
        if (m_handle) handleState(m_handle)->setHovered(false);
    }

private:
    bool ownsResize() const {
        return m_view && m_view->m_resize && m_view->m_resize->handle == m_handle;
    }
    void cancel() {
        m_touch_id = -1;
        QPointer<SplitHandleInput> guard(this);
        if (m_handle) handleState(m_handle)->setHovered(false);
        if (guard && ownsResize()) m_view->endResize();
    }
    QPointer<SplitView>  m_view;
    QPointer<QQuickItem> m_handle;
    int                  m_touch_id = -1;
};

void SplitHandleAttached::setHovered(bool value) {
    if (m_hovered == value) return;
    m_hovered = value;
    Q_EMIT hoveredChanged();
}
SplitHandleAttached* SplitHandleAttached::qmlAttachedProperties(QObject* object) {
    return new SplitHandleAttached(object);
}
void SplitHandleAttached::setPressed(bool value) {
    if (m_pressed == value) return;
    m_pressed = value;
    Q_EMIT pressedChanged();
}
void SplitHandleAttached::setResizable(bool value) {
    if (m_resizable == value) return;
    m_resizable = value;
    Q_EMIT resizableChanged();
}
bool     SplitView::isResizing() const { return bool(m_resize); }
QVariant SplitView::saveState() const {
    QCborArray entries;
    for (int i = 0; i < count(); ++i) {
        auto* item = itemAt(i);
        if (! item) continue;
        auto* info = attached(item);
        if (! info->m_preferred_width && ! info->m_preferred_height) continue;
        QCborMap entry;
        entry.insert(QStringLiteral("index"), i);
        if (info->m_preferred_width)
            entry.insert(QStringLiteral("preferredWidth"), *info->m_preferred_width);
        if (info->m_preferred_height)
            entry.insert(QStringLiteral("preferredHeight"), *info->m_preferred_height);
        entries.append(entry);
    }
    return entries.toCborValue().toCbor();
}
bool SplitView::restoreState(const QVariant& state) {
    if (m_destroying || state.metaType() != QMetaType::fromType<QByteArray>()) return false;
    const auto        bytes = state.toByteArray();
    QCborStreamReader reader(bytes);
    const auto        decoded = QCborValue::fromCbor(reader);
    if (reader.lastError() != QCborError::NoError || reader.currentOffset() != bytes.size() ||
        ! decoded.isArray())
        return false;
    const auto entries = decoded.toArray();
    if (entries.size() > count()) return false;
    struct Entry {
        QPointer<SplitViewAttached> info;
        std::optional<qreal>        width, height;
        bool                        widthChanged = false, heightChanged = false;
    };
    QList<Entry> pending;
    QSet<qint64> indices;
    for (const auto& value : entries) {
        if (! value.isMap()) return false;
        const auto map   = value.toMap();
        const auto index = map.value(QStringLiteral("index"));
        if (! index.isInteger() || index.toInteger() < 0 || index.toInteger() >= count() ||
            indices.contains(index.toInteger()))
            return false;
        indices.insert(index.toInteger());
        Entry entry;
        auto* item = itemAt(index.toInteger());
        if (! item) return false;
        entry.info = attached(item);
        QSet<QString> keys;
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (! it.key().isString()) return false;
            const auto key = it.key().toString();
            if (keys.contains(key)) return false;
            keys.insert(key);
            if (key == QStringLiteral("index")) continue;
            if (key != QStringLiteral("preferredWidth") && key != QStringLiteral("preferredHeight"))
                return false;
            if (! it.value().isInteger() && ! it.value().isDouble()) return false;
            const qreal size = it.value().toDouble();
            if (! std::isfinite(size) || size < 0) return false;
            (key == QStringLiteral("preferredWidth") ? entry.width : entry.height) = size;
        }
        if (! entry.width && ! entry.height) return false;
        pending.append(entry);
    }
    QPointer<SplitView> guard(this);
    const auto          contentRevision = revision();
    const auto          restoreRevision = ++m_restore_revision;
    endResize();
    if (! guard || revision() != contentRevision || m_restore_revision != restoreRevision)
        return false;
    for (const auto& entry : pending)
        if (! entry.info || entry.info->view() != this) return false;
    // Commit all values before notifying bindings, so observers never see a partial restore.
    for (auto& entry : pending) {
        if (entry.width) {
            entry.widthChanged            = entry.info->m_preferred_width != entry.width;
            entry.info->m_preferred_width = entry.width;
        }
        if (entry.height) {
            entry.heightChanged            = entry.info->m_preferred_height != entry.height;
            entry.info->m_preferred_height = entry.height;
        }
    }
    requestLayout();
    for (const auto& entry : pending) {
        if (entry.info && (entry.widthChanged || entry.heightChanged))
            Q_EMIT entry.info->constraintsChanged();
        if (! guard || m_restore_revision != restoreRevision) return true;
        if (entry.info && entry.widthChanged) Q_EMIT entry.info->preferredWidthChanged();
        if (! guard || m_restore_revision != restoreRevision) return true;
        if (entry.info && entry.heightChanged) Q_EMIT entry.info->preferredHeightChanged();
        if (! guard || m_restore_revision != restoreRevision) return true;
    }
    return true;
}
bool SplitView::beginResize(QQuickItem* handle, const QPointF& point) {
    if (m_resize || m_destroying || m_handles_dirty || ! handle || ! handle->isVisible() ||
        ! isEnabled() || ! isVisible())
        return false;
    QPointer<SplitView>  guard(this);
    QPointer<QQuickItem> handleGuard(handle);
    settleExpansions(true);
    if (! guard) return false;
    updatePolish();
    if (! guard || ! handleGuard || ! handleGuard->isVisible()) return false;
    const int   index     = m_handles.indexOf(handle);
    auto        panes     = layoutPanes(m_orientation);
    const qreal available = m_orientation == Qt::Horizontal ? availableWidth() : availableHeight();
    if (! split_layout::resizable(panes, available, index)) return false;
    const auto resized = split_layout::resize(panes, available, index, 0);
    if (resized.index < 0) return false;
    const auto local = contentHost()->mapFromScene(point);
    m_resize         = std::make_unique<SplitResizeState>(
        SplitResizeState { panes,
                           handle,
                           itemAt(resized.index),
                           index,
                           available,
                           m_orientation == Qt::Horizontal ? local.x() : local.y() });
    Q_EMIT resizingChanged();
    if (! guard || ! m_resize) return false;
    handleState(handle)->setPressed(true);
    return guard && m_resize;
}
void SplitView::moveResize(const QPointF& point) {
    if (! m_resize || ! m_resize->item) return;
    const auto local  = contentHost()->mapFromScene(point);
    const auto result = split_layout::resize(
        m_resize->panes,
        m_resize->available,
        m_resize->handleIndex,
        (m_orientation == Qt::Horizontal ? local.x() : local.y()) - m_resize->pressPosition);
    if (result.index < 0) return;
    QPointer<SplitView> guard(this);
    m_writing_preferred = true;
    auto* info          = attached(m_resize->item);
    if (m_orientation == Qt::Horizontal)
        info->setPreferredWidth(result.size);
    else
        info->setPreferredHeight(result.size);
    if (! guard) return;
    m_writing_preferred = false;
    updatePolish();
}
void SplitView::endResize() {
    if (! m_resize) return;
    const auto handle = m_resize->handle;
    m_resize.reset();
    if (m_destroying) return;
    QPointer<SplitView> guard(this);
    if (handle) handleState(handle)->setPressed(false);
    if (guard) Q_EMIT resizingChanged();
}
void SplitView::invalidateLayout() {
    QPointer<SplitView> guard(this);
    for (auto& state : m_panes) state->expandedSize.reset();
    endResize();
    if (guard) requestLayout();
}

SplitView::SplitView(QQuickItem* parent): Container(parent) {
    connect(this, &Control::availableWidthChanged, this, &SplitView::invalidateLayout);
    connect(this, &Control::availableHeightChanged, this, &SplitView::invalidateLayout);
    connect(this, &Control::contentItemChanged, this, &SplitView::invalidateLayout);
    connect(this, &QQuickItem::visibleChanged, this, [this] {
        QPointer<SplitView> guard(this);
        if (! isVisible()) settleExpansions(false);
        if (guard) invalidateLayout();
    });
    connect(this, &QQuickItem::enabledChanged, this, &SplitView::invalidateLayout);
    connect(this, &QQuickItem::windowChanged, this, &SplitView::invalidateLayout);
    connect(contentHost(), &QQuickItem::xChanged, this, &SplitView::invalidateLayout);
    connect(contentHost(), &QQuickItem::yChanged, this, &SplitView::invalidateLayout);
}
SplitView::~SplitView() {
    m_destroying = true;
    m_panes.clear();
    m_resize.reset();
    disconnect(this, nullptr, this, nullptr);
    beginTeardown();
    for (auto& connections : m_connections) utils::disconnectAll(connections);
    utils::disconnectAll(m_handle_connections);
    clearHandles();
}
SplitViewAttached* SplitView::qmlAttachedProperties(QObject* object) {
    return new SplitViewAttached(object);
}
void SplitView::requestLayout() {
    if (m_destroying) return;
    ++m_layout_revision;
    polish();
}
bool        SplitView::isContent(QQuickItem* item) const { return ! m_handles.contains(item); }
QQuickItem* SplitView::handleItemAt(int index) const { return m_handles.value(index); }
void        SplitView::setHandle(QQmlComponent* component) {
    if (m_destroying || m_handle == component) return;
    QPointer<SplitView> guard(this);
    endResize();
    if (! guard) return;
    utils::disconnectAll(m_handle_connections);
    m_handle        = component;
    m_handles_dirty = true;
    if (component) {
        m_handle_connections.append(
            connect(component, &QQmlComponent::statusChanged, this, &SplitView::requestLayout));
        m_handle_connections.append(connect(component, &QObject::destroyed, this, [this] {
            m_handle        = nullptr;
            m_handles_dirty = true;
            requestLayout();
            QPointer<SplitView> guard(this);
            endResize();
            if (! guard) return;
            Q_EMIT handleChanged();
        }));
    }
    requestLayout();
    Q_EMIT handleChanged();
}
void SplitView::clearHandles() {
    QPointer<SplitView> guard(this);
    const auto          handles = std::exchange(m_handles, {});
    for (auto handle : handles) {
        if (handle) {
            disconnect(handle, nullptr, this, nullptr);
            delete handle;
        }
        if (! guard) return;
    }
}
bool SplitView::syncHandles() {
    QPointer<SplitView> guard(this);
    const auto          contentRevision = revision();
    if (m_handles_dirty) {
        m_handles_dirty = false;
        clearHandles();
        if (! guard || m_handles_dirty || revision() != contentRevision) return false;
    }
    QPointer<QQmlComponent> component = m_handle;
    const int wanted = component && component->isReady() ? std::max(0, count() - 1) : 0;
    while (m_handles.size() > wanted) {
        auto handle = m_handles.takeLast();
        if (handle) {
            disconnect(handle, nullptr, this, nullptr);
            delete handle;
        }
        if (! guard || m_handles_dirty || revision() != contentRevision) return false;
    }
    while (m_handles.size() < wanted) {
        auto*                context = component->creationContext();
        QPointer<QObject>    object  = component->beginCreate(context ? context : qmlContext(this));
        const bool           began   = object;
        QPointer<QQuickItem> item    = qobject_cast<QQuickItem*>(object.data());
        if (guard && item) {
            m_handles.append(item);
            QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
            item->setParent(this);
            if (guard && item) item->setParentItem(this);
            if (guard && item) item->setZ(1);
        } else if (guard) {
            m_handles.append(nullptr);
        }
        // Complete every successful beginCreate, even if a callback invalidates the owner.
        if (component && began) component->completeCreate();
        if (! guard) {
            if (object) delete object;
            return false;
        }
        if (component && component->isError()) {
            qmlWarning(this) << component->errorString();
            if (object) delete object;
            return false;
        }
        if (! item && object) {
            qmlWarning(this) << "SplitView handle must be an Item";
            delete object;
            if (! guard) return false;
        }
        if (item) {
            new SplitHandleInput(this, item);
            connect(item, &QQuickItem::implicitWidthChanged, this, &SplitView::invalidateLayout);
            connect(item, &QQuickItem::implicitHeightChanged, this, &SplitView::invalidateLayout);
            connect(item, &QQuickItem::parentChanged, this, [this] {
                m_handles_dirty = true;
                invalidateLayout();
            });
            connect(item, &QObject::destroyed, this, [this] {
                m_handles_dirty = true;
                invalidateLayout();
            });
        }
        if (! component || m_handle != component || m_handles_dirty ||
            revision() != contentRevision)
            return false;
    }
    return true;
}
void SplitView::componentComplete() {
    QPointer<SplitView> guard(this);
    Container::componentComplete();
    if (guard) settleExpansions(false);
}
void SplitView::setExpandTransition(QQuickTransition* value) {
    if (m_expand == value) return;
    m_expand = value;
    Q_EMIT transitionsChanged();
}
void SplitView::setCollapseTransition(QQuickTransition* value) {
    if (m_collapse == value) return;
    m_collapse = value;
    Q_EMIT transitionsChanged();
}
void SplitView::changeExpanded(QQuickItem* item) {
    const auto state = m_panes.value(item);
    if (! state || ! state->info) return;
    const auto generation = ++state->generation;
    if (! state->info->isTransitioning()) state->expandedSize.reset();
    state->motion.cancel();
    state->pending = false;
    if (! isComponentComplete() || ! isVisible() ||
        ! QQuickItemPrivate::get(item)->explicitVisible) {
        cancelExpansion(item);
        return;
    }
    QPointer<SplitView> guard(this);
    endResize();
    if (! guard || state->generation != generation || ! state->info) return;
    if (state->host) state->host->setEnabled(state->info->isExpanded());
    if (! guard || state->generation != generation || ! state->info) return;
    if (! state->info->m_transitioning) {
        state->info->m_transitioning = true;
        Q_EMIT state->info->transitioningChanged();
    }
    if (! guard || state->generation != generation || ! state->info) return;
    QPointer<QQuickTransition> transition = state->info->isExpanded() ? m_expand : m_collapse;
    if (transition && ! transition->enabled()) transition = nullptr;
    if (transition) {
        qmlExecuteDeferred(transition);
        if (! guard || state->generation != generation || ! state->info) return;
    }
    state->motion.completed = [guard, item, generation] {
        if (! guard) return;
        const auto current = guard->m_panes.value(item);
        if (! current || current->generation != generation) return;
        current->pending = true;
        guard->requestLayout();
    };
    requestLayout();
    // The Transition animates the private proxy's "progress", never pane geometry.
    state->motion.transition({ QQuickStateAction(&state->target,
                                                 QStringLiteral("progress"),
                                                 state->info->isExpanded() ? 1.0 : 0.0) },
                             transition,
                             &state->target);
}
void SplitView::cancelExpansion(QQuickItem* item) {
    const auto state = m_panes.value(item);
    if (! state || ! state->info) return;
    ++state->generation;
    state->motion.cancel();
    state->pending = false;
    state->target.setProgress(state->info->isExpanded() ? 1 : 0);
    if (state->info->m_transitioning) {
        state->info->m_transitioning = false;
        Q_EMIT state->info->transitioningChanged();
    }
}
void SplitView::completeExpansion(QQuickItem* item, quint64 generation) {
    const auto state = m_panes.value(item);
    if (! state || ! state->info || state->generation != generation) return;
    state->pending = false;
    state->motion.cancel();
    state->info->m_transitioning = false;
    QPointer<SplitView> guard(this);
    requestLayout();
    Q_EMIT state->info->transitioningChanged();
    if (! guard || ! state->info || state->generation != generation) return;
    if (state->info->isExpanded())
        Q_EMIT state->info->expandedCompleted();
    else
        Q_EMIT state->info->collapsedCompleted();
}
void SplitView::settleExpansions(bool notify) {
    const auto          snapshot = items();
    QPointer<SplitView> guard(this);
    for (auto item : snapshot) {
        const auto state = m_panes.value(item);
        if (! state || ! state->info) continue;
        if (notify && state->info->isTransitioning()) {
            state->motion.cancel();
            state->target.setProgress(state->info->isExpanded() ? 1 : 0);
            state->pending = true;
            requestLayout();
        } else {
            cancelExpansion(item);
        }
        if (! guard) return;
    }
}
void SplitView::setOrientation(Qt::Orientation value) {
    if ((value != Qt::Horizontal && value != Qt::Vertical) || m_orientation == value) return;
    m_orientation = value;
    QPointer<SplitView> guard(this);
    settleExpansions(true);
    if (! guard) return;
    invalidateLayout();
    if (! guard) return;
    Q_EMIT orientationChanged();
}
void SplitView::itemAdded(QQuickItem* item) {
    QPointer<SplitView> guard(this);
    auto                state = std::make_shared<SplitPaneState>();
    state->item               = item;
    state->info               = attached(item);
    state->host               = new SplitPaneHost;
    state->target.setProgress(state->info->isExpanded() ? 1 : 0);
    state->target.changed = [guard = QPointer<SplitView>(this)] {
        if (guard) guard->requestLayout();
    };
    m_panes.insert(item, state);
    setPresentationHost(item, state->host);
    if (! guard || ! state->item || ! state->host) return;
    state->host->setZ(item->z());
    auto& connections = m_connections[item];
    connections.append(connect(item, &QQuickItem::zChanged, this, [state] {
        if (state->item && state->host) state->host->setZ(state->item->z());
    }));
    connections.append(connect(item, &QObject::destroyed, this, [this, item] {
        if (auto state = m_panes.take(item)) {
            state->target.changed = {};
            state->motion.cancel();
        }
        utils::disconnectAll(m_connections[item]);
        m_connections.remove(item);
        invalidateLayout();
    }));
    connections.append(
        connect(attached(item), &SplitViewAttached::constraintsChanged, this, [this] {
            if (m_writing_preferred) {
                m_writing_preferred = false;
                requestLayout();
            } else
                invalidateLayout();
        }));
    connections.append(
        connect(item, &QQuickItem::implicitWidthChanged, this, &SplitView::invalidateLayout));
    connections.append(
        connect(item, &QQuickItem::implicitHeightChanged, this, &SplitView::invalidateLayout));
    connections.append(connect(item, &QQuickItem::visibleChanged, this, [this, item] {
        if (m_updating_hosts) return;
        QPointer<SplitView> guard(this);
        if (! item->isVisible()) cancelExpansion(item);
        if (guard) invalidateLayout();
    }));
}
void SplitView::itemRemoved(QQuickItem* item) {
    auto connections = m_connections.take(item);
    utils::disconnectAll(connections);
    if (auto state = m_panes.take(item)) {
        state->target.changed = {};
        state->motion.cancel();
        if (state->info && state->info->m_transitioning) {
            state->info->m_transitioning = false;
            Q_EMIT state->info->transitioningChanged();
        }
    }
}
void                      SplitView::itemsChanged() { invalidateLayout(); }
QList<split_layout::Pane> SplitView::layoutPanes(Qt::Orientation orientation) const {
    const bool                horizontal = orientation == Qt::Horizontal;
    QList<split_layout::Pane> panes;
    for (int i = 0; i < count(); ++i) {
        auto*              item = itemAt(i);
        split_layout::Pane pane;
        pane.visible = item && QQuickItemPrivate::get(item)->explicitVisible &&
                       (attached(item)->isExpanded() || attached(item)->isTransitioning());
        if (item) {
            auto* info        = attached(item);
            pane.implicitSize = horizontal ? item->implicitWidth() : item->implicitHeight();
            pane.preferred    = horizontal ? info->m_preferred_width : info->m_preferred_height;
            pane.minimum      = horizontal ? info->minimumWidth() : info->minimumHeight();
            pane.maximum      = horizontal ? info->maximumWidth() : info->maximumHeight();
            pane.fill         = horizontal ? info->fillWidth() : info->fillHeight();
        }
        if (orientation == m_orientation) {
            if (auto* handle = handleItemAt(i))
                pane.handleSize = horizontal ? handle->implicitWidth() : handle->implicitHeight();
        }
        panes.append(pane);
    }
    return panes;
}
void SplitView::updatePolish() {
    if (! isComponentComplete() || ! isVisible()) return;
    if (m_laying_out) {
        requestLayout();
        return;
    }
    QPointer<SplitView> guard(this);
    m_laying_out = true;
    Container::updatePolish();
    if (! guard) return;
    if (! syncHandles()) {
        if (guard) {
            m_laying_out = false;
            polish();
        }
        return;
    }
    const auto   contentRevision = revision();
    const auto   layoutRevision  = m_layout_revision;
    const auto   snapshot        = items();
    const bool   horizontal      = m_orientation == Qt::Horizontal;
    const auto   widths          = layoutPanes(Qt::Horizontal);
    const auto   heights         = layoutPanes(Qt::Vertical);
    const auto   normalX         = split_layout::calculate(widths, availableWidth());
    const auto   normalY         = split_layout::calculate(heights, availableHeight());
    const auto&  normal          = horizontal ? normalX : normalY;
    QList<qreal> expandedSizes;
    QList<qreal> progress;
    bool         transitioning = false;
    for (auto item : snapshot) {
        const auto state = m_panes.value(item);
        progress.append(state ? state->target.progress() : 1);
        const auto size = normal.panes[expandedSizes.size()].size;
        if (state && state->info && state->info->isTransitioning()) {
            if (! state->expandedSize) state->expandedSize = size;
            expandedSizes.append(*state->expandedSize);
        } else {
            expandedSizes.append(size);
        }
        transitioning |= state && state->info && state->info->isTransitioning();
    }
    const auto  x = horizontal && transitioning
                        ? split_layout::reveal(widths, progress, expandedSizes, availableWidth())
                        : normalX;
    const auto  y = ! horizontal && transitioning
                        ? split_layout::reveal(heights, progress, expandedSizes, availableHeight())
                        : normalY;
    const auto& main          = horizontal ? x : y;
    const auto& cross         = horizontal ? y : x;
    qreal       crossImplicit = 0;
    for (const auto& pane : cross.panes)
        crossImplicit = std::max(crossImplicit, pane.preferredSize);
    const QSizeF implicitSize = horizontal ? QSizeF(main.implicitExtent, crossImplicit)
                                           : QSizeF(crossImplicit, main.implicitExtent);
    const QSizeF available(availableWidth(), availableHeight());
    // Geometry callbacks may remove/reparent content or replace constraints synchronously.
    const auto valid = [&] {
        return guard && contentRevision == revision() && layoutRevision == m_layout_revision;
    };
    for (int i = 0; i < snapshot.size() && valid(); ++i) {
        auto item = snapshot[i];
        if (! item) continue;
        const auto state = m_panes.value(item);
        if (! state || ! state->host || ! state->info) continue;
        const auto&  geometry = main.panes[i];
        const QRectF bounds(horizontal ? QPointF(geometry.position, 0)
                                       : QPointF(0, geometry.position),
                            horizontal ? QSizeF(geometry.size, available.height())
                                       : QSizeF(available.width(), geometry.size));
        m_updating_hosts = true;
        state->host->setBounds(bounds);
        state->host->setClip(state->info->isTransitioning());
        if (! guard) return;
        if (! valid() || ! state->host || ! state->info) {
            m_updating_hosts = false;
            break;
        }
        state->host->setSize(available);
        if (! guard) return;
        if (! valid() || ! state->host || ! state->info) {
            m_updating_hosts = false;
            break;
        }
        // Do not hide the ancestor for an explicit visible=false on the pane:
        // changing it back to true would otherwise emit no effective visibility signal.
        state->host->setVisible(state->info->isExpanded() || state->info->isTransitioning());
        if (! guard) return;
        if (state->host && state->info) state->host->setEnabled(state->info->isExpanded());
        if (! guard) return;
        m_updating_hosts = false;
        if (! valid() || ! item) break;
        item->setSize(horizontal ? QSizeF(geometry.size, available.height())
                                 : QSizeF(available.width(), geometry.size));
        if (! valid()) break;
        if (item)
            item->setPosition(horizontal ? QPointF(geometry.position, 0)
                                         : QPointF(0, geometry.position));
    }
    if (! guard) return;
    const auto handles = m_handles;
    for (int i = 0; i < handles.size() && valid(); ++i) {
        auto handle = handles[i];
        if (! handle) continue;
        const auto& geometry = main.panes[i];
        handle->setVisible(geometry.handleVisible);
        if (! valid()) break;
        if (handle)
            handleState(handle)->setResizable(
                geometry.handleVisible &&
                split_layout::resizable(horizontal ? widths : heights,
                                        horizontal ? availableWidth() : availableHeight(),
                                        i));
        if (! valid()) break;
        if (! handle || ! geometry.handleVisible) continue;
        handle->setSize(horizontal ? QSizeF(geometry.handleSize, available.height())
                                   : QSizeF(available.width(), geometry.handleSize));
        if (! valid()) break;
        if (handle)
            handle->setPosition(mapFromItem(contentHost(),
                                            horizontal ? QPointF(geometry.handlePosition, 0)
                                                       : QPointF(0, geometry.handlePosition)));
    }
    if (valid()) contentHost()->setSize(available);
    if (valid()) setImplicitContentSize(implicitSize);
    if (! guard) return;
    m_laying_out = false;
    if (! valid()) polish();
    if (! valid()) return;
    const auto states = m_panes;
    for (auto it = states.cbegin(); it != states.cend(); ++it) {
        if (it.value()->pending) completeExpansion(it.key(), it.value()->generation);
        if (! guard) return;
    }
}

SplitViewAttached::SplitViewAttached(QObject* object): QObject(object) {
    if (auto* item = qobject_cast<QQuickItem*>(object)) {
        m_container =
            qobject_cast<ContainerAttached*>(qmlAttachedPropertiesObject<Container>(item, true));
        connect(m_container,
                &ContainerAttached::containerChanged,
                this,
                &SplitViewAttached::viewChanged);
    }
}
SplitView* SplitViewAttached::view() const {
    return m_container ? qobject_cast<SplitView*>(m_container->container()) : nullptr;
}

#define SPLIT_BOUND(Name, property, member, fallback, allowInfinity)                         \
    void SplitViewAttached::set##Name(qreal value) {                                         \
        if (std::isnan(value) || value < 0 || (! allowInfinity && ! std::isfinite(value)) || \
            member == value)                                                                 \
            return;                                                                          \
        member = value;                                                                      \
        QPointer<SplitViewAttached> guard(this);                                             \
        Q_EMIT constraintsChanged();                                                         \
        if (guard) Q_EMIT property##Changed();                                               \
    }                                                                                        \
    void SplitViewAttached::reset##Name() { set##Name(fallback); }

SPLIT_BOUND(MinimumWidth, minimumWidth, m_minimum_width, 0, false)
SPLIT_BOUND(MinimumHeight, minimumHeight, m_minimum_height, 0, false)
SPLIT_BOUND(MaximumWidth, maximumWidth, m_maximum_width, std::numeric_limits<qreal>::infinity(),
            true)
SPLIT_BOUND(MaximumHeight, maximumHeight, m_maximum_height, std::numeric_limits<qreal>::infinity(),
            true)

#undef SPLIT_BOUND

#define SPLIT_PREFERRED(Name, property, member)                             \
    void SplitViewAttached::set##Name(qreal value) {                        \
        if (! std::isfinite(value) || value < 0 || member == value) return; \
        member = value;                                                     \
        QPointer<SplitViewAttached> guard(this);                            \
        Q_EMIT constraintsChanged();                                        \
        if (guard) Q_EMIT property##Changed();                              \
    }                                                                       \
    void SplitViewAttached::reset##Name() {                                 \
        if (! member) return;                                               \
        member.reset();                                                     \
        QPointer<SplitViewAttached> guard(this);                            \
        Q_EMIT constraintsChanged();                                        \
        if (guard) Q_EMIT property##Changed();                              \
    }

SPLIT_PREFERRED(PreferredWidth, preferredWidth, m_preferred_width)
SPLIT_PREFERRED(PreferredHeight, preferredHeight, m_preferred_height)

#undef SPLIT_PREFERRED

void SplitViewAttached::setFillWidth(bool value) {
    if (m_fill_width == value) return;
    m_fill_width = value;
    QPointer<SplitViewAttached> guard(this);
    Q_EMIT constraintsChanged();
    if (guard) Q_EMIT fillWidthChanged();
}
void SplitViewAttached::setFillHeight(bool value) {
    if (m_fill_height == value) return;
    m_fill_height = value;
    QPointer<SplitViewAttached> guard(this);
    Q_EMIT constraintsChanged();
    if (guard) Q_EMIT fillHeightChanged();
}

void SplitViewAttached::setExpanded(bool value) {
    if (m_expanded == value) return;
    m_expanded = value;
    QPointer<SplitViewAttached> guard(this);
    Q_EMIT expandedChanged();
    if (! guard || m_expanded != value) return;
    if (auto* owner = view()) owner->changeExpanded(qobject_cast<QQuickItem*>(parent()));
}

} // namespace qml_material

#include "split_view.moc"
