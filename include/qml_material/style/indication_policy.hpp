#pragma once
#include <QColor>
#include <QProperty>
#include "qml_material/input/interaction_source.hpp"
#include "qml_material/token/token.hpp"

namespace qml_material
{
struct IndicationValues {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_VALUE_TYPE(indicationValues)
    Q_PROPERTY(QColor color MEMBER color CONSTANT FINAL)
    Q_PROPERTY(qreal opacity MEMBER opacity CONSTANT FINAL)
    Q_PROPERTY(qreal pressOpacity MEMBER pressOpacity CONSTANT FINAL)
    Q_PROPERTY(int kind MEMBER kind CONSTANT FINAL)
    Q_PROPERTY(bool available MEMBER available CONSTANT FINAL)
public:
    QColor color { Qt::transparent };
    qreal  opacity = 0, pressOpacity = token::State {}.pressed.state_layer_opacity;
    int    kind                                      = Interaction::None;
    bool   available                                 = false;
    bool   operator==(const IndicationValues&) const = default;
};

class QML_MATERIAL_API IndicationPolicy {
public:
    explicit IndicationPolicy(QObject* context, std::function<void()> changed = {});
    ~IndicationPolicy();
    IndicationPolicy(const IndicationPolicy&)             = delete;
    IndicationPolicy&  operator=(const IndicationPolicy&) = delete;
    InteractionSource* source() const { return m_interactions.source(); }
    void               setSource(InteractionSource* source) { m_interactions.setSource(source); }
    void               setTarget(QQuickItem*);
    const InteractionAccumulator& interactions() const { return m_interactions; }
    QColor                        color() const { return m_color; }
    void                          setColor(const QColor& value) { m_color = value; }
    void                          resetColor();
    void                          bindDefaultColor(const QPropertyBinding<QColor>& binding) {
        m_defaultColor.setBinding(binding);
    }
    QBindable<QColor> bindableColor() { return &m_color; }
    token::State      tokens() const { return m_tokens; }
    void              setTokens(const token::State& value) { m_tokens = value; }
    void              resetTokens();
    void              bindDefaultTokens(const QPropertyBinding<token::State>& binding) {
        m_defaultTokens.setBinding(binding);
    }
    QBindable<token::State> bindableTokens() { return &m_tokens; }
    bool                    pressEnabled() const { return m_pressEnabled; }
    bool                    hoverEnabled() const { return m_hoverEnabled; }
    bool                    focusEnabled() const { return m_focusEnabled; }
    bool                    dragEnabled() const { return m_dragEnabled; }
    void                    setPressEnabled(bool);
    void                    setHoverEnabled(bool);
    void                    setFocusEnabled(bool);
    void                    setDragEnabled(bool);
    IndicationValues        values() const;

private:
    void                           notify();
    QPointer<QObject>              m_context;
    std::function<void()>          m_changed;
    InteractionAccumulator         m_interactions;
    QPointer<QQuickItem>           m_target;
    QList<QMetaObject::Connection> m_targetConnections;
    QProperty<bool>                m_available { true };
    QProperty<QColor>              m_defaultColor { QColor(Qt::transparent) }, m_color;
    QProperty<token::State>        m_defaultTokens, m_tokens;
    QPropertyNotifier              m_colorNotifier, m_tokensNotifier;
    bool m_pressEnabled = true, m_hoverEnabled = true, m_focusEnabled = true, m_dragEnabled = true;
};
} // namespace qml_material
