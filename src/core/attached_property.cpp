#include "qml_material/core/attached_property.hpp"

#include <QHash>
#include <QMetaProperty>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QVariant>
#include <QtQml/qqml.h>

namespace qml_material
{

namespace
{
constexpr auto registryPropertyName = "_qcm_material_attached_property_registry";

QObject* objectProperty(QObject* object, const char* name) {
    if (! object) return nullptr;

    const auto propertyIndex = object->metaObject()->indexOfProperty(name);
    if (propertyIndex < 0) return nullptr;

    const auto value = object->metaObject()->property(propertyIndex).read(object);
    if (auto* item = value.value<QQuickItem*>()) return item;
    return value.value<QObject*>();
}

QQuickItem* itemProperty(QObject* object, const char* name) {
    return qobject_cast<QQuickItem*>(objectProperty(object, name));
}

QMetaObject::Connection connectPropertyNotify(QObject* sender, const char* propertyName,
                                              AttachedPropertyNode* receiver) {
    const auto propertyIndex = sender->metaObject()->indexOfProperty(propertyName);
    if (propertyIndex < 0) return {};

    const auto property = sender->metaObject()->property(propertyIndex);
    if (! property.hasNotifySignal()) return {};

    const auto slotIndex =
        AttachedPropertyNode::staticMetaObject.indexOfSlot("refreshPropagation()");
    return QObject::connect(sender,
                            property.notifySignal(),
                            receiver,
                            AttachedPropertyNode::staticMetaObject.method(slotIndex));
}
} // namespace

class AttachedPropertyRegistry final : public QObject {
public:
    explicit AttachedPropertyRegistry(QQmlEngine* engine): QObject(engine), m_engine(engine) {}

    ~AttachedPropertyRegistry() override {
        if (m_engine) m_engine->setProperty(registryPropertyName, {});

        const auto nodes = allNodes();
        for (const auto& guardedNode : nodes) {
            auto* node = guardedNode.data();
            if (! node) continue;

            disconnectTarget(node);
            QObject::disconnect(node->m_registry_destroyed);
            node->m_registry          = nullptr;
            node->m_registered_target = nullptr;
            node->m_attached_parent   = nullptr;
            node->m_attached_children = {};
        }
    }

    static AttachedPropertyRegistry* get(QQmlEngine* engine) {
        if (! engine) return nullptr;

        const auto value = engine->property(registryPropertyName);
        if (auto* object = value.value<QObject*>())
            return static_cast<AttachedPropertyRegistry*>(object);

        auto* registry = new AttachedPropertyRegistry(engine);
        engine->setProperty(registryPropertyName,
                            QVariant::fromValue(static_cast<QObject*>(registry)));
        return registry;
    }

    void add(AttachedPropertyNode* node) {
        if (! node || ! node->target() || ! node->m_attached_type) return;

        auto& slot = m_nodes[node->target()][node->m_attached_type];
        if (slot && slot != node) {
            qmlWarning(node->target()) << "Duplicate attached property propagation node for"
                                       << node->m_attached_type->className();
            return;
        }

        slot                       = node;
        node->m_registry           = this;
        node->m_registered_target  = node->target();
        node->m_registry_destroyed = connect(this, &QObject::destroyed, node, [node]() {
            node->m_registry = nullptr;
        });

        connectTarget(node);
        updateContentRoot(node);
        setAttachedParent(node, findParent(node));
        adoptAttachedChildren(node);

        if (! qobject_cast<QQuickItem*>(node->target()) &&
            ! qobject_cast<QQuickWindow*>(node->target()) &&
            node->target()->metaObject()->indexOfProperty("contentItem") >= 0) {
            QMetaObject::invokeMethod(
                node, &AttachedPropertyNode::refreshPropagation, Qt::QueuedConnection);
        }
    }

