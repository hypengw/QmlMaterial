#pragma once
#include "qml_material/style/input_state.hpp"
#include "qml_material/token/text_field.hpp"
Q_MOC_INCLUDE("qml_material/control/text_field.hpp")
namespace qml_material
{
class TextField;
class QML_MATERIAL_API TextFieldState : public InputState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateTextField)
    Q_PROPERTY(qml_material::TextField* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY sizeChanged BINDABLE bindableSize)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType)
    Q_PROPERTY(token::TextFieldSize sizeTokens READ sizeTokens WRITE setSizeTokens NOTIFY
                   sizeTokensChanged BINDABLE bindableSizeTokens)
    Q_PROPERTY(token::TypeScaleItem typescale READ typescale WRITE setTypescale RESET resetTypescale
                   NOTIFY typescaleChanged BINDABLE bindableTypescale)
    Q_PROPERTY(
        int indicatorHeight READ indicatorHeight WRITE setIndicatorHeight RESET resetIndicatorHeight
            NOTIFY indicatorHeightChanged BINDABLE bindableIndicatorHeight)
    Q_PROPERTY(QColor indicatorColor READ indicatorColor WRITE setIndicatorColor RESET
                   resetIndicatorColor NOTIFY indicatorColorChanged BINDABLE bindableIndicatorColor)
    Q_PROPERTY(
        QColor placeholderColor READ placeholderColor WRITE setPlaceholderColor RESET
            resetPlaceholderColor NOTIFY placeholderColorChanged BINDABLE bindablePlaceholderColor)
    Q_PROPERTY(qreal placeholderOpacity READ placeholderOpacity WRITE setPlaceholderOpacity RESET
                   resetPlaceholderOpacity NOTIFY placeholderOpacityChanged BINDABLE
                       bindablePlaceholderOpacity)
    Q_PROPERTY(
        qml_material::token::TextFieldSizeItem sizeToken READ sizeToken NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal containerHeight READ containerHeight NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal horizontalPadding READ horizontalPadding NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal verticalPadding READ verticalPadding NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal iconSize READ iconSize NOTIFY sizeTokenChanged)
    Q_PROPERTY(qreal spacing READ spacing NOTIFY sizeTokenChanged)
public:
    explicit TextFieldState(QObject* parent = nullptr);
    ~TextFieldState() override;
    TextField*                      item() const;
    void                            setItem(TextField*);
    int                             size() const;
    void                            setSize(const int&);
    QBindable<int>                  bindableSize();
    Q_SIGNAL void                   sizeChanged();
    int                             type() const;
    void                            setType(const int&);
    QBindable<int>                  bindableType();
    Q_SIGNAL void                   typeChanged();
    token::TextFieldSize            sizeTokens() const;
    void                            setSizeTokens(const token::TextFieldSize&);
    QBindable<token::TextFieldSize> bindableSizeTokens();
    Q_SIGNAL void                   sizeTokensChanged();
    token::TypeScaleItem            typescale() const;
    void                            setTypescale(const token::TypeScaleItem&);
    QBindable<token::TypeScaleItem> bindableTypescale();
    Q_SIGNAL void                   typescaleChanged();
    void                            resetTypescale();
    int                             indicatorHeight() const;
    void                            setIndicatorHeight(const int&);
    QBindable<int>                  bindableIndicatorHeight();
    Q_SIGNAL void                   indicatorHeightChanged();
    void                            resetIndicatorHeight();
    QColor                          indicatorColor() const;
    void                            setIndicatorColor(const QColor&);
    QBindable<QColor>               bindableIndicatorColor();
    Q_SIGNAL void                   indicatorColorChanged();
    void                            resetIndicatorColor();
    QColor                          placeholderColor() const;
    void                            setPlaceholderColor(const QColor&);
    QBindable<QColor>               bindablePlaceholderColor();
    Q_SIGNAL void                   placeholderColorChanged();
    void                            resetPlaceholderColor();
    qreal                           placeholderOpacity() const;
    void                            setPlaceholderOpacity(const qreal&);
    QBindable<qreal>                bindablePlaceholderOpacity();
    Q_SIGNAL void                   placeholderOpacityChanged();
    void                            resetPlaceholderOpacity();
    token::TextFieldSizeItem        sizeToken() const;
    Q_SIGNAL void                   sizeTokenChanged();
    qreal                           containerHeight() const;
    qreal                           horizontalPadding() const;
    qreal                           verticalPadding() const;
    qreal                           iconSize() const;
    qreal                           spacing() const;

private:
    void                           updateNativeInputs();
    QList<QMetaObject::Connection> m_nativeConnections;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, int, m_size, int(Enum::ButtonSize::M),
                                         &TextFieldState::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, int, m_type, 0,
                                         &TextFieldState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, token::TextFieldSize, m_sizeTokens,
                                         token::TextFieldSize {},
                                         &TextFieldState::sizeTokensChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, token::TypeScaleItem, m_typescale,
                                         token::TypeScaleItem {}, &TextFieldState::typescaleChanged)
    PropertyKey<token::TypeScaleItem> m_typescaleKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, int, m_indicatorHeight, 1,
                                         &TextFieldState::indicatorHeightChanged)
    PropertyKey<int> m_indicatorHeightKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, QColor, m_indicatorColor,
                                         QColor(Qt::transparent),
                                         &TextFieldState::indicatorColorChanged)
    PropertyKey<QColor> m_indicatorColorKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, QColor, m_placeholderColor,
                                         QColor(Qt::transparent),
                                         &TextFieldState::placeholderColorChanged)
    PropertyKey<QColor> m_placeholderColorKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TextFieldState, qreal, m_placeholderOpacity, 1,
                                         &TextFieldState::placeholderOpacityChanged)
    PropertyKey<qreal> m_placeholderOpacityKey;
    Q_OBJECT_BINDABLE_PROPERTY(TextFieldState, token::TextFieldSizeItem, m_selectedSize,
                               &TextFieldState::sizeTokenChanged)
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
