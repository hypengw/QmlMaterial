#include "delegate_recycler.hpp"
#include <algorithm>
#include <QQuickWindow>

namespace qml_material
{
DelegateRecycler::DelegateRecycler(QQmlEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {}
DelegateRecycler::~DelegateRecycler() { clear(); }
DelegateRecycler::Entry::~Entry() {
    delete item.data();
    delete host.data();
    delete row.data();
}
std::shared_ptr<DelegateRecycler::Entry> DelegateRecycler::entry(QQuickItem* item) const {
    if (! item) return {};
    for (const auto& value : m_entries)
        if (value->item == item) return value;
    return {};
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
void DelegateRecycler::layout(QQuickItem* item, const QRectF& geometry) {
    auto value = entry(item);
    if (! value || ! value->host || ! value->item) return;
    value->host->setPosition(geometry.topLeft());
    if (! value->host || ! value->item) return;
    value->host->setSize(geometry.size());
    if (! value->item) return;
    value->item->setWidth(geometry.width());
}
void DelegateRecycler::clear() {
    if (m_clearing) return;
    m_clearing = true;
    ++m_generation;
    auto                       entries = std::exchange(m_entries, {});
    QPointer<DelegateRecycler> guard(this);
    entries.clear();
    if (guard) m_clearing = false;
}
void DelegateRecycler::setPoolLimit(int limit) {
    m_poolLimit = std::max(0, limit);
    trim();
}
void DelegateRecycler::trim() {
    QPointer<DelegateRecycler> guard(this);
    while (guard) {
        const bool overflow = poolSize() > m_poolLimit;
        auto       it = std::find_if(m_entries.begin(), m_entries.end(), [overflow](const auto& e) {
            return ! e->item || ! e->row || ! e->host || ! e->component || (overflow && e->idle);
        });
        if (it == m_entries.end()) return;
        auto removed = *it;
        m_entries.erase(it);
        removed.reset();
    }
}
QQuickItem* DelegateRecycler::acquire(QQmlComponent* component, const ItemSnapshotPtr& snapshot,
                                      int index) {
    m_error.clear();
    if (m_clearing || ! m_engine || ! component || ! component->isReady() || ! snapshot ||
        index < 0 || index >= snapshot->rows.size()) {
        m_error = QStringLiteral("Invalid delegate or row");
        return nullptr;
    }
    QPointer<DelegateRecycler> guard(this);
    QPointer<QQmlComponent>    componentGuard(component);
    const auto                 generation = m_generation;
    const auto                 current    = [&] {
        return guard && guard->m_generation == generation;
    };
    trim();
    if (! current() || ! componentGuard || ! m_engine) return nullptr;
    const auto             type = snapshot->rows[index].reuseType;
    std::shared_ptr<Entry> value;
    for (const auto& e : std::as_const(m_entries))
        if (e->idle && ! e->updating && e->item && e->host && e->row && e->component == component &&
            e->type == type) {
            value = e;
            break;
        }
    if (value) {
        value->idle     = false;
        value->updating = true;
        value->row->bind(snapshot, index);
        if (! current() || ! value->item || ! value->row || ! value->host) return nullptr;
        Q_EMIT value->row->reused();
        if (! current() || ! value->item || ! value->row || ! value->host) return nullptr;
        value->host->setEnabled(true);
        if (! current() || ! value->item || ! value->host) return nullptr;
        value->host->setVisible(true);
        if (! current() || ! value->item || ! value->host) return nullptr;
        value->updating = false;
        return value->item;
    }
    value            = std::make_shared<Entry>();
    value->component = component;
    value->type      = type;
    value->host      = new QQuickItem;
    value->row       = new LazyRow;
    QQmlEngine::setObjectOwnership(value->row, QQmlEngine::CppOwnership);
    value->row->bind(snapshot, index);
    auto context = component->creationContext();
    if (! context) context = m_engine->rootContext();
    QPointer<QObject> object = component->createWithInitialProperties(
        { { QStringLiteral("row"), QVariant::fromValue(value->row.data()) } }, context);
    value->item = qobject_cast<QQuickItem*>(object.data());
    if (! value->item) {
        delete object.data();
        if (current()) m_error = QStringLiteral("Delegate creation failed or root is not an Item");
        return nullptr;
    }
    QQmlEngine::setObjectOwnership(value->item, QQmlEngine::CppOwnership);
    if (! current() || ! value->row || ! value->component) return nullptr;
    value->item->setParentItem(value->host);
    if (! current() || ! value->item || ! value->host || ! value->row) return nullptr;
    m_entries.append(value);
    return value->item;
}
bool DelegateRecycler::rebind(QQuickItem* item, const ItemSnapshotPtr& snapshot, int index) {
    auto value = entry(item);
    if (! value || value->idle || value->updating || ! value->row || ! value->component ||
        ! snapshot || index < 0 || index >= snapshot->rows.size() ||
        value->type != snapshot->rows[index].reuseType)
        return false;
    QPointer<DelegateRecycler> guard(this);
    const auto                 generation = m_generation;
    value->updating                       = true;
    value->row->bind(snapshot, index);
    if (! guard || m_generation != generation || ! value->item || ! value->row) return false;
    value->updating = false;
    return true;
}
void DelegateRecycler::release(QQuickItem* item) {
    auto value = entry(item);
    if (! value || value->idle || value->updating || ! value->item || ! value->row) return;
    QPointer<DelegateRecycler> guard(this);
    const auto                 generation = m_generation;
    const auto                 current    = [&] {
        return guard && guard->m_generation == generation && value->item && value->host &&
               value->row;
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
    value->host->setEnabled(false);
    if (! current()) return;
    item->setFocus(false);
    if (! current()) return;
    value->host->setVisible(false);
    if (! current()) return;
    value->host->setParentItem(nullptr);
    if (! current()) return;
    value->row->pool();
    if (! current()) return;
    value->updating = false;
    value->idle     = true;
    trim();
}
} // namespace qml_material
