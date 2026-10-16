#pragma once

#include <QtCore/QObject>
#include <QtQml/qqmlregistration.h>
#include <utility>
#include "qml_material/anim/interpolator.hpp"
#include "qml_material/export.hpp"

namespace qml_material::token
{

/** @ingroup singleton */
struct MotionPreset {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_ANONYMOUS
    Q_PROPERTY(int duration READ duration CONSTANT FINAL)
    Q_PROPERTY(QEasingCurve easing READ easing CONSTANT FINAL)
public:
    MotionPreset() = default;
    MotionPreset(int duration, QEasingCurve easing)
        : m_duration(duration), m_easing(std::move(easing)) {}
    int          duration() const { return m_duration; }
    QEasingCurve easing() const { return m_easing; }
    bool         operator==(const MotionPreset&) const = default;

private:
    int          m_duration = 0;
    QEasingCurve m_easing;
};

/** Fixed-duration Bezier presets, not spring animation specs. @ingroup singleton */
struct ExpressiveMotion {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::token::MotionPreset fastSpatial READ fastSpatial CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::MotionPreset defaultSpatial READ defaultSpatial CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::MotionPreset slowSpatial READ slowSpatial CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::MotionPreset defaultEffects READ defaultEffects CONSTANT FINAL)
public:
    MotionPreset fastSpatial() const { return { 350, anim::expressive::fast_spatial() }; }
    MotionPreset defaultSpatial() const { return { 500, anim::expressive::default_spatial() }; }
    MotionPreset slowSpatial() const { return { 650, anim::expressive::slow_spatial() }; }
    MotionPreset defaultEffects() const { return { 200, anim::expressive::default_effects() }; }
    bool         operator==(const ExpressiveMotion&) const = default;
};

/** @ingroup singleton */
struct Motion {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::token::ExpressiveMotion expressive READ expressive CONSTANT FINAL)
public:
    ExpressiveMotion expressive() const { return {}; }
    bool             operator==(const Motion&) const = default;
};

} // namespace qml_material::token
