#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API SearchBarState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSearchBar)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(qreal placeholderOpacity READ placeholderOpacity WRITE setPlaceholderOpacity RESET
                   resetPlaceholderOpacity NOTIFY placeholderOpacityChanged BINDABLE
                       bindablePlaceholderOpacity)
public:
    explicit SearchBarState(QObject* parent = nullptr);
    Button*          item() const;
    void             setItem(Button*);
    qreal            placeholderOpacity() const;
    void             setPlaceholderOpacity(qreal);
    void             resetPlaceholderOpacity();
    QBindable<qreal> bindablePlaceholderOpacity();
    Q_SIGNAL void    placeholderOpacityChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SearchBarState, qreal, m_placeholderOpacity, 1,
                                         &SearchBarState::placeholderOpacityChanged)
    PropertyKey<qreal> m_placeholderKey;
    BindingLifetime    m_lifetime { bindingSet().lifetime() };
};
} // namespace qml_material
