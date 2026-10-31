#pragma once

#include <QAbstractItemModel>
#include <QMap>
#include <QPointer>
#include <QPersistentModelIndex>
#include <QQmlPropertyMap>
#include "qml_material/model/item_source.hpp"
#include "qml_material/model/item_demand.hpp"

namespace qml_material
{
class QML_MATERIAL_API ItemModelSource : public ItemSource {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(KeyMode keyMode READ keyMode WRITE setKeyMode NOTIFY keyModeChanged FINAL)
    Q_PROPERTY(QString keyRole READ keyRole WRITE setKeyRole NOTIFY keyRoleChanged FINAL)
    Q_PROPERTY(QString typeRole READ typeRole WRITE setTypeRole NOTIFY typeRoleChanged FINAL)
    Q_PROPERTY(QStringList roles READ roles WRITE setRoles NOTIFY rolesChanged FINAL)
    Q_PROPERTY(bool deferDataChanges READ deferDataChanges WRITE setDeferDataChanges NOTIFY
                   deferDataChangesChanged FINAL)
public:
    enum KeyMode
    {
        RoleKey,
        PersistentIndexKey
    };
    Q_ENUM(KeyMode)
    explicit ItemModelSource(QObject* parent = nullptr);
    KeyMode          keyMode() const { return m_keyMode; }
    void             setKeyMode(KeyMode);
    Q_INVOKABLE bool setRole(const QString& key, const QString& role, const QVariant& value);
    Q_INVOKABLE QQmlPropertyMap* objectForKey(const QString& key);
    Q_SIGNAL void                keyModeChanged();
    QAbstractItemModel*          model() const { return m_model; }
    QString                      keyRole() const { return m_keyRole; }
    QString                      typeRole() const { return m_typeRole; }
    QStringList                  roles() const { return m_roles; }
    void                         setModel(QAbstractItemModel*);
    void                         setKeyRole(const QString&);
    void                         setTypeRole(const QString&);
    void                         setRoles(const QStringList&);
    bool                         deferDataChanges() const { return m_deferDataChanges; }
    void                         setDeferDataChanges(bool);
    Q_INVOKABLE void             flushDataChanges();
    Q_SIGNAL void                deferDataChangesChanged();
    Q_INVOKABLE void             requestMore(const qml_material::ItemDemand& demand);
    // QAbstractItemModel has no notification for canFetchMore changes without row changes.
    Q_INVOKABLE void notifyFetchStateChanged();
    Q_SIGNAL void    fetchStateChanged();
    Q_SIGNAL void    modelChanged();
    Q_SIGNAL void    keyRoleChanged();
    Q_SIGNAL void    typeRoleChanged();
    Q_SIGNAL void    rolesChanged();

private:
    void                           refreshObjects(const ItemChangeSet&);
    void                           dataChanged(int first, int last);
    void                           cancelDataChanges();
    void                           update(QVector<ItemChange> changes = {}, bool reset = true);
    QPointer<QAbstractItemModel>   m_model;
    QList<QMetaObject::Connection> m_connections;
    QString                        m_keyRole, m_typeRole;
    QStringList                    m_roles;
    quint64                        m_generation       = 0;
    quint64                        m_modelGeneration  = 0;
    bool                           m_fetchQueued      = false;
    bool                           m_resetRequired    = true;
    bool                           m_deferDataChanges = false;
    bool                           m_dataChangeQueued = false;
    bool                           m_preparing        = false;
    quint64                        m_dataChangeTicket = 0;
    QMap<int, int>                 m_dirtyRanges;
    KeyMode                        m_keyMode      = RoleKey;
    quint64                        m_nextIdentity = 0;
    QVector<QPair<QPersistentModelIndex, QString>> m_identities;
    QHash<QString, QPointer<QQmlPropertyMap>>      m_objects;
    ItemDemand                                     m_demand;
    std::optional<int>                             m_requestedCount;
};
} // namespace qml_material
