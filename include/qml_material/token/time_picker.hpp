#pragma once
#include <QtQml/qqml.h>

namespace qml_material::token
{
struct TimePicker {
    Q_GADGET
    QML_VALUE_TYPE(time_picker_token)
    Q_PROPERTY(qreal dialSize MEMBER dialSize CONSTANT FINAL)
    Q_PROPERTY(qreal selectorSize MEMBER selectorSize CONSTANT FINAL)
    Q_PROPERTY(qreal dialPadding MEMBER dialPadding CONSTANT FINAL)
    Q_PROPERTY(qreal fieldWidth MEMBER fieldWidth CONSTANT FINAL)
    Q_PROPERTY(qreal fieldHeight MEMBER fieldHeight CONSTANT FINAL)
    Q_PROPERTY(qreal separatorWidth MEMBER separatorWidth CONSTANT FINAL)
    Q_PROPERTY(qreal periodWidth MEMBER periodWidth CONSTANT FINAL)
    Q_PROPERTY(qreal periodGap MEMBER periodGap CONSTANT FINAL)
    Q_PROPERTY(qreal padding MEMBER padding CONSTANT FINAL)
    Q_PROPERTY(qreal dialGap MEMBER dialGap CONSTANT FINAL)
    Q_PROPERTY(qreal handWidth MEMBER handWidth CONSTANT FINAL)
    Q_PROPERTY(qreal centerSize MEMBER centerSize CONSTANT FINAL)
public:
    qreal dialSize       = 256;
    qreal selectorSize   = 48;
    qreal dialPadding    = 24;
    qreal fieldWidth     = 96;
    qreal fieldHeight    = 80;
    qreal separatorWidth = 24;
    qreal periodWidth    = 52;
    qreal periodGap      = 12;
    qreal padding        = 24;
    qreal dialGap        = 28;
    qreal handWidth      = 2;
    qreal centerSize     = 8;
};
} // namespace qml_material::token
