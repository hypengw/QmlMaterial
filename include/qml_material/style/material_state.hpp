#pragma once

#include "qml_material/style/common_state.hpp"

namespace qml_material
{
class QML_MATERIAL_API MaterialState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(MState)
    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged FINAL)
public:
    explicit MaterialState(QObject* parent = nullptr);
    using CommonState::setTarget;

private:
    enum class AppearanceState
    {
        Base
    };
    StateBindingSet<AppearanceState>           m_bindings { AppearanceState::Base };
    StateBindingSet<AppearanceState>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
