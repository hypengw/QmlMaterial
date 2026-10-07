#include "qml_material/style/slider_state.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/control/range_slider.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
SliderAppearance::SliderAppearance(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    m_trackColorKey = m_bindings.property<&SliderAppearance::bindableTrackColor>(this);
    m_trackOverlayColorKey =
        m_bindings.property<&SliderAppearance::bindableTrackOverlayColor>(this);
    m_trackInactiveColorKey =
        m_bindings.property<&SliderAppearance::bindableTrackInactiveColor>(this);
    m_trackMarkInactiveColorKey =
        m_bindings.property<&SliderAppearance::bindableTrackMarkInactiveColor>(this);
    m_trackMarkColorKey = m_bindings.property<&SliderAppearance::bindableTrackMarkColor>(this);
    m_trackOverlayOpacityKey =
        m_bindings.property<&SliderAppearance::bindableTrackOverlayOpacity>(this);
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
        if (control() && control()->hovered())
            return Selection { Interaction::Hovered, generation };
        return Selection { Interaction::Base, generation };
    });
    m_ready = true;
}
SliderAppearance::~SliderAppearance() {
    m_bindings.abandon();
    utils::disconnectAll(m_connections);
}
Control* SliderAppearance::control() const { return m_control.value(); }
bool     SliderAppearance::active() const { return control() && m_pressed.value(); }
void     SliderAppearance::selectionChanged() {
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
void SliderAppearance::setControl(Control* value, std::function<bool()> pressed) {
    if (m_control == value) return;
    QPointer<SliderAppearance> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        utils::disconnectAll(m_connections);
        updateStateInput(m_control, value);
        m_pressed.setBinding([this, pressed = std::move(pressed)] {
            return control() && pressed();
        });
        if (value) {
            m_connections.append(connect(value, &QQuickItem::enabledChanged, this, [this] {
                updateStateInput(m_disabled, control() && ! control()->isEnabled());
            }));
            m_connections.append(connect(value, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_connections);
                QPointer<SliderAppearance> guard(this);
                {
                    const QScopedPropertyUpdateGroup group;
                    updateStateInput(m_control, nullptr);
                    updateStateInput(m_disabled, false);
                }
                if (guard && isStateActive()) Q_EMIT controlChanged();
            }));
        }
        setTarget(value);
        if (! guard) return;
        updateStateInput(m_disabled, value && ! value->isEnabled());
    }
    if (guard && isStateActive()) Q_EMIT controlChanged();
}
SliderHandleAppearance::SliderHandleAppearance(QObject* parent): SliderAppearance(parent) {
    m_handleLineWidthKey =
        m_bindings.property<&SliderHandleAppearance::bindableHandleLineWidth>(this);
    m_handleWidthKey  = m_bindings.property<&SliderHandleAppearance::bindableHandleWidth>(this);
    m_handleHeightKey = m_bindings.property<&SliderHandleAppearance::bindableHandleHeight>(this);
    auto base         = m_bindings.base();
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
QColor            SliderAppearance::trackColor() const { return m_trackColor.value(); }
void              SliderAppearance::setTrackColor(const QColor& value) { m_trackColor = value; }
void              SliderAppearance::resetTrackColor() { m_trackColorKey.reset(); }
QBindable<QColor> SliderAppearance::bindableTrackColor() {
    return QBindable<QColor>(&m_trackColor);
}
QColor SliderAppearance::trackOverlayColor() const { return m_trackOverlayColor.value(); }
void   SliderAppearance::setTrackOverlayColor(const QColor& value) { m_trackOverlayColor = value; }
void   SliderAppearance::resetTrackOverlayColor() { m_trackOverlayColorKey.reset(); }
QBindable<QColor> SliderAppearance::bindableTrackOverlayColor() {
    return QBindable<QColor>(&m_trackOverlayColor);
}
QColor SliderAppearance::trackInactiveColor() const { return m_trackInactiveColor.value(); }
void SliderAppearance::setTrackInactiveColor(const QColor& value) { m_trackInactiveColor = value; }
void SliderAppearance::resetTrackInactiveColor() { m_trackInactiveColorKey.reset(); }
QBindable<QColor> SliderAppearance::bindableTrackInactiveColor() {
    return QBindable<QColor>(&m_trackInactiveColor);
}
QColor SliderAppearance::trackMarkInactiveColor() const { return m_trackMarkInactiveColor.value(); }
void   SliderAppearance::setTrackMarkInactiveColor(const QColor& value) {
    m_trackMarkInactiveColor = value;
}
void SliderAppearance::resetTrackMarkInactiveColor() { m_trackMarkInactiveColorKey.reset(); }
QBindable<QColor> SliderAppearance::bindableTrackMarkInactiveColor() {
    return QBindable<QColor>(&m_trackMarkInactiveColor);
}
QColor SliderAppearance::trackMarkColor() const { return m_trackMarkColor.value(); }
void   SliderAppearance::setTrackMarkColor(const QColor& value) { m_trackMarkColor = value; }
void   SliderAppearance::resetTrackMarkColor() { m_trackMarkColorKey.reset(); }
QBindable<QColor> SliderAppearance::bindableTrackMarkColor() {
    return QBindable<QColor>(&m_trackMarkColor);
}
qreal SliderAppearance::trackOverlayOpacity() const { return m_trackOverlayOpacity.value(); }
void SliderAppearance::setTrackOverlayOpacity(const qreal& value) { m_trackOverlayOpacity = value; }
void SliderAppearance::resetTrackOverlayOpacity() { m_trackOverlayOpacityKey.reset(); }
QBindable<qreal> SliderAppearance::bindableTrackOverlayOpacity() {
    return QBindable<qreal>(&m_trackOverlayOpacity);
}
int  SliderHandleAppearance::handleLineWidth() const { return m_handleLineWidth.value(); }
void SliderHandleAppearance::setHandleLineWidth(const int& value) { m_handleLineWidth = value; }
void SliderHandleAppearance::resetHandleLineWidth() { m_handleLineWidthKey.reset(); }
QBindable<int> SliderHandleAppearance::bindableHandleLineWidth() {
    return QBindable<int>(&m_handleLineWidth);
}
int            SliderHandleAppearance::handleWidth() const { return m_handleWidth.value(); }
void           SliderHandleAppearance::setHandleWidth(const int& value) { m_handleWidth = value; }
void           SliderHandleAppearance::resetHandleWidth() { m_handleWidthKey.reset(); }
QBindable<int> SliderHandleAppearance::bindableHandleWidth() {
    return QBindable<int>(&m_handleWidth);
}
int            SliderHandleAppearance::handleHeight() const { return m_handleHeight.value(); }
void           SliderHandleAppearance::setHandleHeight(const int& value) { m_handleHeight = value; }
void           SliderHandleAppearance::resetHandleHeight() { m_handleHeightKey.reset(); }
QBindable<int> SliderHandleAppearance::bindableHandleHeight() {
    return QBindable<int>(&m_handleHeight);
}
SliderM2State::SliderM2State(QObject* parent): SliderAppearance(parent) {
    connect(this, &SliderM2State::controlChanged, this, &SliderM2State::itemChanged);
}
Slider* SliderM2State::item() const { return static_cast<Slider*>(control()); }
void    SliderM2State::setItem(Slider* value) {
    setControl(value, [this] {
        return item()->pressed() || item()->visualFocus();
    });
}
SliderState::SliderState(QObject* parent): SliderHandleAppearance(parent) {
    connect(this, &SliderState::controlChanged, this, &SliderState::itemChanged);
}
Slider* SliderState::item() const { return static_cast<Slider*>(control()); }
void    SliderState::setItem(Slider* value) {
    setControl(value, [this] {
        return item()->pressed() || item()->visualFocus();
    });
}
RangeSliderState::RangeSliderState(QObject* parent): SliderHandleAppearance(parent) {
    connect(this, &RangeSliderState::controlChanged, this, &RangeSliderState::itemChanged);
}
RangeSlider* RangeSliderState::item() const { return static_cast<RangeSlider*>(control()); }
void         RangeSliderState::setItem(RangeSlider* value) {
    setControl(value, [this] {
        if (handleIndex() == 0 || handleIndex() == 1) {
            const auto node = handleIndex() == 0 ? item()->first() : item()->second();
            return node->pressed() || node->focused();
        }
        return item()->pressed() || item()->visualFocus();
    });
}
} // namespace qml_material
