#include "qml_material/style/indication_policy.hpp"

namespace qml_material
{
IndicationPolicy::IndicationPolicy(QObject* context, std::function<void()> changed)
    : m_context(context), m_changed(std::move(changed)), m_interactions(context, [this] {
          notify();
      }) {
    resetColor();
    resetTokens();
    m_colorNotifier  = m_color.addNotifier([this] {
        notify();
    });
    m_tokensNotifier = m_tokens.addNotifier([this] {
        notify();
    });
}
IndicationPolicy::~IndicationPolicy() {
    for (const auto& connection : m_targetConnections) QObject::disconnect(connection);
}
void IndicationPolicy::notify() {
    const auto changed = m_changed;
    if (m_context && changed) changed();
}
void IndicationPolicy::resetColor() {
    m_color.setBinding([this] {
        return m_defaultColor.value();
    });
}
void IndicationPolicy::resetTokens() {
    m_tokens.setBinding([this] {
        return m_defaultTokens.value();
    });
}
void IndicationPolicy::setTarget(QQuickItem* target) {
    if (m_target == target) return;
    for (const auto& connection : m_targetConnections) QObject::disconnect(connection);
    m_targetConnections.clear();
    m_target          = target;
    const auto update = [this] {
        const auto context = m_context;
        m_available        = m_target && m_target->isEnabled() && m_target->isVisible();
        if (context) notify();
    };
    if (target && m_context) {
        m_targetConnections.append(
            QObject::connect(target, &QQuickItem::enabledChanged, m_context, update));
        m_targetConnections.append(
            QObject::connect(target, &QQuickItem::visibleChanged, m_context, update));
        m_targetConnections.append(
            QObject::connect(target, &QObject::destroyed, m_context, update));
    }
    const auto context = m_context;
    m_available        = ! target || (target->isEnabled() && target->isVisible());
    if (context) notify();
}
#define POLICY(Name, Member)                       \
    void IndicationPolicy::set##Name(bool value) { \
        if (Member == value) return;               \
        Member = value;                            \
        notify();                                  \
    }
POLICY(PressEnabled, m_pressEnabled)
POLICY(HoverEnabled, m_hoverEnabled)
POLICY(FocusEnabled, m_focusEnabled)
POLICY(DragEnabled, m_dragEnabled)
#undef POLICY
IndicationValues IndicationPolicy::values() const {
    IndicationValues result;
    result.color     = color();
    result.available = source() && m_available.value();
    if (! result.available) return result;
    const auto t        = tokens();
    result.pressOpacity = m_pressEnabled ? t.pressed.state_layer_opacity : 0;
    result.kind =
        m_interactions.latest({ Interaction::Hover, Interaction::Focus, Interaction::Drag });
    switch (result.kind) {
    case Interaction::Hover:
        result.opacity = m_hoverEnabled ? t.hover.state_layer_opacity : 0;
        break;
    case Interaction::Focus:
        result.opacity = m_focusEnabled ? t.focus.state_layer_opacity : 0;
        break;
    case Interaction::Drag:
        result.opacity = m_dragEnabled ? t.dragged.state_layer_opacity : 0;
        break;
    default: break;
    }
    return result;
}
} // namespace qml_material
