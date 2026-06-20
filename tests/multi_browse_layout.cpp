#include "qml_material/carousel/carousel_strategy.hpp"

#include <QCoreApplication>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace qml_material;

namespace
{

constexpr int kSizeLarge         = 2;
constexpr int kSizeMedium        = 1;
constexpr int kSizeSmall         = 0;

auto makeInput(qreal scroll, int count = 10) -> CarouselLayoutInput
{
    CarouselLayoutInput in;
    in.layout                = CarouselLayoutId::MultiBrowse;
    in.viewport_size         = 480;
    in.cross_size            = 196;
    in.scroll_offset         = scroll;
    in.spacing               = 8;
    in.content_padding_start = 16;
    in.content_padding_end   = 16;
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

    const auto out = CarouselStrategy::compute(makeInput(0));
    if (out.snap_offsets.isEmpty()) {
        return fail("multi-browse should produce snap offsets");
    }
    if (out.snap_offsets.first() > 0.5) {
        return fail("first snap offset should be at start");
    }
    if (out.content_size <= 480) {
        return fail("multi-browse content should exceed viewport");
    }

    bool has_large = false;
    bool has_small = false;
    for (const auto& g : out.items) {
        if (g.size_class == kSizeLarge) {
            has_large = true;
        }
        if (g.size_class == kSizeSmall) {
            has_small = true;
        }
    }
    if (!has_large || !has_small) {
        return fail("multi-browse should include large and small size classes");
    }

    const auto* leading = findItem(out, out.leading_index);
    if (!leading || leading->size_class != kSizeLarge) {
        return fail("leading multi-browse item should be large");
    }

    std::printf("PASS multi_browse_layout\n");
    return EXIT_SUCCESS;
}
