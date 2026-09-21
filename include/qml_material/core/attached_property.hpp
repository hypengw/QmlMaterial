#pragma once

#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <qqmlregistration.h>

#include "qml_material/export.hpp"

class QQuickItem;

namespace qml_material
{

class AttachedPropertyRegistry;

class QML_MATERIAL_API AttachedPropertyNode : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QObject* inheritFrom READ inheritFrom WRITE setInheritFrom RESET resetInheritFrom
                   NOTIFY inheritFromChanged FINAL)

public:
    AttachedPropertyNode(QObject* target, const QMetaObject* attachedType);
    ~AttachedPropertyNode() override;

    QObject* target() const;

    QObject* inheritFrom() const;
    void     setInheritFrom(QObject* source);
    void     resetInheritFrom();

    AttachedPropertyNode*        attachedParent() const;
    QList<AttachedPropertyNode*> attachedChildren() const;

    Q_SIGNAL void inheritFromChanged();

protected:
    void initializeAttachedProperty();
    void propagateAttachedValues();

    virtual void updateInheritedValues() = 0;

private Q_SLOTS:
    void refreshPropagation();

private:
    friend class AttachedPropertyRegistry;

    QPointer<QObject>                     m_target;
    QPointer<QObject>                     m_inherit_from;
    QObject*                              m_registered_target { nullptr };
    const QMetaObject*                    m_attached_type { nullptr };
    AttachedPropertyRegistry*             m_registry { nullptr };
    QPointer<AttachedPropertyNode>        m_attached_parent;
    QList<QPointer<AttachedPropertyNode>> m_attached_children;
    QPointer<QQuickItem>                  m_content_root;
    QList<QMetaObject::Connection>        m_target_connections;
    QMetaObject::Connection               m_registry_destroyed;
    QMetaObject::Connection               m_content_root_destroyed;
    QMetaObject::Connection               m_inherit_from_destroyed;
    bool                                  m_initialized { false };
    bool                                  m_cycle_warning_issued { false };
    bool                                  m_cross_engine_warning_issued { false };
};

} // namespace qml_material
