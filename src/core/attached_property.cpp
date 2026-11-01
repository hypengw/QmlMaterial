#include "qml_material/core/attached_property.hpp"

#include <QQmlEngine>
#include <QQmlInfo>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQml/qqml.h>
#include <utility>

namespace qml_material
{
namespace
{
AttachedPropertyNode* attachedObject(const QMetaObject* type, QObject* target,
                                     bool create = false) {
    if (! target) return nullptr;
    const auto factory = qmlAttachedPropertiesFunction(target, type);
    return qobject_cast<AttachedPropertyNode*>(
        qmlAttachedPropertiesObject(target, factory, create));
}

QQmlEngine* engineFor(QObject* target) { return target ? qmlEngine(target) : nullptr; }

bool sameEngine(QObject* first, QObject* second) {
    const auto firstEngine  = engineFor(first);
    const auto secondEngine = engineFor(second);
    return ! firstEngine || ! secondEngine || firstEngine == secondEngine;
}

void collectChildren(const QMetaObject* type, QQuickItem* item,
                     QList<QPointer<AttachedPropertyNode>>& children) {
    const auto items = item->childItems();
    for (auto* child : items) {
        if (auto* node = attachedObject(type, child))
            children.append(node);
        else
            collectChildren(type, child, children);
    }
    const auto objects = item->children();
    for (auto* child : objects)
        if (auto* window = qobject_cast<QQuickWindow*>(child))
            if (auto* node = attachedObject(type, window)) children.append(node);
}
} // namespace

AttachedPropertyNode::AttachedPropertyNode(QObject* target, const QMetaObject* attachedType)
    : QObject(target), m_target(target), m_attached_type(attachedType) {}

AttachedPropertyNode::~AttachedPropertyNode() {
    m_destroying = true;
    disconnectTarget();
    QObject::disconnect(m_inherit_from_destroyed);
    if (m_attached_parent) m_attached_parent->m_attached_children.removeOne(this);
    for (const auto& child : std::exchange(m_attached_children, {}))
        if (child && child->m_attached_parent == this) child->m_attached_parent = nullptr;
}

QObject* AttachedPropertyNode::target() const { return m_target; }
QObject* AttachedPropertyNode::inheritFrom() const { return m_inherit_from; }

void AttachedPropertyNode::setInheritFrom(QObject* source) {
    if (m_inherit_from == source) return;
    const auto                     revision = ++m_source_revision;
    QPointer<AttachedPropertyNode> guard(this);
    QObject::disconnect(std::exchange(m_inherit_from_destroyed, {}));
    m_inherit_from = source;
    if (source) {
        m_inherit_from_destroyed = connect(source, &QObject::destroyed, this, [this] {
            const auto                     revision = ++m_source_revision;
            QPointer<AttachedPropertyNode> guard(this);
            m_inherit_from           = nullptr;
            m_inherit_from_destroyed = {};
            refreshPropagation();
            if (guard && m_source_revision == revision) emit inheritFromChanged();
        });
    }
    refreshPropagation();
    if (guard && m_source_revision == revision) emit inheritFromChanged();
}

void AttachedPropertyNode::resetInheritFrom() { setInheritFrom(nullptr); }

void AttachedPropertyNode::setInheritanceParent(QQuickItem* item) {
    m_inheritance_parent = item;
    refreshPropagation();
}

AttachedPropertyNode* AttachedPropertyNode::attachedParent() const { return m_attached_parent; }

QList<AttachedPropertyNode*> AttachedPropertyNode::attachedChildren() const {
    QList<AttachedPropertyNode*> children;
    children.reserve(m_attached_children.size());
    for (const auto& child : m_attached_children)
        if (child) children.append(child);
    return children;
}

bool AttachedPropertyNode::acceptsParent(AttachedPropertyNode* node) const {
    return node && sameEngine(m_target, node->target());
}

AttachedPropertyNode* AttachedPropertyNode::findParent() {
    if (auto* source = m_inherit_from.data()) {
        if (! sameEngine(m_target, source)) {
            if (! m_cross_engine_warning_issued) {
                qmlWarning(m_target) << "inheritFrom cannot cross QQmlEngine boundaries";
                m_cross_engine_warning_issued = true;
            }
            return nullptr;
        }
        m_cross_engine_warning_issued = false;
        return attachedObject(m_attached_type, source, true);
    }
    m_cross_engine_warning_issued = false;

    QQuickItem* item = nullptr;
    if (auto* visual = qobject_cast<QQuickItem*>(m_target.data())) {
        item = visual->parentItem();
        for (auto* parent = item; parent; parent = parent->parentItem())
            if (auto* node = attachedObject(m_attached_type, parent); acceptsParent(node))
                return node;
        auto* node = attachedObject(m_attached_type, visual->window());
        return acceptsParent(node) ? node : nullptr;
    }
    if (auto* window = qobject_cast<QQuickWindow*>(m_target.data())) {
        auto* node =
            attachedObject(m_attached_type, qobject_cast<QQuickWindow*>(window->transientParent()));
        return acceptsParent(node) ? node : nullptr;
    }
    item = m_inheritance_parent;
    return sameEngine(m_target, item) ? attachedObject(m_attached_type, item, true) : nullptr;
}

void AttachedPropertyNode::setAttachedParent(AttachedPropertyNode* parent) {
    for (auto* ancestor = parent; ancestor; ancestor = ancestor->m_attached_parent) {
        if (ancestor != this) continue;
        if (! m_cycle_warning_issued) {
            qmlWarning(m_target) << m_attached_type->className()
                                 << "inheritFrom creates an attached property propagation cycle";
            m_cycle_warning_issued = true;
        }
        parent = nullptr;
        break;
    }
    if (parent) m_cycle_warning_issued = false;
    if (m_attached_parent == parent) return;
    ++m_revision;
    if (m_attached_parent) m_attached_parent->m_attached_children.removeOne(this);
    m_attached_parent = parent;
    if (parent) parent->m_attached_children.append(this);
    updateInheritedValues();
}

void AttachedPropertyNode::adoptAttachedChildren() {
    auto* item = qobject_cast<QQuickItem*>(m_target.data());
    if (auto* window = qobject_cast<QQuickWindow*>(m_target.data())) item = window->contentItem();
    if (! item) return;
    QList<QPointer<AttachedPropertyNode>> children;
    collectChildren(m_attached_type, item, children);
    QPointer<AttachedPropertyNode> guard(this);
    for (const auto& child : children) {
        if (! guard) return;
        if (child && ! child->m_inherit_from && child->acceptsParent(this))
            child->setAttachedParent(this);
    }
}

void AttachedPropertyNode::connectTarget() {
    if (auto* item = qobject_cast<QQuickItem*>(m_target.data())) {
        m_parent_connection = connect(
            item, &QQuickItem::parentChanged, this, &AttachedPropertyNode::refreshPropagation);
        m_window_connection = connect(
            item, &QQuickItem::windowChanged, this, &AttachedPropertyNode::refreshPropagation);
    } else if (auto* window = qobject_cast<QQuickWindow*>(m_target.data())) {
        m_parent_connection = connect(window,
                                      &QWindow::transientParentChanged,
                                      this,
                                      &AttachedPropertyNode::refreshPropagation);
    }
}

void AttachedPropertyNode::disconnectTarget() {
    QObject::disconnect(std::exchange(m_parent_connection, {}));
    QObject::disconnect(std::exchange(m_window_connection, {}));
}

void AttachedPropertyNode::initializeAttachedProperty() {
    if (m_initialized) return;
    m_initialized = true;
    connectTarget();
    QPointer<AttachedPropertyNode> guard(this);
    refreshPropagation();
    if (guard) adoptAttachedChildren();
}

void AttachedPropertyNode::propagateAttachedValues() {
    const auto                     children = m_attached_children;
    QPointer<AttachedPropertyNode> guard(this);
    for (const auto& child : children) {
        if (! guard) return;
        if (child && child->m_attached_parent == this) child->updateInheritedValues();
    }
}

void AttachedPropertyNode::refreshPropagation() {
    if (! m_initialized || m_destroying || ! m_target) return;
    const auto                     revision = ++m_revision;
    QPointer<AttachedPropertyNode> guard(this);
    QPointer<AttachedPropertyNode> parent = findParent();
    if (! guard || revision != m_revision) return;
    setAttachedParent(parent);
}

} // namespace qml_material
