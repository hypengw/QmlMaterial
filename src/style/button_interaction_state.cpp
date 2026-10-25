#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/control/toggle_button.hpp"
#include "qml_material/control/button_group_container.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
ButtonInteractionState::ButtonInteractionState(QObject* parent): CommonState(parent) {
    connect(this,
            &ButtonInteractionState::itemChanged,
            this,
            &ButtonInteractionState::observeBackground);
    connect(this, &CommonState::interactionsChanged, this, [this] {
        Q_EMIT cornerShapeChanged();
    });
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
    utils::disconnectAll(m_backgroundConnections);
}

void ButtonInteractionState::bindCornerShape(std::function<CornerShape()> shape, bool uniform) {
    m_cornerShape  = std::move(shape);
    m_uniformShape = uniform;
    if (uniform)
        baseBindings().bind(m_appearance.corner, [this] {
            return m_cornerShape().resolved().radius();
        });
    else
        baseBindings().bind(m_appearance.corners, [this] {
            return m_cornerShape().resolved();
        });
}
CornerShape ButtonInteractionState::cornerShape() const {
    const auto actual = corners();
    if (auto group = groupCornerShape(); group && group->resolved() == actual) return *group;
    if (m_cornerShape) {
        auto shape = m_cornerShape();
        if (shape.resolved() == actual && (! m_uniformShape || CornersGroup(corner()) == actual))
            return shape;
    }
    return { actual, {}, fullCorner() };
}
void ButtonInteractionState::observeBackground() {
    utils::disconnectAll(m_backgroundConnections);
    auto* item = inputItem();
    if (item) {
        const auto update = [this] {
            updateBackgroundSize();
        };
        m_backgroundConnections.append(connect(
            item, &Control::backgroundChanged, this, &ButtonInteractionState::observeBackground));
        m_backgroundConnections.append(connect(item, &QQuickItem::widthChanged, this, update));
        m_backgroundConnections.append(connect(item, &QQuickItem::heightChanged, this, update));
        m_backgroundConnections.append(connect(item, &Control::leftInsetChanged, this, update));
        m_backgroundConnections.append(connect(item, &Control::rightInsetChanged, this, update));
        m_backgroundConnections.append(connect(item, &Control::topInsetChanged, this, update));
        m_backgroundConnections.append(connect(item, &Control::bottomInsetChanged, this, update));
        m_backgroundConnections.append(connect(item, &Control::mirroredChanged, this, [this] {
            if (! isStateActive()) return;
            const QPointer<ButtonInteractionState> guard(this);
            m_geometryRevision = m_geometryRevision.value() + 1;
            if (guard && isStateActive()) Q_EMIT cornerShapeChanged();
        }));
        if (auto* background = item->background()) {
            m_backgroundConnections.append(
                connect(background, &QQuickItem::widthChanged, this, update));
            m_backgroundConnections.append(
                connect(background, &QQuickItem::heightChanged, this, update));
            m_backgroundConnections.append(connect(background, &QObject::destroyed, this, update));
        }
    }
    updateBackgroundSize();
}
void ButtonInteractionState::updateBackgroundSize() {
    if (! isStateActive()) return;
    const auto* item       = inputItem();
    const auto* background = item ? item->background() : nullptr;
    const auto  next       = background ? background->size() : QSizeF();
    if (next == m_backgroundSize.value()) return;
    const QPointer<ButtonInteractionState> guard(this);
    m_backgroundSize = next;
    if (guard && isStateActive()) Q_EMIT backgroundSizeChanged();
    if (guard && isStateActive()) Q_EMIT cornerShapeChanged();
}
qreal ButtonInteractionState::fullCorner(qreal fallback) const {
    (void)m_geometryRevision.value();
    const auto size = m_backgroundSize.value();
    return size.isEmpty() ? fallback / 2 : std::min(size.width(), size.height()) / 2;
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
    if (auto shape = groupCornerShape()) return shape->resolved();
    return CornersGroup(corner());
}
std::optional<CornerShape> ButtonInteractionState::groupCornerShape() const {
    (void)m_groupRevision.value();
    auto* item = inputItem();
    auto* info = item ? qobject_cast<ButtonGroupContainerAttached*>(
                            qmlAttachedPropertiesObject<ButtonGroupContainer>(item, false))
                      : nullptr;
    if (! info || ! info->connected()) return std::nullopt;
    const qreal full =
        fullCorner(std::max(qreal(0), item->height() - item->topInset() - item->bottomInset()));
    const auto pos = Enum::ItemPosition(info->position());
    if (pos == Enum::ItemPosition::PosSingle || (! pressed() && checked()))
        return CornerShape { {}, CornersGroup(1), full };
    const qreal inner = std::min(
        full, pressed() ? token::ButtonGroup::pressedInnerCorner : token::ButtonGroup::innerCorner);
    if (pos == Enum::ItemPosition::PosMiddle) return CornerShape { CornersGroup(inner), {}, full };
    const bool left = (pos == Enum::ItemPosition::PosFirst) != item->mirrored();
    return left ? CornerShape { CornersGroup(inner, inner, 0, 0), CornersGroup(0, 0, 1, 1), full }
                : CornerShape { CornersGroup(0, 0, inner, inner), CornersGroup(1, 1, 0, 0), full };
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
    const auto size = m_backgroundSize.value();
    return size.isValid() ? std::optional<qreal>(size.height()) : std::nullopt;
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
                if (guard && revision == m_inputRevision && isStateActive())
                    Q_EMIT cornerShapeChanged(true);
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
    if (guard && revision == m_inputRevision && isStateActive()) Q_EMIT cornerShapeChanged(true);
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
