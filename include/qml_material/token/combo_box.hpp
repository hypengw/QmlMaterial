#pragma once

#include <QtQml/QQmlEngine>

#include "qml_material/token/type_scale.hpp"

namespace qml_material::token
{

struct ComboBoxSizeItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(combo_box_size_item)

    Q_PROPERTY(qreal container_height MEMBER container_height CONSTANT FINAL)
    Q_PROPERTY(qreal horizontal_padding MEMBER horizontal_padding CONSTANT FINAL)
    Q_PROPERTY(qreal indicator_size MEMBER indicator_size CONSTANT FINAL)
    Q_PROPERTY(qreal indicator_spacing MEMBER indicator_spacing CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::TypeScaleItem type_scale MEMBER type_scale CONSTANT FINAL)

public:
    qreal         container_height;
    qreal         horizontal_padding;
    qreal         indicator_size;
    qreal         indicator_spacing;
    TypeScaleItem type_scale;
};

struct ComboBoxSize {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(combo_box_size)

    Q_PROPERTY(qml_material::token::ComboBoxSizeItem xsmall MEMBER xsmall CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ComboBoxSizeItem small MEMBER small CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ComboBoxSizeItem medium MEMBER medium CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ComboBoxSizeItem large MEMBER large CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::ComboBoxSizeItem xlarge MEMBER xlarge CONSTANT FINAL)

public:
    ComboBoxSizeItem xsmall {
        32.0, 8.0, 18.0, 0.0, { 12, 16, QFont::Normal, QFont::Normal, 0.4 },
    };
    ComboBoxSizeItem small {
        40.0, 10.0, 20.0, 0.0, { 14, 20, QFont::Normal, QFont::Normal, 0.25 },
    };
    ComboBoxSizeItem medium {
        48.0, 12.0, 24.0, 0.0, { 16, 24, QFont::Normal, QFont::Normal, 0.5 },
    };
    ComboBoxSizeItem large {
        56.0, 16.0, 24.0, 0.0, { 16, 24, QFont::Normal, QFont::Normal, 0.5 },
    };
    ComboBoxSizeItem xlarge {
        72.0, 24.0, 32.0, 0.0, { 16, 24, QFont::Normal, QFont::Normal, 0.5 },
    };
};

} // namespace qml_material::token
