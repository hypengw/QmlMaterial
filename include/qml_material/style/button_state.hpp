#pragma once
#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/token/button.hpp"
#include <optional>
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API ButtonAppearanceState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateButtonBase)
    QML_UNCREATABLE("Use StateButton or StateSplitButton")
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY sizeChanged BINDABLE bindableSize FINAL)
    Q_PROPERTY(bool isRound READ isRound WRITE setIsRound NOTIFY isRoundChanged BINDABLE
                   bindableIsRound FINAL)
    Q_PROPERTY(qreal containerHeight READ containerHeight NOTIFY geometryChanged)
    Q_PROPERTY(qreal iconSize READ iconSize NOTIFY geometryChanged)
    Q_PROPERTY(qreal leadingSpace READ leadingSpace NOTIFY geometryChanged)
    Q_PROPERTY(qreal trailingSpace READ trailingSpace NOTIFY geometryChanged)
    Q_PROPERTY(qreal spacing READ spacing NOTIFY geometryChanged)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType FINAL)

public:
    Button*         item() const;
    void            setItem(Button*);
    int             size() const;
    void            setSize(int);
    QBindable<int>  bindableSize();
    bool            isRound() const;
    void            setIsRound(bool);
    QBindable<bool> bindableIsRound();
    virtual qreal   containerHeight() const = 0;
    virtual qreal   iconSize() const        = 0;
    virtual qreal   leadingSpace() const    = 0;
    virtual qreal   trailingSpace() const   = 0;
    virtual qreal   spacing() const         = 0;
    Q_SIGNAL void   geometryChanged();
    Q_SIGNAL void   sizeChanged();
    Q_SIGNAL void   isRoundChanged();
    int             type() const;
    void            setType(int);
    QBindable<int>  bindableType();
    Q_SIGNAL void   typeChanged();

protected:
    explicit ButtonAppearanceState(QObject* parent = nullptr);

private:
    void   bindAppearance();
    QColor defaultTextColor() const;
    QColor defaultBackgroundColor() const;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ButtonAppearanceState, int, m_type,
                                         int(Enum::ButtonType::BtElevated),
                                         &ButtonAppearanceState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ButtonAppearanceState, int, m_size,
                                         int(Enum::ButtonSize::S),
                                         &ButtonAppearanceState::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ButtonAppearanceState, bool, m_isRound, true,
                                         &ButtonAppearanceState::isRoundChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};

class QML_MATERIAL_API ButtonState : public ButtonAppearanceState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateButton)
    Q_PROPERTY(qml_material::token::ButtonSize sizeTokens READ sizeTokens WRITE setSizeTokens NOTIFY
                   sizeTokensChanged BINDABLE bindableSizeTokens FINAL)
    Q_PROPERTY(qml_material::token::ButtonSizeItem sizeToken READ sizeToken NOTIFY sizeTokenChanged)

public:
    explicit ButtonState(QObject* parent = nullptr);
    token::ButtonSize            sizeTokens() const;
    void                         setSizeTokens(const token::ButtonSize&);
    QBindable<token::ButtonSize> bindableSizeTokens();

    token::ButtonSizeItem sizeToken() const;
    qreal                 containerHeight() const override;
    qreal                 iconSize() const override;
    qreal                 leadingSpace() const override;
    qreal                 trailingSpace() const override;
    qreal                 spacing() const override;

    Q_SIGNAL void sizeTokensChanged();
    Q_SIGNAL void sizeTokenChanged();

private:
    void bindTargets();
    Q_OBJECT_BINDABLE_PROPERTY(ButtonState, token::ButtonSize, m_sizeTokens,
                               &ButtonState::sizeTokensChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ButtonState, token::ButtonSizeItem, m_selectedSize,
                               &ButtonState::sizeTokenChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
