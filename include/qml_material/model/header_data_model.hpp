#pragma once

#include <QAbstractTableModel>
#include <QPointer>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API HeaderDataModel : public QAbstractTableModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY
                   sourceModelChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
public:
    explicit HeaderDataModel(QObject* parent = nullptr);
    QAbstractItemModel* sourceModel() const { return m_source; }
    Qt::Orientation     orientation() const { return m_orientation; }
    void                setSourceModel(QAbstractItemModel* source);
    void                setOrientation(Qt::Orientation orientation);
    int                 rowCount(const QModelIndex& parent = {}) const override;
    int                 columnCount(const QModelIndex& parent = {}) const override;
    QVariant            data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;
    Q_SIGNAL void          sourceModelChanged();
    Q_SIGNAL void          orientationChanged();

private:
    bool                         validIndex(const QModelIndex& index) const;
    void                         connectSource();
    QPointer<QAbstractItemModel> m_source;
    Qt::Orientation              m_orientation = Qt::Horizontal;
};
} // namespace qml_material
