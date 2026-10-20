#pragma once

#include "qml_material/control/command_button.hpp"
#include "qml_material/control/menu.hpp"

namespace qml_material
{
class QML_MATERIAL_API MenuButton : public CommandButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuButtonBase)
    Q_PROPERTY(Menu* menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(
        QQuickItem* menuAnchor READ menuAnchor WRITE setMenuAnchor NOTIFY menuAnchorChanged FINAL)
    Q_PROPERTY(bool expanded READ isExpanded NOTIFY expandedChanged FINAL)
public:
    explicit MenuButton(QQuickItem* parent = nullptr): CommandButton(parent) {}
    ~MenuButton() override;
    Menu*         menu() const { return m_menu; }
    void          setMenu(Menu*);
    QQuickItem*   menuAnchor() const;
    void          setMenuAnchor(QQuickItem*);
    bool          isExpanded() const;
    Q_SIGNAL void menuChanged();
    Q_SIGNAL void menuAnchorChanged();
    Q_SIGNAL void expandedChanged();

protected:
    void activate() override;

private:
    QPointer<Menu>                 m_menu;
    QPointer<QQuickItem>           m_anchor;
    QList<QMetaObject::Connection> m_menuConnections;
    QMetaObject::Connection        m_anchorDestroyed;
    quint64                        m_revision = 0;
};
} // namespace qml_material
