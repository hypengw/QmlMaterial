#pragma once
#include "qml_material/style/common_state.hpp"
#include <optional>
namespace qml_material
{
class AbstractButton;
class QML_MATERIAL_API ButtonInteractionState : public CommonState {
    Q_OBJECT
    QML_ANONYMOUS
public:
    ~ButtonInteractionState() override;
    Q_SIGNAL void itemChanged();

protected:
    enum class Interaction
    {
        Base,
        Disabled,
        Pressed,
        Hovered,
        Focus
    };
    using BindingSet      = StateBindingSet<Interaction>;
    using BindingLifetime = BindingSet::Lifetime;
    BindingSet&   bindingSet() { return m_bindings; }
    StateBindings baseBindings() { return m_bindings.base(); }
    StateBindings stateBindings(Interaction state) { return m_bindings.state(state); }
    void          stopBindings() {
        m_hovered.takeBinding();
        m_bindings.abandon();
    }
    void setHoverBinding(const QPropertyBinding<bool>& binding) { m_hovered.setBinding(binding); }
    enum class FocusTreatment
    {
        Pressed,
        Separate,
        Ignored
    };
    enum class PressSource
    {
        Down,
        Pressed
    };
    explicit ButtonInteractionState(QObject*       parent = nullptr,
                                    FocusTreatment focus  = FocusTreatment::Pressed,
                                    PressSource    press  = PressSource::Down);
    AbstractButton*      inputItem() const;
    void                 setInputItem(AbstractButton*);
    bool                 checked() const;
    bool                 checkable() const;
    bool                 down() const;
    bool                 hovered() const;
    bool                 visualFocus() const;
    bool                 disabled() const;
    std::optional<qreal> backgroundHeight() const;
    void                 bindStateLayerOpacity();

private:
    static QString stateName(Interaction);
    BindingSet     m_bindings { Interaction::Base };
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    void                           selectionChanged();
    bool                           m_inputReady = false;
    QProperty<AbstractButton*>     m_item { nullptr };
    QList<QMetaObject::Connection> m_connections;
    QProperty<bool>                m_disabled { false };
    QProperty<bool>                m_hovered { false };
    Q_OBJECT_BINDABLE_PROPERTY(ButtonInteractionState, Selection, m_selection,
                               &ButtonInteractionState::selectionChanged)
};
} // namespace qml_material
