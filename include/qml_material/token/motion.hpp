#pragma once

#include <QtCore/QObject>
#include <QtQml/qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material::token
{

struct QML_MATERIAL_API SpringSpec {
    Q_GADGET
    QML_VALUE_TYPE(springSpec)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(qreal stiffness MEMBER stiffness FINAL)
    Q_PROPERTY(qreal dampingRatio MEMBER dampingRatio FINAL)
public:
    qreal stiffness                           = 1500;
    qreal dampingRatio                        = 1;
    bool  operator==(const SpringSpec&) const = default;
    bool  valid() const;
};

struct SpringScheme {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::token::SpringSpec fastSpatial READ fastSpatial CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SpringSpec defaultSpatial READ defaultSpatial CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SpringSpec slowSpatial READ slowSpatial CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SpringSpec fastEffects READ fastEffects CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SpringSpec defaultEffects READ defaultEffects CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SpringSpec slowEffects READ slowEffects CONSTANT FINAL)
public:
    bool       expressive = false;
    SpringSpec fastSpatial() const {
        return expressive ? SpringSpec { 800, .6 } : SpringSpec { 1400, .9 };
    }
    SpringSpec defaultSpatial() const {
        return expressive ? SpringSpec { 380, .8 } : SpringSpec { 700, .9 };
    }
    SpringSpec slowSpatial() const {
        return expressive ? SpringSpec { 200, .8 } : SpringSpec { 300, .9 };
    }
    SpringSpec fastEffects() const { return { 3800, 1 }; }
    SpringSpec defaultEffects() const { return { 1600, 1 }; }
    SpringSpec slowEffects() const { return { 800, 1 }; }
    bool       operator==(const SpringScheme&) const = default;
};

struct SpringSchemes {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::token::SpringScheme standard READ standard CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SpringScheme expressive READ expressive CONSTANT FINAL)
public:
    SpringScheme standard() const { return {}; }
    SpringScheme expressive() const { return { true }; }
    bool         operator==(const SpringSchemes&) const = default;
};

/** @ingroup singleton */
struct Motion {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::token::SpringSchemes spring READ spring CONSTANT FINAL)
public:
    SpringSchemes spring() const { return {}; }
    bool          operator==(const Motion&) const = default;
};

} // namespace qml_material::token
