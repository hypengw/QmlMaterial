#pragma once
#include <QtQml/qqml.h>

namespace qml_material::token
{
struct ExtendedFAB {
    Q_GADGET
    QML_VALUE_TYPE(extended_fab_token)
    Q_PROPERTY(qreal height MEMBER height CONSTANT FINAL)
    Q_PROPERTY(qreal iconSize MEMBER iconSize CONSTANT FINAL)
    Q_PROPERTY(qreal leadingSpace MEMBER leadingSpace CONSTANT FINAL)
    Q_PROPERTY(qreal trailingSpace MEMBER trailingSpace CONSTANT FINAL)
    Q_PROPERTY(qreal iconSpacing MEMBER iconSpacing CONSTANT FINAL)
    Q_PROPERTY(qreal minimumWidth MEMBER minimumWidth CONSTANT FINAL)
public:
    qreal height        = 56;
    qreal iconSize      = 24;
    qreal leadingSpace  = 16;
    qreal trailingSpace = 20;
    qreal iconSpacing   = 12;
    qreal minimumWidth  = 80;
};
} // namespace qml_material::token
