#pragma once
#include "qml_material/style/common_state.hpp"
Q_MOC_INCLUDE("qml_material/control/popup.hpp")
namespace qml_material
{
class Popup;
class QML_MATERIAL_API BottomSheetState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateBottomSheet)
    Q_PROPERTY(qml_material::Popup* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType)
    Q_PROPERTY(int radius READ radius WRITE setRadius RESET resetRadius NOTIFY radiusChanged
                   BINDABLE bindableRadius)
public:
    explicit BottomSheetState(QObject* parent = nullptr);
    Popup*         item() const;
    void           setItem(Popup*);
    Q_SIGNAL void  itemChanged();
    int            type() const;
    void           setType(int);
    QBindable<int> bindableType();
    Q_SIGNAL void  typeChanged();
    int            radius() const;
    void           setRadius(int);
    void           resetRadius();
    QBindable<int> bindableRadius();
    Q_SIGNAL void  radiusChanged();

private:
    enum class AppearanceState
    {
        Base
    };
    StateBindingSet<AppearanceState> m_bindings { AppearanceState::Base };
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BottomSheetState, int, m_type,
                                         int(Enum::BottomSheetType::BottomSheetModal),
                                         &BottomSheetState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(BottomSheetState, int, m_radius, &BottomSheetState::radiusChanged)
    PropertyKey<int>                           m_radiusKey;
    StateBindingSet<AppearanceState>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
