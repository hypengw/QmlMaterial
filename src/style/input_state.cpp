#include "qml_material/style/input_state.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
InputState::InputState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    resetError();
    m_selection.setBinding([this] {
        const auto generation = targetGeneration();
        return Selection { m_disabled.value() ? Interaction::Disabled
                           : m_error.value()
                               ? (m_hovered.value() ? Interaction::ErrorHover : Interaction::Error)
                           : m_focused.value() ? Interaction::Focus
                           : m_hovered.value() ? Interaction::Hovered
                                               : Interaction::Base,
                           generation };
    });
    m_ready = true;
}
InputState::~InputState() {
    m_bindings.abandon();
    utils::disconnectAll(m_connections);
}
bool InputState::error() const { return m_error.value(); }
void InputState::setError(bool value) { m_error = value; }
void InputState::resetError() {
    m_error.setBinding([this] {
        return ! m_acceptable.value();
    });
}
QBindable<bool> InputState::bindableError() { return QBindable<bool>(&m_error); }
void            InputState::bindInputAppearance(const PropertyKey<QColor>& label,
                                                const PropertyKey<qreal>&  opacity) {
    auto base = m_bindings.base();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(m_appearance.outlineColor, colorBinding(&MdColorMgr::outline));
    base.bind(label, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(opacity, [] {
        return qreal(1);
    });
    auto disabled = m_bindings.state(Interaction::Disabled);
    disabled.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(label, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(opacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    for (auto state : { Interaction::Error, Interaction::ErrorHover }) {
        auto error = m_bindings.state(state);
        error.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
        error.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::error));
        const auto role =
            state == Interaction::Error ? &MdColorMgr::error : &MdColorMgr::on_error_container;
        error.bind(label, colorBinding(role));
        error.bind(m_appearance.outlineColor, colorBinding(role));
    }
    for (auto state : { Interaction::Focus, Interaction::Hovered }) {
        auto       active = m_bindings.state(state);
        const auto role =
            state == Interaction::Focus ? &MdColorMgr::primary : &MdColorMgr::on_surface;
        active.bind(label, colorBinding(role));
        active.bind(m_appearance.outlineColor, colorBinding(role));
    }
}
QQuickItem* InputState::inputItem() const { return m_item.value(); }
void        InputState::setInputItem(QQuickItem* value) {
    if (m_item == value) return;
    QPointer<InputState> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        utils::disconnectAll(m_connections);
        m_item = value;
        if (value) {
            m_connections.append(connect(value, &QQuickItem::enabledChanged, this, [this] {
                m_disabled = inputItem() && ! inputItem()->isEnabled();
            }));
            m_connections.append(connect(value, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_connections);
                QPointer<InputState> guard(this);
                {
                    const QScopedPropertyUpdateGroup group;
                    m_item     = nullptr;
                    m_disabled = false;
                }
                if (guard) Q_EMIT itemChanged();
            }));
        }
        setTarget(value);
        if (! guard) return;
        m_disabled = value && ! value->isEnabled();
    }
    if (guard) Q_EMIT itemChanged();
}
void InputState::selectionChanged() {
    if (! m_ready) return;
    const auto selection = m_selection.value();
    QString    name;
    switch (selection.state) {
    case Interaction::Disabled: name = QStringLiteral("disabled"); break;
    case Interaction::Error: name = QStringLiteral("error"); break;
    case Interaction::ErrorHover: name = QStringLiteral("errorHover"); break;
    case Interaction::Focus: name = QStringLiteral("focus"); break;
    case Interaction::Hovered: name = QStringLiteral("hovered"); break;
    default: break;
    }
    publishState(name, [this, next = selection.state] {
        m_bindings.select(next);
    });
}
} // namespace qml_material
