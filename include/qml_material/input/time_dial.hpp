#pragma once

#include <QQuickItem>
#include <QPointer>
#include "qml_material/input/time_state.hpp"

namespace qml_material
{
class QML_MATERIAL_API TimeDial : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qml_material::TimeState* time READ time WRITE setTime NOTIFY timeChanged FINAL)
    Q_PROPERTY(qreal handAngle READ handAngle NOTIFY metricsChanged FINAL)
    Q_PROPERTY(qreal handLength READ handLength NOTIFY metricsChanged FINAL)
    Q_PROPERTY(QVariantList labels READ labels NOTIFY metricsChanged FINAL)
    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged FINAL)
public:
    explicit TimeDial(QQuickItem* parent = nullptr);
    TimeState*    time() const { return m_time; }
    void          setTime(TimeState*);
    qreal         handAngle() const;
    qreal         handLength() const;
    QVariantList  labels() const;
    bool          pressed() const { return m_active; }
    Q_SIGNAL void timeChanged();
    Q_SIGNAL void metricsChanged();
    Q_SIGNAL void pressedChanged();

protected:
    void geometryChange(const QRectF&, const QRectF&) override;
    void itemChange(ItemChange, const ItemChangeData&) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseUngrabEvent() override;
    void touchEvent(QTouchEvent*) override;
    void touchUngrabEvent() override;
    void keyPressEvent(QKeyEvent*) override;

private:
    qreal                   radius() const;
    bool                    begin(const QPointF&);
    void                    update(const QPointF&, bool complete);
    void                    cancel();
    QPointer<TimeState>     m_time;
    QMetaObject::Connection m_changed, m_destroyed, m_windowActive;
    QPointF                 m_start;
    bool                    m_active = false, m_moved = false, m_updating = false;
    int                     m_touchId  = -1;
    quint64                 m_revision = 0;
};
} // namespace qml_material
