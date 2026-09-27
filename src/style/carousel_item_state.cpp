#include "qml_material/style/carousel_item_state.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
CarouselItemState::CarouselItemState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    connect(this, &CommonState::targetChanged, this, [this] {
        const QPointer<CarouselItemState> guard(this);
        m_disabled = item() && ! item()->isEnabled();
        if (guard) Q_EMIT itemChanged();
    });
    auto base = m_bindings.base();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level1;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::surface_container_low));
    base.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    m_selection.setBinding([this] {
        return Selection { m_disabled.value() ? Interaction::Disabled
                           : down()           ? Interaction::Pressed
                           : hovered()        ? Interaction::Hovered
                                              : Interaction::Base,
                           targetGeneration() };
    });
    m_ready = true;
}
CarouselItemState::~CarouselItemState() {
    m_bindings.abandon();
    disconnect(m_enabledConnection);
}
QQuickItem* CarouselItemState::item() const { return static_cast<QQuickItem*>(target()); }
void        CarouselItemState::setItem(QQuickItem* value) {
    if (item() == value) return;
    const QScopedPropertyUpdateGroup group;
    disconnect(m_enabledConnection);
    if (value)
        m_enabledConnection = connect(value, &QQuickItem::enabledChanged, this, [this] {
            m_disabled = item() && ! item()->isEnabled();
        });
    setTarget(value);
}
void CarouselItemState::selectionChanged() {
    if (! m_ready) return;
    const auto next = m_selection.value();
    QString    name;
    switch (next.state) {
    case Interaction::Disabled: name = QStringLiteral("disabled"); break;
    case Interaction::Pressed: name = QStringLiteral("pressed"); break;
    case Interaction::Hovered: name = QStringLiteral("hovered"); break;
    default: break;
    }
    publishState(name, [this, state = next.state] {
        m_bindings.select(state);
    });
}
bool            CarouselItemState::down() const { return m_down.value(); }
void            CarouselItemState::setDown(bool value) { m_down = value; }
QBindable<bool> CarouselItemState::bindableDown() { return QBindable<bool>(&m_down); }
bool            CarouselItemState::hovered() const { return m_hovered.value(); }
void            CarouselItemState::setHovered(bool value) { m_hovered = value; }
QBindable<bool> CarouselItemState::bindableHovered() { return QBindable<bool>(&m_hovered); }
} // namespace qml_material
