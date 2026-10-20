#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/control/toggle_button.hpp"
#include "qml_material/control/button_group_container.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
ButtonInteractionState::ButtonInteractionState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    m_hovered.setBinding([this] {
        return interactions().hovered;
    });
    m_selection.setBinding([this] {
        const auto generation = targetGeneration();
        return Selection { disabled() ? Availability::Disabled : Availability::Base, generation };
    });
    m_inputReady = true;
}
void ButtonInteractionState::selectionChanged() {
    if (m_inputReady) {
        const auto selection = m_selection.value();
        applyAppearance([this, next = selection.state] {
            m_bindings.select(next);
        });
    }
}
ButtonInteractionState::~ButtonInteractionState() {
    stopBindings();
    utils::disconnectAll(m_groupConnections);
    utils::disconnectAll(m_connections);
}
void ButtonInteractionState::enableGroupShape() {
    connect(this,
            &ButtonInteractionState::itemChanged,
            this,
            &ButtonInteractionState::updateGroupContext);
    baseBindings().bind(m_appearance.corners, [this] {
        return groupCorners();
    });
    updateGroupContext();
}
void ButtonInteractionState::updateGroupContext() {
    utils::disconnectAll(m_groupConnections);
    if (auto* item = inputItem()) {
        auto* info = qobject_cast<ButtonGroupContainerAttached*>(
            qmlAttachedPropertiesObject<ButtonGroupContainer>(item, true));
        const auto changed = [this] {
            if (isStateActive()) m_groupRevision = m_groupRevision.value() + 1;
        };
        m_groupConnections.append(
            connect(info, &ButtonGroupContainerAttached::contextChanged, this, changed));
        m_groupConnections.append(connect(item, &Control::mirroredChanged, this, changed));
        m_groupConnections.append(connect(item, &QQuickItem::heightChanged, this, changed));
        m_groupConnections.append(connect(item, &Control::topInsetChanged, this, changed));
        m_groupConnections.append(connect(item, &Control::bottomInsetChanged, this, changed));
    }
    m_groupRevision = m_groupRevision.value() + 1;
}
int ButtonInteractionState::groupSize() const {
    (void)m_groupRevision.value();
    auto* info = inputItem()
                     ? qobject_cast<ButtonGroupContainerAttached*>(
                           qmlAttachedPropertiesObject<ButtonGroupContainer>(inputItem(), false))
                     : nullptr;
    return info ? info->buttonSize() : int(Enum::ButtonSize::S);
}
CornersGroup ButtonInteractionState::groupCorners() const {
    (void)m_groupRevision.value();
    auto* item = inputItem();
    auto* info = item ? qobject_cast<ButtonGroupContainerAttached*>(
                            qmlAttachedPropertiesObject<ButtonGroupContainer>(item, false))
                      : nullptr;
    if (! info || ! info->connected()) return CornersGroup(corner());
    const qreal full =
        std::max(qreal(0), item->height() - item->topInset() - item->bottomInset()) / 2;
    const auto pos = Enum::ItemPosition(info->position());
    if (pos == Enum::ItemPosition::PosSingle || (! pressed() && checked()))
        return CornersGroup(full);
    const qreal inner = std::min(
        full, pressed() ? token::ButtonGroup::pressedInnerCorner : token::ButtonGroup::innerCorner);
    if (pos == Enum::ItemPosition::PosMiddle) return CornersGroup(inner);
    const bool left = (pos == Enum::ItemPosition::PosFirst) != item->mirrored();
    return left ? CornersGroup(inner, inner, full, full) : CornersGroup(full, full, inner, inner);
}
qreal ButtonInteractionState::groupOpticalOffset(const CornersGroup& corners) const {
    return .055 *
           (corners.topLeft() + corners.bottomLeft() - corners.topRight() - corners.bottomRight());
}
ButtonControl* ButtonInteractionState::inputItem() const { return m_item.value(); }
bool ButtonInteractionState::checked() const { return inputItem() && m_checked && m_checked(); }
bool ButtonInteractionState::checkable() const {
    return inputItem() && m_checkable && m_checkable();
}
bool                 ButtonInteractionState::pressed() const { return interactions().pressed; }
bool                 ButtonInteractionState::hovered() const { return m_hovered.value(); }
bool                 ButtonInteractionState::visualFocus() const { return m_focusVisible.value(); }
bool                 ButtonInteractionState::disabled() const { return m_disabled.value(); }
std::optional<qreal> ButtonInteractionState::backgroundHeight() const {
    const auto item       = inputItem();
    const auto background = item ? item->background() : nullptr;
    return background ? std::optional<qreal>(background->height()) : std::nullopt;
}
void ButtonInteractionState::setInputItem(SelectionButton* item) {
    setInputItem(
        item,
        [item] {
            return item->isSelected();
        },
        [] {
            return true;
        });
}
void ButtonInteractionState::setInputItem(ToggleButton* item) {
    setInputItem(
        item,
        [item] {
            return item->isChecked();
        },
        [] {
            return true;
        });
}
void ButtonValueState::setItem(ButtonControl* item) {
    setInputItem(
        item,
        [this] {
            return isSelected();
        },
        [this] {
            return isToggleable();
        });
}
void ButtonInteractionState::setInputItem(ButtonControl* item, std::function<bool()> checked,
                                          std::function<bool()> checkable) {
    if (m_item == item) return;
    const QPointer<ButtonInteractionState> guard(this);
    const auto                             revision = ++m_inputRevision;
    {
        utils::disconnectAll(m_connections);
        m_checked   = std::move(checked);
        m_checkable = std::move(checkable);
        // A target change can grow binding dependency arrays. Qt's grouped notifications
        // retain their old observer heads, so publish the complete snapshot before notifying.
        m_item.setValueBypassingBindings(item);
        m_focusVisible.setValueBypassingBindings(item && item->focusVisible());
        m_disabled.setValueBypassingBindings(item && ! item->isEnabled());
        if (item) {
            m_connections.append(
                connect(item, &ButtonControl::interactionSourceChanged, this, [this] {
                    setInteractionSource(inputItem() ? inputItem()->interactionSource() : nullptr);
                }));
            m_connections.append(connect(item, &ButtonControl::focusVisibleChanged, this, [this] {
                updateStateInput(m_focusVisible, inputItem() && inputItem()->focusVisible());
            }));
            // QQuickItem::enabled has no bindable API in Qt 6.8.
            m_connections.append(connect(item, &QQuickItem::enabledChanged, this, [this] {
                updateStateInput(m_disabled, inputItem() && ! inputItem()->isEnabled());
            }));
            m_connections.append(connect(item, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_connections);
                const QPointer<ButtonInteractionState> guard(this);
                const auto                             revision = ++m_inputRevision;
                m_item.setValueBypassingBindings(nullptr);
                m_disabled.setValueBypassingBindings(false);
                m_focusVisible.setValueBypassingBindings(false);
                if (isStateActive()) m_item.notify();
                if (! guard || revision != m_inputRevision) return;
                if (isStateActive()) m_disabled.notify();
                if (! guard || revision != m_inputRevision) return;
                if (isStateActive()) m_focusVisible.notify();
                if (! guard || revision != m_inputRevision) return;
                setInteractionSource(nullptr);
                if (guard && revision == m_inputRevision && isStateActive()) Q_EMIT itemChanged();
            }));
        }
        setTarget(item);
        if (! guard || revision != m_inputRevision) return;
        setInteractionSource(item ? item->interactionSource() : nullptr);
        if (! guard || revision != m_inputRevision) return;
        if (isStateActive()) m_item.notify();
        if (! guard || revision != m_inputRevision) return;
        if (isStateActive()) m_focusVisible.notify();
        if (! guard || revision != m_inputRevision) return;
        if (isStateActive()) m_disabled.notify();
    }
    if (guard && revision == m_inputRevision && isStateActive()) Q_EMIT itemChanged();
}
void ButtonInteractionState::bindStateLayerOpacity(bool focus) {
    appearanceWhen([this, focus] {
        return pressed() || hovered() || (focus && visualFocus());
    }).bind(m_appearance.stateLayerOpacity, [this]() -> qreal {
        if (pressed()) return stateTokens().pressed.state_layer_opacity;
        if (hovered()) return stateTokens().hover.state_layer_opacity;
        return stateTokens().focus.state_layer_opacity;
    });
}
Interaction::Kind ButtonInteractionState::elevationInteraction(bool includeDrag) const {
    return interactionAccumulator().latest({ Interaction::Press,
                                             Interaction::Hover,
                                             Interaction::Focus,
                                             includeDrag ? Interaction::Drag : Interaction::None });
}
} // namespace qml_material
