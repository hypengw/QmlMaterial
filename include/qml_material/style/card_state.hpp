#pragma once
#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/core/enum.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API CardState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateCard)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType FINAL)
    Q_PROPERTY(
        int radius READ radius WRITE setRadius NOTIFY radiusChanged BINDABLE bindableRadius FINAL)
public:
    explicit CardState(QObject* parent = nullptr);
    Button*        item() const;
    void           setItem(Button*);
    int            type() const;
    void           setType(int);
    QBindable<int> bindableType();
    int            radius() const;
    void           setRadius(int);
    QBindable<int> bindableRadius();
    Q_SIGNAL void  typeChanged();
    Q_SIGNAL void  radiusChanged();

private:
    qreal baseElevation() const;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CardState, int, m_type, 0, &CardState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CardState, int, m_radius, 12, &CardState::radiusChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
