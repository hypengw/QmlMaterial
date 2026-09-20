#pragma once
#include "qml_material/control/abstract_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API Switch : public AbstractButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(SwitchBase)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
public:
    explicit Switch(QQuickItem* parent = nullptr);
    qreal         position() const { return m_position; }
    qreal         visualPosition() const { return mirrored() ? 1 - m_position : m_position; }
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void visualPositionChanged();

protected:
    void nextCheckState() override;
    void pointerStarted(const QPointF&) override;
    void pointerMoved(const QPointF&) override;
    void interactionEnded() override;
    bool retainPressOutside() const override { return true; }

private:
    qreal   positionAt(const QPointF&) const;
    void    setPosition(qreal);
    qreal   m_position = 0;
    QPointF m_origin;
    bool    m_dragging = false;
};
} // namespace qml_material
