#pragma once
#include <QtQml/qqml.h>

namespace qml_material::token
{
struct FloatingToolbar {
    Q_GADGET
    QML_VALUE_TYPE(floating_toolbar_token)
    Q_PROPERTY(qreal thickness MEMBER thickness CONSTANT FINAL)
    Q_PROPERTY(qreal padding MEMBER padding CONSTANT FINAL)
public:
    qreal thickness = 64;
    qreal padding   = 8;
};
} // namespace qml_material::token
