#pragma once

#include "qml_material/carousel/carousel_keylines.hpp"
#include "qml_material/carousel/carousel_strategy.hpp"

namespace qml_material
{

enum class HeroScrollZone
{
    Leading,
    Middle,
    TrailingLerp,
    TrailingRest,
};

struct CarouselScrollStrategy
{
    KeylineList default_keylines;
    QVector<KeylineList> start_steps;
    QVector<KeylineList> end_steps;
    qreal start_shift_distance = 0;
    qreal end_shift_distance   = 0;
    bool  hero_lerp            = false;
    bool  keep_leading_peek    = false;
    qreal viewport_size        = 0;

    static auto build(const KeylineList& default_keylines, qreal carousel_main_axis_size) -> CarouselScrollStrategy;

    static auto build(const CarouselLayoutInput& in, const KeylineList& default_keylines)
        -> CarouselScrollStrategy;

    static auto buildHero(const CarouselLayoutInput& in, bool keep_leading_peek,
                          const QVector<qreal>& snap_offsets, qreal max_scroll_offset)
        -> CarouselScrollStrategy;

    auto keylinesForScrollOffset(qreal scroll_offset, qreal max_scroll_offset, bool round_to_step = false) const
        -> KeylineList;

    auto zoneFor(qreal scroll_offset, qreal keyline_max) const -> HeroScrollZone;
};

} // namespace qml_material
