#pragma once

#include <QtQml/qqml.h>

namespace qml_material::token {

struct BadgeSizeItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(badge_size_item)

    Q_PROPERTY(qreal small_diameter MEMBER small_diameter CONSTANT FINAL)
    Q_PROPERTY(qreal large_height MEMBER large_height CONSTANT FINAL)
    Q_PROPERTY(qreal large_min_width MEMBER large_min_width CONSTANT FINAL)
    Q_PROPERTY(qreal large_horizontal_padding MEMBER large_horizontal_padding CONSTANT FINAL)
    Q_PROPERTY(qreal offset_x MEMBER offset_x CONSTANT FINAL)
    Q_PROPERTY(qreal offset_y MEMBER offset_y CONSTANT FINAL)

public:
    qreal small_diameter;
    qreal large_height;
    qreal large_min_width;
    qreal large_horizontal_padding;
    qreal offset_x;
    qreal offset_y;
};

struct BadgeSize {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(badge_size)

    Q_PROPERTY(qml_material::token::BadgeSizeItem def MEMBER def CONSTANT FINAL)

public:
    BadgeSizeItem def{ 6.0, 16.0, 16.0, 4.0, 4.0, -4.0 };
};

} // namespace qml_material::token

