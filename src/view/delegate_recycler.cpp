#include "delegate_recycler.hpp"
#include <algorithm>
#include <QQuickWindow>

namespace qml_material
{
class DelegateRecycler::Incubator final : public QQmlIncubator {
public:
    Incubator(DelegateRecycler* owner, const std::shared_ptr<Entry>& entry, IncubationMode mode)
        : QQmlIncubator(mode), m_owner(owner), m_entry(entry) {}

protected:
    void setInitialState(QObject* object) override {
        auto value = m_entry.lock();
        if (! value || value->cancelled) return;
        value->object = object;
        value->item   = qobject_cast<QQuickItem*>(object);
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
        if (! m_owner || ! value->item) return;
        value->itemIdentity = value->item;
        m_owner->m_items.insert(value->item, value);
        m_owner->place(value, value->placement);
    }
    void statusChanged(Status status) override {
        if (status != Ready && status != Error) return;
        auto value = m_entry.lock();
        if (! value || value->cancelled) return;
        if (status == Error) {
            QStringList messages;
            for (const auto& error : errors()) messages.append(error.toString());
            value->error = messages.join('\n');
        } else if (! value->item) {
            value->error = QStringLiteral("Delegate creation failed or root is not an Item");
        }
        if (m_owner) m_owner->notifyReady();
    }

private:
    QPointer<DelegateRecycler> m_owner;
    std::weak_ptr<Entry>       m_entry;
};

DelegateRecycler::DelegateRecycler(QQmlEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {}
DelegateRecycler::~DelegateRecycler() { clear(); }
DelegateRecycler::Entry::~Entry() {
    if (incubator) incubator->clear();
    delete object.data();
    delete host.data();
    delete row.data();
}
std::shared_ptr<DelegateRecycler::Entry> DelegateRecycler::entry(QQuickItem* item) const {
    return item ? m_items.value(item).lock() : nullptr;
}
LazyRow* DelegateRecycler::row(QQuickItem* item) const {
    auto value = entry(item);
    return value ? value->row.data() : nullptr;
}
void DelegateRecycler::attach(QQuickItem* item, QQuickItem* parent) {
    auto value = entry(item);
    if (value && value->host && ! value->idle && ! value->updating)
        value->host->setParentItem(parent);
}
int DelegateRecycler::poolSize() const {
    return int(std::count_if(m_entries.cbegin(), m_entries.cend(), [](const auto& e) {
        return e->idle;
    }));
}
int DelegateRecycler::pendingCount() const {
    return int(std::count_if(m_pending.cbegin(), m_pending.cend(), [](const auto& e) {
        return e->error.isEmpty();
    }));
}
bool DelegateRecycler::pending(const QString& key) const {
    const auto value = m_pending.value(key);
    return value && value->incubator && value->incubator->isLoading();
}
void DelegateRecycler::notifyReady() {
    if (m_clearing || m_readyQueued) return;
    m_readyQueued         = true;
    const auto generation = m_generation;
    QMetaObject::invokeMethod(
        this,
        [this, generation] {
            if (generation != m_generation) return;
            m_readyQueued = false;
            if (m_ready) m_ready();
        },
        Qt::QueuedConnection);
}
void DelegateRecycler::layout(QQuickItem* item, const QRectF& geometry, Qt::Orientation orientation,
                              bool fixedExtent) {
    auto value = entry(item);
    if (! value || ! value->host || ! value->item) return;
    value->host->setPosition(geometry.topLeft());
    if (! value->host || ! value->item) return;
    value->host->setSize(geometry.size());
    if (! value->item) return;
    if (fixedExtent)
        value->item->setSize(geometry.size());
    else if (orientation == Qt::Vertical)
        value->item->setWidth(geometry.width());
    else
        value->item->setHeight(geometry.height());
}
bool DelegateRecycler::place(const std::shared_ptr<Entry>& value, const Placement& placement) {
    QPointer<DelegateRecycler> guard(this);
    const auto                 generation = m_generation;
    const auto                 current    = [&] {
        return guard && m_generation == generation && ! value->cancelled && value->host &&
               value->item;
    };
    if (! current()) return false;
    if (placement.valid) {
        value->host->setParentItem(placement.parent);
        if (! current()) return false;
        value->host->setPosition(placement.geometry.topLeft());
        if (! current()) return false;
        value->host->setSize(placement.geometry.size());
        if (! current()) return false;
    }
    value->item->setParentItem(value->host);
    if (! current()) return false;
    if (placement.valid) {
        if (placement.fixedExtent)
            value->item->setSize(placement.geometry.size());
        else if (placement.orientation == Qt::Vertical)
            value->item->setWidth(placement.geometry.width());
        else
            value->item->setHeight(placement.geometry.height());
    }
    return current();
}
void DelegateRecycler::retire(const std::shared_ptr<Entry>& value, QQmlEngine* engine) {
    value->cancelled = true;
    if (! value->incubator || ! value->incubator->isLoading()) return;
    value->incubator->clear();
    // Cancellation can occur inside Component.onCompleted; retain its row through that callback.
    if (engine)
        QMetaObject::invokeMethod(
            engine,
            [value] {
            },
            Qt::QueuedConnection);
}
void DelegateRecycler::remove(const std::shared_ptr<Entry>& value) {
    if (m_pending.value(value->key) == value) m_pending.remove(value->key);
    if (value->itemIdentity) m_items.remove(value->itemIdentity);
    m_entries.removeOne(value);
    retire(value, m_engine);
}
void DelegateRecycler::clear() {
    if (m_clearing) return;
    m_clearing = true;
    ++m_generation;
    m_readyQueued = false;
    auto entries  = std::exchange(m_entries, {});
    m_pending.clear();
    m_items.clear();
    QPointer<DelegateRecycler> guard(this);
    QPointer<QQmlEngine>       engine(m_engine);
    for (const auto& value : entries) value->cancelled = true;
    for (const auto& value : entries) retire(value, engine);
    entries.clear();
    if (guard) m_clearing = false;
}
void DelegateRecycler::cancelPreparations() {
    QPointer<DelegateRecycler> guard(this);
    const auto                 entries = m_pending.values();
    for (const auto& value : entries) {
        remove(value);
        if (! guard) return;
    }
}
void DelegateRecycler::finishPreparation(const std::shared_ptr<Entry>& value) {
    if (m_pending.value(value->key) == value) m_pending.remove(value->key);
    value->preparing = false;
    value->incubator->clear();
    value->incubator.reset();
}
void DelegateRecycler::retainPreparations(QQmlComponent* component, const ItemSnapshotPtr& snapshot,
                                          const QSet<QString>& wanted) {
    QPointer<DelegateRecycler> guard(this);
    const auto                 entries = m_pending.values();
    for (const auto& value : entries) {
        const int  index = snapshot ? snapshot->indexOfKey(value->key) : -1;
        const bool keep =
            value->component == component && wanted.contains(value->key) && value->row &&
            index >= 0 && snapshot->descriptorAt(index).reuseType == value->type &&
            snapshot->descriptorAt(index).contentRevision == value->row->contentRevision();
        if (! keep) {
            if (value->component == component && value->error.isEmpty() && value->incubator &&
                value->incubator->isReady() && value->item && value->row && value->host) {
                finishPreparation(value);
                release(value->item);
            } else {
                remove(value);
            }
        }
        if (! guard) return;
    }
}
void DelegateRecycler::setPoolLimit(int limit) {
    m_poolLimit = std::max(0, limit);
    trim();
}
void DelegateRecycler::beginUpdate() { ++m_updateDepth; }
void DelegateRecycler::endUpdate() {
    Q_ASSERT(m_updateDepth > 0);
    if (--m_updateDepth == 0) trim();
}
void DelegateRecycler::trim() {
    QPointer<DelegateRecycler> guard(this);
    while (guard) {
        const bool overflow = m_updateDepth == 0 && poolSize() > m_poolLimit;
        auto       it = std::find_if(m_entries.begin(), m_entries.end(), [overflow](const auto& e) {
            return ! e->row || ! e->host || ! e->component || (! e->preparing && ! e->item) ||
                   (overflow && e->idle);
        });
        if (it == m_entries.end()) return;
        auto value = *it;
        remove(value);
        value.reset();
    }
}
QQuickItem* DelegateRecycler::acquire(QQmlComponent* component, const ItemSnapshotPtr& snapshot,
                                      int index) {
    return acquire(component, snapshot, index, Placement {}, false);
}
QQuickItem* DelegateRecycler::acquire(QQmlComponent* component, const ItemSnapshotPtr& snapshot,
                                      int index, const Placement& placement, bool asynchronous) {
    m_error.clear();
    if (m_clearing || ! m_engine || ! component || ! component->isReady() || ! snapshot ||
        index < 0 || index >= snapshot->count()) {
        m_error = QStringLiteral("Invalid delegate or row");
        return nullptr;
    }
    QPointer<DelegateRecycler> guard(this);
    QPointer<QQmlComponent>    componentGuard(component);
    const auto                 generation = m_generation;
    const auto                 current    = [&] {
        return guard && m_generation == generation;
    };
    trim();
    if (! current() || ! componentGuard || ! m_engine) return nullptr;
    const auto& descriptor = snapshot->descriptorAt(index);
    auto        value      = m_pending.value(descriptor.key);
    const bool  reuseReady = value && value->incubator && value->incubator->isLoading() &&
                             std::any_of(m_entries.cbegin(), m_entries.cend(), [&](const auto& e) {
                                return e->idle && ! e->updating && e->item && e->host && e->row &&
                                       e->component == component && e->type == descriptor.reuseType;
                             });
    if (value &&
        (value->component != component || value->type != descriptor.reuseType || ! value->row ||
         value->row->contentRevision() != descriptor.contentRevision || reuseReady)) {
        remove(value);
        value.reset();
        if (! current()) return nullptr;
    }
    if (! value) {
        for (const auto& e : std::as_const(m_entries))
            if (e->idle && ! e->updating && e->item && e->host && e->row &&
                e->component == component && e->type == descriptor.reuseType) {
                value = e;
                break;
            }
        if (value) {
            value->idle = false;
            if (! rebind(value->item, snapshot, index, placement) || ! current()) return nullptr;
            Q_EMIT value->row->reused();
            if (! current() || ! value->item || ! value->row || ! value->host) return nullptr;
        } else {
            value            = std::make_shared<Entry>();
            value->component = component;
            value->key       = descriptor.key;
            value->type      = descriptor.reuseType;
            value->placement = placement;
            value->preparing = true;
            value->host      = new QQuickItem;
            value->host->setVisible(false);
            value->host->setEnabled(false);
            if (placement.valid) {
                value->host->setParentItem(placement.parent);
                value->host->setPosition(placement.geometry.topLeft());
                value->host->setSize(placement.geometry.size());
            }
            value->row = new LazyRow;
            QQmlEngine::setObjectOwnership(value->row, QQmlEngine::CppOwnership);
            value->row->bind(snapshot, index, placement.view, placement.usage);
            if (! current() || ! value->row || ! value->host) return nullptr;
            const auto mode =
                asynchronous ? QQmlIncubator::Asynchronous : QQmlIncubator::Synchronous;
            value->incubator = std::make_shared<Incubator>(this, value, mode);
            value->incubator->setInitialProperties(
                { { QStringLiteral("row"), QVariant::fromValue(value->row.data()) } });
            m_entries.append(value);
            m_pending.insert(value->key, value);
            auto context = component->creationContext();
            if (! context) context = m_engine->rootContext();
            const auto incubator = value->incubator;
            component->create(*incubator, context);
            if (! current() || value->cancelled) return nullptr;
        }
    }
    if (value->preparing) {
        const auto incubator = value->incubator;
        if (incubator->isLoading() && ! asynchronous) incubator->forceCompletion();
        if (! current() || value->cancelled) return nullptr;
        if (incubator->isLoading()) return nullptr;
        if (! value->error.isEmpty() || ! incubator->isReady() || ! value->item) {
            m_error = value->error.isEmpty()
                          ? QStringLiteral("Delegate creation failed or root is not an Item")
                          : value->error;
            return nullptr;
        }
        finishPreparation(value);
        if (value->row->revision() != snapshot->revision || value->row->index() != index ||
            value->row->view() != placement.view || value->row->usage() != placement.usage) {
            if (! rebind(value->item, snapshot, index, placement) || ! current()) return nullptr;
        } else if (! place(value, placement) || ! current()) {
            return nullptr;
        }
    }
    if (! current() || ! value->item || ! value->row || ! value->host) return nullptr;
    value->host->setEnabled(true);
    if (! current() || ! value->item || ! value->host) return nullptr;
    value->host->setVisible(true);
    if (! current() || ! value->item || ! value->host) return nullptr;
    return value->item;
}
bool DelegateRecycler::rebind(QQuickItem* item, const ItemSnapshotPtr& snapshot, int index) {
    return rebind(item, snapshot, index, Placement {});
}
bool DelegateRecycler::rebind(QQuickItem* item, const ItemSnapshotPtr& snapshot, int index,
                              const Placement& placement) {
    auto value = entry(item);
    if (! value || value->idle || value->updating || ! value->row || ! value->component ||
        ! snapshot || index < 0 || index >= snapshot->count() ||
        value->type != snapshot->descriptorAt(index).reuseType)
        return false;
    QPointer<DelegateRecycler> guard(this);
    const auto                 generation = m_generation;
    value->updating                       = true;
    if (! place(value, placement) || ! guard || m_generation != generation) return false;
    value->row->bind(snapshot, index, placement.view, placement.usage);
    if (! guard || m_generation != generation || value->cancelled || ! value->item || ! value->row)
        return false;
    value->key      = snapshot->descriptorAt(index).key;
    value->updating = false;
    return true;
}
void DelegateRecycler::release(QQuickItem* item) {
    auto value = entry(item);
    if (! value || value->idle || value->updating || ! value->item || ! value->row) return;
    QPointer<DelegateRecycler> guard(this);
    const auto                 generation = m_generation;
    const auto                 current    = [&] {
        return guard && m_generation == generation && ! value->cancelled && value->item &&
               value->host && value->row;
    };
    value->updating = true;
    item->ungrabMouse();
    if (! current()) return;
    item->ungrabTouchPoints();
    if (! current()) return;
    if (auto window = item->window()) {
        auto focused = window->activeFocusItem();
        if (focused && item->isAncestorOf(focused)) focused->setFocus(false);
        if (! current()) return;
    }
    value->row->pool();
    if (! current()) return;
    value->host->setEnabled(false);
    if (! current()) return;
    item->setFocus(false);
    if (! current()) return;
    value->host->setVisible(false);
    if (! current()) return;
    value->host->setParentItem(nullptr);
    if (! current()) return;
    Q_EMIT value->row->pooled();
    if (! current()) return;
    value->updating = false;
    value->idle     = true;
    trim();
}
} // namespace qml_material
