#include "qml_material/carousel/carousel_keylines.hpp"

#include "qml_material/carousel/carousel_scroll_strategy.hpp"

#include <QtMath>
#include <algorithm>
#include <limits>

namespace qml_material
{

namespace
{

constexpr int kLayoutUncontained = 0;
constexpr int kLayoutMultiBrowse = 1;
constexpr int kLayoutHero        = 2;
constexpr int kLayoutHeroCenter  = 3;
constexpr int kLayoutFullScreen  = 4;

constexpr int kSizeSmall  = 0;
constexpr int kSizeMedium = 1;
constexpr int kSizeLarge  = 2;

auto lerp(qreal a, qreal b, qreal t) -> qreal
{
    return a + (b - a) * t;
}

auto clamp(qreal v, qreal lo, qreal hi) -> qreal
{
    return qBound(lo, v, hi);
}

auto sizeClassFor(qreal size, qreal large, qreal medium, qreal small) -> int
{
    if (size >= large - 1) {
        return kSizeLarge;
    }
    if (size >= medium - 1) {
        return kSizeMedium;
    }
    return kSizeSmall;
}

auto sizeAtCenter(qreal center, const QVector<Keyline>& keylines) -> qreal
{
    if (keylines.isEmpty()) {
        return 0;
    }
    if (center <= keylines.first().offset) {
        return keylines.first().size;
    }
    for (int i = 0; i + 1 < keylines.size(); ++i) {
        const qreal a = keylines[i].offset;
        const qreal b = keylines[i + 1].offset;
        if (center >= a && center <= b) {
            const qreal span = b - a;
            const qreal t    = span > 0 ? (center - a) / span : 0;
            return lerp(keylines[i].size, keylines[i + 1].size, t);
        }
    }
    return keylines.last().size;
}

auto resolveSize(qreal anchor, qreal scroll_offset, const QVector<Keyline>& keylines, qreal guess) -> qreal
{
    qreal size = guess;
    for (int i = 0; i < 6; ++i) {
        const qreal center = anchor + size * 0.5 - scroll_offset;
        const qreal next   = sizeAtCenter(center, keylines);
        if (qFuzzyCompare(size, next)) {
            break;
        }
        size = next;
    }
    return size;
}

void applyMaskAndParallax(CarouselItemGeometry& g, qreal view_start, qreal view_end, qreal parallax_ratio,
                          qreal min_peek_px, bool hide_narrow_peek)
{
    const qreal item_end = g.position + g.size;
    if (item_end <= view_start || g.position >= view_end) {
        g.mask_start = 1;
        g.mask_end   = 1;
        return;
    }

    if (g.position < view_start) {
        g.mask_start = clamp((view_start - g.position) / g.size, 0.0, 1.0);
    }
    if (item_end > view_end) {
        g.mask_end = clamp((item_end - view_end) / g.size, 0.0, 1.0);
    }

    if (hide_narrow_peek) {
        const qreal visible = g.size * (1.0 - g.mask_start - g.mask_end);
        if (visible < min_peek_px) {
            g.mask_start = 1;
            g.mask_end   = 1;
            return;
        }
    }

    g.parallax_shift = (g.mask_start - g.mask_end) * g.size * parallax_ratio * 0.5;
}

auto fitMultiBrowseSizes(qreal available, qreal spacing, qreal preferred_large, qreal min_small,
                         qreal max_small) -> std::tuple<qreal, qreal, qreal>
{
    const qreal gaps = spacing * 2;
    qreal       best_large = 0;
    qreal       best_medium = 0;
    qreal       best_small  = 0;
    qreal       best_err    = std::numeric_limits<qreal>::max();

    for (qreal small = min_small; small <= max_small + 0.5; small += 1) {
        const qreal remain = available - small - gaps;
        if (remain <= 0) {
            continue;
        }
        qreal large  = remain * 0.5;
        qreal medium = remain - large;
        if (preferred_large > 0) {
            large  = clamp(preferred_large, small, remain - min_small);
            medium = remain - large;
        }
        if (medium < small || large < medium) {
            medium = clamp(remain * 0.4, small, remain - small);
            large  = remain - medium;
        }
        const qreal err = qAbs(large + medium + small + gaps - available);
        if (err < best_err) {
            best_err    = err;
            best_large  = large;
            best_medium = medium;
            best_small  = small;
        }
    }

    if (best_large <= 0) {
        const qreal remain = qMax(0.0, available - gaps);
        best_small         = clamp(remain * 0.2, min_small, max_small);
        best_medium        = (remain - best_small) * 0.45;
        best_large         = remain - best_small - best_medium;
    }
    return { best_large, best_medium, best_small };
}

auto buildMultiBrowse(const CarouselLayoutInput& in) -> KeylineList
{
    KeylineList list;
    const qreal available =
        in.viewport_size - in.content_padding_start - in.content_padding_end;
    const qreal min_small = in.small_item_min > 0 ? in.small_item_min : 40;
    const qreal max_small = in.small_item_max > 0 ? in.small_item_max : 56;

    auto [large, medium, small] =
        fitMultiBrowseSizes(available, in.spacing, in.item_extent, min_small, max_small);

    list.large_size  = large;
    list.medium_size = medium;
    list.small_size  = small;
    list.scroll_step = medium + in.spacing;

    qreal cursor = in.content_padding_start;
    auto  add    = [&](qreal size, int size_class) {
        list.keylines.append({ cursor + size * 0.5, size, size_class });
        cursor += size + in.spacing;
    };

    add(large, kSizeLarge);
    add(medium, kSizeMedium);
    add(small, kSizeSmall);
    while (cursor + min_small * 0.5 < in.viewport_size - in.content_padding_end) {
        add(small, kSizeSmall);
    }
    return list;
}

auto buildHero(const CarouselLayoutInput& in, bool center) -> KeylineList
{
    KeylineList list;
    const qreal available =
        in.viewport_size - in.content_padding_start - in.content_padding_end;
    const qreal min_small = in.small_item_min > 0 ? in.small_item_min : 40;
    const qreal max_small = in.small_item_max > 0 ? in.small_item_max : 56;

    qreal small = clamp(available * 0.18, min_small, max_small);
    qreal large = available - small - in.spacing;
    if (center) {
        large = clamp(available * 0.72, min_small * 2, available - small - in.spacing);
        small = clamp((available - large - in.spacing) * 0.5, min_small, max_small);
    }

    list.large_size  = large;
    list.small_size  = small;
    list.medium_size = (large + small) * 0.5;
    list.scroll_step = list.medium_size + in.spacing;

    if (center) {
        const qreal start = in.content_padding_start;
        const qreal large_center =
            in.content_padding_start + (available - large) * 0.5 + large * 0.5;
        list.keylines.append({ start + small * 0.5, small, kSizeSmall });
        list.keylines.append({ large_center, large, kSizeLarge });
        list.keylines.append(
            { in.viewport_size - in.content_padding_end - small * 0.5, small, kSizeSmall });
    } else {
        qreal cursor = in.content_padding_start;
        list.keylines.append({ cursor + large * 0.5, large, kSizeLarge });
        cursor += large + in.spacing;
        list.keylines.append({ cursor + small * 0.5, small, kSizeSmall });
        while (cursor + min_small < in.viewport_size - in.content_padding_end) {
            cursor += small + in.spacing;
            list.keylines.append({ cursor - in.spacing + small * 0.5, small, kSizeSmall });
        }
    }
    return list;
}

auto buildUncontained(const CarouselLayoutInput& in) -> KeylineList
{
    KeylineList list;
    const qreal size = in.item_extent;
    list.large_size  = size;
    list.medium_size = size;
    list.small_size  = size;
    list.scroll_step = size + in.spacing;
    list.keylines.append({ in.content_padding_start + size * 0.5, size, kSizeLarge });
    return list;
}

auto buildFullScreen(const CarouselLayoutInput& in) -> KeylineList
{
    KeylineList list;
    const qreal size = in.viewport_size > 0 ? in.viewport_size : in.item_extent;
    list.large_size  = size;
    list.medium_size = size;
    list.small_size  = size;
    list.scroll_step = size + in.spacing;
    list.keylines.append({ size * 0.5, size, kSizeLarge });
    return list;
}

void finalizeSnapAndScrollBounds(CarouselLayoutOutput& out, const CarouselLayoutInput& in, qreal stride)
{
    const qreal max_scroll = qMax(0.0, out.content_size - in.viewport_size);
    out.max_scroll_offset  = max_scroll;
    out.end_snap_offset    = max_scroll;
    out.scroll_step        = stride;

    out.snap_offsets.resize(in.count);
    for (int i = 0; i < in.count; ++i) {
        out.snap_offsets[i] = qMin(qreal(i) * stride, max_scroll);
    }

    if (in.count > 0 && in.scroll_offset >= max_scroll - stride * 0.25) {
        out.leading_index = in.count - 1;
        return;
    }

    int focal = 0;
    for (int i = 0; i < in.count; ++i) {
        if (out.snap_offsets[i] <= in.scroll_offset + stride * 0.5) {
            focal = i;
        }
    }
    out.leading_index = qBound(0, focal, in.count - 1);
}

auto layoutFixedStride(const CarouselLayoutInput& in, const KeylineList& kl) -> CarouselLayoutOutput
{
    CarouselLayoutOutput out;
    const qreal stride = kl.scroll_step;
    const qreal view_start = in.scroll_offset;
    const qreal view_end   = in.scroll_offset + in.viewport_size;
    const qreal size       = kl.large_size;

    for (int i = 0; i < in.count; ++i) {
        const qreal pos = in.content_padding_start + qreal(i) * stride;
        const qreal end = pos + size;
        if (end <= view_start || pos >= view_end) {
            continue;
        }

        CarouselItemGeometry g;
        g.index      = i;
        g.position   = pos;
        g.size       = size;
        g.size_class = kSizeLarge;
        applyMaskAndParallax(g, view_start, view_end, in.parallax_ratio, in.min_peek_px, true);
        if (g.mask_start + g.mask_end < 1.0) {
            out.items.append(g);
        }
    }

    out.content_size = in.count > 0
        ? in.content_padding_start + stride * in.count - in.spacing + in.content_padding_end
        : 0;

    finalizeSnapAndScrollBounds(out, in, stride);
    return out;
}

auto layoutKeylineItems(const CarouselLayoutInput& in, const KeylineList& kl) -> CarouselLayoutOutput
{
    CarouselLayoutOutput out;
    if (in.count <= 0 || in.viewport_size <= 0 || kl.keylines.isEmpty()) {
        return out;
    }

    const qreal view_start = in.scroll_offset;
    const qreal view_end   = in.scroll_offset + in.viewport_size;
    const qreal guess = kl.large_size;

    const auto strategy = CarouselScrollStrategy::build(kl, in.viewport_size);
    const qreal tentative_max =
        qMax(0.0, in.content_padding_start + in.count * kl.scroll_step - in.spacing
                 + in.content_padding_end - in.viewport_size);
    const KeylineList active_kl =
        strategy.keylinesForScrollOffset(in.scroll_offset, tentative_max);

    qreal pos       = in.content_padding_start;
    qreal prev_size = 0;
    qreal last_pos  = 0;
    qreal last_size = 0;

    for (int i = 0; i < in.count; ++i) {
        const qreal anchor = in.content_padding_start + qreal(i) * kl.scroll_step;
        const qreal size   = resolveSize(anchor, in.scroll_offset, active_kl.keylines, guess);
        if (i > 0) {
            pos = pos + prev_size + in.spacing;
        } else {
            pos = anchor;
        }
        prev_size = size;
        last_pos    = pos;
        last_size   = size;

        const qreal item_end = pos + size;
        if (item_end <= view_start || pos >= view_end) {
            continue;
        }

        CarouselItemGeometry g;
        g.index      = i;
        g.position   = pos;
        g.size       = size;
        g.size_class = sizeClassFor(size, kl.large_size, kl.medium_size, kl.small_size);
        applyMaskAndParallax(g, view_start, view_end, in.parallax_ratio, in.min_peek_px, true);
        if (g.mask_start + g.mask_end < 1.0) {
            out.items.append(g);
        }
    }

    out.content_size = in.count > 0 ? last_pos + last_size + in.content_padding_end : 0;

    finalizeSnapAndScrollBounds(out, in, kl.scroll_step);
    return out;
}

} // namespace

auto CarouselKeylines::buildList(const CarouselLayoutInput& input) -> KeylineList
{
    switch (input.layout) {
    case kLayoutMultiBrowse:
        return buildMultiBrowse(input);
    case kLayoutHero:
        return buildHero(input, false);
    case kLayoutHeroCenter:
        return buildHero(input, true);
    case kLayoutUncontained:
        return buildUncontained(input);
    case kLayoutFullScreen:
        return buildFullScreen(input);
    default:
        return buildUncontained(input);
    }
}

auto CarouselKeylines::layoutItems(const CarouselLayoutInput& in, const KeylineList& kl)
    -> CarouselLayoutOutput
{
    if (in.layout == kLayoutUncontained || in.layout == kLayoutFullScreen) {
        return layoutFixedStride(in, kl);
    }
    return layoutKeylineItems(in, kl);
}

} // namespace qml_material
