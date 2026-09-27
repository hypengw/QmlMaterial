#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/control/abstract_button.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
ButtonInteractionState::ButtonInteractionState(QObject* parent, FocusTreatment focus,
                                               PressSource press)
    : CommonState(parent) {
    initializeAppearance(m_bindings);
    m_hovered.setBinding([this] {
        auto* item = inputItem();
        return item && item->hovered();
    });
    m_selection.setBinding([this, focus, press] {
        const auto generation = targetGeneration();
        if (disabled()) return Selection { Interaction::Disabled, generation };
        const auto* item   = inputItem();
        const bool  active = press == PressSource::Down ? down() : item && item->isPressed();
        if (active || (focus == FocusTreatment::Pressed && visualFocus()))
            return Selection { Interaction::Pressed, generation };
        if (hovered()) return Selection { Interaction::Hovered, generation };
        if (focus == FocusTreatment::Separate && visualFocus())
            return Selection { Interaction::Focus, generation };
        return Selection { Interaction::Base, generation };
    });
    m_inputReady = true;
}
void ButtonInteractionState::selectionChanged() {
    if (m_inputReady) {
        const auto selection = m_selection.value();
        publishState(stateName(selection.state), [this, next = selection.state] {
            m_bindings.select(next);
        });
    }
}
QString ButtonInteractionState::stateName(Interaction state) {
    switch (state) {
    case Interaction::Disabled: return QStringLiteral("disabled");
    case Interaction::Pressed: return QStringLiteral("pressed");
    case Interaction::Hovered: return QStringLiteral("hovered");
    case Interaction::Focus: return QStringLiteral("focus");
    default: return {};
    }
}
ButtonInteractionState::~ButtonInteractionState() {
    stopBindings();
    utils::disconnectAll(m_connections);
}
AbstractButton* ButtonInteractionState::inputItem() const { return m_item.value(); }
bool            ButtonInteractionState::checked() const {
    const auto item = inputItem();
    return item && item->isChecked();
}
bool ButtonInteractionState::checkable() const {
    const auto item = inputItem();
    return item && item->isCheckable();
}
bool ButtonInteractionState::down() const {
    const auto item = inputItem();
    return item && item->isDown();
}
bool ButtonInteractionState::hovered() const { return m_hovered.value(); }
bool ButtonInteractionState::visualFocus() const {
    const auto item = inputItem();
    return item && item->visualFocus();
}
bool                 ButtonInteractionState::disabled() const { return m_disabled.value(); }
std::optional<qreal> ButtonInteractionState::backgroundHeight() const {
    const auto item       = inputItem();
    const auto background = item ? item->background() : nullptr;
    return background ? std::optional<qreal>(background->height()) : std::nullopt;
}
void ButtonInteractionState::setInputItem(AbstractButton* item) {
    if (m_item == item) return;
    const QPointer<ButtonInteractionState> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        utils::disconnectAll(m_connections);
        m_item = item;
        if (item) {
            // QQuickItem::enabled has no bindable API in Qt 6.8.
            m_connections.append(connect(item, &AbstractButton::enabledChanged, this, [this] {
                m_disabled = inputItem() && ! inputItem()->isEnabled();
            }));
            m_connections.append(connect(item, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_connections);
                const QPointer<ButtonInteractionState> guard(this);
                {
                    const QScopedPropertyUpdateGroup group;
                    m_item     = nullptr;
                    m_disabled = false;
                }
                if (guard) Q_EMIT itemChanged();
            }));
        }
        setTarget(item);
        if (! guard) return;
        m_disabled = item && ! item->isEnabled();
    }
    if (guard) Q_EMIT itemChanged();
}
void ButtonInteractionState::bindStateLayerOpacity() {
    stateBindings(Interaction::Pressed).bind(m_appearance.stateLayerOpacity, [this]() -> qreal {
        return stateTokens().pressed.state_layer_opacity;
    });
    stateBindings(Interaction::Hovered).bind(m_appearance.stateLayerOpacity, [this]() -> qreal {
        return stateTokens().hover.state_layer_opacity;
    });
    stateBindings(Interaction::Focus).bind(m_appearance.stateLayerOpacity, [this]() -> qreal {
        return stateTokens().focus.state_layer_opacity;
    });
}
} // namespace qml_material
