#pragma once

#include "qml_material/carousel/carousel_keylines.hpp"

namespace qml_material
{

struct CarouselScrollStrategy
{
    KeylineList default_keylines;
    QVector<KeylineList> start_steps;
    QVector<KeylineList> end_steps;
    qreal start_shift_distance = 0;
    qreal end_shift_distance   = 0;

    static auto build(const KeylineList& default_keylines, qreal carousel_main_axis_size) -> CarouselScrollStrategy;

    auto keylinesForScrollOffset(qreal scroll_offset, qreal max_scroll_offset, bool round_to_step = false) const
        -> KeylineList;
};

} // namespace qml_material
