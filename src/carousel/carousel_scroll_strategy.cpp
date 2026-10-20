#include "qml_material/carousel/carousel_scroll_strategy.hpp"
#include <algorithm>

namespace qml_material
{
namespace
{
constexpr int small = 0, large = 2;
qreal         lerp(qreal a, qreal b, qreal t) { return a + (b - a) * t; }
KeylineList   interpolate(const KeylineList& a, const KeylineList& b, qreal t) {
    auto result = a;
    for (int i = 0; i < result.keylines.size(); ++i) {
        auto& key  = result.keylines[i];
        key.offset = lerp(a.keylines[i].offset, b.keylines[i].offset, t);
        key.size   = lerp(a.keylines[i].size, b.keylines[i].size, t);
        key.unadjusted_offset =
            lerp(a.keylines[i].unadjusted_offset, b.keylines[i].unadjusted_offset, t);
        key.size_class = t < .5 ? a.keylines[i].size_class : b.keylines[i].size_class;
    }
    result.large_size  = lerp(a.large_size, b.large_size, t);
    result.medium_size = lerp(a.medium_size, b.medium_size, t);
    result.small_size  = lerp(a.small_size, b.small_size, t);
    return result;
}
qreal focalCenter(const KeylineList& list) {
    for (const auto& key : list.keylines)
        if (key.size_class == large) return key.unadjusted_offset;
    return 0;
}
void logicalOffsets(KeylineList& list, qreal stride) {
    int focal = 0;
    for (int i = 0; i < list.keylines.size(); ++i)
        if (list.keylines[i].size_class == large) {
            focal = i;
            break;
        }
    const auto pivot = list.keylines.value(focal).offset;
    list.scroll_step = stride;
    for (int i = 0; i < list.keylines.size(); ++i)
        list.keylines[i].unadjusted_offset = pivot + (i - focal) * stride;
}
} // namespace

auto CarouselScrollStrategy::build(const CarouselLayoutInput& in, const KeylineList& keylines)
    -> CarouselScrollStrategy {
    CarouselScrollStrategy result;
    result.viewport_size = in.viewport_size;
    if (keylines.keylines.isEmpty()) return result;
    result.centered_hero =
        in.layout == CarouselLayoutId::HeroCenter && in.count > 1 && keylines.keylines.size() > 1;
    if (result.centered_hero) {
        auto       middle = keylines;
        const auto stride = middle.large_size + in.spacing;
        auto       leading =
            CarouselHeroKeylines::phaseKeylines(in, CarouselHeroKeylines::HeroPhase::Leading, true);
        auto trailing = leading;
        leading.keylines.prepend({ in.content_padding_start - in.spacing, 0, small });
        trailing.keylines = {
            { in.content_padding_start + trailing.small_size / 2, trailing.small_size, small },
            { in.viewport_size - in.content_padding_end - trailing.large_size / 2,
              trailing.large_size,
              large },
            { in.viewport_size - in.content_padding_end + in.spacing, 0, small }
        };
        logicalOffsets(leading, stride);
        logicalOffsets(middle, stride);
        logicalOffsets(trailing, stride);
        result.default_keylines     = middle;
        result.start_steps          = { leading, middle };
        result.end_steps            = { middle, trailing };
        const auto snaps            = result.snapOffsets(in);
        result.start_shift_distance = snaps[1];
        result.end_shift_distance   = snaps.last() - snaps[snaps.size() - 2];
        return result;
    }
    auto adjusted = keylines;
    if (in.count > 0 && adjusted.keylines.size() > in.count) {
        while (adjusted.keylines.size() > in.count) adjusted.keylines.removeLast();
        qreal remainder = in.viewport_size - in.content_padding_start - in.content_padding_end -
                          (in.count - 1) * in.spacing;
        for (const auto& key : adjusted.keylines)
            if (key.size_class != large) remainder -= key.size;
        adjusted.large_size = std::max(qreal(1), remainder);
        for (auto& key : adjusted.keylines)
            if (key.size_class == large) key.size = adjusted.large_size;
    }
    const auto arrange = [&](KeylineList list) {
        qreal cursor = in.content_padding_start;
        for (auto& key : list.keylines) {
            key.offset = cursor + key.size / 2;
            cursor += key.size + in.spacing;
        }
        logicalOffsets(list, list.large_size + in.spacing);
        return list;
    };
    result.default_keylines = arrange(adjusted);
    result.start_steps      = { result.default_keylines };
    result.end_steps        = { result.default_keylines };
    result.end_shift_points = { 0 };
    auto       step         = result.default_keylines;
    const auto lastLogical  = step.keylines.last().unadjusted_offset;
    while (step.keylines.last().size_class != large) {
        const auto moved = step.keylines.takeLast();
        int        index = 0;
        while (index < step.keylines.size() && step.keylines[index].size < moved.size) ++index;
        step.keylines.insert(index, moved);
        step = arrange(step);
        result.end_steps.append(step);
        result.end_shift_points.append(
            std::max(qreal(0), lastLogical - step.keylines.last().unadjusted_offset));
    }
    result.end_shift_distance = result.end_shift_points.last();
    return result;
}

auto CarouselScrollStrategy::initialCenter(const CarouselLayoutInput& in) const -> qreal {
    return centered_hero ? focalCenter(start_steps.first())
                         : in.content_padding_start + default_keylines.large_size / 2;
}
auto CarouselScrollStrategy::snapOffsets(const CarouselLayoutInput& in) const -> QVector<qreal> {
    QVector<qreal> result;
    if (default_keylines.keylines.isEmpty()) return result;
    const auto stride = default_keylines.scroll_step, center = initialCenter(in);
    for (int i = 0; i < in.count; ++i) {
        qreal focal;
        if (centered_hero)
            focal = i == 0              ? focalCenter(start_steps.first())
                    : i == in.count - 1 ? focalCenter(end_steps.last())
                                        : focalCenter(default_keylines);
        else {
            const auto step =
                std::clamp(int(end_steps.size()) - in.count + i, 0, int(end_steps.size()) - 1);
            focal = focalCenter(end_steps[step]);
        }
        const auto offset = std::max(qreal(0), center + i * stride - focal);
        result.append(result.isEmpty() ? offset : std::max(result.last(), offset));
    }
    return result;
}
auto CarouselScrollStrategy::keylinesForScrollOffset(qreal offset, qreal maximum, bool round) const
    -> KeylineList {
    if (default_keylines.keylines.isEmpty()) return default_keylines;
    if (centered_hero) {
        if (start_shift_distance + end_shift_distance > maximum + .01) {
            const auto t = maximum > 0 ? std::clamp(offset / maximum, qreal(0), qreal(1)) : 0;
            return interpolate(start_steps.first(), end_steps.last(), round ? (t < .5 ? 0 : 1) : t);
        }
        if (offset < start_shift_distance) {
            const auto t = std::clamp(offset / start_shift_distance, qreal(0), qreal(1));
            return interpolate(start_steps.first(), default_keylines, round ? (t < .5 ? 0 : 1) : t);
        }
        if (offset > maximum - end_shift_distance) {
            const auto t = std::clamp(
                (offset - maximum + end_shift_distance) / end_shift_distance, qreal(0), qreal(1));
            return interpolate(default_keylines, end_steps.last(), round ? (t < .5 ? 0 : 1) : t);
        }
        return default_keylines;
    }
    const auto distance = offset - std::max(qreal(0), maximum - end_shift_distance);
    if (distance <= 0 || end_steps.size() < 2) return default_keylines;
    for (int i = 1; i < end_steps.size(); ++i) {
        if (distance > end_shift_points[i]) continue;
        const auto span = end_shift_points[i] - end_shift_points[i - 1];
        const auto t =
            span > 0 ? std::clamp((distance - end_shift_points[i - 1]) / span, qreal(0), qreal(1))
                     : 1;
        if (round) return end_steps[t < .5 ? i - 1 : i];
        return interpolate(end_steps[i - 1], end_steps[i], t);
    }
    return end_steps.last();
}
} // namespace qml_material
