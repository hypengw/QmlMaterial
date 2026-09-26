#pragma once

#include "qml_material/control/container.hpp"
#include <QHash>
#include <QQmlComponent>
#include <limits>
#include <memory>
#include <QtQuick/private/qquicktransition_p.h>

namespace qml_material
{
class SplitViewAttached;
class SplitHandleInput;
struct SplitResizeState;
struct SplitPaneState;
namespace split_layout
{
struct Pane;
}

class QML_MATERIAL_API SplitView : public Container {
    Q_OBJECT
    QML_NAMED_ELEMENT(SplitViewBase)
    QML_ATTACHED(SplitViewAttached)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
    Q_PROPERTY(QQmlComponent* handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    Q_PROPERTY(bool resizing READ isResizing NOTIFY resizingChanged FINAL)
    Q_PROPERTY(QQuickTransition* expandTransition READ expandTransition WRITE setExpandTransition
                   NOTIFY transitionsChanged FINAL)
    Q_PROPERTY(QQuickTransition* collapseTransition READ collapseTransition WRITE
                   setCollapseTransition NOTIFY transitionsChanged FINAL)
public:
    explicit SplitView(QQuickItem* parent = nullptr);
    ~SplitView() override;
    static SplitViewAttached* qmlAttachedProperties(QObject*);
    Qt::Orientation           orientation() const { return m_orientation; }
    void                      setOrientation(Qt::Orientation);
    Q_SIGNAL void             orientationChanged();
    QQmlComponent*            handle() const { return m_handle; }
    void                      setHandle(QQmlComponent*);
    Q_INVOKABLE QQuickItem*   handleItemAt(int index) const;
    Q_SIGNAL void             handleChanged();
    bool                      isResizing() const;
    Q_SIGNAL void             resizingChanged();
    Q_INVOKABLE QVariant      saveState() const;
    Q_INVOKABLE bool          restoreState(const QVariant&);
    QQuickTransition*         expandTransition() const { return m_expand; }
    QQuickTransition*         collapseTransition() const { return m_collapse; }
    void                      setExpandTransition(QQuickTransition*);
    void                      setCollapseTransition(QQuickTransition*);
    Q_SIGNAL void             transitionsChanged();

protected:
    bool isContent(QQuickItem*) const override;
    void itemAdded(QQuickItem*) override;
    void itemRemoved(QQuickItem*) override;
    void itemsChanged() override;
    void updatePolish() override;
    void componentComplete() override;

private:
    friend class SplitHandleInput;
    friend class SplitViewAttached;
    void                                                changeExpanded(QQuickItem*);
    void                                                completeExpansion(QQuickItem*, quint64);
    void                                                settleExpansions(bool notify);
    void                                                cancelExpansion(QQuickItem*);
    QHash<QQuickItem*, std::shared_ptr<SplitPaneState>> m_panes;
    QPointer<QQuickTransition>                          m_expand, m_collapse;
    bool                                                m_updating_hosts = false;
    bool                                                beginResize(QQuickItem*, const QPointF&);
    void                                                moveResize(const QPointF&);
    void                                                endResize();
    void                                                invalidateLayout();
    QList<split_layout::Pane>                           layoutPanes(Qt::Orientation) const;
    std::unique_ptr<SplitResizeState>                   m_resize;
    bool                                                m_writing_preferred = false;
    void                                                requestLayout();
    bool                                                syncHandles();
    void                                                clearHandles();
    QPointer<QQmlComponent>                             m_handle;
    QList<QPointer<QQuickItem>>                         m_handles;
    QList<QMetaObject::Connection>                      m_handle_connections;
    bool                                                m_handles_dirty = false;
    bool                                                m_destroying    = false;
    Qt::Orientation                                     m_orientation   = Qt::Horizontal;
    QHash<QQuickItem*, QList<QMetaObject::Connection>>  m_connections;
    quint64                                             m_layout_revision  = 0;
    bool                                                m_laying_out       = false;
    quint64                                             m_restore_revision = 0;
};

class QML_MATERIAL_API SplitHandleAttached : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(SplitHandle)
    QML_UNCREATABLE("SplitHandle is an attached property")
    QML_ATTACHED(SplitHandleAttached)
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)
public:
    explicit SplitHandleAttached(QObject* parent): QObject(parent) {}
    static SplitHandleAttached* qmlAttachedProperties(QObject* object) {
        return new SplitHandleAttached(object);
    }
    bool          isHovered() const { return m_hovered; }
    bool          isPressed() const { return m_pressed; }
    Q_SIGNAL void hoveredChanged();
    Q_SIGNAL void pressedChanged();

private:
    friend class SplitHandleInput;
    friend class SplitView;
    void setHovered(bool);
    void setPressed(bool);
    bool m_hovered = false;
    bool m_pressed = false;
};