    void remove(AttachedPropertyNode* node) {
        if (! node || node->m_registry != this) return;

        disconnectTarget(node);
        QObject::disconnect(node->m_registry_destroyed);
        removeNodeIndex(node);
        removeContentRootIndex(node);
        detachFromAttachedParent(node);

        const auto children = node->m_attached_children;
        for (const auto& guardedChild : children) {
            auto* child = guardedChild.data();
            if (child && child->m_attached_parent == node) child->m_attached_parent = nullptr;
        }

        node->m_attached_children = {};
        node->m_registry          = nullptr;
        node->m_registered_target = nullptr;
    }

    void refreshNode(AttachedPropertyNode* node) {
        if (! node || node->m_registry != this) return;

        const auto contentRootChanged = updateContentRoot(node);
        reconnectNode(node);
        if (! contentRootChanged) return;

        const auto children = node->m_attached_children;
        for (const auto& guardedChild : children) {
            auto* child = guardedChild.data();
            if (child && child->m_registry == this) reconnectNode(child);
        }
        adoptAttachedChildren(node);
    }

    void refreshChildren(AttachedPropertyNode* node) {
        if (! node) return;

        const auto children = node->m_attached_children;
        for (const auto& guardedChild : children) {
            auto* child = guardedChild.data();
            if (child && child->m_attached_parent == node) child->updateInheritedValues();
        }
    }

private:
    using TypeNodes = QHash<const QMetaObject*, QPointer<AttachedPropertyNode>>;

    QList<QPointer<AttachedPropertyNode>> allNodes() const {
        QList<QPointer<AttachedPropertyNode>> nodes;
        for (auto targetIt = m_nodes.cbegin(); targetIt != m_nodes.cend(); ++targetIt) {
            for (auto typeIt = targetIt->cbegin(); typeIt != targetIt->cend(); ++typeIt) {
                if (typeIt.value()) nodes.append(typeIt.value());
            }
        }
        return nodes;
    }

    AttachedPropertyNode* nodeForTarget(QObject* target, const QMetaObject* attachedType) const {
        const auto targetIt = m_nodes.constFind(target);
        if (targetIt == m_nodes.cend()) return nullptr;

        const auto typeIt = targetIt->constFind(attachedType);
        return typeIt == targetIt->cend() ? nullptr : typeIt.value().data();
    }

    AttachedPropertyNode* nodeForContentRoot(QQuickItem*        item,
                                             const QMetaObject* attachedType) const {
        const auto itemIt = m_content_roots.constFind(item);
        if (itemIt == m_content_roots.cend()) return nullptr;

        const auto typeIt = itemIt->constFind(attachedType);
        return typeIt == itemIt->cend() ? nullptr : typeIt.value().data();
    }

    AttachedPropertyNode* nodeForVisualRoot(QQuickItem*        item,
                                            const QMetaObject* attachedType) const {
        if (! item) return nullptr;
        if (auto* direct = nodeForTarget(item, attachedType)) return direct;
        return nodeForContentRoot(item, attachedType);
    }

    AttachedPropertyNode* findFromWindow(QQuickWindow* window, const QMetaObject* attachedType,
                                         bool includeWindow) const {
        auto* current =
            includeWindow
                ? window
                : qobject_cast<QQuickWindow*>(window ? window->transientParent() : nullptr);
        while (current) {
            if (auto* node = nodeForTarget(current, attachedType)) return node;
            current = qobject_cast<QQuickWindow*>(current->transientParent());
        }
        return nullptr;
    }

    AttachedPropertyNode* findFromItem(QQuickItem* item, const QMetaObject* attachedType,
                                       bool includeItem) const {
        if (! item) return nullptr;

        if (! includeItem) {
            if (auto* owner = nodeForContentRoot(item, attachedType)) return owner;
        }

        auto* current = includeItem ? item : item->parentItem();
        while (current) {
            if (auto* node = nodeForVisualRoot(current, attachedType)) return node;
            current = current->parentItem();
        }
        return findFromWindow(item->window(), attachedType, true);
    }

