#pragma once

#include "qml_material/control/command_button.hpp"

namespace qml_material
{
/** @ingroup control */
class QML_MATERIAL_API RepeatButton : public CommandButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(RepeatButtonBase)
    Q_PROPERTY(int repeatDelay READ repeatDelay WRITE setRepeatDelay NOTIFY repeatChanged FINAL)
    Q_PROPERTY(
        int repeatInterval READ repeatInterval WRITE setRepeatInterval NOTIFY repeatChanged FINAL)
public:
    explicit RepeatButton(QQuickItem* parent = nullptr): CommandButton(parent) {
        inputHost().click().repeat();
    }
    int           repeatDelay() { return inputHost().click().repeat().delay; }
    int           repeatInterval() { return inputHost().click().repeat().interval; }
    void          setRepeatDelay(int value) { configure(value, repeatInterval()); }
    void          setRepeatInterval(int value) { configure(repeatDelay(), value); }
    Q_SIGNAL void repeatChanged();

private:
    void configure(int delay, int interval) {
        delay    = qMax(1, delay);
        interval = qMax(1, interval);
        if (delay == repeatDelay() && interval == repeatInterval()) return;
        const QPointer<RepeatButton> guard(this);
        cancelActivation();
        if (! guard) return;
        auto& repeat    = inputHost().click().repeat();
        repeat.delay    = delay;
        repeat.interval = interval;
        Q_EMIT repeatChanged();
    }
};
} // namespace qml_material
