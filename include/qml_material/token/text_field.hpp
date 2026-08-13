#pragma once

#include <QtQml/QQmlEngine>

#include "qml_material/token/type_scale.hpp"

namespace qml_material::token
{

struct TextFieldSizeItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(text_field_size_item)

    Q_PROPERTY(qreal container_height MEMBER container_height CONSTANT FINAL)
    Q_PROPERTY(qreal horizontal_padding MEMBER horizontal_padding CONSTANT FINAL)
    Q_PROPERTY(qreal vertical_padding MEMBER vertical_padding CONSTANT FINAL)
    Q_PROPERTY(qreal icon_size MEMBER icon_size CONSTANT FINAL)
    Q_PROPERTY(qreal icon_spacing MEMBER icon_spacing CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::TypeScaleItem type_scale MEMBER type_scale CONSTANT FINAL)

public:
    qreal         container_height;
    qreal         horizontal_padding;
    qreal         vertical_padding;
    qreal         icon_size;
    qreal         icon_spacing;
    TypeScaleItem type_scale;
};

struct TextFieldSize {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(text_field_size)

    Q_PROPERTY(qml_material::token::TextFieldSizeItem xsmall MEMBER xsmall CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::TextFieldSizeItem small MEMBER small CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::TextFieldSizeItem medium MEMBER medium CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::TextFieldSizeItem large MEMBER large CONSTANT FINAL)
    Q_PROPERTY(qml_material::token::TextFieldSizeItem xlarge MEMBER xlarge CONSTANT FINAL)

public:
    TextFieldSizeItem xsmall {
        40.0, 12.0, 8.0, 18.0, 8.0, { 12, 16, QFont::Normal, QFont::Normal, 0.4 },
    };
    TextFieldSizeItem small {
        48.0, 12.0, 12.0, 20.0, 8.0, { 14, 20, QFont::Normal, QFont::Normal, 0.25 },
    };
    TextFieldSizeItem medium {
        56.0, 16.0, 16.0, 24.0, 12.0, { 16, 24, QFont::Normal, QFont::Normal, 0.5 },
    };
    TextFieldSizeItem large {
        64.0, 20.0, 20.0, 24.0, 12.0, { 16, 24, QFont::Normal, QFont::Normal, 0.5 },
    };
    TextFieldSizeItem xlarge {
        72.0, 24.0, 24.0, 32.0, 16.0, { 16, 24, QFont::Normal, QFont::Normal, 0.5 },
    };
};

} // namespace qml_material::token
