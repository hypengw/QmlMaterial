#pragma once

#include <QtQml/QQmlEngine>
#include "qml_material/core.hpp"

namespace qml_material::token
{

/**
 * @brief Button size tokens for a specific size class
 */
struct ButtonSizeItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(button_size_item)
    
    /// Button container height (dp)
    Q_PROPERTY(qreal container_height MEMBER container_height CONSTANT FINAL)
    /// Corner radius (dp)
    Q_PROPERTY(qreal corner_size MEMBER corner_size CONSTANT FINAL)
    /// Corner radius when pressed (dp)
    Q_PROPERTY(qreal pressed_corner_size MEMBER pressed_corner_size CONSTANT FINAL)
    /// Icon size (dp)
    Q_PROPERTY(qreal icon_size MEMBER icon_size CONSTANT FINAL)
    /// Leading space (padding-left) (dp)
    Q_PROPERTY(qreal leading_space MEMBER leading_space CONSTANT FINAL)
    /// Trailing space (padding-right) (dp)
    Q_PROPERTY(qreal trailing_space MEMBER trailing_space CONSTANT FINAL)
    /// Spacing between icon and text (dp)
    Q_PROPERTY(qreal spacing MEMBER spacing CONSTANT FINAL)

public:
    qreal container_height;
    qreal corner_size;
    qreal pressed_corner_size;
    qreal icon_size;
    qreal leading_space;
    qreal trailing_space;
    qreal spacing;
};

/**
 * @brief Collection of button size tokens for different size classes
 */
struct ButtonSize {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(button_size)
    
    Q_PROPERTY(qml_material::token::ButtonSizeItem xsmall MEMBER xsmall CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ButtonSizeItem small MEMBER small CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ButtonSizeItem medium MEMBER medium CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ButtonSizeItem large MEMBER large CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ButtonSizeItem xlarge MEMBER xlarge CONSTANT FINAL)

public:
    ButtonSizeItem xsmall {
        32.0, // container_height
        12.0, // corner_size
        8.0,  // pressed_corner_size
        18.0, // icon_size
        16.0, // leading_space
        16.0, // trailing_space
        8.0   // spacing
    };

    ButtonSizeItem small {
        40.0, // container_height
        12.0, // corner_size
        8.0,  // pressed_corner_size
        18.0, // icon_size
        24.0, // leading_space
        24.0, // trailing_space
        8.0   // spacing
    };

    ButtonSizeItem medium {
        48.0, // container_height
        16.0, // corner_size
        12.0, // pressed_corner_size
        24.0, // icon_size
        24.0, // leading_space
        24.0, // trailing_space
        8.0   // spacing
    };

    ButtonSizeItem large {
        56.0, // container_height
        28.0, // corner_size
        16.0, // pressed_corner_size
        24.0, // icon_size
        32.0, // leading_space
        32.0, // trailing_space
        12.0  // spacing
    };

    ButtonSizeItem xlarge {
        72.0, // container_height
        28.0, // corner_size
        16.0, // pressed_corner_size
        32.0, // icon_size
        32.0, // leading_space
        32.0, // trailing_space
        12.0  // spacing
    };
};

/**
 * @brief Icon button size tokens for a specific size class
 */
struct IconButtonSizeItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(icon_button_size_item)

    /// Icon size (dp)
    Q_PROPERTY(qreal icon_size MEMBER icon_size CONSTANT FINAL)
    /// Button container height (dp)
    Q_PROPERTY(qreal container_height MEMBER container_height CONSTANT FINAL)
    /// Default button container width (dp)
    Q_PROPERTY(qreal default_width MEMBER default_width CONSTANT FINAL)
    /// Narrow button container width (dp)
    Q_PROPERTY(qreal narrow_width MEMBER narrow_width CONSTANT FINAL)
    /// Wide button container width (dp)
    Q_PROPERTY(qreal wide_width MEMBER wide_width CONSTANT FINAL)
    /// Corner radius (dp)
    Q_PROPERTY(qreal corner_size MEMBER corner_size CONSTANT FINAL)
    /// Corner radius when pressed or checked (dp)
    Q_PROPERTY(qreal pressed_corner_size MEMBER pressed_corner_size CONSTANT FINAL)

public:
    qreal icon_size;
    qreal container_height;
    qreal default_width;
    qreal narrow_width;
    qreal wide_width;
    qreal corner_size;
    qreal pressed_corner_size;
};

/**
 * @brief Collection of icon button size tokens for different size classes
 */
struct IconButtonSize {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(icon_button_size)

    Q_PROPERTY(qml_material::token::IconButtonSizeItem xsmall MEMBER xsmall CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::IconButtonSizeItem small MEMBER small CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::IconButtonSizeItem medium MEMBER medium CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::IconButtonSizeItem large MEMBER large CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::IconButtonSizeItem xlarge MEMBER xlarge CONSTANT FINAL)

public:
    IconButtonSizeItem xsmall {
        20.0, // icon_size
        32.0, // container_height
        32.0, // default_width
        28.0, // narrow_width
        40.0, // wide_width
        12.0, // corner_size
        8.0   // pressed_corner_size
    };

    IconButtonSizeItem small {
        24.0, // icon_size
        40.0, // container_height
        40.0, // default_width
        32.0, // narrow_width
        52.0, // wide_width
        12.0, // corner_size
        8.0   // pressed_corner_size
    };

    IconButtonSizeItem medium {
        24.0, // icon_size
        56.0, // container_height
        56.0, // default_width
        48.0, // narrow_width
        72.0, // wide_width
        16.0, // corner_size
        12.0  // pressed_corner_size
    };

    IconButtonSizeItem large {
        32.0, // icon_size
        96.0, // container_height
        96.0, // default_width
        64.0, // narrow_width
        128.0,// wide_width
        28.0, // corner_size
        16.0  // pressed_corner_size
    };

    IconButtonSizeItem xlarge {
        40.0, // icon_size
        136.0,// container_height
        136.0,// default_width
        104.0,// narrow_width
        184.0,// wide_width
        28.0, // corner_size
        16.0  // pressed_corner_size
    };
};

} // namespace qml_material::token
