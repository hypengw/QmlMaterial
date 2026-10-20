#pragma once

#include <QQuickItem>
#include <QPointer>
#include "qml_material/input/time_state.hpp"
#include "qml_material/input/input_host.hpp"

namespace qml_material
{
class QML_MATERIAL_API TimeDial : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qml_material::TimeState* time READ time WRITE setTime NOTIFY timeChanged FINAL)
    Q_PROPERTY(qreal handAngle READ handAngle NOTIFY metricsChanged FINAL)
    Q_PROPERTY(qreal handLength READ handLength NOTIFY metricsChanged FINAL)
    Q_PROPERTY(QVariantList labels READ labels NOTIFY metricsChanged FINAL)
    Q_PROPERTY(InteractionSource* interactionSource READ interactionSource WRITE
                   setInteractionSource NOTIFY interactionSourceChanged FINAL)
public:
    explicit TimeDial(QQuickItem* parent = nullptr);
    ~TimeDial() override;
    TimeState*         time() const { return m_time; }
    void               setTime(TimeState*);
    qreal              handAngle() const;
    qreal              handLength() const;
    QVariantList       labels() const;
    InteractionSource* interactionSource() { return m_input.source(); }
    void          setInteractionSource(InteractionSource* source) { m_input.setSource(source); }
    Q_SIGNAL void timeChanged();
    Q_SIGNAL void metricsChanged();
    Q_SIGNAL void interactionSourceChanged();

protected:
    void geometryChange(const QRectF&, const QRectF&) override;
    bool event(QEvent*) override;
    void touchUngrabEvent() override;
    void mouseUngrabEvent() override;
    void keyPressEvent(QKeyEvent*) override;

private:
    qreal                   radius() const;
    void                    update(const QPointF&, bool tap);
    void                    select(int value, bool complete);
    void                    cancel();
    QPointer<TimeState>     m_time;
    QMetaObject::Connection m_changed, m_destroyed;
    bool                    m_updating = false;
    quint64                 m_revision = 0;
    InputHost               m_input;
};
} // namespace qml_material
