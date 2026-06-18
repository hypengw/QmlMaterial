#include "qml_material/carousel/carousel_strategy.hpp"

#include <QCoreApplication>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace qml_material;

namespace
{

constexpr qreal kViewport       = 360;

auto makeInput(qreal scroll, int count = 10) -> CarouselLayoutInput
{
    CarouselLayoutInput in;
    in.layout                = CarouselLayoutId::FullScreen;
    in.viewport_size         = kViewport;
    in.cross_size            = 320;
    in.scroll_offset         = scroll;
    in.spacing               = 0;
    in.content_padding_start = 0;
    in.content_padding_end   = 0;
    in.parallax_ratio        = 0;
    in.count                 = count;
    return in;
}

auto fail(const char* msg) -> int
{
    std::fprintf(stderr, "FAIL: %s\n", msg);
    return EXIT_FAILURE;
}

auto nearEqual(qreal a, qreal b, qreal eps = 1.0) -> bool
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    constexpr int count = 10;

    CarouselLayoutInput in = makeInput(0);
    const auto          base = CarouselStrategy::compute(in);

    if (base.snap_offsets.size() != count) {
        return fail("snap_offsets size mismatch");
    }

    if (base.items.size() != 1) {
        return fail("rest at scroll 0 should show one item");
    }

    const auto& item0 = base.items.first();
    if (item0.index != 0 || !nearEqual(item0.size, kViewport)
        || item0.mask_start > 0.001 || item0.mask_end > 0.001) {
        return fail("first item not full viewport at rest");
    }

    if (std::fabs(item0.parallax_shift) > 0.001) {
        return fail("parallax should be zero");
    }

    for (int i = 1; i < base.snap_offsets.size(); ++i) {
        if (!nearEqual(base.snap_offsets.at(i) - base.snap_offsets.at(i - 1), kViewport)) {
            return fail("snap stride not equal to viewport");
        }
        if (base.snap_offsets.at(i) <= base.snap_offsets.at(i - 1)) {
            return fail("snap_offsets not strictly monotonic");
        }
    }

    in.spacing = 8;
    const auto spaced = CarouselStrategy::compute(in);
    if (!nearEqual(spaced.snap_offsets.at(1) - spaced.snap_offsets.at(0), kViewport + 8.0)) {
        return fail("snap stride with spacing");
    }

    in.scroll_offset = base.snap_offsets.at(1);
    const auto at_one = CarouselStrategy::compute(in);
    if (at_one.leading_index != 1 || at_one.items.size() != 1) {
        return fail("rest at snap[1]");
    }

    in.scroll_offset = base.snap_offsets.first() + kViewport * 0.5;
    const auto mid = CarouselStrategy::compute(in);
    if (mid.items.size() != 2) {
        return fail("mid-transition should show two items");
    }
    for (const auto& g : mid.items) {
        if (!nearEqual(g.size, kViewport)) {
            return fail("transition items not full page size");
        }
        if (g.mask_start > 0.001 || g.mask_end > 0.001) {
            return fail("fullscreen must not double-mask during transition");
        }
        if (std::fabs(g.parallax_shift) > 0.001) {
            return fail("parallax during transition");
        }
    }

    std::printf("OK: fullscreen layout checks passed\n");
    return EXIT_SUCCESS;
}
