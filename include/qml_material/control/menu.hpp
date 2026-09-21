#pragma once

#include "qml_material/control/popup.hpp"

class QQmlObjectModel;

namespace qml_material
{
class QML_MATERIAL_API Menu : public Popup {
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuBase)
    Q_PROPERTY(QObject* contentModel READ contentModel CONSTANT FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(
        int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(Menu* parentMenu READ parentMenu WRITE setParentMenu NOTIFY parentMenuChanged FINAL)
public:
    explicit Menu(QObject* parent = nullptr);
    ~Menu() override;
    QObject*                contentModel() const;
    int                     count() const { return m_items.size(); }
    int                     currentIndex() const { return m_index; }
    void                    setCurrentIndex(int);
    QString                 title() const { return m_title; }
    void                    setTitle(const QString&);
    QQmlComponent*          delegate() const { return m_delegate; }
    void                    setDelegate(QQmlComponent*);
    Menu*                   parentMenu() const { return m_parentMenu; }
    void                    setParentMenu(Menu*);
    Q_INVOKABLE QQuickItem* itemAt(int) const;
    Q_INVOKABLE int         indexOf(QQuickItem*) const;
    Q_INVOKABLE void        addItem(QQuickItem*);
    Q_INVOKABLE void        insertItem(int, QQuickItem*);
    Q_INVOKABLE void        moveItem(int, int);
    Q_INVOKABLE QQuickItem* takeItem(int);
    Q_INVOKABLE void        removeItem(QQuickItem*);
    Q_INVOKABLE void        dismiss();
    Q_INVOKABLE void        openSubMenu(Menu*, QQuickItem* anchor);
    Q_INVOKABLE void        popup(qreal x, qreal y);
    Q_SIGNAL void           countChanged();
    Q_SIGNAL void           currentIndexChanged();
    Q_SIGNAL void           titleChanged();
    Q_SIGNAL void           delegateChanged();
    Q_SIGNAL void           parentMenuChanged();

private:
    void                                        removeAt(int);
    QQmlObjectModel*                            m_model;
    QList<QQuickItem*>                          m_items;
    QHash<QQuickItem*, QMetaObject::Connection> m_destroyConnections;
    QPointer<QQmlComponent>                     m_delegate;
    QPointer<Menu>                              m_parentMenu;
    QPointer<Menu>                              m_childMenu;
    QMetaObject::Connection                     m_parentClosing;
    QMetaObject::Connection                     m_parentDestroyed;
    QString                                     m_title;
    int                                         m_index = -1;
};
} // namespace qml_material
