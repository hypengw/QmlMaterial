#pragma once

#include <QQmlComponent>
#include <QQmlContext>
#include "qml_material/control/control.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API PageIndicator : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(PageIndicatorBase)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged FINAL)
    Q_PROPERTY(
        int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(
        bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
public:
    explicit PageIndicator(QQuickItem* parent = nullptr);
    ~PageIndicator() override;
    int                     count() const { return m_count; }
    int                     currentIndex() const { return m_current_index; }
    bool                    interactive() const { return m_interactive; }
    QQmlComponent*          delegate() const { return m_delegate; }
    void                    setCount(int);
    void                    setCurrentIndex(int);
    void                    setInteractive(bool);
    void                    setDelegate(QQmlComponent*);
    Q_INVOKABLE QQuickItem* itemAt(int index) const;
    Q_SIGNAL void           countChanged();
    Q_SIGNAL void           currentIndexChanged();
    Q_SIGNAL void           interactiveChanged();
    Q_SIGNAL void           delegateChanged();

protected:
    void componentComplete() override;
    void updatePolish() override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseUngrabEvent() override;
    void touchEvent(QTouchEvent*) override;
    void touchUngrabEvent() override;
    void itemChange(ItemChange, const ItemChangeData&) override;

private:
    struct Entry {
        QPointer<QQuickItem>  item;
        QPointer<QQmlContext> context;
        bool                  pressedProperty = false;
    };
    void scheduleDelegates();
    void clearDelegates();
    void setPressed(int);
    void cancelPress();
    int  hitIndex(const QPointF&) const;
    void releasePress(const QPointF&);

    QList<Entry>                   m_items;
    QPointer<QQmlComponent>        m_delegate;
    QList<QMetaObject::Connection> m_delegate_connections;
    int                            m_count         = 0;
    int                            m_current_index = 0;
    int                            m_pressed_index = -1;
    int                            m_touch_id      = -1;
    bool                           m_interactive   = false;
    bool                           m_mouse_pressed = false;
    bool                           m_rebuild       = false;
    bool                           m_syncing       = false;
};

} // namespace qml_material
