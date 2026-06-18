#include "qml_material/carousel/carousel_keylines.hpp"

#include "qml_material/carousel/carousel_scroll_strategy.hpp"

#include <QtMath>
#include <algorithm>
#include <limits>

namespace qml_material
{

namespace
{

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

void layoutItemChain(const CarouselLayoutInput& in, const QVector<Keyline>& keylines,
                     qreal scroll_offset, const KeylineList& kl, bool chain_anchor,
                     QVector<qreal>& item_starts, QVector<qreal>& item_sizes)
{
    const qreal guess = kl.large_size;
    qreal       pos   = in.content_padding_start;
    qreal       prev_size = 0;
    item_starts.resize(in.count);
    item_sizes.resize(in.count);

    for (int i = 0; i < in.count; ++i) {
        if (i > 0) {
            pos += prev_size + in.spacing;
        }
        const qreal grid_anchor = in.content_padding_start + qreal(i) * kl.scroll_step;
        const qreal size_anchor = chain_anchor ? pos : grid_anchor;
        const qreal size        = resolveSize(size_anchor, scroll_offset, keylines, guess);
        prev_size      = size;
        item_starts[i] = pos;
        item_sizes[i]  = size;
    }
}

void layoutHeroTrailingRest(const CarouselLayoutInput& in, const KeylineList& trailing_kl,
                            qreal scroll_offset, QVector<qreal>& item_starts,
                            QVector<qreal>& item_sizes)
{
    const qreal large = trailing_kl.large_size;
    const qreal small = trailing_kl.small_size;
    item_starts.resize(in.count);
    item_sizes.resize(in.count);

    if (in.count <= 0) {
        return;
    }

    const qreal last_start =
        scroll_offset + in.viewport_size - in.content_padding_end - large;
    item_starts[in.count - 1] = last_start;
    item_sizes[in.count - 1]  = large;

    if (in.count >= 2) {
        item_sizes[in.count - 2]  = small;
        item_starts[in.count - 2] = last_start - in.spacing - small;
    }

    qreal pos = in.count >= 2 ? item_starts[in.count - 2] : last_start;
    for (int i = in.count - 3; i >= 0; --i) {
        pos -= in.spacing + small;
        item_starts[i] = pos;
        item_sizes[i]  = small;
    }
}

void layoutHeroLeadingTransition(const CarouselLayoutInput& in, const KeylineList& leading_kl,
                                 const KeylineList& middle_kl, qreal first_scroll, qreal second_scroll,
                                 qreal scroll_offset, QVector<qreal>& item_starts,
                                 QVector<qreal>& item_sizes)
{
    QVector<qreal> start_starts;
    QVector<qreal> start_sizes;
    QVector<qreal> end_starts;
    QVector<qreal> end_sizes;

    layoutItemChain(in, leading_kl.keylines, first_scroll, leading_kl, true, start_starts,
                    start_sizes);
    layoutItemChain(in, middle_kl.keylines, second_scroll, middle_kl, true, end_starts, end_sizes);

    const qreal span = qMax(1.0, second_scroll - first_scroll);
    const qreal t    = qBound(0.0, (scroll_offset - first_scroll) / span, 1.0);

    item_starts.resize(in.count);
    item_sizes.resize(in.count);
    for (int i = 0; i < in.count; ++i) {
        item_starts[i] = lerp(start_starts[i], end_starts[i], t);
        item_sizes[i]  = lerp(start_sizes[i], end_sizes[i], t);
    }
}

void layoutHeroTrailingTransition(const CarouselLayoutInput& in, const KeylineList& middle_kl,
                                  const KeylineList& trailing_kl, qreal penult_scroll,
                                  qreal last_scroll, qreal scroll_offset,
                                  QVector<qreal>& item_starts, QVector<qreal>& item_sizes)
{
    QVector<qreal> penult_starts;
    QVector<qreal> penult_sizes;
    QVector<qreal> rest_starts;
    QVector<qreal> rest_sizes;

    layoutItemChain(in, middle_kl.keylines, penult_scroll, middle_kl, true, penult_starts,
                    penult_sizes);
    layoutHeroTrailingRest(in, trailing_kl, last_scroll, rest_starts, rest_sizes);

    const qreal span = qMax(1.0, last_scroll - penult_scroll);
    const qreal t      = qBound(0.0, (scroll_offset - penult_scroll) / span, 1.0);

    item_starts.resize(in.count);
    item_sizes.resize(in.count);
    for (int i = 0; i < in.count; ++i) {
        item_starts[i] = lerp(penult_starts[i], rest_starts[i], t);
        item_sizes[i]  = lerp(penult_sizes[i], rest_sizes[i], t);
    }
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

void finalizeKeylineSnapAndScrollBounds(CarouselLayoutOutput& out, const CarouselLayoutInput& in,
                                        const QVector<qreal>& item_starts,
                                        const QVector<qreal>& item_sizes,
                                        qreal stride_override = 0)
{
    const qreal layout_max = qMax(0.0, out.content_size - in.viewport_size);
    const qreal scroll_max = layout_max;

    const qreal last_size = item_sizes.isEmpty() ? 0 : item_sizes.last();
    out.scroll_step        = last_size + in.spacing;
    const qreal stride     = stride_override > 0 ? stride_override : out.scroll_step;

    out.snap_offsets.resize(in.count);
    out.max_scroll_offset = scroll_max;
    out.end_snap_offset   = scroll_max;
    for (int i = 0; i < in.count; ++i) {
        const qreal snap    = i < item_starts.size() ? item_starts[i] - in.content_padding_start : 0;
        out.snap_offsets[i] = qMin(snap, scroll_max);
    }

    if (in.count > 0 && in.scroll_offset >= scroll_max - stride * 0.25) {
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

auto computeStartAlignedEndScroll(const CarouselLayoutInput& in) -> qreal
{
    const KeylineList middle_kl = CarouselHeroKeylines::phaseKeylines(
        in, CarouselHeroKeylines::HeroPhase::Middle, false);
    if (middle_kl.keylines.isEmpty() || in.viewport_size <= 0 || in.count <= 0) {
        return 0;
    }

    const qreal max_scroll = qMax(
        0.0, in.content_padding_start + in.count * middle_kl.scroll_step - in.spacing
                 + in.content_padding_end - in.viewport_size);

    const auto        strategy  = CarouselScrollStrategy::build(middle_kl, in.viewport_size);
    const KeylineList active_kl = strategy.keylinesForScrollOffset(max_scroll, max_scroll);

    QVector<qreal> starts;
    QVector<qreal> sizes;
    layoutItemChain(in, active_kl.keylines, max_scroll, active_kl, false, starts, sizes);
    if (sizes.isEmpty()) {
        return max_scroll;
    }

    const qreal content_size = starts.last() + sizes.last() + in.content_padding_end;
    return qMax(0.0, content_size - in.viewport_size);
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

auto layoutFullScreenItems(const CarouselLayoutInput& in) -> CarouselLayoutOutput
{
    CarouselLayoutOutput out;
    if (in.count <= 0 || in.viewport_size <= 0) {
        return out;
    }

    const qreal page_size  = in.viewport_size;
    const qreal stride     = page_size + in.spacing;
    const qreal view_start = in.scroll_offset;
    const qreal view_end   = in.scroll_offset + in.viewport_size;

    for (int i = 0; i < in.count; ++i) {
        const qreal pos = in.content_padding_start + qreal(i) * stride;
        const qreal end = pos + page_size;
        if (end <= view_start || pos >= view_end) {
            continue;
        }

        CarouselItemGeometry g;
        g.index      = i;
        g.position   = pos;
        g.size       = page_size;
        g.size_class = kSizeLarge;
        g.mask_start = 0;
        g.mask_end   = 0;
        out.items.append(g);
    }

    out.content_size = in.count > 0
        ? in.content_padding_start + stride * (in.count - 1) + page_size + in.content_padding_end
        : 0;

    finalizeSnapAndScrollBounds(out, in, stride);
    return out;
}

auto itemWidthForAspect(const CarouselLayoutInput& in, int index, qreal cross) -> qreal
{
    qreal aspect = 1.0;
    if (index >= 0 && index < in.item_aspects.size()) {
        aspect = in.item_aspects[index];
    } else if (in.item_extent > 0 && cross > 0) {
        aspect = in.item_extent / cross;
    }
    aspect = clamp(aspect, in.min_item_aspect, in.max_item_aspect);
    return cross * aspect;
}

auto layoutVariableStride(const CarouselLayoutInput& in, const KeylineList& kl) -> CarouselLayoutOutput
{
    Q_UNUSED(kl);
    CarouselLayoutOutput out;
    const qreal cross = in.cross_size > 0
        ? in.cross_size - 2.0 * in.content_padding_cross
        : in.item_extent;
    const qreal view_start = in.scroll_offset;
    const qreal view_end   = in.scroll_offset + in.viewport_size;

    QVector<qreal> widths;
    QVector<qreal> positions;
    widths.resize(in.count);
    positions.resize(in.count);

    qreal cursor     = in.content_padding_start;
    qreal last_width = 0;
    for (int i = 0; i < in.count; ++i) {
        const qreal w = itemWidthForAspect(in, i, cross);
        widths[i]     = w;
        positions[i]  = cursor;
        cursor += w + in.spacing;
        last_width = w;
    }

    for (int i = 0; i < in.count; ++i) {
        const qreal pos = positions[i];
        const qreal size = widths[i];
        const qreal end  = pos + size;
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

    out.content_size = in.count > 0 ? positions[in.count - 1] + last_width + in.content_padding_end : 0;

    const qreal max_scroll = qMax(0.0, out.content_size - in.viewport_size);
    out.max_scroll_offset  = max_scroll;
    out.end_snap_offset    = max_scroll;
    out.scroll_step        = last_width + in.spacing;

    out.snap_offsets.resize(in.count);
    for (int i = 0; i < in.count; ++i) {
        const qreal snap = positions[i] - in.content_padding_start;
        out.snap_offsets[i] = qMin(snap, max_scroll);
    }

    if (in.count > 0 && in.scroll_offset >= max_scroll - out.scroll_step * 0.25) {
        out.leading_index = in.count - 1;
    } else {
        int focal = 0;
        for (int i = 0; i < in.count; ++i) {
            if (out.snap_offsets[i] <= in.scroll_offset + out.scroll_step * 0.5) {
                focal = i;
            }
        }
        out.leading_index = qBound(0, focal, in.count - 1);
    }

    return out;
}

auto layoutKeylineUniform(const CarouselLayoutInput& in, const KeylineList& kl) -> CarouselLayoutOutput
{
    CarouselLayoutOutput out;
    const qreal size   = kl.medium_size > 0 ? kl.medium_size : kl.large_size;
    const qreal stride = size + in.spacing;
    const qreal view_start = in.scroll_offset;
    const qreal view_end   = in.scroll_offset + in.viewport_size;

    QVector<qreal> item_starts;
    QVector<qreal> item_sizes;
    item_starts.resize(in.count);
    item_sizes.resize(in.count);

    for (int i = 0; i < in.count; ++i) {
        const qreal pos = in.content_padding_start + qreal(i) * stride;
        item_starts[i]  = pos;
        item_sizes[i]   = size;

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

    finalizeKeylineSnapAndScrollBounds(out, in, item_starts, item_sizes);
    return out;
}

auto layoutKeylineItems(const CarouselLayoutInput& in, const KeylineList& kl) -> CarouselLayoutOutput
{
    CarouselLayoutOutput out;
    if (in.count <= 0 || in.viewport_size <= 0 || kl.keylines.isEmpty()) {
        return out;
    }

    if (in.reduce_motion && in.layout == CarouselLayoutId::MultiBrowse) {
        return layoutKeylineUniform(in, kl);
    }

    const qreal view_start = in.scroll_offset;
    const qreal view_end   = in.scroll_offset + in.viewport_size;

    const auto strategy = CarouselScrollStrategy::build(in, kl);
    const qreal max_scroll =
        qMax(0.0, in.content_padding_start + in.count * kl.scroll_step - in.spacing
                 + in.content_padding_end - in.viewport_size);

    QVector<qreal> item_starts;
    QVector<qreal> item_sizes;
    item_starts.resize(in.count);
    item_sizes.resize(in.count);

    const KeylineList active_kl =
        strategy.keylinesForScrollOffset(in.scroll_offset, max_scroll);
    layoutItemChain(in, active_kl.keylines, in.scroll_offset, active_kl, false, item_starts,
                    item_sizes);

    qreal last_pos  = 0;
    qreal last_size = 0;
    for (int i = 0; i < in.count; ++i) {
        const qreal pos  = item_starts[i];
        const qreal size = item_sizes[i];
        last_pos         = pos;
        last_size        = size;

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
    finalizeKeylineSnapAndScrollBounds(out, in, item_starts, item_sizes);
    return out;
}

auto layoutHeroItems(const CarouselLayoutInput& in) -> CarouselLayoutOutput
{
    CarouselLayoutOutput out;
    if (in.count <= 0 || in.viewport_size <= 0) {
        return out;
    }

    const bool keep_peek = CarouselHeroKeylines::keepLeadingPeek(in);
    const KeylineList metrics_kl =
        CarouselHeroKeylines::phaseKeylines(in, CarouselHeroKeylines::HeroPhase::Middle, keep_peek);
    if (metrics_kl.keylines.isEmpty()) {
        return out;
    }

    const qreal view_start = in.scroll_offset;
    const qreal view_end   = in.scroll_offset + in.viewport_size;

    QVector<qreal> item_starts;
    QVector<qreal> item_sizes;

    if (!keep_peek) {
        const auto strategy = CarouselScrollStrategy::build(metrics_kl, in.viewport_size);
        qreal max_scroll = qMax(
            0.0, in.content_padding_start + in.count * metrics_kl.scroll_step - in.spacing
                     + in.content_padding_end - in.viewport_size);

        const KeylineList active_kl =
            strategy.keylinesForScrollOffset(in.scroll_offset, max_scroll);
        layoutItemChain(in, active_kl.keylines, in.scroll_offset, active_kl, false, item_starts,
                        item_sizes);

        qreal last_pos  = 0;
        qreal last_size = 0;
        for (int i = 0; i < in.count; ++i) {
            const qreal pos  = item_starts[i];
            const qreal size = item_sizes[i];
            last_pos         = pos;
            last_size        = size;

            const qreal item_end = pos + size;
            if (item_end <= view_start || pos >= view_end) {
                continue;
            }

            CarouselItemGeometry g;
            g.index      = i;
            g.position   = pos;
            g.size       = size;
            g.size_class = sizeClassFor(size, metrics_kl.large_size, metrics_kl.medium_size,
                                        metrics_kl.small_size);
            applyMaskAndParallax(g, view_start, view_end, in.parallax_ratio, in.min_peek_px, true);
            if (g.mask_start + g.mask_end < 1.0) {
                out.items.append(g);
            }
        }

        out.content_size = in.count > 0 ? last_pos + last_size + in.content_padding_end : 0;
        finalizeKeylineSnapAndScrollBounds(out, in, item_starts, item_sizes);

        const qreal scroll_max = qMax(0.0, out.content_size - in.viewport_size);
        QVector<qreal> end_starts;
        QVector<qreal> end_sizes;
        const KeylineList trailing_kl = CarouselHeroKeylines::phaseKeylines(
            in, CarouselHeroKeylines::HeroPhase::Trailing, false);
        layoutItemChain(in, trailing_kl.keylines, out.end_snap_offset, trailing_kl, false,
                        end_starts, end_sizes);
        if (!end_sizes.isEmpty()) {
            const qreal end_content_size =
                end_starts.last() + end_sizes.last() + in.content_padding_end;
            out.content_size = qMax(out.content_size, end_content_size);
        }
        out.max_scroll_offset = qMax(qMax(out.max_scroll_offset, scroll_max), out.end_snap_offset);
        return out;
    }

    CarouselHeroKeylines::computeHeroSnapOffsets(out, in, keep_peek);

    const qreal keyline_scroll_max = out.snap_offsets.isEmpty()
        ? out.max_scroll_offset
        : out.snap_offsets.last();

    const auto strategy = CarouselScrollStrategy::buildHero(
        in, keep_peek, out.snap_offsets, keyline_scroll_max);

    const HeroScrollZone zone = strategy.zoneFor(in.scroll_offset, keyline_scroll_max);

    KeylineList active_kl;
    const KeylineList leading_kl = CarouselHeroKeylines::phaseKeylines(
        in, CarouselHeroKeylines::HeroPhase::Leading, true);
    const KeylineList trailing_kl = CarouselHeroKeylines::phaseKeylines(
        in, CarouselHeroKeylines::HeroPhase::Trailing, false);

    if (zone == HeroScrollZone::TrailingRest) {
        layoutHeroTrailingRest(in, trailing_kl, in.scroll_offset, item_starts, item_sizes);
    } else if (zone == HeroScrollZone::TrailingLerp && in.count >= 2
               && out.snap_offsets.size() >= in.count) {
        const qreal penult_scroll = out.snap_offsets.at(in.count - 2);
        const qreal last_scroll   = out.snap_offsets.last();
        layoutHeroTrailingTransition(in, metrics_kl, trailing_kl, penult_scroll, last_scroll,
                                     in.scroll_offset, item_starts, item_sizes);
    } else if (zone == HeroScrollZone::Leading && out.snap_offsets.size() >= 2) {
        const qreal second_scroll = out.snap_offsets.at(1);
        layoutHeroLeadingTransition(in, leading_kl, metrics_kl, out.snap_offsets.first(),
                                    second_scroll, in.scroll_offset, item_starts, item_sizes);
    } else {
        const bool chain_anchor = zone != HeroScrollZone::Leading;
        active_kl               = strategy.keylinesForScrollOffset(in.scroll_offset, keyline_scroll_max);
        layoutItemChain(in, active_kl.keylines, in.scroll_offset, active_kl, chain_anchor,
                        item_starts, item_sizes);
    }

    int focal_index = 0;
    const bool restrict_focal_window =
        zone == HeroScrollZone::Middle || zone == HeroScrollZone::TrailingLerp;
    if (restrict_focal_window && !item_starts.isEmpty()) {
        qreal best_focal_dist = std::numeric_limits<qreal>::max();
        const qreal view_center = in.viewport_size * 0.5;
        for (int i = 0; i < in.count; ++i) {
            const qreal cx = item_starts[i] + item_sizes[i] * 0.5 - view_start;
            const qreal d  = qAbs(cx - view_center);
            if (d < best_focal_dist) {
                best_focal_dist = d;
                focal_index     = i;
            }
        }
    }

    qreal last_pos  = 0;
    qreal last_size = 0;
    for (int i = 0; i < in.count; ++i) {
        const qreal pos  = item_starts[i];
        const qreal size = item_sizes[i];
        last_pos         = pos;
        last_size        = size;

        if (restrict_focal_window && (i < focal_index - 1 || i > focal_index + 1)) {
            continue;
        }

        const qreal item_end = pos + size;
        if (item_end <= view_start || pos >= view_end) {
            continue;
        }

        CarouselItemGeometry g;
        g.index      = i;
        g.position   = pos;
        g.size       = size;
        g.size_class =
            sizeClassFor(size, metrics_kl.large_size, metrics_kl.medium_size, metrics_kl.small_size);
        applyMaskAndParallax(g, view_start, view_end, in.parallax_ratio, in.min_peek_px, true);
        if (g.mask_start + g.mask_end < 1.0) {
            out.items.append(g);
        }
    }

    out.content_size = in.count > 0 ? last_pos + last_size + in.content_padding_end : 0;

    const qreal layout_max = qMax(0.0, out.content_size - in.viewport_size);

    QVector<qreal> end_starts;
    QVector<qreal> end_sizes;
    layoutHeroTrailingRest(in, trailing_kl, out.end_snap_offset, end_starts, end_sizes);
    if (!end_sizes.isEmpty()) {
        const qreal end_content_size =
            end_starts.last() + end_sizes.last() + in.content_padding_end;
        out.content_size = qMax(out.content_size, end_content_size);
    }

    const qreal scroll_max = qMax(0.0, out.content_size - in.viewport_size);
    out.max_scroll_offset  = qMax(qMax(out.max_scroll_offset, scroll_max), out.end_snap_offset);
    out.end_snap_offset    = out.snap_offsets.isEmpty() ? scroll_max : out.snap_offsets.last();
    out.scroll_step        = metrics_kl.scroll_step;

    int   best_index = 0;
    qreal best_dist  = std::numeric_limits<qreal>::max();
    for (int i = 0; i < out.snap_offsets.size(); ++i) {
        const qreal d = qAbs(out.snap_offsets.at(i) - in.scroll_offset);
        if (d < best_dist) {
            best_dist  = d;
            best_index = i;
        }
    }
    out.leading_index = best_index;
    return out;
}

} // namespace

namespace CarouselHeroKeylines
{

namespace
{

constexpr int kSizeSmall = 0;
constexpr int kSizeLarge = 2;

auto clamp(qreal v, qreal lo, qreal hi) -> qreal
{
    return qBound(lo, v, hi);
}

auto buildStartAlignedKeylines(const CarouselLayoutInput& in, const HeroMetrics& metrics) -> KeylineList
{
    KeylineList list;
    const qreal min_small = in.small_item_min > 0 ? in.small_item_min : 40;

    list.large_size  = metrics.large_leading;
    list.small_size  = metrics.small_leading;
    list.medium_size = metrics.medium_leading;
    list.scroll_step = metrics.scroll_step_leading;

    qreal cursor = in.content_padding_start;
    list.keylines.append({ cursor + metrics.large_leading * 0.5, metrics.large_leading, kSizeLarge });
    cursor += metrics.large_leading + in.spacing;
    list.keylines.append({ cursor + metrics.small_leading * 0.5, metrics.small_leading, kSizeSmall });
    while (cursor + min_small < in.viewport_size - in.content_padding_end) {
        cursor += metrics.small_leading + in.spacing;
        list.keylines.append(
            { cursor - in.spacing + metrics.small_leading * 0.5, metrics.small_leading, kSizeSmall });
    }
    return list;
}

auto buildCenterMiddleKeylines(const CarouselLayoutInput& in, const HeroMetrics& metrics) -> KeylineList
{
    KeylineList list;
    const qreal available =
        in.viewport_size - in.content_padding_start - in.content_padding_end;

    list.large_size  = metrics.large_center;
    list.small_size  = metrics.small_center;
    list.medium_size = metrics.medium_center;
    list.scroll_step = metrics.scroll_step_center;

    const qreal start        = in.content_padding_start;
    const qreal large_center = in.content_padding_start + (available - metrics.large_center) * 0.5
        + metrics.large_center * 0.5;
    list.keylines.append({ start + metrics.small_center * 0.5, metrics.small_center, kSizeSmall });
    list.keylines.append({ large_center, metrics.large_center, kSizeLarge });
    list.keylines.append({ in.viewport_size - in.content_padding_end - metrics.small_center * 0.5,
                           metrics.small_center,
                           kSizeSmall });
    return list;
}

auto buildStartTrailingKeylines(const CarouselLayoutInput& in, const HeroMetrics& metrics) -> KeylineList
{
    KeylineList list;
    list.large_size  = metrics.large_leading;
    list.small_size  = metrics.small_leading;
    list.medium_size = metrics.medium_leading;
    list.scroll_step = metrics.scroll_step_leading;

    const qreal large_center =
        in.viewport_size - in.content_padding_end - metrics.large_leading * 0.5;
    list.keylines.append({ large_center, metrics.large_leading, kSizeLarge });
    return list;
}

} // namespace

auto keepLeadingPeek(const CarouselLayoutInput& in) -> bool
{
    return in.layout == CarouselLayoutId::HeroCenter;
}

auto computeMetrics(const CarouselLayoutInput& in) -> HeroMetrics
{
    HeroMetrics metrics;
    const qreal available =
        in.viewport_size - in.content_padding_start - in.content_padding_end;
    const qreal min_small = in.small_item_min > 0 ? in.small_item_min : 40;
    const qreal max_small = in.small_item_max > 0 ? in.small_item_max : 56;

    metrics.small_leading = clamp(available * 0.18, min_small, max_small);
    metrics.large_leading = available - metrics.small_leading - in.spacing;

    metrics.large_center =
        clamp(available * 0.72, min_small * 2, available - metrics.small_leading - in.spacing);
    metrics.small_center = clamp((available - metrics.large_center - in.spacing) * 0.5, min_small,
                                 max_small);

    metrics.medium_leading = (metrics.large_leading + metrics.small_leading) * 0.5;
    metrics.medium_center  = (metrics.large_center + metrics.small_center) * 0.5;
    metrics.scroll_step_leading = metrics.medium_leading + in.spacing;
    metrics.scroll_step_center  = metrics.medium_center + in.spacing;
    return metrics;
}

auto phaseKeylines(const CarouselLayoutInput& in, HeroPhase phase, bool keep_leading_peek) -> KeylineList
{
    const HeroMetrics metrics = computeMetrics(in);
    switch (phase) {
    case HeroPhase::Leading:
        return buildStartAlignedKeylines(in, metrics);
    case HeroPhase::Middle:
        if (keep_leading_peek) {
            return buildCenterMiddleKeylines(in, metrics);
        }
        return buildStartAlignedKeylines(in, metrics);
    case HeroPhase::Trailing:
        return buildStartTrailingKeylines(in, metrics);
    }
    return {};
}

void computeHeroSnapOffsets(CarouselLayoutOutput& out, const CarouselLayoutInput& in,
                            bool keep_leading_peek)
{
    if (in.count <= 0 || in.viewport_size <= 0 || !keep_leading_peek) {
        return;
    }

    struct SnapCache
    {
        CarouselLayoutInput key;
        QVector<qreal>      snap_offsets;
        qreal               end_snap_offset   = 0;
        qreal               max_scroll_offset = 0;
        bool                valid             = false;
    };
    static SnapCache cache;

    auto cacheKey = in;
    cacheKey.scroll_offset = 0;

    if (cache.valid && cache.key.layout == cacheKey.layout && cache.key.count == cacheKey.count
        && qFuzzyCompare(cache.key.viewport_size, cacheKey.viewport_size)
        && qFuzzyCompare(cache.key.cross_size, cacheKey.cross_size)
        && qFuzzyCompare(cache.key.item_extent, cacheKey.item_extent)
        && qFuzzyCompare(cache.key.spacing, cacheKey.spacing)
        && qFuzzyCompare(cache.key.content_padding_start, cacheKey.content_padding_start)
        && qFuzzyCompare(cache.key.content_padding_end, cacheKey.content_padding_end)
        && qFuzzyCompare(cache.key.small_item_min, cacheKey.small_item_min)
        && qFuzzyCompare(cache.key.small_item_max, cacheKey.small_item_max)
        && cache.key.item_aspects == cacheKey.item_aspects) {
        out.snap_offsets      = cache.snap_offsets;
        out.end_snap_offset   = cache.end_snap_offset;
        out.max_scroll_offset = cache.max_scroll_offset;
        return;
    }

    const KeylineList middle_kl = phaseKeylines(in, HeroPhase::Middle, true);
    qreal             max_scroll = qMax(
        0.0, in.content_padding_start + in.count * middle_kl.scroll_step - in.spacing
                 + in.content_padding_end - in.viewport_size);

    QVector<qreal> starts;
    QVector<qreal> sizes;
    out.snap_offsets.resize(in.count);
    out.snap_offsets[0] = 0;

    const qreal focal_center = in.viewport_size * 0.5;
    const KeylineList center_kl = middle_kl;

    for (int i = 1; i < in.count - 1; ++i) {
        qreal lo     = out.snap_offsets[i - 1];
        qreal hi     = max_scroll;
        qreal scroll = (lo + hi) * 0.5;
        for (int iter = 0; iter < 16; ++iter) {
            scroll = (lo + hi) * 0.5;
            layoutItemChain(in, center_kl.keylines, scroll, center_kl, true, starts, sizes);
            const qreal focal_x = starts[i] + sizes[i] * 0.5 - scroll;
            if (focal_x < focal_center - 0.5) {
                hi = scroll;
            } else if (focal_x > focal_center + 0.5) {
                lo = scroll;
            } else {
                break;
            }
        }
        out.snap_offsets[i] = qMin(max_scroll, qMax(out.snap_offsets[i - 1] + 0.5, scroll));
    }

    if (in.count >= 2) {
        out.snap_offsets[in.count - 1] =
            qMax(out.snap_offsets[in.count - 2] + 0.5,
                 out.snap_offsets[in.count - 2] + middle_kl.scroll_step);
        max_scroll = qMax(max_scroll, out.snap_offsets[in.count - 1]);
    }

    out.end_snap_offset   = out.snap_offsets.last();
    out.max_scroll_offset = max_scroll;

    cache.key               = cacheKey;
    cache.snap_offsets      = out.snap_offsets;
    cache.end_snap_offset   = out.end_snap_offset;
    cache.max_scroll_offset = out.max_scroll_offset;
    cache.valid             = true;
}

} // namespace CarouselHeroKeylines

auto CarouselKeylines::buildList(const CarouselLayoutInput& input) -> KeylineList
{
    switch (input.layout) {
    case CarouselLayoutId::MultiBrowse:
        return buildMultiBrowse(input);
    case CarouselLayoutId::Hero:
    case CarouselLayoutId::HeroCenter:
        return CarouselHeroKeylines::phaseKeylines(
            input, CarouselHeroKeylines::HeroPhase::Middle,
            CarouselHeroKeylines::keepLeadingPeek(input));
    case CarouselLayoutId::Uncontained:
        return buildUncontained(input);
    case CarouselLayoutId::UncontainedMultiAspect:
        return buildUncontained(input);
    case CarouselLayoutId::FullScreen:
        return buildFullScreen(input);
    default:
        return buildUncontained(input);
    }
}

auto CarouselKeylines::layoutItems(const CarouselLayoutInput& in, const KeylineList& kl)
    -> CarouselLayoutOutput
{
    if (in.layout == CarouselLayoutId::UncontainedMultiAspect) {
        return layoutVariableStride(in, kl);
    }
    if (in.layout == CarouselLayoutId::FullScreen) {
        return layoutFullScreenItems(in);
    }
    if (in.layout == CarouselLayoutId::Uncontained) {
        return layoutFixedStride(in, kl);
    }
    if (in.layout == CarouselLayoutId::Hero || in.layout == CarouselLayoutId::HeroCenter) {
        return layoutHeroItems(in);
    }
    return layoutKeylineItems(in, kl);
}

} // namespace qml_material
