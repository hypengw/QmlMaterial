#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API FABState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateFAB)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType FINAL)
    Q_PROPERTY(int color READ color WRITE setColor NOTIFY colorChanged BINDABLE bindableColor FINAL)
    Q_PROPERTY(qml_material::token::Shape shapeTokens READ shapeTokens WRITE setShapeTokens NOTIFY
                   shapeTokensChanged BINDABLE bindableShapeTokens FINAL)
public:
    explicit FABState(QObject* parent = nullptr);
    Button*                 item() const;
    void                    setItem(Button*);
    int                     type() const;
    void                    setType(int);
    QBindable<int>          bindableType();
    int                     color() const;
    void                    setColor(int);
    QBindable<int>          bindableColor();
    token::Shape            shapeTokens() const;
    void                    setShapeTokens(const token::Shape&);
    QBindable<token::Shape> bindableShapeTokens();
    Q_SIGNAL void           typeChanged();
    Q_SIGNAL void           colorChanged();
    Q_SIGNAL void           shapeTokensChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FABState, int, m_type, 1, &FABState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FABState, int, m_color, 0, &FABState::colorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(FABState, token::Shape, m_shapeTokens, &FABState::shapeTokensChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
