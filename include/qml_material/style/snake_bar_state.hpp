#pragma once
#include "qml_material/style/common_state.hpp"
Q_MOC_INCLUDE("qml_material/control/control.hpp")
Q_MOC_INCLUDE("qml_material/control/button_control.hpp")
namespace qml_material
{
class Control;
class ButtonControl;
class QML_MATERIAL_API SnakeBarState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSnakeBar)
    Q_PROPERTY(qml_material::IndicationValues actionIndication READ actionIndication NOTIFY
                   actionIndicationChanged FINAL)
    Q_PROPERTY(qml_material::IndicationValues iconIndication READ iconIndication NOTIFY
                   iconIndicationChanged FINAL)
    Q_PROPERTY(qml_material::Control* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(qml_material::ButtonControl* actionItem READ actionItem WRITE setActionItem NOTIFY
                   actionItemChanged)
    Q_PROPERTY(qml_material::ButtonControl* iconItem READ iconItem WRITE setIconItem NOTIFY
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
    IndicationValues  actionIndication() const { return m_actionIndication.values(); }
    IndicationValues  iconIndication() const { return m_iconIndication.values(); }
    Q_SIGNAL void     actionIndicationChanged();
    Q_SIGNAL void     iconIndicationChanged();
    Control*          item() const;
    void              setItem(Control*);
    Q_SIGNAL void     itemChanged();
    ButtonControl*    actionItem() const;
    void              setActionItem(ButtonControl*);
    Q_SIGNAL void     actionItemChanged();
    ButtonControl*    iconItem() const;
    void              setIconItem(ButtonControl*);
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
    enum class Appearance
    {
        Base
    };
    StateBindingSet<Appearance> m_bindings { Appearance::Base };
    QProperty<ButtonControl*>   m_actionItem { nullptr };
    QProperty<ButtonControl*>   m_iconItem { nullptr };
    QMetaObject::Connection     m_actionDestroyed;
    QMetaObject::Connection     m_iconDestroyed;
    QMetaObject::Connection     m_actionSourceChanged, m_iconSourceChanged;
    Q_OBJECT_BINDABLE_PROPERTY(SnakeBarState, QColor, m_iconColor, &SnakeBarState::iconColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SnakeBarState, QColor, m_iconStateLayerColor,
                               &SnakeBarState::iconStateLayerColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SnakeBarState, qreal, m_iconStateLayerOpacity,
                               &SnakeBarState::iconStateLayerOpacityChanged)
    PropertyKey<QColor>                   m_iconColorKey;
    PropertyKey<QColor>                   m_iconLayerColorKey;
    PropertyKey<qreal>                    m_iconLayerOpacityKey;
    IndicationPolicy                      m_actionIndication;
    IndicationPolicy                      m_iconIndication;
    StateBindingSet<Appearance>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
