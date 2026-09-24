#pragma once

#include "qml_material/model/item_source.hpp"

namespace qml_material
{
class QML_MATERIAL_API LazyRow : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Rows are supplied by a lazy view")
    Q_PROPERTY(int index READ index NOTIFY changed FINAL)
    Q_PROPERTY(QString key READ key NOTIFY changed FINAL)
    Q_PROPERTY(QVariant value READ value NOTIFY changed FINAL)
    Q_PROPERTY(quint64 revision READ revision NOTIFY changed FINAL)
    Q_PROPERTY(quint64 contentRevision READ contentRevision NOTIFY changed FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
public:
    using QObject::QObject;
    int           index() const { return m_index; }
    QString       key() const { return m_record.key; }
    QVariant      value() const { return m_record.value; }
    quint64       revision() const { return m_revision; }
    quint64       contentRevision() const { return m_record.contentRevision; }
    bool          active() const { return m_active; }
    void          bind(const ItemSnapshotPtr&, int index);
    void          pool();
    Q_SIGNAL void changed();
    Q_SIGNAL void activeChanged();
    Q_SIGNAL void pooled();
    Q_SIGNAL void reused();

private:
    ItemRecord m_record;
    int        m_index      = -1;
    quint64    m_revision   = 0;
    bool       m_active     = false;
    quint64    m_generation = 0;
};
} // namespace qml_material
