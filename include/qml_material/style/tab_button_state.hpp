#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/tab_button.hpp")
namespace qml_material
{
class TabButton;
class QML_MATERIAL_API TabButtonState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateTabButton)
    Q_PROPERTY(qml_material::TabButton* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType FINAL)
    Q_PROPERTY(QColor baseTextColor READ baseTextColor WRITE setBaseTextColor RESET
                   resetBaseTextColor NOTIFY baseTextColorChanged BINDABLE bindableBaseTextColor)
public:
    explicit TabButtonState(QObject* parent = nullptr);
    TabButton*        item() const;
    void              setItem(TabButton*);
    int               type() const;
    void              setType(int);
    QBindable<int>    bindableType();
    QColor            baseTextColor() const;
    void              setBaseTextColor(const QColor&);
    Q_INVOKABLE void  resetBaseTextColor();
    QBindable<QColor> bindableBaseTextColor();
    Q_SIGNAL void     typeChanged();
    Q_SIGNAL void     baseTextColorChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TabButtonState, int, m_type,
                                         int(Enum::TabType::PrimaryTab),
                                         &TabButtonState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TabButtonState, QColor, m_baseTextColor,
                               &TabButtonState::baseTextColorChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
