#include "qml_material/style/motion_settings.hpp"
#include "qml_material/token/token.hpp"
#include <cmath>
namespace qml_material
{
void MotionSettings::inherit(MotionSettings* parent) {
    if (parent == m_inherited || parent == this) return;
    disconnect(m_connection);
    disconnect(m_destroyed);
    m_inherited = parent;
    if (parent) {
        m_connection = connect(parent, &MotionSettings::changed, this, &MotionSettings::changed);
        m_destroyed  = connect(parent, &QObject::destroyed, this, [this] {
            m_inherited = nullptr;
            emit changed();
        });
    }
    emit changed();
}
AnimationSpec MotionSettings::spatial() const {
    return m_spatial.value_or(m_inherited ? m_inherited->spatial() : Motion::spring());
}
void MotionSettings::setSpatial(AnimationSpec value) {
    if (! value.valid() || m_spatial == value) return;
    m_spatial = value;
    emit changed();
}
void MotionSettings::resetSpatial() {
    if (! m_spatial) return;
    m_spatial.reset();
    emit changed();
}
AnimationSpec MotionSettings::effects() const {
    return m_effects.value_or(m_inherited ? m_inherited->effects() : Motion::spring(1600, 1));
}
void MotionSettings::setEffects(AnimationSpec value) {
    if (! value.valid() || m_effects == value) return;
    m_effects = value;
    emit changed();
}
void MotionSettings::resetEffects() {
    if (! m_effects) return;
    m_effects.reset();
    emit changed();
}
AnimationSpec MotionSettings::page() const {
    return m_page.value_or(m_inherited ? m_inherited->page()
                                       : Motion::tween(450, token::Easing {}.emphasized()));
}
void MotionSettings::setPage(AnimationSpec value) {
    if (! value.valid() || m_page == value) return;
    m_page = value;
    emit changed();
}
void MotionSettings::resetPage() {
    if (! m_page) return;
    m_page.reset();
    emit changed();
}
AnimationSpec MotionSettings::container() const {
    return m_container.value_or(m_inherited ? m_inherited->container()
                                            : Motion::tween(450, token::Easing {}.emphasized()));
}
void MotionSettings::setContainer(AnimationSpec value) {
    if (! value.valid() || m_container == value) return;
    m_container = value;
    emit changed();
}
void MotionSettings::resetContainer() {
    if (! m_container) return;
    m_container.reset();
    emit changed();
}
qreal MotionSettings::durationScale() const {
    return m_durationScale.value_or(m_inherited ? m_inherited->durationScale() : 1);
}
void MotionSettings::setDurationScale(qreal value) {
    if ((! std::isfinite(value) || value < 0) || m_durationScale == value) return;
    m_durationScale = value;
    emit changed();
}
void MotionSettings::resetDurationScale() {
    if (! m_durationScale) return;
    m_durationScale.reset();
    emit changed();
}
bool MotionSettings::reducedMotion() const {
    return m_reducedMotion.value_or(m_inherited ? m_inherited->reducedMotion() : false);
}
void MotionSettings::setReducedMotion(bool value) {
    if (m_reducedMotion == value) return;
    m_reducedMotion = value;
    emit changed();
}
void MotionSettings::resetReducedMotion() {
    if (! m_reducedMotion) return;
    m_reducedMotion.reset();
    emit changed();
}
} // namespace qml_material
