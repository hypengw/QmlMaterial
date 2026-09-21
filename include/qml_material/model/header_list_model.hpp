#pragma once

#include <QAbstractTableModel>
#include <QPointer>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API HeaderListModel : public QAbstractTableModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
public:
    explicit HeaderListModel(QObject* parent = nullptr);
    QVariant        source() const;
    void            setSource(const QVariant& source);
    bool            trySetSource(const QVariant& source);
    Qt::Orientation orientation() const { return m_orientation; }
    void            setOrientation(Qt::Orientation orientation);
    int             rowCount(const QModelIndex& parent = {}) const override;
    int             columnCount(const QModelIndex& parent = {}) const override;
    QVariant        data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_SIGNAL void          sourceChanged();
    Q_SIGNAL void          orientationChanged();

private:
    int                          count() const;
    QModelIndex                  sectionIndex(int section) const;
    void                         connectSource();
    QVariant                     m_source;
    QPointer<QAbstractItemModel> m_model;
    QVariantList                 m_values;
    QHash<int, QByteArray>       m_roles;
    Qt::Orientation              m_orientation  = Qt::Horizontal;
    int                          m_integerCount = 0;
};
} // namespace qml_material
