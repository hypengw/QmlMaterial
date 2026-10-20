#pragma once

#include "qml_material/control/button_control.hpp"

namespace qml_material
{
class QML_MATERIAL_API CommandButton : public ButtonControl {
    Q_OBJECT
    QML_NAMED_ELEMENT(CommandButtonBase)
public:
    explicit CommandButton(QQuickItem* parent = nullptr): ButtonControl(parent) {}
    Q_SIGNAL void clicked();

protected:
    void activate() override {
        if (canActivate()) Q_EMIT clicked();
    }

private:
    friend class ActionCommand;
    friend class SnakeBar;
    friend class CarouselControl;
    friend class StandardDrawer;
};
} // namespace qml_material
