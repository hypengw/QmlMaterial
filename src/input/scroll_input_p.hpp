#pragma once

#include "qml_material/scrollable/flickable.hpp"

namespace qml_material
{
class NestedScroll;

class ScrollInputAdapter {
public:
    static void                         attach(Flickable*);
    static void                         cancel(Flickable*);
    static void                         cancelMotion(Flickable*);
    static void                         ensure(NestedScroll*);
    static void                         stop(NestedScroll*);
    static void                         destroy(NestedScroll*);
    static bool                         coordinates(QQuickItem* receiver, QQuickItem* ancestor);
    static Flickable::ScrollConsumption scroll(Flickable*, QPointF, Flickable::ScrollInput,
                                               Qt::ScrollPhase);
};
} // namespace qml_material
