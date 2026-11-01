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

    // Non-visual owners publish this again when their logical context changes.
    void setInheritanceParent(QQuickItem*);

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
    AttachedPropertyNode* findParent();
    bool                  acceptsParent(AttachedPropertyNode*) const;
    void                  setAttachedParent(AttachedPropertyNode*);
    void                  adoptAttachedChildren();
    void                  connectTarget();
    void                  disconnectTarget();

    QPointer<QObject>                     m_target;
    QPointer<QObject>                     m_inherit_from;
    QPointer<QQuickItem>                  m_inheritance_parent;
    const QMetaObject*                    m_attached_type { nullptr };
    QPointer<AttachedPropertyNode>        m_attached_parent;
    QList<QPointer<AttachedPropertyNode>> m_attached_children;
    QMetaObject::Connection               m_parent_connection;
    QMetaObject::Connection               m_window_connection;
    QMetaObject::Connection               m_inherit_from_destroyed;
    quint64                               m_revision { 0 };
    quint64                               m_source_revision { 0 };
    bool                                  m_initialized { false };
    bool                                  m_destroying { false };
    bool                                  m_cycle_warning_issued { false };
    bool                                  m_cross_engine_warning_issued { false };
};

} // namespace qml_material
