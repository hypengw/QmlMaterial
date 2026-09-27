#include "qml_material/style/slider_state.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
SliderM2State::SliderM2State(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    m_trackColorKey         = m_bindings.property<&SliderM2State::bindableTrackColor>(this);
    m_trackOverlayColorKey  = m_bindings.property<&SliderM2State::bindableTrackOverlayColor>(this);
    m_trackInactiveColorKey = m_bindings.property<&SliderM2State::bindableTrackInactiveColor>(this);
    m_trackMarkInactiveColorKey =
        m_bindings.property<&SliderM2State::bindableTrackMarkInactiveColor>(this);
    m_trackMarkColorKey = m_bindings.property<&SliderM2State::bindableTrackMarkColor>(this);
    m_trackOverlayOpacityKey =
        m_bindings.property<&SliderM2State::bindableTrackOverlayOpacity>(this);
    auto base = m_bindings.base();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_primary));
    base.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::primary));
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_primary));
    base.bind(m_trackColorKey, [this] {
        return backgroundColor();
    });
    base.bind(m_trackOverlayColorKey, [this] {
        return backgroundColor();
    });
    base.bind(m_trackInactiveColorKey, colorBinding(&MdColorMgr::surface_container_highest));
    base.bind(m_trackMarkInactiveColorKey, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(m_trackMarkColorKey, [this] {
        return supportTextColor();
    });
    base.bind(m_trackOverlayOpacityKey, [] {
        return qreal(.12);
    });
    auto disabled = m_bindings.state(Interaction::Disabled);
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_trackInactiveColorKey, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundOpacity, [] {
        return qreal(.38);
    });
    for (auto state : { Interaction::Pressed, Interaction::Hovered })
        m_bindings.state(state).bind(m_appearance.stateLayerColor, [this, state] {
            auto c = color(&MdColorMgr::primary);
            c.setAlphaF(state == Interaction::Pressed ? stateTokens().pressed.state_layer_opacity
                                                      : stateTokens().hover.state_layer_opacity);
            return c;
        });
    m_selection.setBinding([this] {
        const auto generation = targetGeneration();
        if (m_disabled.value()) return Selection { Interaction::Disabled, generation };
        if (active()) return Selection { Interaction::Pressed, generation };
        if (item() && item()->hovered()) return Selection { Interaction::Hovered, generation };
        return Selection { Interaction::Base, generation };
    });
    m_ready = true;
}
SliderM2State::~SliderM2State() {
    m_bindings.abandon();
    utils::disconnectAll(m_connections);
}
Slider* SliderM2State::item() const { return m_item.value(); }
bool    SliderM2State::active() const {
    return item() && (item()->pressed() || item()->visualFocus());
}
void SliderM2State::selectionChanged() {
    if (! m_ready) return;
    const auto selection = m_selection.value();
    QString    name;
    switch (selection.state) {
    case Interaction::Disabled: name = QStringLiteral("disabled"); break;
    case Interaction::Pressed: name = QStringLiteral("pressed"); break;
    case Interaction::Hovered: name = QStringLiteral("hovered"); break;
    default: break;
    }
    publishState(name, [this, next = selection.state] {
        m_bindings.select(next);
    });
}
void SliderM2State::setItem(Slider* value) {
    if (m_item == value) return;
    QPointer<SliderM2State> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        utils::disconnectAll(m_connections);
        m_item = value;
        if (value) {
            m_connections.append(connect(value, &QQuickItem::enabledChanged, this, [this] {
                m_disabled = item() && ! item()->isEnabled();
            }));
            m_connections.append(connect(value, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_connections);
                QPointer<SliderM2State> guard(this);
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
SliderState::SliderState(QObject* parent): SliderM2State(parent) {
    m_handleLineWidthKey = m_bindings.property<&SliderState::bindableHandleLineWidth>(this);
    m_handleWidthKey     = m_bindings.property<&SliderState::bindableHandleWidth>(this);
    m_handleHeightKey    = m_bindings.property<&SliderState::bindableHandleHeight>(this);
    auto base            = m_bindings.base();
    base.bind(m_handleLineWidthKey, [this] {
        return active() ? 2 : 4;
    });
    base.bind(m_handleWidthKey, [] {
        return int(token::Slider {}.handle_width);
    });
    base.bind(m_handleHeightKey, [] {
        return int(token::Slider {}.handle_height);
    });
}
QColor            SliderM2State::trackColor() const { return m_trackColor.value(); }
void              SliderM2State::setTrackColor(const QColor& value) { m_trackColor = value; }
void              SliderM2State::resetTrackColor() { m_trackColorKey.reset(); }
QBindable<QColor> SliderM2State::bindableTrackColor() { return QBindable<QColor>(&m_trackColor); }
QColor            SliderM2State::trackOverlayColor() const { return m_trackOverlayColor.value(); }
void SliderM2State::setTrackOverlayColor(const QColor& value) { m_trackOverlayColor = value; }
void SliderM2State::resetTrackOverlayColor() { m_trackOverlayColorKey.reset(); }
QBindable<QColor> SliderM2State::bindableTrackOverlayColor() {
    return QBindable<QColor>(&m_trackOverlayColor);
}
QColor SliderM2State::trackInactiveColor() const { return m_trackInactiveColor.value(); }
void   SliderM2State::setTrackInactiveColor(const QColor& value) { m_trackInactiveColor = value; }
void   SliderM2State::resetTrackInactiveColor() { m_trackInactiveColorKey.reset(); }
QBindable<QColor> SliderM2State::bindableTrackInactiveColor() {
    return QBindable<QColor>(&m_trackInactiveColor);
}
QColor SliderM2State::trackMarkInactiveColor() const { return m_trackMarkInactiveColor.value(); }
void   SliderM2State::setTrackMarkInactiveColor(const QColor& value) {
    m_trackMarkInactiveColor = value;
}
void SliderM2State::resetTrackMarkInactiveColor() { m_trackMarkInactiveColorKey.reset(); }
QBindable<QColor> SliderM2State::bindableTrackMarkInactiveColor() {
    return QBindable<QColor>(&m_trackMarkInactiveColor);
}
QColor SliderM2State::trackMarkColor() const { return m_trackMarkColor.value(); }
void   SliderM2State::setTrackMarkColor(const QColor& value) { m_trackMarkColor = value; }
void   SliderM2State::resetTrackMarkColor() { m_trackMarkColorKey.reset(); }
QBindable<QColor> SliderM2State::bindableTrackMarkColor() {
    return QBindable<QColor>(&m_trackMarkColor);
}
qreal SliderM2State::trackOverlayOpacity() const { return m_trackOverlayOpacity.value(); }
void  SliderM2State::setTrackOverlayOpacity(const qreal& value) { m_trackOverlayOpacity = value; }
void  SliderM2State::resetTrackOverlayOpacity() { m_trackOverlayOpacityKey.reset(); }
QBindable<qreal> SliderM2State::bindableTrackOverlayOpacity() {
    return QBindable<qreal>(&m_trackOverlayOpacity);
}
int            SliderState::handleLineWidth() const { return m_handleLineWidth.value(); }
void           SliderState::setHandleLineWidth(const int& value) { m_handleLineWidth = value; }
void           SliderState::resetHandleLineWidth() { m_handleLineWidthKey.reset(); }
QBindable<int> SliderState::bindableHandleLineWidth() { return QBindable<int>(&m_handleLineWidth); }
int            SliderState::handleWidth() const { return m_handleWidth.value(); }
void           SliderState::setHandleWidth(const int& value) { m_handleWidth = value; }
void           SliderState::resetHandleWidth() { m_handleWidthKey.reset(); }
QBindable<int> SliderState::bindableHandleWidth() { return QBindable<int>(&m_handleWidth); }
int            SliderState::handleHeight() const { return m_handleHeight.value(); }
void           SliderState::setHandleHeight(const int& value) { m_handleHeight = value; }
void           SliderState::resetHandleHeight() { m_handleHeightKey.reset(); }
QBindable<int> SliderState::bindableHandleHeight() { return QBindable<int>(&m_handleHeight); }
} // namespace qml_material
