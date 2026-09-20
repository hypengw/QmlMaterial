#pragma once
#include "qml_material/control/abstract_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API RadioButton : public AbstractButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(RadioButtonBase)
public:
    explicit RadioButton(QQuickItem* parent = nullptr): AbstractButton(parent) {
        setCheckable(true);
        setAutoExclusive(true);
    }
};
} // namespace qml_material
