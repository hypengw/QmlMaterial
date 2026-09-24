#pragma once

#include "qml_material/control/control.hpp"
#include <memory>

namespace qml_material
{

class ScrollIndicatorAttached;
class ScrollViewport;

/** @ingroup control */
class QML_MATERIAL_API ScrollIndicator : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(ScrollIndicatorBase)
    QML_ATTACHED(ScrollIndicatorAttached)
    Q_PROPERTY(qreal size READ size WRITE setSize NOTIFY sizeChanged FINAL)
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(
        qreal minimumSize READ minimumSize WRITE setMinimumSize NOTIFY minimumSizeChanged FINAL)
    Q_PROPERTY(qreal visualSize READ visualSize NOTIFY visualSizeChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
    Q_PROPERTY(bool horizontal READ horizontal NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool vertical READ vertical NOTIFY orientationChanged FINAL)
public:
    explicit ScrollIndicator(QQuickItem* parent = nullptr);
    static ScrollIndicatorAttached* qmlAttachedProperties(QObject*);
    qreal                           size() const { return m_size; }
    qreal                           position() const { return m_position; }
    qreal                           minimumSize() const { return m_minimum_size; }
    qreal                           visualSize() const;
    qreal                           visualPosition() const;
    bool                            active() const { return m_active; }
    Qt::Orientation                 orientation() const { return m_orientation; }
    bool                            horizontal() const { return m_orientation == Qt::Horizontal; }
    bool                            vertical() const { return m_orientation == Qt::Vertical; }
    Q_SLOT void                     setSize(qreal);
    Q_SLOT void                     setPosition(qreal);
    void                            setMinimumSize(qreal);
    void                            setActive(bool);
    void                            setOrientation(Qt::Orientation);
    Q_SIGNAL void                   sizeChanged();
    Q_SIGNAL void                   positionChanged();
    Q_SIGNAL void                   minimumSizeChanged();
    Q_SIGNAL void                   visualSizeChanged();
    Q_SIGNAL void                   visualPositionChanged();
    Q_SIGNAL void                   activeChanged();
    Q_SIGNAL void                   orientationChanged();

protected:
    QRectF       contentRect() const override;
    virtual void setMoving(bool value) { setActive(value); }
    virtual void disconnectFlickable() {}

private:
    friend class ScrollIndicatorAttached;
    QPointF                           visualArea() const;
    void                              updateVisualArea(const QPointF& old);
    qreal                             m_size         = 0;
    qreal                             m_position     = 0;
    qreal                             m_minimum_size = 0;
    bool                              m_active       = false;
    Qt::Orientation                   m_orientation  = Qt::Vertical;
    QPointer<ScrollIndicatorAttached> m_attachment;
};

class QML_MATERIAL_API ScrollIndicatorAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(ScrollIndicator* horizontal READ horizontal WRITE setHorizontal NOTIFY
                   horizontalChanged FINAL)
    Q_PROPERTY(
        ScrollIndicator* vertical READ vertical WRITE setVertical NOTIFY verticalChanged FINAL)
public:
    explicit ScrollIndicatorAttached(QObject* parent);
    ScrollIndicatorAttached(QObject* target, QObject* parent, bool bidirectional);
    ~ScrollIndicatorAttached() override;
    ScrollIndicator* horizontal() const { return m_horizontal.item; }
    ScrollIndicator* vertical() const { return m_vertical.item; }
    void             setHorizontal(ScrollIndicator*);
    void             setVertical(ScrollIndicator*);
    Q_SIGNAL void    horizontalChanged();
    Q_SIGNAL void    verticalChanged();

private:
    struct Axis {
        QPointer<ScrollIndicator>      item;
        QList<QMetaObject::Connection> connections;
        qreal                          edge        = 0;
        bool                           layingOut   = false;
        bool                           syncing     = false;
        bool                           syncPending = false;
    };
    void                      attach(Axis&, ScrollIndicator*, bool horizontal);
    void                      detach(Axis&);
    void                      layout(Axis&, bool horizontal, bool force = false);
    void                      activate(Axis&, bool horizontal);
    void                      sync(Axis&, bool horizontal);
    void                      scroll(Axis&, bool horizontal);
    Q_SLOT void               syncHorizontal();
    Q_SLOT void               syncVertical();
    std::unique_ptr<ScrollViewport> m_viewport;
    QPointer<QQuickItem> m_flickable;
    bool                      m_bidirectional = false;
    Axis                      m_horizontal, m_vertical;
};

} // namespace qml_material

QML_DECLARE_TYPEINFO(qml_material::ScrollIndicator, QML_HAS_ATTACHED_PROPERTIES)
