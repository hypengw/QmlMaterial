#pragma once
#include "qml_material/style/button_state.hpp"
#include "qml_material/token/split_button.hpp"
namespace qml_material
{
class QML_MATERIAL_API SplitButtonState : public ButtonAppearanceState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSplitButton)
    Q_PROPERTY(qml_material::token::SplitButtonSize sizeTokens READ sizeTokens WRITE setSizeTokens
                   NOTIFY sizeTokensChanged BINDABLE bindableSizeTokens FINAL)
    Q_PROPERTY(
        qml_material::token::SplitButtonSizeItem sizeToken READ sizeToken NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal innerCorner READ innerCorner NOTIFY innerCornerChanged)
    Q_PROPERTY(qreal outerCorner READ outerCorner NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal betweenSpace READ betweenSpace NOTIFY sizeTokenChanged)
    Q_PROPERTY(qml_material::CornersGroup trailingCorners READ trailingCorners WRITE
                   setTrailingCorners RESET resetTrailingCorners NOTIFY trailingCornersChanged
                       BINDABLE bindableTrailingCorners)
public:
    explicit SplitButtonState(QObject* parent = nullptr);
    token::SplitButtonSize            sizeTokens() const;
    void                              setSizeTokens(const token::SplitButtonSize&);
    QBindable<token::SplitButtonSize> bindableSizeTokens();
    token::SplitButtonSizeItem        sizeToken() const;
    qreal                             containerHeight() const override;
    qreal                             iconSize() const override;
    qreal                             leadingSpace() const override;
    qreal                             trailingSpace() const override;
    qreal                             spacing() const override;
    qreal                             innerCorner() const;
    qreal                             outerCorner() const;
    qreal                             betweenSpace() const;
    CornersGroup                      trailingCorners() const;
    void                              setTrailingCorners(const CornersGroup&);
    void                              resetTrailingCorners();
    QBindable<CornersGroup>           bindableTrailingCorners();
    Q_SIGNAL void                     sizeTokensChanged();
    Q_SIGNAL void                     sizeTokenChanged();
    Q_SIGNAL void                     innerCornerChanged();
    Q_SIGNAL void                     trailingCornersChanged();

private:
    token::ButtonSizeItem buttonSize() const;
    Q_OBJECT_BINDABLE_PROPERTY(SplitButtonState, token::SplitButtonSize, m_sizeTokens,
                               &SplitButtonState::sizeTokensChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SplitButtonState, token::SplitButtonSizeItem, m_sizeToken,
                               &SplitButtonState::sizeTokenChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SplitButtonState, qreal, m_innerCorner,
                               &SplitButtonState::innerCornerChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SplitButtonState, CornersGroup, m_trailingCorners,
                               &SplitButtonState::trailingCornersChanged)
    PropertyKey<CornersGroup> m_trailingCornersKey;
    BindingLifetime           m_lifetime { bindingSet().lifetime() };
};
} // namespace qml_material
