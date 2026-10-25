#pragma once

#include "qml_material/input/nested_scroll.hpp"
#include "scroll_participant_p.hpp"
#include <QHash>

namespace qml_material
{
struct ScrollDispatchContext {
    QList<QPointer<NestedScroll>> chain;
    QHash<NestedScroll*, QPointF> origins;
    std::function<bool()>         current;
    std::function<void(bool)>     applying;
    std::function<void()>         cancel;
};
QPointF dispatchNestedScroll(const ScrollDispatchContext&, QPointF delta,
                             ScrollParticipant::Activity, QPointF  velocity);
} // namespace qml_material