    AttachedPropertyNode* findParent(AttachedPropertyNode* node) {
        if (! node || ! node->target()) return nullptr;

        auto* source = node->inheritFrom();
        if (source) {
            const auto* sourceEngine = qmlEngine(source);
            if (sourceEngine && sourceEngine != m_engine) {
                if (! node->m_cross_engine_warning_issued) {
                    qmlWarning(node->target()) << "inheritFrom cannot cross QQmlEngine boundaries";
                    node->m_cross_engine_warning_issued = true;
                }
                return nullptr;
            }
            node->m_cross_engine_warning_issued = false;

            if (auto* direct = nodeForTarget(source, node->m_attached_type)) return direct;
            if (auto* item = qobject_cast<QQuickItem*>(source))
                return findFromItem(item, node->m_attached_type, true);
            if (auto* window = qobject_cast<QQuickWindow*>(source))
                return findFromWindow(window, node->m_attached_type, true);
            if (auto* parentItem = itemProperty(source, "parent"))
                return findFromItem(parentItem, node->m_attached_type, true);
            return nullptr;
        }
        node->m_cross_engine_warning_issued = false;

        if (auto* item = qobject_cast<QQuickItem*>(node->target()))
            return findFromItem(item, node->m_attached_type, false);
        if (auto* window = qobject_cast<QQuickWindow*>(node->target()))
            return findFromWindow(window, node->m_attached_type, false);
        if (auto* parentItem = itemProperty(node->target(), "parent"))
            return findFromItem(parentItem, node->m_attached_type, true);
        return nullptr;
    }

    void collectAttachedChildren(QQuickItem* item, AttachedPropertyNode* owner, bool includeItem,
                                 QList<QPointer<AttachedPropertyNode>>& children,
                                 QSet<QQuickItem*>&                     visitedItems,
                                 QSet<AttachedPropertyNode*>&           visitedNodes) const {
        if (! item || visitedItems.contains(item)) return;
        visitedItems.insert(item);

        if (includeItem) {
            auto* attached = nodeForVisualRoot(item, owner->m_attached_type);
            if (attached && attached != owner) {
                if (! visitedNodes.contains(attached)) {
                    visitedNodes.insert(attached);
                    children.append(attached);
                }
                return;
            }
        }

        const auto childItems = item->childItems();
        for (auto* child : childItems)
            collectAttachedChildren(child, owner, true, children, visitedItems, visitedNodes);

        const auto objectChildren = item->children();
        for (auto* objectChild : objectChildren) {
            auto* childWindow = qobject_cast<QQuickWindow*>(objectChild);
            if (! childWindow) continue;

            auto* attached = nodeForTarget(childWindow, owner->m_attached_type);
            if (attached && attached != owner && ! visitedNodes.contains(attached)) {
                visitedNodes.insert(attached);
                children.append(attached);
            }
        }
    }

    QList<QPointer<AttachedPropertyNode>> findAttachedChildren(AttachedPropertyNode* node) const {
        QList<QPointer<AttachedPropertyNode>> children;
        QSet<QQuickItem*>                     visitedItems;
        QSet<AttachedPropertyNode*>           visitedNodes;

        if (auto* item = qobject_cast<QQuickItem*>(node->target())) {
            collectAttachedChildren(item, node, false, children, visitedItems, visitedNodes);
            return children;
        }

        if (auto* window = qobject_cast<QQuickWindow*>(node->target())) {
            collectAttachedChildren(
                window->contentItem(), node, false, children, visitedItems, visitedNodes);
            return children;
        }

        collectAttachedChildren(
            node->m_content_root, node, true, children, visitedItems, visitedNodes);
        return children;
    }

    void adoptAttachedChildren(AttachedPropertyNode* node) {
        const auto children = findAttachedChildren(node);
        for (const auto& guardedChild : children) {
            auto* child = guardedChild.data();
            if (! child || child->m_registry != this) continue;
            if (findParent(child) == node) setAttachedParent(child, node);
        }
    }

