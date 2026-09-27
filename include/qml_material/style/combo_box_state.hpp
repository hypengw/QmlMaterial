#pragma once
#include "qml_material/style/input_state.hpp"
#include "qml_material/token/combo_box.hpp"
Q_MOC_INCLUDE("qml_material/control/combo_box.hpp")
namespace qml_material
{
class ComboBox;
class QML_MATERIAL_API ComboBoxState : public InputState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateComboBox)
    Q_PROPERTY(qml_material::ComboBox* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY sizeChanged BINDABLE bindableSize)
    Q_PROPERTY(token::ComboBoxSize sizeTokens READ sizeTokens WRITE setSizeTokens NOTIFY
                   sizeTokensChanged BINDABLE bindableSizeTokens)
    Q_PROPERTY(token::TypeScaleItem typescale READ typescale WRITE setTypescale RESET resetTypescale
                   NOTIFY typescaleChanged BINDABLE bindableTypescale)
    Q_PROPERTY(QColor labelColor READ labelColor WRITE setLabelColor RESET resetLabelColor NOTIFY
                   labelColorChanged BINDABLE bindableLabelColor)
    Q_PROPERTY(qreal labelOpacity READ labelOpacity WRITE setLabelOpacity RESET resetLabelOpacity
                   NOTIFY labelOpacityChanged BINDABLE bindableLabelOpacity)
    Q_PROPERTY(
        qml_material::token::ComboBoxSizeItem sizeToken READ sizeToken NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal containerHeight READ containerHeight NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal horizontalPadding READ horizontalPadding NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal indicatorSize READ indicatorSize NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal spacing READ spacing NOTIFY sizeTokenChanged)
public:
    explicit ComboBoxState(QObject* parent = nullptr);
    ~ComboBoxState() override;
    ComboBox*                       item() const;
    void                            setItem(ComboBox*);
    int                             size() const;
    void                            setSize(const int&);
    QBindable<int>                  bindableSize();
    Q_SIGNAL void                   sizeChanged();
    token::ComboBoxSize             sizeTokens() const;
    void                            setSizeTokens(const token::ComboBoxSize&);
    QBindable<token::ComboBoxSize>  bindableSizeTokens();
    Q_SIGNAL void                   sizeTokensChanged();
    token::TypeScaleItem            typescale() const;
    void                            setTypescale(const token::TypeScaleItem&);
    QBindable<token::TypeScaleItem> bindableTypescale();
    Q_SIGNAL void                   typescaleChanged();
    void                            resetTypescale();
    QColor                          labelColor() const;
    void                            setLabelColor(const QColor&);
    QBindable<QColor>               bindableLabelColor();
    Q_SIGNAL void                   labelColorChanged();
    void                            resetLabelColor();
    qreal                           labelOpacity() const;
    void                            setLabelOpacity(const qreal&);
    QBindable<qreal>                bindableLabelOpacity();
    Q_SIGNAL void                   labelOpacityChanged();
    void                            resetLabelOpacity();
    token::ComboBoxSizeItem         sizeToken() const;
    Q_SIGNAL void                   sizeTokenChanged();
    qreal                           containerHeight() const;
    qreal                           horizontalPadding() const;
    qreal                           indicatorSize() const;
    qreal                           spacing() const;

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ComboBoxState, int, m_size, int(Enum::ButtonSize::M),
                                         &ComboBoxState::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ComboBoxState, token::ComboBoxSize, m_sizeTokens,
                                         token::ComboBoxSize {}, &ComboBoxState::sizeTokensChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ComboBoxState, token::TypeScaleItem, m_typescale,
                                         token::TypeScaleItem {}, &ComboBoxState::typescaleChanged)
    PropertyKey<token::TypeScaleItem> m_typescaleKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ComboBoxState, QColor, m_labelColor,
                                         QColor(Qt::transparent), &ComboBoxState::labelColorChanged)
    PropertyKey<QColor> m_labelColorKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ComboBoxState, qreal, m_labelOpacity, 1,
                                         &ComboBoxState::labelOpacityChanged)
    PropertyKey<qreal> m_labelOpacityKey;
    Q_OBJECT_BINDABLE_PROPERTY(ComboBoxState, token::ComboBoxSizeItem, m_selectedSize,
                               &ComboBoxState::sizeTokenChanged)
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
