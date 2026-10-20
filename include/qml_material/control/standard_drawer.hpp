#pragma once

#include "qml_material/control/action.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/drawer.hpp"
#include "qml_material/control/navigation_control.hpp"

namespace qml_material
{
class QML_MATERIAL_API StandardDrawer : public NavigationControl {
    Q_OBJECT
    QML_NAMED_ELEMENT(StandardDrawerBase)
    Q_PROPERTY(bool useEmbed READ useEmbed WRITE setUseEmbed NOTIFY useEmbedChanged FINAL)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(Drawer* drawer READ drawer WRITE setDrawer NOTIFY drawerChanged FINAL)
    Q_PROPERTY(CommandButton* headerControl READ headerControl WRITE setHeaderControl NOTIFY
                   headerControlChanged FINAL)
    Q_PROPERTY(Action* defaultHeaderAction READ defaultHeaderAction CONSTANT FINAL)
public:
    explicit StandardDrawer(QQuickItem* parent = nullptr);
    ~StandardDrawer() override;
    bool             useEmbed() const { return m_useEmbed; }
    void             setUseEmbed(bool);
    bool             expanded() const { return m_expanded; }
    void             setExpanded(bool);
    Drawer*          drawer() const { return m_drawer; }
    void             setDrawer(Drawer*);
    CommandButton*   headerControl() const { return m_header; }
    void             setHeaderControl(CommandButton*);
    Action*          defaultHeaderAction() const { return m_defaultHeaderAction; }
    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
    Q_INVOKABLE void toggle();
    Q_SIGNAL void    useEmbedChanged();
    Q_SIGNAL void    expandedChanged();
    Q_SIGNAL void    drawerChanged();
    Q_SIGNAL void    headerControlChanged();

private:
    void                           invalidateHeader();
    Action* const                  m_defaultHeaderAction;
    QPointer<Drawer>               m_drawer;
    QPointer<CommandButton>        m_header;
    QList<QMetaObject::Connection> m_drawerConnections;
    QList<QMetaObject::Connection> m_headerConnections;
    quint64                        m_revision = 0;
    bool                           m_useEmbed = false;
    bool                           m_expanded = false;
};
} // namespace qml_material
