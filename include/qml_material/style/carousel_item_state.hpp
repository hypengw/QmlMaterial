#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button_control.hpp")
namespace qml_material
{
class QML_MATERIAL_API CarouselItemState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateCarouselItem)
    Q_PROPERTY(qml_material::ButtonControl* item READ item WRITE setItem NOTIFY itemChanged FINAL)
public:
    explicit CarouselItemState(QObject* parent = nullptr);
    ~CarouselItemState() override;
    ButtonControl* item() const;
    void           setItem(ButtonControl*);
};
} // namespace qml_material
