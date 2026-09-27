#pragma once
#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/token/segmented_button.hpp"
#include "qml_material/token/type_scale.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API SegmentedButtonState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSegmentedButton)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged BINDABLE
                   bindablePosition FINAL)
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY sizeChanged BINDABLE bindableSize FINAL)
    Q_PROPERTY(qml_material::token::SegmentedButtonSize sizeTokens READ sizeTokens WRITE
                   setSizeTokens NOTIFY sizeTokensChanged BINDABLE bindableSizeTokens FINAL)
    Q_PROPERTY(qml_material::token::SegmentedButtonSizeItem sizeToken READ sizeToken NOTIFY
                   sizeTokenChanged)
    Q_PROPERTY(qml_material::token::TypeScaleItem typescale READ typescale NOTIFY typescaleChanged)
    Q_PROPERTY(qreal containerHeight READ containerHeight NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal iconSize READ iconSize NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal leadingSpace READ leadingSpace NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal trailingSpace READ trailingSpace NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal spacing READ spacing NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal outlineWidth READ outlineWidth NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal cornerRadius READ cornerRadius WRITE setCornerRadius RESET resetCornerRadius
                   NOTIFY cornerRadiusChanged BINDABLE bindableCornerRadius)
public:
    explicit SegmentedButtonState(QObject* parent = nullptr);
    Button*                               item() const;
    void                                  setItem(Button*);
    int                                   position() const;
    void                                  setPosition(int);
    QBindable<int>                        bindablePosition();
    int                                   size() const;
    void                                  setSize(int);
    QBindable<int>                        bindableSize();
    token::SegmentedButtonSize            sizeTokens() const;
    void                                  setSizeTokens(const token::SegmentedButtonSize&);
    QBindable<token::SegmentedButtonSize> bindableSizeTokens();
    token::SegmentedButtonSizeItem        sizeToken() const;
    token::TypeScaleItem                  typescale() const;
    qreal                                 containerHeight() const;
    qreal                                 iconSize() const;
    qreal                                 leadingSpace() const;
    qreal                                 trailingSpace() const;
    qreal                                 spacing() const;
    qreal                                 outlineWidth() const;
    qreal                                 cornerRadius() const;
    void                                  setCornerRadius(qreal);
    void                                  resetCornerRadius();
    QBindable<qreal>                      bindableCornerRadius();
    Q_SIGNAL void                         positionChanged();
    Q_SIGNAL void                         sizeChanged();
    Q_SIGNAL void                         sizeTokensChanged();
    Q_SIGNAL void                         sizeTokenChanged();
    Q_SIGNAL void                         typescaleChanged();
    Q_SIGNAL void                         cornerRadiusChanged();

private:
    void                    updateMirrorSource();
    QMetaObject::Connection m_mirrorConnection;
    QProperty<bool>         m_mirrored { false };
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SegmentedButtonState, int, m_position,
                                         int(Enum::ItemPosition::PosSingle),
                                         &SegmentedButtonState::positionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SegmentedButtonState, int, m_size,
                                         int(Enum::ButtonSize::S),
                                         &SegmentedButtonState::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SegmentedButtonState, token::SegmentedButtonSize, m_sizeTokens,
                               &SegmentedButtonState::sizeTokensChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SegmentedButtonState, token::SegmentedButtonSizeItem, m_selectedSize,
                               &SegmentedButtonState::sizeTokenChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SegmentedButtonState, token::TypeScaleItem, m_typescale,
                               &SegmentedButtonState::typescaleChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SegmentedButtonState, qreal, m_cornerRadius,
                               &SegmentedButtonState::cornerRadiusChanged)
    PropertyKey<qreal> m_radiusKey;
    BindingLifetime    m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
