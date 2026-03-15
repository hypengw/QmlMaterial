#pragma once

#include <QtQml/QQmlEngine>
#include "qml_material/core.hpp"

namespace qml_material::token
{

/**
 * @brief Split button size tokens for a specific size class
 */
struct SplitButtonSizeItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(split_button_size_item)
    
    /// Split button container height (dp)
    Q_PROPERTY(qreal container_height MEMBER container_height CONSTANT FINAL)
    /// Space between leading and trailing buttons (dp)
    Q_PROPERTY(qreal between_space MEMBER between_space CONSTANT FINAL)
    /// Inner corner radius where buttons meet (dp)
    Q_PROPERTY(qreal inner_corner_size MEMBER inner_corner_size CONSTANT FINAL)
    /// Outer corner radius (dp)
    Q_PROPERTY(qreal outer_corner_size MEMBER outer_corner_size CONSTANT FINAL)
    /// Leading space of leading button (dp)
    Q_PROPERTY(qreal leading_button_leading_space MEMBER leading_button_leading_space CONSTANT FINAL)
    /// Trailing space of leading button (dp)
    Q_PROPERTY(qreal leading_button_trailing_space MEMBER leading_button_trailing_space CONSTANT FINAL)
    /// Icon size for trailing button (dp)
    Q_PROPERTY(qreal trailing_button_icon_size MEMBER trailing_button_icon_size CONSTANT FINAL)
    /// Leading space of trailing button (dp)
    Q_PROPERTY(qreal trailing_button_leading_space MEMBER trailing_button_leading_space CONSTANT FINAL)
    /// Trailing space of trailing button (dp)
    Q_PROPERTY(qreal trailing_button_trailing_space MEMBER trailing_button_trailing_space CONSTANT FINAL)
    /// Inner corner radius when hovered (dp)
    Q_PROPERTY(qreal inner_corner_hovered_size MEMBER inner_corner_hovered_size CONSTANT FINAL)
    /// Inner corner radius when pressed (dp)
    Q_PROPERTY(qreal inner_corner_pressed_size MEMBER inner_corner_pressed_size CONSTANT FINAL)
    /// Inner corner radius for trailing button when selected (dp)
    Q_PROPERTY(qreal trailing_button_inner_corner_selected_size MEMBER trailing_button_inner_corner_selected_size CONSTANT FINAL)

public:
    qreal container_height;
    qreal between_space;
    qreal inner_corner_size;
    qreal outer_corner_size;
    qreal leading_button_leading_space;
    qreal leading_button_trailing_space;
    qreal trailing_button_icon_size;
    qreal trailing_button_leading_space;
    qreal trailing_button_trailing_space;
    qreal inner_corner_hovered_size;
    qreal inner_corner_pressed_size;
    qreal trailing_button_inner_corner_selected_size;
};

/**
 * @brief Collection of split button size tokens for different size classes
 */
struct SplitButtonSize {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(split_button_size)
    
    /// Size tokens for extra small (XS) split buttons
    Q_PROPERTY(qml_material::token::SplitButtonSizeItem xsmall MEMBER xsmall CONSTANT FINAL)
    /// Size tokens for small (S) split buttons
    Q_PROPERTY(qml_material::token::SplitButtonSizeItem small MEMBER small CONSTANT FINAL)
    /// Size tokens for medium (M) split buttons
    Q_PROPERTY(qml_material::token::SplitButtonSizeItem medium MEMBER medium CONSTANT FINAL)
    /// Size tokens for large (L) split buttons
    Q_PROPERTY(qml_material::token::SplitButtonSizeItem large MEMBER large CONSTANT FINAL)
    /// Size tokens for extra large (XL) split buttons
    Q_PROPERTY(qml_material::token::SplitButtonSizeItem xlarge MEMBER xlarge CONSTANT FINAL)

public:
    // md.comp.split-button.xsmall.* tokens
    SplitButtonSizeItem xsmall {
        32.0, // container.height
        2.0,  // between-space
        4.0,  // inner-corner.corner-size
        16.0, // outer-corner.corner-size (50% of 32)
        12.0, // leading-button.leading-space
        10.0, // leading-button.trailing-space
        22.0, // trailing-button.icon.size
        13.0, // trailing-button.leading-space
        13.0, // trailing-button.trailing-space
        8.0,  // inner-corner.hovered.corner-size
        8.0,  // inner-corner.pressed.corner-size
        16.0  // trailing-button.inner-corner.selected.corner-size (50% of 32)
    };

    // md.comp.split-button.small.* tokens
    SplitButtonSizeItem small {
        40.0, // container.height
        2.0,  // between-space
        4.0,  // inner-corner.corner-size
        20.0, // outer-corner.corner-size (50% of 40)
        16.0, // leading-button.leading-space
        12.0, // leading-button.trailing-space
        22.0, // trailing-button.icon.size
        13.0, // trailing-button.leading-space
        13.0, // trailing-button.trailing-space
        12.0, // inner-corner.hovered.corner-size
        12.0, // inner-corner.pressed.corner-size
        20.0  // trailing-button.inner-corner.selected.corner-size (50% of 40)
    };

    // md.comp.split-button.medium.* tokens
    SplitButtonSizeItem medium {
        56.0, // container.height
        2.0,  // between-space
        4.0,  // inner-corner.corner-size
        28.0, // outer-corner.corner-size (50% of 56)
        24.0, // leading-button.leading-space
        24.0, // leading-button.trailing-space
        26.0, // trailing-button.icon.size
        15.0, // trailing-button.leading-space
        15.0, // trailing-button.trailing-space
        12.0, // inner-corner.hovered.corner-size
        12.0, // inner-corner.pressed.corner-size
        28.0  // trailing-button.inner-corner.selected.corner-size (50% of 56)
    };

    // md.comp.split-button.large.* tokens
    SplitButtonSizeItem large {
        96.0, // container.height
        2.0,  // between-space
        8.0,  // inner-corner.corner-size
        48.0, // outer-corner.corner-size (50% of 96)
        48.0, // leading-button.leading-space
        48.0, // leading-button.trailing-space
        38.0, // trailing-button.icon.size
        29.0, // trailing-button.leading-space
        29.0, // trailing-button.trailing-space
        20.0, // inner-corner.hovered.corner-size
        20.0, // inner-corner.pressed.corner-size
        48.0  // trailing-button.inner-corner.selected.corner-size (50% of 96)
    };

    // md.comp.split-button.xlarge.* tokens
    SplitButtonSizeItem xlarge {
        136.0, // container.height
        2.0,   // between-space
        12.0,  // inner-corner.corner-size
        68.0,  // outer-corner.corner-size (50% of 136)
        64.0,  // leading-button.leading-space
        64.0,  // leading-button.trailing-space
        50.0,  // trailing-button.icon.size
        43.0,  // trailing-button.leading-space
        43.0,  // trailing-button.trailing-space
        20.0,  // inner-corner.hovered.corner-size
        20.0,  // inner-corner.pressed.corner-size
        68.0   // trailing-button.inner-corner.selected.corner-size (50% of 136)
    };
};

} // namespace qml_material::token
