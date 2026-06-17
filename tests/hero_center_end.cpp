#include "qml_material/carousel/carousel_strategy.hpp"

#include <QCoreApplication>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace qml_material;

namespace
{

constexpr int kLayoutHeroCenter = 3;
constexpr int kSizeLarge        = 2;

auto makeInput(int count, qreal scroll, qreal viewport = 480) -> CarouselLayoutInput
{
    CarouselLayoutInput in;
    in.layout                = kLayoutHeroCenter;
    in.viewport_size         = viewport;
    in.cross_size            = 240;
    in.scroll_offset         = scroll;
    in.spacing               = 8;
    in.content_padding_start = 0;
    in.content_padding_end   = 0;
    in.small_item_min        = 40;
    in.small_item_max        = 56;
    in.min_peek_px           = 16;
    in.parallax_ratio        = 0.35;
    in.count                 = count;
    return in;
}

auto findItem(const CarouselLayoutOutput& out, int index) -> const CarouselItemGeometry*
{
    for (const auto& g : out.items) {
        if (g.index == index) {
            return &g;
        }
    }
    return nullptr;
}

auto fail(const char* msg) -> int
{
    std::fprintf(stderr, "FAIL: %s\n", msg);
    return EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    constexpr int   count    = 10;
    constexpr qreal viewport = 480;

    CarouselLayoutInput in = makeInput(count, 0, viewport);
    const auto          base = CarouselStrategy::compute(in);

    if (base.snap_offsets.size() != count) {
        return fail("snap_offsets size mismatch");
    }

    const auto* item0 = findItem(base, 0);
    const auto* item1 = findItem(base, 1);
    if (!item0 || item0->size_class != kSizeLarge) {
        return fail("start layout item0");
    }
    if (!item1 || item1->size >= 80.0) {
        return fail("start layout item1 peek");
    }

    const qreal snap1 = base.snap_offsets.at(1);
    in.scroll_offset  = snap1 * 0.5;
    const auto mid_lead = CarouselStrategy::compute(in);
    if (mid_lead.items.size() < 2) {
        return fail("leading transition visible");
    }
    const auto* grow = findItem(mid_lead, 1);
    if (!grow || grow->size <= item1->size + 4.0) {
        return fail("leading transition grows focal");
    }

    for (int i = 1; i < base.snap_offsets.size(); ++i) {
        if (base.snap_offsets.at(i) <= base.snap_offsets.at(i - 1)) {
            return fail("snap_offsets not strictly monotonic");
        }
    }

    const qreal penult_snap = base.snap_offsets.at(count - 2);
    const qreal last_snap   = base.snap_offsets.last();
    if (last_snap - penult_snap > viewport) {
        return fail("last snap too far from penultimate");
    }

    in.scroll_offset = penult_snap;
    const auto penult = CarouselStrategy::compute(in);
    if (penult.leading_index != count - 2 || penult.items.size() < 2) {
        return fail("penultimate layout");
    }

    in.scroll_offset = base.snap_offsets.at(1);
    const auto middle = CarouselStrategy::compute(in);
    if (middle.leading_index != 1 || middle.items.size() > 3) {
        return fail("middle layout");
    }

    in.scroll_offset = (penult_snap + last_snap) * 0.5;
    if (CarouselStrategy::compute(in).items.isEmpty()) {
        return fail("trailing lerp has no visible items");
    }

    in.scroll_offset = last_snap;
    const auto end = CarouselStrategy::compute(in);
    if (end.leading_index != count - 1) {
        return fail("end leading_index");
    }

    const auto* last_item = findItem(end, count - 1);
    if (!last_item || last_item->size_class != kSizeLarge) {
        return fail("last item not large at end");
    }

    const qreal trailing = last_item->position + last_item->size - in.scroll_offset;
    if (trailing < viewport - 8.0) {
        return fail("last item trailing edge");
    }

    if (last_item->size < viewport * 0.7) {
        return fail("last item too small for hero end");
    }

    std::printf("OK: hero center end checks passed\n");
    return EXIT_SUCCESS;
}