    void reconnectNode(AttachedPropertyNode* node) {
        if (! node || node->m_registry != this) return;
        setAttachedParent(node, findParent(node));
    }

    bool createsCycle(AttachedPropertyNode* node, AttachedPropertyNode* parent) const {
        QSet<AttachedPropertyNode*> visited;
        for (auto* current = parent; current; current = current->m_attached_parent) {
            if (current == node) return true;
            if (visited.contains(current)) return true;
            visited.insert(current);
        }
        return false;
    }

    void setAttachedParent(AttachedPropertyNode* node, AttachedPropertyNode* parent) {
        if (! node) return;

        if (parent && createsCycle(node, parent)) {
            if (! node->m_cycle_warning_issued) {
                qmlWarning(node->target())
                    << node->m_attached_type->className()
                    << "inheritFrom creates an attached property propagation cycle";
                node->m_cycle_warning_issued = true;
            }
            parent = nullptr;
        } else {
            node->m_cycle_warning_issued = false;
        }

        if (node->m_attached_parent == parent) return;

        detachFromAttachedParent(node);
        node->m_attached_parent = parent;
        if (parent && ! parent->m_attached_children.contains(node))
            parent->m_attached_children.append(node);
        node->updateInheritedValues();
    }

    void detachFromAttachedParent(AttachedPropertyNode* node) {
        if (! node) return;
        if (node->m_attached_parent) node->m_attached_parent->m_attached_children.removeAll(node);
        node->m_attached_parent = nullptr;
    }

    void connectTarget(AttachedPropertyNode* node) {
        auto* target = node->target();
        if (auto* item = qobject_cast<QQuickItem*>(target)) {
            node->m_target_connections.append(connect(
                item, &QQuickItem::parentChanged, node, &AttachedPropertyNode::refreshPropagation));
            node->m_target_connections.append(connect(
                item, &QQuickItem::windowChanged, node, &AttachedPropertyNode::refreshPropagation));
            return;
        }
        if (auto* window = qobject_cast<QQuickWindow*>(target)) {
            node->m_target_connections.append(connect(window,
                                                      &QWindow::transientParentChanged,
                                                      node,
                                                      &AttachedPropertyNode::refreshPropagation));
            return;
        }

        const auto parentConnection = connectPropertyNotify(target, "parent", node);
        if (parentConnection) node->m_target_connections.append(parentConnection);
        const auto contentConnection = connectPropertyNotify(target, "contentItem", node);
        if (contentConnection) node->m_target_connections.append(contentConnection);
    }

    void disconnectTarget(AttachedPropertyNode* node) {
        for (const auto& connection : node->m_target_connections) QObject::disconnect(connection);
        node->m_target_connections.clear();
        QObject::disconnect(node->m_content_root_destroyed);
        node->m_content_root_destroyed = {};
    }

    bool updateContentRoot(AttachedPropertyNode* node) {
        QQuickItem* contentRoot = nullptr;
        if (! qobject_cast<QQuickItem*>(node->target()) &&
            ! qobject_cast<QQuickWindow*>(node->target())) {
            contentRoot = itemProperty(node->target(), "contentItem");
        }

        if (node->m_content_root == contentRoot) return false;

        removeContentRootIndex(node);
        node->m_content_root = contentRoot;
        if (! contentRoot) return true;

        m_content_roots[contentRoot][node->m_attached_type] = node;
        node->m_content_root_destroyed =
            connect(contentRoot, &QObject::destroyed, node, [this, node, contentRoot]() {
                auto itemIt = m_content_roots.find(contentRoot);
                if (itemIt != m_content_roots.end()) {
                    auto typeIt = itemIt->find(node->m_attached_type);
                    if (typeIt != itemIt->end() && typeIt.value() == node) itemIt->erase(typeIt);
                    if (itemIt->isEmpty()) m_content_roots.erase(itemIt);
                }
                node->m_content_root           = nullptr;
                node->m_content_root_destroyed = {};
            });
        return true;
    }

