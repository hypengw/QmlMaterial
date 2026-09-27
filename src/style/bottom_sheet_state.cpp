#include "qml_material/style/bottom_sheet_state.hpp"
#include "qml_material/control/popup.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qml_util.hpp"
namespace qml_material
{
BottomSheetState::BottomSheetState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    connect(this, &CommonState::targetChanged, this, &BottomSheetState::itemChanged);
    m_radiusKey = m_bindings.property<&BottomSheetState::bindableRadius>(this);
    auto base   = m_bindings.base();
    base.bind(m_radiusKey, [] {
        return int(token::Shape {}.corner.extra_large);
    });
    base.bind(m_appearance.corners, [this] {
        return Util::corners(radius(), radius(), 0, 0);
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level1;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::surface_container_low));
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
}
Popup*         BottomSheetState::item() const { return static_cast<Popup*>(target()); }
void           BottomSheetState::setItem(Popup* value) { setTarget(value); }
int            BottomSheetState::type() const { return m_type.value(); }
void           BottomSheetState::setType(int value) { m_type = value; }
QBindable<int> BottomSheetState::bindableType() { return QBindable<int>(&m_type); }
int            BottomSheetState::radius() const { return m_radius.value(); }
void           BottomSheetState::setRadius(int value) { m_radius = value; }
void           BottomSheetState::resetRadius() { m_radiusKey.reset(); }
QBindable<int> BottomSheetState::bindableRadius() { return QBindable<int>(&m_radius); }
} // namespace qml_material
