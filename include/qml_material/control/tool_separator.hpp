#pragma once

#include "qml_material/control/control.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API ToolSeparator : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(ToolSeparator)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
    Q_PROPERTY(bool horizontal READ horizontal NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool vertical READ vertical NOTIFY orientationChanged FINAL)

public:
    explicit ToolSeparator(QQuickItem* parent = nullptr): Control(parent) {}
    Qt::Orientation orientation() const { return m_orientation; }
    bool            horizontal() const { return m_orientation == Qt::Horizontal; }
    bool            vertical() const { return m_orientation == Qt::Vertical; }
    void            setOrientation(Qt::Orientation value) {
        if (m_orientation == value) return;
        m_orientation = value;
        Q_EMIT orientationChanged();
    }
    Q_SIGNAL void orientationChanged();

private:
    Qt::Orientation m_orientation = Qt::Vertical;
};

} // namespace qml_material
