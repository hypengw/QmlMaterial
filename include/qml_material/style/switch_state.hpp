#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/switch.hpp")
namespace qml_material
{
class Switch;
class QML_MATERIAL_API SwitchState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSwitch)
    Q_PROPERTY(qml_material::Switch* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(QColor handleColor READ handleColor WRITE setHandleColor RESET resetHandleColor
                   NOTIFY handleColorChanged BINDABLE bindableHandleColor)
    Q_PROPERTY(int handleSize READ handleSize WRITE setHandleSize RESET resetHandleSize NOTIFY
                   handleSizeChanged BINDABLE bindableHandleSize)
    Q_PROPERTY(bool hasIcon READ hasIcon NOTIFY hasIconChanged)
public:
    explicit SwitchState(QObject* parent = nullptr);
    Switch*           item() const;
    void              setItem(Switch*);
    QColor            handleColor() const;
    void              setHandleColor(const QColor&);
    void              resetHandleColor();
    QBindable<QColor> bindableHandleColor();
    int               handleSize() const;
    void              setHandleSize(int);
    void              resetHandleSize();
    QBindable<int>    bindableHandleSize();
    bool              hasIcon() const;
    Q_SIGNAL void     handleColorChanged();
    Q_SIGNAL void     handleSizeChanged();
    Q_SIGNAL void     hasIconChanged();

private:
    PropertyKey<QColor> m_handleColorKey;
    PropertyKey<int>    m_handleSizeKey;
    Q_OBJECT_BINDABLE_PROPERTY(SwitchState, QColor, m_handleColor, &SwitchState::handleColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SwitchState, int, m_handleSize, &SwitchState::handleSizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SwitchState, bool, m_hasIcon, &SwitchState::hasIconChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
