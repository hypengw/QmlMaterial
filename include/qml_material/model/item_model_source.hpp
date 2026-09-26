#pragma once

#include <QAbstractItemModel>
#include <QPointer>
#include "qml_material/model/item_source.hpp"

namespace qml_material
{
class QML_MATERIAL_API ItemModelSource : public ItemSource {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QString keyRole READ keyRole WRITE setKeyRole NOTIFY keyRoleChanged FINAL)
    Q_PROPERTY(QString typeRole READ typeRole WRITE setTypeRole NOTIFY typeRoleChanged FINAL)
    Q_PROPERTY(QStringList roles READ roles WRITE setRoles NOTIFY rolesChanged FINAL)
public:
    using ItemSource::ItemSource;
    QAbstractItemModel* model() const { return m_model; }
    QString             keyRole() const { return m_keyRole; }
    QString             typeRole() const { return m_typeRole; }
    QStringList         roles() const { return m_roles; }
    void                setModel(QAbstractItemModel*);
    void                setKeyRole(const QString&);
    void                setTypeRole(const QString&);
    void                setRoles(const QStringList&);
    Q_INVOKABLE void    requestMore() override;
    // QAbstractItemModel has no notification for canFetchMore changes without row changes.
    Q_INVOKABLE void notifyFetchStateChanged();
    Q_SIGNAL void    modelChanged();
    Q_SIGNAL void    keyRoleChanged();
    Q_SIGNAL void    typeRoleChanged();
    Q_SIGNAL void    rolesChanged();

private:
    void                           update(QVector<ItemChange> changes = {}, bool reset = true);
    QPointer<QAbstractItemModel>   m_model;
    QList<QMetaObject::Connection> m_connections;
    QString                        m_keyRole, m_typeRole;
    QStringList                    m_roles;
    quint64                        m_generation      = 0;
    quint64                        m_modelGeneration = 0;
    bool                           m_fetchQueued     = false;
};
} // namespace qml_material
