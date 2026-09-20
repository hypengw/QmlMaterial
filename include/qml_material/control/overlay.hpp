#pragma once

#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QPointer>
#include "qml_material/export.hpp"

namespace qml_material
{
class Popup;
class Drawer;
class OverlayAttached;

class QML_MATERIAL_API OverlayManager : public QQuickItem {
    Q_OBJECT
    QML_ANONYMOUS
public:
    static OverlayManager* get(QQuickWindow*);
    ~OverlayManager() override;
    void add(Popup*);
    void remove(Popup*);
    void refresh();
    void updateDimmer(Popup*);
    void registerDrawer(Drawer*);
    void unregisterDrawer(Drawer*);
    void releaseDrawer(Drawer*);
    int  popupCount() const { return m_entries.size(); }

protected:
    bool eventFilter(QObject*, QEvent*) override;

private:
    explicit OverlayManager(QQuickWindow*);
    void    resize();
    bool    press(const QPointF&);
    bool    release(const QPointF&);
    bool    blocks(const QPointF&) const;
    Popup*  topModal() const;
    void    restoreFocus(Popup*, QQuickItem*);
    Drawer* dragCandidate(const QPointF&) const;
    bool    drawerPointer(QPointerEvent*, const QEventPoint&);
    void    cancelDrawerDrag();
    struct Entry {
        QPointer<Popup>         popup;
        QPointer<QQuickItem>    dimmer;
        QPointer<QQmlComponent> component;
        QPointer<QQuickItem>    previousFocus;
        qreal                   opacity = 1;
    };
    QList<Entry>                    m_entries;
    QPointer<QQuickWindow>          m_window;
    QPointer<Popup>                 m_pressOwner;
    QList<QPointer<Drawer>>         m_drawers;
    QPointer<Drawer>                m_dragDrawer;
    QPointer<const QPointingDevice> m_dragDevice;
    int                             m_dragId = -1;
    bool m_pressed = false, m_blocked = false, m_outside = false, m_outsideParent = false;
    bool m_destroying = false;
    int  m_touchId    = -1;
};

class QML_MATERIAL_API Overlay : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Overlay)
    QML_UNCREATABLE("Overlay is an attached type")
    QML_ATTACHED(OverlayAttached)
public:
    static OverlayAttached* qmlAttachedProperties(QObject*);
};

class QML_MATERIAL_API OverlayAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickItem* overlay READ overlay NOTIFY overlayChanged FINAL)
    Q_PROPERTY(QQmlComponent* modal READ modal WRITE setModal NOTIFY modalChanged FINAL)
    Q_PROPERTY(QQmlComponent* modeless READ modeless WRITE setModeless NOTIFY modelessChanged FINAL)
public:
    explicit OverlayAttached(QObject*);
    QQuickItem*    overlay() const;
    QQmlComponent* modal() const { return m_modal; }
    QQmlComponent* modeless() const { return m_modeless; }
    void           setModal(QQmlComponent*);
    void           setModeless(QQmlComponent*);
    Q_SIGNAL void  overlayChanged();
    Q_SIGNAL void  modalChanged();
    Q_SIGNAL void  modelessChanged();

private:
    QQuickWindow*           window() const;
    QPointer<QQmlComponent> m_modal, m_modeless;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::Overlay, QML_HAS_ATTACHED_PROPERTIES)
