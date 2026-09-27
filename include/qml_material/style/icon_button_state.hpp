#pragma once
#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/token/button.hpp"
#include <optional>
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API IconButtonState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateIconButton)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType FINAL)
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY sizeChanged BINDABLE bindableSize FINAL)
    Q_PROPERTY(int widthMode READ widthMode WRITE setWidthMode NOTIFY widthModeChanged BINDABLE
                   bindableWidthMode FINAL)
    Q_PROPERTY(qreal containerWidth READ containerWidth NOTIFY containerWidthChanged)
    Q_PROPERTY(bool isRound READ isRound WRITE setIsRound NOTIFY isRoundChanged BINDABLE
                   bindableIsRound FINAL)
    Q_PROPERTY(qml_material::token::IconButtonSize sizeTokens READ sizeTokens WRITE setSizeTokens
                   NOTIFY sizeTokensChanged BINDABLE bindableSizeTokens FINAL)
    Q_PROPERTY(
        qml_material::token::IconButtonSizeItem sizeToken READ sizeToken NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal containerHeight READ containerHeight NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal iconSize READ iconSize NOTIFY sizeTokenChanged)

public:
    explicit IconButtonState(QObject* parent = nullptr);
    Button*                          item() const;
    void                             setItem(Button*);
    int                              type() const;
    void                             setType(int);
    QBindable<int>                   bindableType();
    int                              size() const;
    void                             setSize(int);
    QBindable<int>                   bindableSize();
    bool                             isRound() const;
    void                             setIsRound(bool);
    QBindable<bool>                  bindableIsRound();
    token::IconButtonSize            sizeTokens() const;
    void                             setSizeTokens(const token::IconButtonSize&);
    QBindable<token::IconButtonSize> bindableSizeTokens();

    token::IconButtonSizeItem sizeToken() const;
    qreal                     containerHeight() const;
    qreal                     iconSize() const;

    int            widthMode() const;
    void           setWidthMode(int);
    QBindable<int> bindableWidthMode();
    qreal          containerWidth() const;
    Q_SIGNAL void  widthModeChanged();
    Q_SIGNAL void  containerWidthChanged();
    Q_SIGNAL void  typeChanged();
    Q_SIGNAL void  sizeChanged();
    Q_SIGNAL void  isRoundChanged();
    Q_SIGNAL void  sizeTokensChanged();
    Q_SIGNAL void  sizeTokenChanged();

private:
    void   bindTargets();
    QColor defaultTextColor() const;
    QColor defaultBackgroundColor() const;

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(IconButtonState, int, m_type,
                                         int(Enum::IconButtonType::IBtStandard),
                                         &IconButtonState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(IconButtonState, int, m_size, int(Enum::ButtonSize::S),
                                         &IconButtonState::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(IconButtonState, bool, m_isRound, true,
                                         &IconButtonState::isRoundChanged)
    Q_OBJECT_BINDABLE_PROPERTY(IconButtonState, token::IconButtonSize, m_sizeTokens,
                               &IconButtonState::sizeTokensChanged)
    Q_OBJECT_BINDABLE_PROPERTY(IconButtonState, token::IconButtonSizeItem, m_selectedSize,
                               &IconButtonState::sizeTokenChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(IconButtonState, int, m_widthMode,
                                         int(Enum::ButtonWidthMode::DefaultWidth),
                                         &IconButtonState::widthModeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(IconButtonState, qreal, m_containerWidth,
                               &IconButtonState::containerWidthChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
