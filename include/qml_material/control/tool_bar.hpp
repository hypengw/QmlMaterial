#pragma once

#include "qml_material/control/panel.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API ToolBar : public Panel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ToolBarBase)
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)

public:
    enum Position
    {
        Header,
        Footer
    };
    Q_ENUM(Position)

    explicit ToolBar(QQuickItem* parent = nullptr): Panel(parent) {}

    Position position() const { return m_position; }
    void     setPosition(Position value) {
        if (m_position == value) return;
        m_position = value;
        Q_EMIT positionChanged();
    }

    Q_SIGNAL void positionChanged();

private:
    Position m_position = Header;
};

} // namespace qml_material
