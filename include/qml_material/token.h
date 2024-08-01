#pragma once

#include <QQmlEngine>
#include <QFont>
#include "core/core.h"

namespace qml_material::token
{
struct Elevation {
    Q_GADGET
    QML_VALUE_TYPE(t_elevation)

    Q_PROPERTY(i32 level0 MEMBER level0 CONSTANT FINAL)
    Q_PROPERTY(i32 level1 MEMBER level1 CONSTANT FINAL)
    Q_PROPERTY(i32 level2 MEMBER level2 CONSTANT FINAL)
    Q_PROPERTY(i32 level3 MEMBER level3 CONSTANT FINAL)
    Q_PROPERTY(i32 level4 MEMBER level4 CONSTANT FINAL)
    Q_PROPERTY(i32 level5 MEMBER level5 CONSTANT FINAL)

public:
    i32 level0 { 0 };
    i32 level1 { 1 };
    i32 level2 { 3 };
    i32 level3 { 6 };
    i32 level4 { 8 };
    i32 level5 { 12 };
};
} // namespace qml_material::token