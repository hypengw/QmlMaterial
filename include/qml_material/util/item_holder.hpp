#pragma once

#include <QtQml/QQmlEngine>
#include <QPointer>

namespace qml_material
{
class ItemHolder : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL)
public:
    ItemHolder(QObject* parent = nullptr);
    ~ItemHolder();

    QObject* item() const;
    bool     visible() const;

    Q_SLOT void setItem(QObject*);
    Q_SLOT void setVisible(bool);
    Q_SLOT void refreshParent();

    Q_SIGNAL void itemChanged();
    Q_SIGNAL void visibleChanged();

private:
    QPointer<QObject> m_item;
    bool              m_visible;
};

} // namespace qml_material