#pragma once

#include "qml_material/control/action.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/drawer.hpp"
#include "qml_material/control/navigation_control.hpp"
#include "qml_material/control/adaptive_presenter.hpp"

namespace qml_material
{
class QML_MATERIAL_API NavigationRail : public NavigationControl {
    Q_OBJECT
    QML_NAMED_ELEMENT(NavigationRailBase)
    Q_PROPERTY(bool useEmbed READ useEmbed WRITE setUseEmbed NOTIFY useEmbedChanged FINAL)
    Q_PROPERTY(bool forceModal READ forceModal WRITE setForceModal NOTIFY forceModalChanged FINAL)
    Q_PROPERTY(bool useModal READ useModal NOTIFY useModalChanged FINAL)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(bool autoExpand READ autoExpand WRITE setAutoExpand NOTIFY autoExpandChanged FINAL)
    Q_PROPERTY(bool drawerRequested READ drawerRequested NOTIFY drawerRequestedChanged FINAL)
    Q_PROPERTY(Drawer* drawer READ drawer WRITE setDrawer NOTIFY drawerChanged FINAL)
    Q_PROPERTY(CommandButton* headerControl READ headerControl WRITE setHeaderControl NOTIFY
                   headerControlChanged FINAL)
    Q_PROPERTY(
        PresentationSite* popupSite READ popupSite WRITE setPopupSite NOTIFY popupSiteChanged FINAL)
    Q_PROPERTY(Action* defaultHeaderAction READ defaultHeaderAction CONSTANT FINAL)
public:
    explicit NavigationRail(QQuickItem* parent = nullptr);
    ~NavigationRail() override;
    bool              useEmbed() const { return m_embed; }
    void              setUseEmbed(bool);
    bool              forceModal() const { return m_forceModal; }
    void              setForceModal(bool);
    bool              useModal() const { return m_forceModal || ! m_embed; }
    bool              expanded() const { return m_expanded; }
    void              setExpanded(bool);
    bool              autoExpand() const { return m_autoExpand; }
    void              setAutoExpand(bool);
    bool              drawerRequested() const { return m_requested; }
    Drawer*           drawer() const { return m_drawer; }
    void              setDrawer(Drawer*);
    CommandButton*    headerControl() const { return m_header; }
    void              setHeaderControl(CommandButton*);
    PresentationSite* popupSite() const { return m_site; }
    void              setPopupSite(PresentationSite*);
    Action*           defaultHeaderAction() const { return m_action; }
    Q_INVOKABLE void  open();
    Q_INVOKABLE void  close();
    Q_INVOKABLE void  toggle();
    Q_SIGNAL void     useEmbedChanged();
    Q_SIGNAL void     forceModalChanged();
    Q_SIGNAL void     useModalChanged();
    Q_SIGNAL void     expandedChanged();
    Q_SIGNAL void     autoExpandChanged();
    Q_SIGNAL void     drawerRequestedChanged();
    Q_SIGNAL void     drawerChanged();
    Q_SIGNAL void     headerControlChanged();
    Q_SIGNAL void     popupSiteChanged();

protected:
    void componentComplete() override;

private:
    void                           cancelHeader();
    void                           setRequested(bool);
    Action* const                  m_action;
    QPointer<Drawer>               m_drawer;
    QPointer<CommandButton>        m_header;
    QPointer<PresentationSite>     m_site;
    QList<QMetaObject::Connection> m_drawerConnections, m_headerConnections, m_siteConnections;
    quint64                        m_revision = 0;
    bool m_embed = false, m_forceModal = false, m_expanded = false, m_autoExpand = true,
         m_requested = false;
};
} // namespace qml_material
