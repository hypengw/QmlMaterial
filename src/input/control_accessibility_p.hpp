#pragma once
#include <QVariant>
class QQuickItem;
namespace qml_material::detail
{
void ensureControlAccessibility();
void notifyAccessibleValue(QQuickItem*, const QVariant&);
} // namespace qml_material::detail
