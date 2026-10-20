#pragma once
#include "qml_material/style/common_state.hpp"
#include <optional>
Q_MOC_INCLUDE("qml_material/control/button_control.hpp")
namespace qml_material
{
class ButtonControl;
class SelectionButton;
class ToggleButton;
class QML_MATERIAL_API ButtonInteractionState : public CommonState {
    Q_OBJECT
    QML_ANONYMOUS
public:
    ~ButtonInteractionState() override;
    Q_INVOKABLE qreal groupOpticalOffset(const qml_material::CornersGroup&) const;
    Q_SIGNAL void     itemChanged();

protected:
    void enableGroupShape();
    int  groupSize() const;
    enum class Availability
    {
        Base,
        Disabled
    };
    using BindingSet      = StateBindingSet<Availability>;
    using BindingLifetime = BindingSet::Lifetime;
    BindingSet&   bindingSet() { return m_bindings; }
    StateBindings baseBindings() { return m_bindings.base(); }
    StateBindings disabledBindings() { return m_bindings.state(Availability::Disabled); }
    StateBindings appearanceWhen(std::function<bool()> predicate) {
        return m_bindings.when([this, predicate = std::move(predicate)] {
            return ! disabled() && predicate();
        });
    }
    void stopBindings() {
        m_inputReady = false;
        m_selection.takeBinding();
        m_hovered.takeBinding();
        m_bindings.abandon();
    }
    void setHoverBinding(const QPropertyBinding<bool>& binding) { m_hovered.setBinding(binding); }
    explicit ButtonInteractionState(QObject* parent = nullptr);
    ButtonControl*       inputItem() const;
    void                 setInputItem(SelectionButton*);
    void                 setInputItem(ToggleButton*);
    void                 setInputItem(ButtonControl*, std::function<bool()> checked,
                                      std::function<bool()> checkable);
    bool                 checked() const;
    bool                 checkable() const;
    bool                 pressed() const;
    bool                 hovered() const;
    bool                 visualFocus() const;
    bool                 disabled() const;
    std::optional<qreal> backgroundHeight() const;
    void                 bindStateLayerOpacity(bool focus = true);
    Interaction::Kind    elevationInteraction(bool includeDrag = false) const;

private:
    void                           updateGroupContext();
    CornersGroup                   groupCorners() const;
    QList<QMetaObject::Connection> m_groupConnections;
    QProperty<quint64>             m_groupRevision { 0 };
    BindingSet                     m_bindings { Availability::Base };
    struct Selection {
        Availability state                              = Availability::Base;
        quint64      generation                         = 0;
        bool         operator==(const Selection&) const = default;
    };
    void    selectionChanged();
    bool    m_inputReady    = false;
    quint64 m_inputRevision = 0;
    Q_OBJECT_BINDABLE_PROPERTY(ButtonInteractionState, ButtonControl*, m_item)
    std::function<bool()>          m_checked, m_checkable;
    QList<QMetaObject::Connection> m_connections;
    Q_OBJECT_BINDABLE_PROPERTY(ButtonInteractionState, bool, m_disabled)
    QProperty<bool> m_hovered { false };
    Q_OBJECT_BINDABLE_PROPERTY(ButtonInteractionState, bool, m_focusVisible)
    Q_OBJECT_BINDABLE_PROPERTY(ButtonInteractionState, Selection, m_selection,
                               &ButtonInteractionState::selectionChanged)
};
class QML_MATERIAL_API ButtonValueState : public ButtonInteractionState {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::ButtonControl* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged BINDABLE
                   bindableSelected FINAL)
    Q_PROPERTY(bool toggleable READ isToggleable WRITE setToggleable NOTIFY toggleableChanged
                   BINDABLE bindableToggleable FINAL)
public:
    explicit ButtonValueState(QObject* parent = nullptr): ButtonInteractionState(parent) {}
    ~ButtonValueState() override { stopBindings(); }
    ButtonControl*  item() const { return inputItem(); }
    void            setItem(ButtonControl*);
    bool            isSelected() const { return m_selected; }
    void            setSelected(bool value) { m_selected = value; }
    QBindable<bool> bindableSelected() { return &m_selected; }
    bool            isToggleable() const { return m_toggleable; }
    void            setToggleable(bool value) { m_toggleable = value; }
    QBindable<bool> bindableToggleable() { return &m_toggleable; }
    Q_SIGNAL void   selectedChanged();
    Q_SIGNAL void   toggleableChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(ButtonValueState, bool, m_selected,
                               &ButtonValueState::selectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ButtonValueState, bool, m_toggleable,
                               &ButtonValueState::toggleableChanged)
};
} // namespace qml_material
