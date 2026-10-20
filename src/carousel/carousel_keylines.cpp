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

auto lerp(qreal a, qreal b, qreal t) -> qreal { return a + (b - a) * t; }

auto clamp(qreal v, qreal lo, qreal hi) -> qreal { return qBound(lo, v, hi); }

auto sizeClassFor(qreal size, qreal large, qreal medium, qreal small) -> int {
    if (size >= large - 1) {
        return kSizeLarge;
    }
    if (size >= medium - 1) {
        return kSizeMedium;
    }
    return kSizeSmall;
}

void applyMaskAndParallax(CarouselItemGeometry& g, qreal view_start, qreal view_end,
                          qreal parallax_ratio, qreal min_peek_px, bool hide_narrow_peek) {
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
                         qreal max_small) -> std::tuple<qreal, qreal, qreal> {
    const qreal gaps        = spacing * 2;
    qreal       best_large  = 0;
    qreal       best_medium = 0;
    qreal       best_small  = 0;
    qreal       best_err    = std::numeric_limits<qreal>::max();

    for (qreal small = min_small; small <= max_small + 0.5; small += 1) {
        const qreal remain = available - small - gaps;
        if (remain < 2 * small) {
            continue;
        }
        qreal large  = remain * 0.5;
        qreal medium = remain - large;
        if (preferred_large > 0) {
            large  = std::max(small, (available - gaps) * 2 / 3 - small);
            medium = (large + small) / 2;
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

auto buildMultiBrowse(const CarouselLayoutInput& in) -> KeylineList {
    KeylineList list;
    const qreal available = in.viewport_size - in.content_padding_start - in.content_padding_end;
    const qreal min_small = in.small_item_min > 0 ? in.small_item_min : 40;
    const qreal max_small = qMax(min_small, in.small_item_max > 0 ? in.small_item_max : 56);

    // Initial layouts can be narrower than three minimum-sized items and their gaps.
    if (available < 3 * min_small + 2 * in.spacing) {
        const qreal size = qMax(1.0, available);
        list.large_size = list.medium_size = list.small_size = size;
        list.scroll_step                                     = size + in.spacing;
        list.keylines.append({ in.content_padding_start + size * 0.5, size, kSizeLarge });
        return list;
    }

    auto [large, medium, small] =
        fitMultiBrowseSizes(available, in.spacing, in.item_extent, min_small, max_small);

    list.large_size  = large;
    list.medium_size = medium;
    list.small_size  = small;
    list.scroll_step = large + in.spacing;

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

auto buildUncontained(const CarouselLayoutInput& in) -> KeylineList {
    KeylineList list;
    const qreal size = in.item_extent;
    list.large_size  = size;
    list.medium_size = size;
    list.small_size  = size;
    list.scroll_step = size + in.spacing;
    list.keylines.append({ in.content_padding_start + size * 0.5, size, kSizeLarge });
    return list;
}

auto buildFullScreen(const CarouselLayoutInput& in) -> KeylineList {
    KeylineList list;
    const qreal size = in.viewport_size > 0 ? in.viewport_size : in.item_extent;
    list.large_size  = size;
    list.medium_size = size;
    list.small_size  = size;
    list.scroll_step = size + in.spacing;
    list.keylines.append({ size * 0.5, size, kSizeLarge });
    return list;
}

void finalizeSnapAndScrollBounds(CarouselLayoutOutput& out, const CarouselLayoutInput& in,
                                 qreal stride) {
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
                                        qreal                 stride_override = 0) {
    const qreal layout_max = qMax(0.0, out.content_size - in.viewport_size);
    const qreal scroll_max = layout_max;

    const qreal last_size = item_sizes.isEmpty() ? 0 : item_sizes.last();
    out.scroll_step       = last_size + in.spacing;
    const qreal stride    = stride_override > 0 ? stride_override : out.scroll_step;

    out.snap_offsets.resize(in.count);
    out.max_scroll_offset = scroll_max;
    out.end_snap_offset   = scroll_max;
    for (int i = 0; i < in.count; ++i) {
        const qreal snap = i < item_starts.size() ? item_starts[i] - in.content_padding_start : 0;
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

auto layoutFixedStride(const CarouselLayoutInput& in, const KeylineList& kl)
    -> CarouselLayoutOutput {
    CarouselLayoutOutput out;
    const qreal          stride     = kl.scroll_step;
    const qreal          view_start = in.scroll_offset;
    const qreal          view_end   = in.scroll_offset + in.viewport_size;
    const qreal          size       = kl.large_size;

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

    out.content_size = in.count > 0 ? in.content_padding_start + stride * in.count - in.spacing +
                                          in.content_padding_end
                                    : 0;

    finalizeSnapAndScrollBounds(out, in, stride);
    return out;
}

auto layoutFullScreenItems(const CarouselLayoutInput& in) -> CarouselLayoutOutput {
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

    out.content_size = in.count > 0 ? in.content_padding_start + stride * (in.count - 1) +
                                          page_size + in.content_padding_end
                                    : 0;

    finalizeSnapAndScrollBounds(out, in, stride);
    return out;
}

auto itemWidthForAspect(const CarouselLayoutInput& in, int index, qreal cross) -> qreal {
    qreal aspect = 1.0;
    if (index >= 0 && index < in.item_aspects.size()) {
        aspect = in.item_aspects[index];
    } else if (in.item_extent > 0 && cross > 0) {
        aspect = in.item_extent / cross;
    }
    aspect = clamp(aspect, in.min_item_aspect, in.max_item_aspect);
    return cross * aspect;
}

auto layoutVariableStride(const CarouselLayoutInput& in, const KeylineList& kl)
    -> CarouselLayoutOutput {
    Q_UNUSED(kl);
    CarouselLayoutOutput out;
    const qreal          cross =
        in.cross_size > 0 ? in.cross_size - 2.0 * in.content_padding_cross : in.item_extent;
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
        const qreal pos  = positions[i];
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

    out.content_size =
        in.count > 0 ? positions[in.count - 1] + last_width + in.content_padding_end : 0;

    const qreal max_scroll = qMax(0.0, out.content_size - in.viewport_size);
    out.max_scroll_offset  = max_scroll;
    out.end_snap_offset    = max_scroll;
    out.scroll_step        = last_width + in.spacing;

    out.snap_offsets.resize(in.count);
    for (int i = 0; i < in.count; ++i) {
        const qreal snap    = positions[i] - in.content_padding_start;
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

auto layoutKeylineUniform(const CarouselLayoutInput& in, const KeylineList& kl)
    -> CarouselLayoutOutput {
    CarouselLayoutOutput out;
    const qreal          size       = kl.medium_size > 0 ? kl.medium_size : kl.large_size;
    const qreal          stride     = size + in.spacing;
    const qreal          view_start = in.scroll_offset;
    const qreal          view_end   = in.scroll_offset + in.viewport_size;

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

    out.content_size = in.count > 0 ? in.content_padding_start + stride * in.count - in.spacing +
                                          in.content_padding_end
                                    : 0;

    finalizeKeylineSnapAndScrollBounds(out, in, item_starts, item_sizes);
    return out;
}

auto layoutKeylineItems(const CarouselLayoutInput& in, const KeylineList& kl)
    -> CarouselLayoutOutput {
    CarouselLayoutOutput out;
    if (in.count <= 0 || in.viewport_size <= 0 || kl.keylines.isEmpty()) return out;
    if (in.reduce_motion && in.layout == CarouselLayoutId::MultiBrowse)
        return layoutKeylineUniform(in, kl);

    const auto strategy   = CarouselScrollStrategy::build(in, kl);
    out.snap_offsets      = strategy.snapOffsets(in);
    out.max_scroll_offset = out.end_snap_offset = out.snap_offsets.last();
    out.scroll_step                             = strategy.default_keylines.scroll_step;
    out.content_size                            = in.viewport_size + out.max_scroll_offset;
    const auto  active = strategy.keylinesForScrollOffset(in.scroll_offset, out.max_scroll_offset);
    const auto& keys   = active.keylines;
    const auto  viewStart = in.scroll_offset, viewEnd = viewStart + in.viewport_size;
    const auto  initialCenter = strategy.initialCenter(in);
    qreal       nearest       = std::numeric_limits<qreal>::max();
    for (int i = 0; i < in.count; ++i) {
        const auto logical = initialCenter + i * out.scroll_step - viewStart;
        Keyline    mapped;
        if (logical <= keys.first().unadjusted_offset || logical >= keys.last().unadjusted_offset) {
            mapped = logical <= keys.first().unadjusted_offset ? keys.first() : keys.last();
            mapped.offset +=
                (logical - mapped.unadjusted_offset) * (mapped.size + in.spacing) / out.scroll_step;
        } else {
            for (int j = 1; j < keys.size(); ++j) {
                if (logical > keys[j].unadjusted_offset) continue;
                const auto& a = keys[j - 1];
                const auto& b = keys[j];
                const auto  t =
                    (logical - a.unadjusted_offset) / (b.unadjusted_offset - a.unadjusted_offset);
                mapped.offset = lerp(a.offset, b.offset, t);
                mapped.size   = lerp(a.size, b.size, t);
                break;
            }
        }
        const auto distance = qAbs(out.snap_offsets[i] - viewStart);
        if (distance < nearest) {
            nearest           = distance;
            out.leading_index = i;
        }
        CarouselItemGeometry geometry;
        geometry.index    = i;
        geometry.position = viewStart + mapped.offset - mapped.size / 2;
        geometry.size     = mapped.size;
        geometry.size_class =
            sizeClassFor(mapped.size, active.large_size, active.medium_size, active.small_size);
        applyMaskAndParallax(geometry, viewStart, viewEnd, in.parallax_ratio, in.min_peek_px, true);
        if (geometry.mask_start + geometry.mask_end < 1) out.items.append(geometry);
    }
    return out;
}

auto layoutHeroItems(const CarouselLayoutInput& in, const KeylineList& keylines)
    -> CarouselLayoutOutput {
    if (in.count <= 0 || keylines.keylines.isEmpty()) return {};
    if (in.count == 1) {
        auto       single = keylines;
        const auto size   = std::max(
            qreal(1), in.viewport_size - in.content_padding_start - in.content_padding_end);
        single.large_size = single.medium_size = single.small_size = size;
        single.scroll_step                                         = size + in.spacing;
        single.keylines = { { in.content_padding_start + size / 2, size, kSizeLarge } };
        return layoutFixedStride(in, single);
    }
    if (keylines.keylines.size() == 1) return layoutFixedStride(in, keylines);
    return layoutKeylineItems(in, keylines);
}

} // namespace

namespace CarouselHeroKeylines
{

namespace
{

constexpr int kSizeSmall = 0;
constexpr int kSizeLarge = 2;

auto clamp(qreal v, qreal lo, qreal hi) -> qreal { return qBound(lo, v, hi); }

auto buildStartAlignedKeylines(const CarouselLayoutInput& in, const HeroMetrics& metrics)
    -> KeylineList {
    KeylineList list;
    list.large_size  = metrics.large_leading;
    list.small_size  = metrics.small_leading;
    list.medium_size = metrics.medium_leading;
    list.scroll_step = metrics.scroll_step_leading;

    qreal cursor = in.content_padding_start;
    list.keylines.append(
        { cursor + metrics.large_leading * 0.5, metrics.large_leading, kSizeLarge });
    cursor += metrics.large_leading + in.spacing;
    list.keylines.append(
        { cursor + metrics.small_leading * 0.5, metrics.small_leading, kSizeSmall });
    return list;
}

auto buildCenterMiddleKeylines(const CarouselLayoutInput& in, const HeroMetrics& metrics)
    -> KeylineList {
    KeylineList list;
    const qreal available = in.viewport_size - in.content_padding_start - in.content_padding_end;

    list.large_size  = metrics.large_center;
    list.small_size  = metrics.small_center;
    list.medium_size = metrics.medium_center;
    list.scroll_step = metrics.scroll_step_center;

    const qreal start        = in.content_padding_start;
    const qreal large_center = in.content_padding_start + (available - metrics.large_center) * 0.5 +
                               metrics.large_center * 0.5;
    list.keylines.append({ start + metrics.small_center * 0.5, metrics.small_center, kSizeSmall });
    list.keylines.append({ large_center, metrics.large_center, kSizeLarge });
    list.keylines.append({ in.viewport_size - in.content_padding_end - metrics.small_center * 0.5,
                           metrics.small_center,
                           kSizeSmall });
    return list;
}

auto buildStartTrailingKeylines(const CarouselLayoutInput& in, const HeroMetrics& metrics)
    -> KeylineList {
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

auto keepLeadingPeek(const CarouselLayoutInput& in) -> bool {
    return in.layout == CarouselLayoutId::HeroCenter;
}

auto computeMetrics(const CarouselLayoutInput& in) -> HeroMetrics {
    HeroMetrics metrics;
    const qreal available = in.viewport_size - in.content_padding_start - in.content_padding_end;
    const qreal min_small = in.small_item_min > 0 ? in.small_item_min : 40;
    const qreal max_small = qMax(min_small, in.small_item_max > 0 ? in.small_item_max : 56);

    metrics.small_leading = clamp(available * 0.18, min_small, max_small);
    metrics.large_leading = available - metrics.small_leading - in.spacing;

    // A narrow viewport cannot fit the minimum focal item plus the trailing peek.
    if (metrics.large_leading < min_small * 2) {
        metrics.single_item   = true;
        const qreal size      = qMax(1.0, available);
        metrics.small_leading = metrics.small_center = size;
        metrics.large_leading = metrics.large_center = size;
        metrics.medium_leading = metrics.medium_center = size;
        metrics.scroll_step_leading = metrics.scroll_step_center = size + in.spacing;
        return metrics;
    }

    metrics.large_center =
        clamp(available * 0.72, min_small * 2, available - metrics.small_leading - in.spacing);
    metrics.small_center =
        clamp((available - metrics.large_center - in.spacing) * 0.5, min_small, max_small);

    metrics.medium_leading      = (metrics.large_leading + metrics.small_leading) * 0.5;
    metrics.medium_center       = (metrics.large_center + metrics.small_center) * 0.5;
    metrics.scroll_step_leading = metrics.medium_leading + in.spacing;
    metrics.scroll_step_center  = metrics.medium_center + in.spacing;
    return metrics;
}

auto phaseKeylines(const CarouselLayoutInput& in, HeroPhase phase, bool keep_leading_peek)
    -> KeylineList {
    const HeroMetrics metrics = computeMetrics(in);
    if (metrics.single_item) {
        KeylineList list;
        list.large_size = list.medium_size = list.small_size = metrics.large_leading;
        list.scroll_step                                     = metrics.scroll_step_leading;
        list.keylines.append({ in.content_padding_start + metrics.large_leading * 0.5,
                               metrics.large_leading,
                               kSizeLarge });
        return list;
    }
    switch (phase) {
    case HeroPhase::Leading: return buildStartAlignedKeylines(in, metrics);
    case HeroPhase::Middle:
        if (keep_leading_peek) {
            return buildCenterMiddleKeylines(in, metrics);
        }
        return buildStartAlignedKeylines(in, metrics);
    case HeroPhase::Trailing: return buildStartTrailingKeylines(in, metrics);
    }
    return {};
}

} // namespace CarouselHeroKeylines

auto CarouselKeylines::buildList(const CarouselLayoutInput& input) -> KeylineList {
    switch (input.layout) {
    case CarouselLayoutId::MultiBrowse: return buildMultiBrowse(input);
    case CarouselLayoutId::Hero:
    case CarouselLayoutId::HeroCenter:
        return CarouselHeroKeylines::phaseKeylines(input,
                                                   CarouselHeroKeylines::HeroPhase::Middle,
                                                   CarouselHeroKeylines::keepLeadingPeek(input));
    case CarouselLayoutId::Uncontained: return buildUncontained(input);
    case CarouselLayoutId::UncontainedMultiAspect: return buildUncontained(input);
    case CarouselLayoutId::FullScreen: return buildFullScreen(input);
    default: return buildUncontained(input);
    }
}

auto CarouselKeylines::layoutItems(const CarouselLayoutInput& in, const KeylineList& kl)
    -> CarouselLayoutOutput {
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
        return layoutHeroItems(in, kl);
    }
    return layoutKeylineItems(in, kl);
}

} // namespace qml_material
