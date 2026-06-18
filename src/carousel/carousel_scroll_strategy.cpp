#include "qml_material/carousel/carousel_scroll_strategy.hpp"

#include <QtMath>

namespace qml_material
{

namespace
{

constexpr int kSizeSmall = 0;
constexpr int kSizeLarge = 2;

auto lerp(qreal a, qreal b, qreal t) -> qreal
{
    return a + (b - a) * t;
}

auto lerpKeyline(const Keyline& a, const Keyline& b, qreal t) -> Keyline
{
    return {
        lerp(a.offset, b.offset, t),
        lerp(a.size, b.size, t),
        t < 0.5 ? a.size_class : b.size_class,
    };
}

auto lerpKeylineLists(const KeylineList& a, const KeylineList& b, qreal t) -> KeylineList
{
    KeylineList out = a;
    const int   n   = qMin(a.keylines.size(), b.keylines.size());
    out.keylines.resize(n);
    for (int i = 0; i < n; ++i) {
        out.keylines[i] = lerpKeyline(a.keylines[i], b.keylines[i], t);
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

struct HeroKeylineTriple
{
    Keyline left;
    Keyline focal;
    Keyline right;
};

auto anchorKeyline(qreal viewport, qreal small, bool left_side) -> Keyline
{
    return { left_side ? -small : viewport + small, small, kSizeSmall };
}

auto collapsedKeyline(bool left_side, qreal viewport) -> Keyline
{
    return { left_side ? -1.0 : viewport + 1.0, 0.0, kSizeSmall };
}

auto extractHeroTriple(const KeylineList& kl, qreal viewport) -> HeroKeylineTriple
{
    HeroKeylineTriple triple;
    int               focal_idx = -1;
    for (int i = 0; i < kl.keylines.size(); ++i) {
        if (kl.keylines[i].size_class == kSizeLarge) {
            focal_idx = i;
            break;
        }
    }
    if (focal_idx < 0) {
        return triple;
    }

    triple.focal = kl.keylines[focal_idx];

    if (focal_idx > 0 && kl.keylines[focal_idx - 1].size_class == kSizeSmall) {
        triple.left = kl.keylines[focal_idx - 1];
    } else {
        triple.left = anchorKeyline(viewport, kl.small_size, true);
    }

    if (focal_idx + 1 < kl.keylines.size() && kl.keylines[focal_idx + 1].size_class == kSizeSmall) {
        triple.right = kl.keylines[focal_idx + 1];
    } else {
        triple.right = anchorKeyline(viewport, kl.small_size, false);
    }

    return triple;
}

auto tripleToKeylineList(const HeroKeylineTriple& triple, const KeylineList& meta, qreal viewport)
    -> KeylineList
{
    KeylineList out = meta;
    out.keylines.clear();

    const qreal view_lo = -meta.small_size * 0.5;
    const qreal view_hi = viewport + meta.small_size * 0.5;

    if (triple.left.offset >= view_lo && triple.left.size > 1.0) {
        out.keylines.append(triple.left);
    }
    out.keylines.append(triple.focal);
    if (triple.right.offset <= view_hi && triple.right.size > 1.0) {
        out.keylines.append(triple.right);
    }
    return out;
}

auto lerpHeroPhaseKeylines(const KeylineList& a, const KeylineList& b, qreal t, qreal viewport,
                           bool keep_leading_peek) -> KeylineList
{
    HeroKeylineTriple ta = extractHeroTriple(a, viewport);
    HeroKeylineTriple tb = extractHeroTriple(b, viewport);

    if (!keep_leading_peek) {
        ta.left = collapsedKeyline(true, viewport);
        tb.left = collapsedKeyline(true, viewport);
    }

    Keyline right = ta.right;
    if (t >= 0.85) {
        const qreal rt = (t - 0.85) / 0.15;
        right          = lerpKeyline(ta.right, tb.right, rt);
    }

    const HeroKeylineTriple out {
        lerpKeyline(ta.left, tb.left, t),
        lerpKeyline(ta.focal, tb.focal, t),
        right,
    };
    return tripleToKeylineList(out, b, viewport);
}

} // namespace

auto CarouselScrollStrategy::build(const KeylineList& default_keylines, qreal carousel_main_axis_size)
    -> CarouselScrollStrategy
{
    CarouselScrollStrategy strategy;
    strategy.default_keylines = default_keylines;
    strategy.viewport_size    = carousel_main_axis_size;
    strategy.start_steps.append(default_keylines);
    strategy.end_steps.append(default_keylines);

    if (default_keylines.keylines.isEmpty() || carousel_main_axis_size <= 0) {
        return strategy;
    }

    const qreal first_leading = default_keylines.keylines.first().offset
        - default_keylines.keylines.first().size * 0.5;
    strategy.start_shift_distance = qMax(0.0, first_leading);

    const Keyline& last          = default_keylines.keylines.last();
    const qreal    last_trailing = last.offset + last.size * 0.5;
    strategy.end_shift_distance  = qMax(0.0, last_trailing - carousel_main_axis_size);

    if (strategy.start_shift_distance > 0.5) {
        strategy.start_steps.append(shiftKeylines(default_keylines, strategy.start_shift_distance));
    }

    if (strategy.end_shift_distance > 0.5) {
        strategy.end_steps.append(shiftKeylines(default_keylines, -strategy.end_shift_distance));
    }

    return strategy;
}

auto CarouselScrollStrategy::buildHero(const CarouselLayoutInput& in, bool keep_leading_peek,
                                       const QVector<qreal>& snap_offsets, qreal max_scroll_offset)
    -> CarouselScrollStrategy
{
    CarouselScrollStrategy strategy;
    strategy.keep_leading_peek = keep_leading_peek;
    strategy.viewport_size     = in.viewport_size;
    strategy.hero_lerp         = keep_leading_peek;

    const KeylineList leading  = CarouselHeroKeylines::phaseKeylines(in, CarouselHeroKeylines::HeroPhase::Leading,
                                                                     keep_leading_peek);
    const KeylineList middle   = CarouselHeroKeylines::phaseKeylines(in, CarouselHeroKeylines::HeroPhase::Middle,
                                                                     keep_leading_peek);
    const KeylineList trailing = CarouselHeroKeylines::phaseKeylines(in, CarouselHeroKeylines::HeroPhase::Trailing,
                                                                     keep_leading_peek);

    strategy.default_keylines = middle;
    strategy.start_steps      = { leading, middle };
    strategy.end_steps        = { middle, trailing };

    if (middle.keylines.isEmpty() || in.viewport_size <= 0) {
        return strategy;
    }

    if (snap_offsets.size() >= 2) {
        strategy.start_shift_distance = qMax(0.0, snap_offsets.at(1));
    } else {
        strategy.start_shift_distance = middle.scroll_step;
    }

    if (snap_offsets.size() >= in.count && in.count >= 2) {
        strategy.end_shift_distance =
            qMax(0.0, snap_offsets.at(in.count - 1) - snap_offsets.at(in.count - 2));
    } else {
        strategy.end_shift_distance = middle.scroll_step;
    }

    if (!keep_leading_peek) {
        strategy.hero_lerp = false;
        const KeylineList trailing = CarouselHeroKeylines::phaseKeylines(
            in, CarouselHeroKeylines::HeroPhase::Trailing, false);
        strategy.end_steps = { middle, trailing };
        if (strategy.start_shift_distance > 0.5) {
            strategy.start_steps[1] = shiftKeylines(middle, strategy.start_shift_distance);
        }
        if (strategy.end_shift_distance > 0.5) {
            strategy.end_steps[1] = trailing;
        }
    }

    return strategy;
}

auto CarouselScrollStrategy::build(const CarouselLayoutInput& in, const KeylineList& default_keylines)
    -> CarouselScrollStrategy
{
    return build(default_keylines, in.viewport_size);
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
        const qreal t      = start_zone_end > 0 ? qBound(0.0, scroll_offset / start_zone_end, 1.0) : 1.0;
        const qreal step_t = round_to_step ? (t >= 0.5 ? 1.0 : 0.0) : t;
        if (hero_lerp) {
            if (step_t <= 0.0) {
                return start_steps.first();
            }
            if (step_t >= 1.0) {
                return start_steps.last();
            }
            return lerpHeroPhaseKeylines(
                start_steps.first(), start_steps.last(), step_t, viewport_size, keep_leading_peek);
        }
        return lerpKeylineLists(start_steps.first(), start_steps.last(), step_t);
    }

    if (scroll_offset >= end_zone_start && end_steps.size() > 1 && max_scroll_offset > end_zone_start) {
        const qreal span   = max_scroll_offset - end_zone_start;
        const qreal t      = qBound(0.0, (scroll_offset - end_zone_start) / span, 1.0);
        const qreal step_t = round_to_step ? (t >= 0.5 ? 1.0 : 0.0) : t;
        if (hero_lerp) {
            if (step_t <= 0.0) {
                return end_steps.first();
            }
            if (step_t >= 1.0) {
                return end_steps.last();
            }
            return lerpHeroPhaseKeylines(
                end_steps.first(), end_steps.last(), step_t, viewport_size, keep_leading_peek);
        }
        return lerpKeylineLists(end_steps.first(), end_steps.last(), step_t);
    }

    return default_keylines;
}

auto CarouselScrollStrategy::zoneFor(qreal scroll_offset, qreal keyline_max) const -> HeroScrollZone
{
    if (!hero_lerp || default_keylines.keylines.isEmpty()) {
        return HeroScrollZone::Middle;
    }

    const qreal start_zone_end = start_shift_distance;
    const qreal end_zone_start = qMax(0.0, keyline_max - end_shift_distance);

    if (scroll_offset <= start_zone_end + 0.5) {
        return HeroScrollZone::Leading;
    }

    if (scroll_offset >= keyline_max - 2.0) {
        return HeroScrollZone::TrailingRest;
    }

    if (scroll_offset >= end_zone_start && end_steps.size() > 1 && keyline_max > end_zone_start) {
        return HeroScrollZone::TrailingLerp;
    }

    return HeroScrollZone::Middle;
}

} // namespace qml_material
