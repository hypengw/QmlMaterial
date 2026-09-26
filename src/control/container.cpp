#include "qml_material/control/container.hpp"
#include "qml_material/util/qt.hpp"
#include <QtQml/qqml.h>
#include <algorithm>

namespace qml_material
{
namespace
{
ContainerAttached* attached(QQuickItem* item) {
    return qobject_cast<ContainerAttached*>(qmlAttachedPropertiesObject<Container>(item, true));
}
} // namespace

Container::Container(QQuickItem* parent): Control(parent), m_host(new QQuickItem(this)) {
    setFlag(ItemIsFocusScope);
    connect(m_host, &QQuickItem::childrenChanged, this, &Container::refreshItems);
    connect(this, &Control::contentItemChanged, this, [this] {
        if (m_destroying) return;
        QPointer<Container> guard(this);
        utils::disconnectAll(m_measure_connections);
        if (contentItem() != m_host) m_host->setParentItem(contentItem() ? contentItem() : this);
        if (! guard) return;
        if (auto* item = contentItem()) {
            m_measure_connections.append(connect(
                item, &QQuickItem::implicitWidthChanged, this, &Container::updateContentSize));
            m_measure_connections.append(connect(
                item, &QQuickItem::implicitHeightChanged, this, &Container::updateContentSize));
        }
        updateContentSize();
    });
    setContentItem(m_host);
}
Container::~Container() {
    beginTeardown();
    const auto snapshot = m_items;
    for (auto item : snapshot)
        if (item) attached(item)->update(nullptr, -1);
}
void Container::beginTeardown() {
    m_destroying = true;
    disconnect(m_host, nullptr, this, nullptr);
    utils::disconnectAll(m_connections);
    utils::disconnectAll(m_measure_connections);
}
ContainerAttached* Container::qmlAttachedProperties(QObject* object) {
    return new ContainerAttached(object);
}
QQmlListProperty<QObject> Container::contentData() {
    return m_host->property("data").value<QQmlListProperty<QObject>>();
}
QQmlListProperty<QQuickItem> Container::contentChildren() {
    return { this,
             this,
             [](QQmlListProperty<QQuickItem>* p, QQuickItem* item) {
                 static_cast<Container*>(p->data)->addItem(item);
             },
             [](QQmlListProperty<QQuickItem>* p) -> qsizetype {
                 return static_cast<Container*>(p->data)->count();
             },
             [](QQmlListProperty<QQuickItem>* p, qsizetype index) {
                 return static_cast<Container*>(p->data)->itemAt(index);
             },
             [](QQmlListProperty<QQuickItem>* p) {
                 auto*               owner = static_cast<Container*>(p->data);
                 QPointer<Container> guard(owner);
                 const auto          snapshot = owner->items();
                 for (auto item : snapshot) {
                     if (item) owner->takeItem(owner->indexOf(item));
                     if (! guard) return;
                 }
             } };
}
QQuickItem* Container::itemAt(int index) const { return m_items.value(index); }
int         Container::indexOf(QQuickItem* item) const { return item ? m_items.indexOf(item) : -1; }
bool        Container::isContent(QQuickItem*) const { return true; }
QQuickItem* Container::presentationItem(QQuickItem* item) const {
    for (auto it = m_presentation_hosts.cbegin(); it != m_presentation_hosts.cend(); ++it)
        if (it.value() == item && item->parentItem() == it.key()) return it.key();
    return item;
}
void Container::setPresentationHost(QQuickItem* item, QQuickItem* host) {
    // Register before reparenting: refreshItems can run synchronously from childrenChanged.
    m_presentation_hosts.insert(host, item);
    QPointer<Container>  guard(this);
    QPointer<QQuickItem> aliveHost(host);
    QPointer<QQuickItem> aliveItem(item);
    host->setParent(m_host);
    if (! guard || ! aliveHost || ! aliveItem) return;
    aliveItem->setParentItem(aliveHost);
    if (guard && aliveHost) aliveHost->setParentItem(m_host);
}
void Container::addItem(QQuickItem* item) { insertItem(count(), item); }
void Container::insertItem(int index, QQuickItem* item) {
    if (! item || m_destroying || item == this || item == m_host || item == background() ||
        item == contentItem() || item->isAncestorOf(this) || ! isContent(item))
        return;
    QPointer<Container>  guard(this);
    QPointer<QQuickItem> alive(item);
    index = std::clamp(index, 0, count());
    item->setParentItem(m_host);
    if (! guard || ! alive) return;
    refreshItems();
    if (! guard || ! alive) return;
    const int from = indexOf(alive);
    if (from >= 0) moveItem(from, std::min(index, count() - 1));
}
void Container::moveItem(int from, int to) {
    if (from < 0 || to < 0 || from >= count() || to >= count() || from == to) return;
    QPointer<Container> guard(this);
    auto                item     = m_items[from];
    auto                neighbor = m_items[to];
    if (! item || ! neighbor) return;
    if (from < to)
        presentationItem(item)->stackAfter(presentationItem(neighbor));
    else
        presentationItem(item)->stackBefore(presentationItem(neighbor));
    if (guard) refreshItems();
}
QQuickItem* Container::takeItem(int index) {
    QPointer<QQuickItem> item(itemAt(index));
    if (! item) return nullptr;
    if (item->parent() == this || item->parent() == m_host) item->setParent(nullptr);
    if (! item) return nullptr;
    item->setParentItem(nullptr);
    return item;
}
void Container::removeItem(QQuickItem* item) {
    if (auto* removed = takeItem(indexOf(item))) removed->deleteLater();
}
void Container::setCurrentIndex(int value) {
    if (! isComponentComplete()) m_explicit_index = true;
    if (! isComponentComplete() && count() == 0) {
        if (m_index == value) return;
        m_index = value;
        Q_EMIT currentIndexChanged();
        return;
    }
    value                   = std::clamp(value, -1, count() - 1);
    auto*      item         = itemAt(value);
    const bool indexChanged = m_index != value;
    const bool itemChanged  = m_current_identity != item;
    m_index                 = value;
    m_current               = item;
    m_current_identity      = item;
    QPointer<Container> guard(this);
    currentItemChange();
    if (! guard || m_index != value || m_current != item) return;
    if (indexChanged) Q_EMIT currentIndexChanged();
    if (guard && m_index == value && m_current == item && itemChanged) Q_EMIT currentItemChanged();
}
void Container::incrementCurrentIndex() {
    if (m_index < count() - 1) setCurrentIndex(m_index + 1);
}
void Container::decrementCurrentIndex() {
    if (m_index > 0) setCurrentIndex(m_index - 1);
}
int  Container::initialIndex() const { return count() ? 0 : -1; }
void Container::componentComplete() {
    QPointer<Container> guard(this);
    Control::componentComplete();
    if (! guard) return;
    const int index = m_explicit_index ? m_index : initialIndex();
    refreshItems();
    if (! guard) return;
    setCurrentIndex(index);
    if (guard) polish();
}
void Container::refreshItems() {
    if (m_destroying) return;
    if (m_refreshing) {
        m_refresh_again = true;
        return;
    }
    m_refreshing = true;
    QPointer<Container> guard(this);
    do {
        m_refresh_again = false;
        QList<QPointer<QQuickItem>> next;
        QList<QPointer<QQuickItem>> observed;
        for (auto* child : m_host->childItems()) {
            if (m_presentation_hosts.contains(child)) {
                auto item = m_presentation_hosts.value(child);
                if (! item || item->parentItem() != child) continue;
                child = item;
            }
            observed.append(child);
            if (attached(child)->isManaged() && isContent(child)) next.append(child);
        }
        if (next == m_items && observed == m_observed) continue;
        m_observed = observed;
        utils::disconnectAll(m_connections);
        for (auto child : observed) {
            auto* info = attached(child);
            m_connections.append(
                connect(info, &ContainerAttached::managedChanged, this, &Container::refreshItems));
            if (! info->isManaged() || ! isContent(child)) continue;
            m_connections.append(
                connect(child, &QQuickItem::parentChanged, this, &Container::refreshItems));
            m_connections.append(
                connect(child, &QObject::destroyed, this, &Container::refreshItems));
            m_connections.append(
                connect(child, &QQuickItem::widthChanged, this, &QQuickItem::polish));
            m_connections.append(
                connect(child, &QQuickItem::heightChanged, this, &QQuickItem::polish));
            m_connections.append(
                connect(child, &QQuickItem::implicitWidthChanged, this, &QQuickItem::polish));
            m_connections.append(
                connect(child, &QQuickItem::implicitHeightChanged, this, &QQuickItem::polish));
            m_connections.append(
                connect(child, &QQuickItem::visibleChanged, this, &QQuickItem::polish));
        }
        if (next == m_items) continue;
        const auto old = m_items;
        m_items        = next;
        ++m_revision;
        for (auto item : old) {
            if (! item || next.contains(item)) continue;
            itemRemoved(item);
            if (! guard) return;
            if (item && attached(item)->container() == this) attached(item)->update(nullptr, -1);
            if (! guard) return;
        }
        for (auto it = m_presentation_hosts.begin(); it != m_presentation_hosts.end();) {
            if (! it.value() || ! next.contains(it.value())) {
                QPointer<QQuickItem> host = it.key();
                auto                 item = it.value();
                it                        = m_presentation_hosts.erase(it);
                if (item && item->parentItem() == host) item->setParentItem(m_host);
                if (! guard) return;
                if (host) host->setParentItem(nullptr);
                if (! guard) return;
                if (host) host->deleteLater();
            } else {
                ++it;
            }
        }
        for (int i = 0; i < next.size(); ++i) {
            auto item = next[i];
            if (! item || presentationItem(item)->parentItem() != m_host) continue;
            attached(item)->update(this, i);
            if (! guard) return;
            if (item && presentationItem(item)->parentItem() == m_host && ! old.contains(item))
                itemAdded(item);
            if (! guard) return;
        }
        itemsChanged();
        if (! guard) return;
        if (isComponentComplete()) {
            const int  retained   = indexOf(m_current);
            const bool unselected = m_index < 0 && ! old.isEmpty();
            setCurrentIndex(
                retained >= 0
                    ? retained
                    : (count() && ! unselected ? std::clamp(m_index, 0, count() - 1) : -1));
        }
        if (! guard) return;
        polish();
        if (old.size() != count()) Q_EMIT countChanged();
        if (! guard) return;
        Q_EMIT contentChildrenChanged();
        if (! guard) return;
    } while (m_refresh_again);
    m_refreshing = false;
}
void Container::updatePolish() {
    QPointer<Container> guard(this);
    Control::updatePolish();
    if (guard) refreshItems();
}
qreal  Container::contentWidth() const { return m_content_size.width(); }
qreal  Container::contentHeight() const { return m_content_size.height(); }
QSizeF Container::measureImplicitContent() const { return m_content_size; }
void   Container::setContentWidth(qreal value) {
    m_content_width = value;
    updateContentSize();
}
void Container::setContentHeight(qreal value) {
    m_content_height = value;
    updateContentSize();
}
void Container::resetContentWidth() {
    m_content_width.reset();
    updateContentSize();
}
void Container::resetContentHeight() {
    m_content_height.reset();
    updateContentSize();
}
void Container::setImplicitContentSize(const QSizeF& size) {
    if (m_implicit_content == size) return;
    m_implicit_content = size;
    updateContentSize();
}
void Container::updateContentSize() {
    const auto old          = m_content_size;
    const auto implicitSize = m_implicit_content.value_or(
        contentItem() ? QSizeF(contentItem()->implicitWidth(), contentItem()->implicitHeight())
                      : QSizeF(0, 0));
    m_content_size = QSizeF(m_content_width.value_or(implicitSize.width()),
                            m_content_height.value_or(implicitSize.height()));
    QPointer<Container> guard(this);
    updateImplicitMetrics();
    if (! guard) return;
    if (old.width() != contentWidth()) Q_EMIT contentWidthChanged();
    if (guard && old.height() != contentHeight()) Q_EMIT contentHeightChanged();
}
void ContainerAttached::setManaged(bool value) {
    if (m_managed == value) return;
    m_managed = value;
    Q_EMIT managedChanged();
}
void ContainerAttached::update(Container* owner, int index) {
    const bool changedOwner = m_container != owner;
    const bool changedIndex = m_index != index;
    m_container             = owner;
    m_index                 = index;
    QPointer<ContainerAttached> guard(this);
    if (changedOwner) Q_EMIT containerChanged();
    if (guard && changedIndex) Q_EMIT indexChanged();
}
} // namespace qml_material
