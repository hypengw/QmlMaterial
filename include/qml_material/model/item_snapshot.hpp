#pragma once

#include <QString>
#include <QVariant>
#include <QVector>
#include <memory>
#include <optional>
#include "qml_material/export.hpp"

namespace qml_material
{
struct ItemDescriptor {
    QString key;
    QString reuseType;
    quint64 contentRevision                         = 0;
    bool    operator==(const ItemDescriptor&) const = default;
};

class QML_MATERIAL_API ItemDataSnapshot {
public:
    virtual ~ItemDataSnapshot();
    virtual int                count() const = 0;
    virtual bool               stableKeys() const { return true; }
    virtual ItemDescriptor     descriptorAt(int index) const        = 0;
    virtual std::optional<int> indexOfKey(const QString& key) const = 0;
    virtual QVariant           valueAt(int index) const             = 0;
};
using ItemDataSnapshotPtr = std::shared_ptr<const ItemDataSnapshot>;

template<class T>
class TypedItemDataSnapshot : public ItemDataSnapshot {
public:
    virtual T itemAt(int index) const = 0;
    QVariant  valueAt(int index) const final { return QVariant::fromValue(itemAt(index)); }
};

struct ItemRecord {
    QString  key;
    QString  reuseType;
    QVariant value;
    quint64  contentRevision = 0;
};

struct ItemValueUpdate {
    int                    index = -1;
    QVariant               value;
    std::optional<QString> reuseType;
    quint64                contentRevision = 0;
};

class ItemSnapshotRows;

class QML_MATERIAL_API ItemSnapshot {
public:
    ItemSnapshot();
    const quint64         revision;
    int                   count() const { return m_count; }
    bool                  stableKeys() const { return m_data && m_data->stableKeys(); }
    const ItemDescriptor& descriptorAt(int index) const;
    int                   indexOfKey(const QString& key) const;
    QVariant              valueAt(int index) const;
    ItemRecord            recordAt(int index) const;

private:
    friend class ItemSource;
    ItemSnapshot(quint64, ItemDataSnapshotPtr, QVector<ItemDescriptor>);
    ItemSnapshot(quint64, const ItemSnapshot&, const QVector<ItemValueUpdate>&);
    ItemDataSnapshotPtr                     m_data;
    std::shared_ptr<const ItemSnapshotRows> m_rows;
    int                                     m_count = 0;
};
using ItemSnapshotPtr = std::shared_ptr<const ItemSnapshot>;

QML_MATERIAL_API ItemDataSnapshotPtr makeItemDataSnapshot(QVector<ItemRecord>    rows,
                                                          bool                   stableKeys = true,
                                                          const ItemSnapshotPtr& before     = {});
} // namespace qml_material