    void removeNodeIndex(AttachedPropertyNode* node) {
        auto targetIt = m_nodes.find(node->m_registered_target);
        if (targetIt == m_nodes.end()) return;

        auto typeIt = targetIt->find(node->m_attached_type);
        if (typeIt != targetIt->end() && typeIt.value() == node) targetIt->erase(typeIt);
        if (targetIt->isEmpty()) m_nodes.erase(targetIt);
    }

    void removeContentRootIndex(AttachedPropertyNode* node) {
        QObject::disconnect(node->m_content_root_destroyed);
        node->m_content_root_destroyed = {};

        auto* contentRoot = node->m_content_root.data();
        if (! contentRoot) {
            node->m_content_root = nullptr;
            return;
        }

        auto itemIt = m_content_roots.find(contentRoot);
        if (itemIt != m_content_roots.end()) {
            auto typeIt = itemIt->find(node->m_attached_type);
            if (typeIt != itemIt->end() && typeIt.value() == node) itemIt->erase(typeIt);
            if (itemIt->isEmpty()) m_content_roots.erase(itemIt);
        }
        node->m_content_root = nullptr;
    }

    QPointer<QQmlEngine>          m_engine;
    QHash<QObject*, TypeNodes>    m_nodes;
    QHash<QQuickItem*, TypeNodes> m_content_roots;
};

AttachedPropertyNode::AttachedPropertyNode(QObject* target, const QMetaObject* attachedType)
    : QObject(target), m_target(target), m_attached_type(attachedType) {}

AttachedPropertyNode::~AttachedPropertyNode() {
    QObject::disconnect(m_inherit_from_destroyed);
    if (m_registry) m_registry->remove(this);
}

QObject* AttachedPropertyNode::target() const { return m_target; }

QObject* AttachedPropertyNode::inheritFrom() const { return m_inherit_from; }

void AttachedPropertyNode::setInheritFrom(QObject* source) {
    if (m_inherit_from == source) return;

    QObject::disconnect(m_inherit_from_destroyed);
    m_inherit_from = source;
    if (source) {
        m_inherit_from_destroyed = connect(source, &QObject::destroyed, this, [this]() {
            m_inherit_from = nullptr;
            emit inheritFromChanged();
            refreshPropagation();
        });
    } else {
        m_inherit_from_destroyed = {};
    }

    emit inheritFromChanged();
    refreshPropagation();
}

void AttachedPropertyNode::resetInheritFrom() { setInheritFrom(nullptr); }

AttachedPropertyNode* AttachedPropertyNode::attachedParent() const { return m_attached_parent; }

QList<AttachedPropertyNode*> AttachedPropertyNode::attachedChildren() const {
    QList<AttachedPropertyNode*> children;
    children.reserve(m_attached_children.size());
    for (const auto& child : m_attached_children) {
        if (child) children.append(child);
    }
    return children;
}

void AttachedPropertyNode::initializeAttachedProperty() {
    if (m_initialized) return;
    m_initialized = true;
    refreshPropagation();

    if (! m_registry) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                if (! m_registry) refreshPropagation();
            },
            Qt::QueuedConnection);
    }
}

void AttachedPropertyNode::propagateAttachedValues() {
    if (m_registry) m_registry->refreshChildren(this);
}

void AttachedPropertyNode::refreshPropagation() {
    if (! m_initialized) return;

    if (! m_registry) {
        auto* engine = qmlEngine(m_target);
        if (engine) {
            AttachedPropertyRegistry::get(engine)->add(this);
            return;
        }
    }

    if (m_registry) {
        m_registry->refreshNode(this);
    } else {
        updateInheritedValues();
    }
}

} // namespace qml_material
