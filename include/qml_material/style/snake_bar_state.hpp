#pragma once
#include "qml_material/style/common_state.hpp"
Q_MOC_INCLUDE("qml_material/control/control.hpp")
Q_MOC_INCLUDE("qml_material/control/abstract_button.hpp")
namespace qml_material
{
class Control;
class AbstractButton;
class QML_MATERIAL_API SnakeBarState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSnakeBar)
    Q_PROPERTY(qml_material::Control* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(qml_material::AbstractButton* actionItem READ actionItem WRITE setActionItem NOTIFY
                   actionItemChanged)
    Q_PROPERTY(qml_material::AbstractButton* iconItem READ iconItem WRITE setIconItem NOTIFY
                   iconItemChanged)
    Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor RESET resetIconColor NOTIFY
                   iconColorChanged BINDABLE bindableIconColor)
    Q_PROPERTY(QColor iconStateLayerColor READ iconStateLayerColor WRITE setIconStateLayerColor
                   RESET resetIconStateLayerColor NOTIFY iconStateLayerColorChanged BINDABLE
                       bindableIconStateLayerColor)
    Q_PROPERTY(qreal iconStateLayerOpacity READ iconStateLayerOpacity WRITE setIconStateLayerOpacity
                   RESET resetIconStateLayerOpacity NOTIFY iconStateLayerOpacityChanged BINDABLE
                       bindableIconStateLayerOpacity)
public:
    explicit SnakeBarState(QObject* parent = nullptr);
    ~SnakeBarState() override;
    Control*          item() const;
    void              setItem(Control*);
    Q_SIGNAL void     itemChanged();
    AbstractButton*   actionItem() const;
    void              setActionItem(AbstractButton*);
    Q_SIGNAL void     actionItemChanged();
    AbstractButton*   iconItem() const;
    void              setIconItem(AbstractButton*);
    Q_SIGNAL void     iconItemChanged();
    QColor            iconColor() const;
    void              setIconColor(const QColor&);
    QBindable<QColor> bindableIconColor();
    void              resetIconColor();
    Q_SIGNAL void     iconColorChanged();
    QColor            iconStateLayerColor() const;
    void              setIconStateLayerColor(const QColor&);
    QBindable<QColor> bindableIconStateLayerColor();
    void              resetIconStateLayerColor();
    Q_SIGNAL void     iconStateLayerColorChanged();
    qreal             iconStateLayerOpacity() const;
    void              setIconStateLayerOpacity(qreal);
    QBindable<qreal>  bindableIconStateLayerOpacity();
    void              resetIconStateLayerOpacity();
    Q_SIGNAL void     iconStateLayerOpacityChanged();

private:
    enum class Interaction
    {
        Base,
        ActionHover,
        ActionFocus,
        ActionPress,
        IconHover,
        IconFocus,
        IconPress
    };
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    void                         selectionChanged();
    StateBindingSet<Interaction> m_bindings { Interaction::Base };
    QProperty<AbstractButton*>   m_actionItem { nullptr };
    QProperty<AbstractButton*>   m_iconItem { nullptr };
    QMetaObject::Connection      m_actionDestroyed;
    QMetaObject::Connection      m_iconDestroyed;
    bool                         m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY(SnakeBarState, Selection, m_selection,
                               &SnakeBarState::selectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SnakeBarState, QColor, m_iconColor, &SnakeBarState::iconColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SnakeBarState, QColor, m_iconStateLayerColor,
                               &SnakeBarState::iconStateLayerColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SnakeBarState, qreal, m_iconStateLayerOpacity,
                               &SnakeBarState::iconStateLayerOpacityChanged)
    PropertyKey<QColor>                    m_iconColorKey;
    PropertyKey<QColor>                    m_iconLayerColorKey;
    PropertyKey<qreal>                     m_iconLayerOpacityKey;
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