class QML_MATERIAL_API SplitViewAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(SplitView* view READ view NOTIFY viewChanged FINAL)
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth RESET resetMinimumWidth
                   NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(qreal minimumHeight READ minimumHeight WRITE setMinimumHeight RESET
                   resetMinimumHeight NOTIFY minimumHeightChanged FINAL)
    Q_PROPERTY(qreal preferredWidth READ preferredWidth WRITE setPreferredWidth RESET
                   resetPreferredWidth NOTIFY preferredWidthChanged FINAL)
    Q_PROPERTY(qreal preferredHeight READ preferredHeight WRITE setPreferredHeight RESET
                   resetPreferredHeight NOTIFY preferredHeightChanged FINAL)
    Q_PROPERTY(qreal maximumWidth READ maximumWidth WRITE setMaximumWidth RESET resetMaximumWidth
                   NOTIFY maximumWidthChanged FINAL)
    Q_PROPERTY(qreal maximumHeight READ maximumHeight WRITE setMaximumHeight RESET
                   resetMaximumHeight NOTIFY maximumHeightChanged FINAL)
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged FINAL)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight NOTIFY fillHeightChanged FINAL)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(bool transitioning READ isTransitioning NOTIFY transitioningChanged FINAL)
public:
    explicit SplitViewAttached(QObject*);
    SplitView*    view() const;
    qreal         minimumWidth() const { return m_minimum_width; }
    qreal         minimumHeight() const { return m_minimum_height; }
    qreal         preferredWidth() const { return m_preferred_width.value_or(-1); }
    qreal         preferredHeight() const { return m_preferred_height.value_or(-1); }
    qreal         maximumWidth() const { return m_maximum_width; }
    qreal         maximumHeight() const { return m_maximum_height; }
    bool          fillWidth() const { return m_fill_width; }
    bool          fillHeight() const { return m_fill_height; }
    bool          isExpanded() const { return m_expanded; }
    bool          isTransitioning() const { return m_transitioning; }
    void          setExpanded(bool);
    Q_SIGNAL void expandedChanged();
    Q_SIGNAL void transitioningChanged();
    Q_SIGNAL void expandedCompleted();
    Q_SIGNAL void collapsedCompleted();
    void          setMinimumWidth(qreal);
    void          setMinimumHeight(qreal);
    void          setPreferredWidth(qreal);
    void          setPreferredHeight(qreal);
    void          setMaximumWidth(qreal);
    void          setMaximumHeight(qreal);
    void          setFillWidth(bool);
    void          setFillHeight(bool);
    void          resetMinimumWidth();
    void          resetMinimumHeight();
    void          resetPreferredWidth();
    void          resetPreferredHeight();
    void          resetMaximumWidth();
    void          resetMaximumHeight();
    Q_SIGNAL void viewChanged();
    Q_SIGNAL void minimumWidthChanged();
    Q_SIGNAL void minimumHeightChanged();
    Q_SIGNAL void preferredWidthChanged();
    Q_SIGNAL void preferredHeightChanged();
    Q_SIGNAL void maximumWidthChanged();
    Q_SIGNAL void maximumHeightChanged();
    Q_SIGNAL void fillWidthChanged();
    Q_SIGNAL void fillHeightChanged();
    Q_SIGNAL void constraintsChanged();

private:
    friend class SplitView;
    QPointer<ContainerAttached> m_container;
    qreal                       m_minimum_width  = 0;
    qreal                       m_minimum_height = 0;
    qreal                       m_maximum_width  = std::numeric_limits<qreal>::infinity();
    qreal                       m_maximum_height = std::numeric_limits<qreal>::infinity();
    std::optional<qreal>        m_preferred_width;
    std::optional<qreal>        m_preferred_height;
    bool                        m_fill_width    = false;
    bool                        m_fill_height   = false;
    bool                        m_expanded      = true;
    bool                        m_transitioning = false;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::SplitView, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPEINFO(qml_material::SplitHandleAttached, QML_HAS_ATTACHED_PROPERTIES)
