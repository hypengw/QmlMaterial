#pragma once

#include "qml_material/control/control.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API BusyIndicator : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(BusyIndicatorBase)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged FINAL)

public:
    explicit BusyIndicator(QQuickItem* parent = nullptr): Control(parent) {}
    bool running() const { return m_running; }
    void setRunning(bool value) {
        if (m_running == value) return;
        m_running = value;
        Q_EMIT runningChanged();
    }
    Q_SIGNAL void runningChanged();

private:
    bool m_running = true;
};

} // namespace qml_material
