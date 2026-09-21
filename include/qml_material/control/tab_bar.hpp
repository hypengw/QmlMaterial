#pragma once
#include "qml_material/control/container.hpp"
#include "qml_material/control/tab_button.hpp"
#include "qml_material/control/button_group.hpp"

namespace qml_material
{
class TabBarAttached;
class QML_MATERIAL_API TabBar : public Container {
    Q_OBJECT
    QML_NAMED_ELEMENT(TabBarBase)
    QML_ATTACHED(TabBarAttached)
    Q_PROPERTY(
        qml_material::TabButton* currentItem READ currentItem NOTIFY currentItemChanged FINAL)
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)
public:
    enum Position
    {
        Header,
        Footer
    };
    Q_ENUM(Position)
    explicit TabBar(QQuickItem* parent = nullptr);
    ~TabBar() override;
    static TabBarAttached* qmlAttachedProperties(QObject*);
    TabButton* currentItem() const { return qobject_cast<TabButton*>(Container::currentItem()); }
    Position   position() const { return m_position; }
    void       setPosition(Position);
    Q_INVOKABLE TabButton* itemAt(int index) const {
        return qobject_cast<TabButton*>(Container::itemAt(index));
    }
    void          incrementCurrentIndex() override;
    void          decrementCurrentIndex() override;
    Q_SIGNAL void positionChanged();

protected:
    bool isContent(QQuickItem*) const override;
    void itemAdded(QQuickItem*) override;
    void itemRemoved(QQuickItem*) override;
    void itemsChanged() override;
    void currentItemChange() override;
    int  initialIndex() const override;
    void updatePolish() override;
    void keyPressEvent(QKeyEvent*) override;

private:
    void                                               navigate(int);
    ButtonGroup*                                       m_group;
    QHash<QQuickItem*, QList<QMetaObject::Connection>> m_connections;
    Position                                           m_position        = Header;
    bool                                               m_selecting       = false;
    bool                                               m_selection_dirty = false;
};
class QML_MATERIAL_API TabBarAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(TabBar* tabBar READ tabBar NOTIFY tabBarChanged FINAL)
    Q_PROPERTY(TabBar::Position position READ position NOTIFY positionChanged FINAL)
public:
    explicit TabBarAttached(QObject* parent): QObject(parent) {}
    int              index() const { return m_index; }
    TabBar*          tabBar() const { return m_bar; }
    TabBar::Position position() const { return m_bar ? m_bar->position() : TabBar::Header; }
    Q_SIGNAL void    indexChanged();
    Q_SIGNAL void    tabBarChanged();
    Q_SIGNAL void    positionChanged();

private:
    friend class TabBar;
    void             update(TabBar*, int);
    QPointer<TabBar> m_bar;
    int              m_index = -1;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::TabBar, QML_HAS_ATTACHED_PROPERTIES)
