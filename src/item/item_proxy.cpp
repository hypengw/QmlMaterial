#include "qml_material/item/item_proxy.hpp"
#include <QQmlInfo>
#include <QQuickWindow>

namespace qml_material
{
namespace
{
class ProxyControl : public QObject {
public:
    explicit ProxyControl(QQuickItem* target): QObject(target) {}
    QPointer<ItemProxy> owner;
};
ProxyControl* controlFor(QQuickItem* target, bool create = true) {
    for (auto* child : target->children())
        if (auto* control = dynamic_cast<ProxyControl*>(child)) return control;
    return create ? new ProxyControl(target) : nullptr;
}
} // namespace

ItemProxy::ItemProxy(QQuickItem* parent): QQuickItem(parent) {}
ItemProxy::~ItemProxy() {
    m_destroying = true;
    for (auto connection : m_connections) disconnect(connection);
    release();
}
void ItemProxy::setTarget(QQuickItem* value) {
    if (m_target == value || m_destroying) return;
    QPointer<ItemProxy>  guard(this);
    QPointer<QQuickItem> next(value);
    const auto           revision = ++m_revision;
    release();
    if (! guard || revision != m_revision) return;
    for (auto connection : m_connections) disconnect(connection);
    m_connections.clear();
    m_target = next;
    if (next) {
        m_connections << connect(
            next, &QQuickItem::implicitWidthChanged, this, &ItemProxy::syncImplicitSize);
        m_connections << connect(
            next, &QQuickItem::implicitHeightChanged, this, &ItemProxy::syncImplicitSize);
        m_connections << connect(next, &QQuickItem::parentChanged, this, [this] {
            if (m_controlling && m_target && m_target->parentItem() != this) {
                qmlWarning(this) << "target was reparented outside its controlling ItemProxy";
                release();
            }
        });
        m_connections << connect(next, &QObject::destroyed, this, [this] {
            ++m_revision;
            m_target              = nullptr;
            const bool controlled = m_controlling;
            m_controlling         = false;
            QPointer<ItemProxy> guard(this);
            if (controlled) Q_EMIT controllingChanged();
            if (! guard || m_target) return;
            syncImplicitSize();
            if (guard && ! m_target) Q_EMIT targetChanged();
        });
    }
    syncImplicitSize();
    if (! guard || revision != m_revision) return;
    acquire();
    if (guard && revision == m_revision) Q_EMIT targetChanged();
}
void ItemProxy::setActive(bool value) {
    if (m_active == value || m_destroying) return;
    m_active                     = value;
    const auto          revision = ++m_revision;
    QPointer<ItemProxy> guard(this);
    if (value)
        acquire();
    else
        release();
    if (guard && revision == m_revision) Q_EMIT activeChanged();
}
QString ItemProxy::acquisitionError(QQuickItem* target, const ItemProxy* releasing) const {
    if (! target) return QStringLiteral("missing target");
    for (auto* ancestor = static_cast<const QQuickItem*>(this); ancestor;
         ancestor       = ancestor->parentItem()) {
        if (ancestor == target) return QStringLiteral("target would create a visual parent cycle");
    }
    if (window() && target->window() && window() != target->window())
        return QStringLiteral("target belongs to a different window");
    auto* control = controlFor(target, false);
    if (control && control->owner && control->owner != this && control->owner != releasing)
        return QStringLiteral("target is already controlled by another ItemProxy");
    return {};
}
void ItemProxy::acquire() {
    if (! isComponentComplete() || ! m_active || ! m_target || m_controlling || m_destroying)
        return;
    const auto error = acquisitionError(m_target);
    if (! error.isEmpty()) {
        qmlWarning(this) << error;
        return;
    }
    auto* control  = controlFor(m_target);
    control->owner = this;
    m_controlling  = true;
    QPointer<ItemProxy> guard(this);
    const auto          revision = m_revision;
    m_target->setParentItem(this);
    if (! guard || revision != m_revision || ! m_controlling) return;
    syncGeometry();
    if (guard && revision == m_revision && m_controlling) Q_EMIT controllingChanged();
}
void ItemProxy::release() {
    if (! m_controlling) return;
    m_controlling = false;
    QPointer<QQuickItem> item(m_target);
    if (item) controlFor(item)->owner = nullptr;
    QPointer<ItemProxy> guard(this);
    // Detach the visual input route without taking QObject ownership.
    if (item && item->parentItem() == this) item->setParentItem(nullptr);
    if (guard && ! m_destroying) Q_EMIT controllingChanged();
}
void ItemProxy::syncGeometry() {
    if (! m_controlling || ! m_target) return;
    QPointer<ItemProxy> guard(this);
    const auto          revision = m_revision;
    m_target->setPosition({ 0, 0 });
    if (guard && revision == m_revision && m_controlling && m_target) m_target->setSize(size());
}
void ItemProxy::syncImplicitSize() {
    QPointer<ItemProxy> guard(this);
    const auto          revision = m_revision;
    setImplicitWidth(m_target ? m_target->implicitWidth() : 0);
    if (guard && revision == m_revision)
        setImplicitHeight(m_target ? m_target->implicitHeight() : 0);
}
void ItemProxy::geometryChange(const QRectF& current, const QRectF& previous) {
    QQuickItem::geometryChange(current, previous);
    syncGeometry();
}
void ItemProxy::componentComplete() {
    QQuickItem::componentComplete();
    acquire();
}
} // namespace qml_material

#include "qml_material/item/moc_item_proxy.cpp"
