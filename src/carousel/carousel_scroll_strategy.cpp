#include "qml_material/carousel/carousel_scroll_strategy.hpp"

#include <QtMath>

namespace qml_material
{

namespace
{

auto lerpKeylineLists(const KeylineList& a, const KeylineList& b, qreal t) -> KeylineList
{
    KeylineList out = a;
    const int   n   = qMin(a.keylines.size(), b.keylines.size());
    out.keylines.resize(n);
    for (int i = 0; i < n; ++i) {
        out.keylines[i].offset = a.keylines[i].offset + (b.keylines[i].offset - a.keylines[i].offset) * t;
        out.keylines[i].size   = a.keylines[i].size + (b.keylines[i].size - a.keylines[i].size) * t;
        out.keylines[i].size_class = t < 0.5 ? a.keylines[i].size_class : b.keylines[i].size_class;
    }
    if (!b.keylines.isEmpty()) {
        out.large_size  = b.large_size;
        out.medium_size = b.medium_size;
        out.small_size  = b.small_size;
        out.scroll_step = b.scroll_step;
    }
    return out;
}

auto shiftKeylines(const KeylineList& from, qreal delta) -> KeylineList
{
    KeylineList out = from;
    for (auto& kl : out.keylines) {
        kl.offset += delta;
    }
    return out;
}

} // namespace

auto CarouselScrollStrategy::build(const KeylineList& default_keylines, qreal carousel_main_axis_size)
    -> CarouselScrollStrategy
{
    CarouselScrollStrategy strategy;
    strategy.default_keylines = default_keylines;
    strategy.start_steps.append(default_keylines);
    strategy.end_steps.append(default_keylines);

    if (default_keylines.keylines.isEmpty() || carousel_main_axis_size <= 0) {
        return strategy;
    }

    const qreal first_leading = default_keylines.keylines.first().offset
        - default_keylines.keylines.first().size * 0.5;
    strategy.start_shift_distance = qMax(0.0, first_leading);

  const Keyline& last = default_keylines.keylines.last();
    const qreal last_trailing = last.offset + last.size * 0.5;
    strategy.end_shift_distance = qMax(0.0, last_trailing - carousel_main_axis_size);

    if (strategy.start_shift_distance > 0.5) {
        strategy.start_steps.append(shiftKeylines(default_keylines, strategy.start_shift_distance));
    }

    if (strategy.end_shift_distance > 0.5) {
        strategy.end_steps.append(shiftKeylines(default_keylines, -strategy.end_shift_distance));
    }

    return strategy;
}

auto CarouselScrollStrategy::keylinesForScrollOffset(qreal scroll_offset, qreal max_scroll_offset,
                                                     bool round_to_step) const -> KeylineList
{
    if (default_keylines.keylines.isEmpty()) {
        return default_keylines;
    }

    const qreal start_zone_end = start_shift_distance;
    const qreal end_zone_start = qMax(0.0, max_scroll_offset - end_shift_distance);

    if (scroll_offset <= start_zone_end && start_steps.size() > 1) {
        const qreal t = start_zone_end > 0 ? qBound(0.0, scroll_offset / start_zone_end, 1.0) : 1.0;
        const qreal step_t = round_to_step ? (t >= 0.5 ? 1.0 : 0.0) : t;
        return lerpKeylineLists(start_steps.first(), start_steps.last(), step_t);
    }

    if (scroll_offset >= end_zone_start && end_steps.size() > 1 && max_scroll_offset > end_zone_start) {
        const qreal span = max_scroll_offset - end_zone_start;
        const qreal t    = qBound(0.0, (scroll_offset - end_zone_start) / span, 1.0);
        const qreal step_t = round_to_step ? (t >= 0.5 ? 1.0 : 0.0) : t;
        return lerpKeylineLists(end_steps.first(), end_steps.last(), step_t);
    }

    return default_keylines;
}

} // namespace qml_material
