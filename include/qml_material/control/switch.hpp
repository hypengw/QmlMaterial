#pragma once
#include "qml_material/control/toggle_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API Switch : public ToggleButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(SwitchBase)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool dragging READ isDragging NOTIFY draggingChanged FINAL)
public:
    explicit Switch(QQuickItem* parent = nullptr);
    ~Switch() override;
    qreal         position() const { return m_dragPosition.value_or(isChecked() ? 1 : 0); }
    qreal         visualPosition() const { return mirrored() ? 1 - position() : position(); }
    bool          isDragging() const { return m_dragPosition.has_value(); }
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void visualPositionChanged();
    Q_SIGNAL void draggingChanged();

private:
    void                           checkedChange() override;
    void                           observeIndicator();
    qreal                          positionAt(QPointF) const;
    void                           setDragPosition(qreal);
    void                           finishDrag(QPointF, bool cancelled);
    void                           notifyPosition();
    std::optional<qreal>           m_dragPosition;
    quint64                        m_dragRevision = 0;
    QList<QMetaObject::Connection> m_indicatorConnections;
};
} // namespace qml_material
