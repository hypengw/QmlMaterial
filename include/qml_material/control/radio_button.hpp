#pragma once
#include "qml_material/control/selection_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API RadioButton : public SelectionButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(RadioButtonBase)
public:
    explicit RadioButton(QQuickItem* parent = nullptr): SelectionButton(parent) {}
};
} // namespace qml_material
