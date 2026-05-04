#pragma once

#include <QtQml/QQmlEngine>
#include "qml_material/core.hpp"

namespace qml_material::token
{

/**
 * @brief Segmented button size tokens for a specific size class
 *
 * Mirrors `md.comp.button.{xsmall,small,medium,large,xlarge}` from
 * material-components-android. Container height is M3 spec.
 */
struct SegmentedButtonSizeItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(segmented_button_size_item)

    /// Segment container height (dp)
    Q_PROPERTY(qreal container_height MEMBER container_height CONSTANT FINAL)
    /// Icon size (dp)
    Q_PROPERTY(qreal icon_size MEMBER icon_size CONSTANT FINAL)
    /// Leading space / padding-start (dp)
    Q_PROPERTY(qreal leading_space MEMBER leading_space CONSTANT FINAL)
    /// Trailing space / padding-end (dp)
    Q_PROPERTY(qreal trailing_space MEMBER trailing_space CONSTANT FINAL)
    /// Spacing between icon and label (dp)
    Q_PROPERTY(qreal icon_label_space MEMBER icon_label_space CONSTANT FINAL)
    /// Outlined variant outline width (dp)
    Q_PROPERTY(qreal outline_width MEMBER outline_width CONSTANT FINAL)

public:
    qreal container_height;
    qreal icon_size;
    qreal leading_space;
    qreal trailing_space;
    qreal icon_label_space;
    qreal outline_width;
};

/**
 * @brief Collection of segmented button size tokens for different size classes
 */
struct SegmentedButtonSize {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(segmented_button_size)

    Q_PROPERTY(qml_material::token::SegmentedButtonSizeItem xsmall MEMBER xsmall CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SegmentedButtonSizeItem small MEMBER small CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SegmentedButtonSizeItem medium MEMBER medium CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SegmentedButtonSizeItem large MEMBER large CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::SegmentedButtonSizeItem xlarge MEMBER xlarge CONSTANT FINAL)

public:
    SegmentedButtonSizeItem xsmall {
        32.0, // container_height
        20.0, // icon_size
        12.0, // leading_space
        12.0, // trailing_space
        8.0,  // icon_label_space
        1.0   // outline_width
    };
    SegmentedButtonSizeItem small {
        40.0, // container_height
        20.0, // icon_size
        16.0, // leading_space
        16.0, // trailing_space
        8.0,  // icon_label_space
        1.0   // outline_width
    };
    SegmentedButtonSizeItem medium {
        56.0, // container_height
        24.0, // icon_size
        24.0, // leading_space
        24.0, // trailing_space
        8.0,  // icon_label_space
        1.0   // outline_width
    };
    SegmentedButtonSizeItem large {
        96.0, // container_height
        32.0, // icon_size
        48.0, // leading_space
        48.0, // trailing_space
        12.0, // icon_label_space
        2.0   // outline_width
    };
    SegmentedButtonSizeItem xlarge {
        136.0, // container_height
        40.0,  // icon_size
        64.0,  // leading_space
        64.0,  // trailing_space
        16.0,  // icon_label_space
        3.0    // outline_width
    };
};

} // namespace qml_material::token
