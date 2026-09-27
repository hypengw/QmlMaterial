#pragma once

#include <QQuickItem>
#include <QPointer>
#include <functional>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API ItemHolder : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickItem* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(qreal itemImplicitWidth READ itemImplicitWidth NOTIFY itemImplicitSizeChanged FINAL)
    Q_PROPERTY(
        qreal itemImplicitHeight READ itemImplicitHeight NOTIFY itemImplicitSizeChanged FINAL)
public:
    explicit ItemHolder(QQuickItem* parent = nullptr): QQuickItem(parent) {}
    ~ItemHolder() override;
    QQuickItem*   item() const { return m_item; }
    void          setItem(QQuickItem*);
    qreal         itemImplicitWidth() const;
    qreal         itemImplicitHeight() const;
    Q_SIGNAL void itemChanged();
    Q_SIGNAL void itemImplicitSizeChanged();

protected:
    void classBegin() override;

private:
    QPointer<QQuickItem>           m_item;
    QQuickItem*                    m_identity = nullptr;
    QList<QMetaObject::Connection> m_connections;
    std::function<void()>          m_releaseBinding;
    quint64                        m_revision = 0;
};
} // namespace qml_material
