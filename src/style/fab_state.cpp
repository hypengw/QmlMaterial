#include "qml_material/style/fab_state.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/core/enum.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
FABState::FABState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level3;
    });
    base.bind(m_appearance.corner, [this]() -> qreal {
        const auto shape = shapeTokens().corner;
        switch (Enum::FABType(type())) {
        case Enum::FABType::FABSmall: return shape.extra_small;
        case Enum::FABType::FABLarge: return shape.extra_large;
        default: return shape.large;
        }
    });
    base.bind(m_appearance.textColor, [this] {
        switch (Enum::FABColor(color())) {
        case Enum::FABColor::FABColorSurface: return CommonState::color(&MdColorMgr::primary);
        case Enum::FABColor::FABColorSecondary:
            return CommonState::color(&MdColorMgr::on_secondary_container);
        case Enum::FABColor::FABColorTertiary:
            return CommonState::color(&MdColorMgr::on_tertiary_container);
        default: return CommonState::color(&MdColorMgr::on_primary_container);
        }
    });
    base.bind(m_appearance.backgroundColor, [this] {
        switch (Enum::FABColor(color())) {
        case Enum::FABColor::FABColorSurface:
            return CommonState::color(&MdColorMgr::surface_container_high);
        case Enum::FABColor::FABColorSecondary:
            return CommonState::color(&MdColorMgr::secondary_container);
        case Enum::FABColor::FABColorTertiary:
            return CommonState::color(&MdColorMgr::tertiary_container);
        default: return CommonState::color(&MdColorMgr::primary_container);
        }
    });
    base.bind(m_appearance.stateLayerColor, [this] {
        return textColor();
    });
    appearanceWhen([this] {
        return elevationInteraction() != qml_material::Interaction::None;
    }).bind(m_appearance.elevation, [this]() -> qreal {
        return elevationInteraction() == qml_material::Interaction::Hover
                   ? elevationTokens().level4
                   : elevationTokens().level3;
    });
    bindStateLayerOpacity();
}
ButtonControl* FABState::item() const { return inputItem(); }
void           FABState::setItem(ButtonControl* item) { setInputItem(item, {}, {}); }
int            FABState::type() const { return m_type.value(); }
void           FABState::setType(int value) { m_type = value; }
QBindable<int> FABState::bindableType() { return QBindable<int>(&m_type); }
int            FABState::color() const { return m_color.value(); }
void           FABState::setColor(int value) { m_color = value; }
QBindable<int> FABState::bindableColor() { return QBindable<int>(&m_color); }
token::Shape   FABState::shapeTokens() const { return m_shapeTokens.value(); }
void           FABState::setShapeTokens(const token::Shape& value) { m_shapeTokens = value; }
QBindable<token::Shape> FABState::bindableShapeTokens() {
    return QBindable<token::Shape>(&m_shapeTokens);
}
} // namespace qml_material
