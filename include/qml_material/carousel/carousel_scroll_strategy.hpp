#pragma once

#include "qml_material/carousel/carousel_keylines.hpp"
#include "qml_material/carousel/carousel_strategy.hpp"

namespace qml_material
{

struct CarouselScrollStrategy {
    KeylineList          default_keylines;
    QVector<KeylineList> start_steps;
    QVector<KeylineList> end_steps;
    qreal                start_shift_distance = 0;
    qreal                end_shift_distance   = 0;
    bool                 centered_hero        = false;
    qreal                viewport_size        = 0;
    QVector<qreal>       end_shift_points;

    static auto build(const CarouselLayoutInput& in, const KeylineList& default_keylines)
        -> CarouselScrollStrategy;

    auto keylinesForScrollOffset(qreal scroll_offset, qreal max_scroll_offset,
                                 bool round_to_step = false) const -> KeylineList;

    auto snapOffsets(const CarouselLayoutInput&) const -> QVector<qreal>;
    auto initialCenter(const CarouselLayoutInput&) const -> qreal;
};

} // namespace qml_material
