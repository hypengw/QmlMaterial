#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QtQml/qqmlregistration.h>
#include "qml_material/model/item_snapshot.hpp"

namespace qml_material
{
struct ItemChange {
    enum Kind
    {
        Insert,
        Remove,
        Move,
        Update
    };
    Kind kind  = Update;
    int  index = 0;
    int  count = 0;
    // Move destination is an index after the moved range has been removed.
    int destination = 0;
};

struct ItemChangeSet {
    ItemSnapshotPtr     before;
    ItemSnapshotPtr     after;
    QVector<ItemChange> changes;
    bool                reset = false;
};

struct ItemPublishResult {
    enum Status
    {
        Accepted,
        Rejected
    };
    Status   status = Rejected;
    QString  error;
    explicit operator bool() const { return status == Accepted; }
};

class QML_MATERIAL_API ItemSource : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ItemSource)
    QML_UNCREATABLE("Use an item source adapter")
    Q_PROPERTY(int count READ count NOTIFY stateChanged FINAL)
    Q_PROPERTY(quint64 revision READ revision NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool stableKeys READ stableKeys NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString errorString READ errorString NOTIFY stateChanged FINAL)
public:
    explicit ItemSource(QObject* parent = nullptr);
    ItemSnapshotPtr snapshot() const { return m_snapshot; }
    int             count() const { return m_snapshot->count(); }
    quint64         revision() const { return m_snapshot->revision; }
    bool            stableKeys() const { return m_snapshot->stableKeys(); }
    QString         errorString() const { return m_error; }
    Q_INVOKABLE int indexOfKey(const QString& key) const { return m_snapshot->indexOfKey(key); }
    Q_INVOKABLE QVariant valueAt(int index) const {
        return index >= 0 && index < count() ? m_snapshot->valueAt(index) : QVariant {};
    }
    Q_SIGNAL void stateChanged();
    Q_SIGNAL void committed(const qml_material::ItemChangeSet& change);

protected:
    ItemPublishResult publish(ItemDataSnapshotPtr, QVector<ItemChange> changes = {},
                              bool reset = true);
    ItemPublishResult reject(QString error);
    ItemSnapshotPtr   publicationBase() const { return m_tail; }

private:
    struct Pending {
        ItemChangeSet change;
        QString       error;
    };
    void            drain();
    ItemSnapshotPtr m_snapshot;
    ItemSnapshotPtr m_tail;
    QList<Pending>  m_pending;
    QString         m_error;
    bool            m_publishing = false;
};

class QML_MATERIAL_API ListSnapshotSource : public ItemSource {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList items READ items WRITE setItems NOTIFY itemsChanged FINAL)
    Q_PROPERTY(QString keyRole READ keyRole WRITE setKeyRole NOTIFY keyRoleChanged FINAL)
    Q_PROPERTY(QString typeRole READ typeRole WRITE setTypeRole NOTIFY typeRoleChanged FINAL)
public:
    using ItemSource::ItemSource;
    QVariantList  items() const { return m_items; }
    QString       keyRole() const { return m_keyRole; }
    QString       typeRole() const { return m_typeRole; }
    void          setItems(const QVariantList&);
    void          setKeyRole(const QString&);
    void          setTypeRole(const QString&);
    Q_SIGNAL void itemsChanged();
    Q_SIGNAL void keyRoleChanged();
    Q_SIGNAL void typeRoleChanged();

private:
    void         rebuild();
    QVariantList m_items;
    QString      m_keyRole, m_typeRole;
};
} // namespace qml_material
Q_DECLARE_METATYPE(qml_material::ItemChangeSet)
