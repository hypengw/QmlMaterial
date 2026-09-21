#include "qml_material/carousel/carousel_strategy.hpp"

#include <QCoreApplication>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace qml_material;

namespace
{

constexpr int kSizeLarge = 2;

auto makeInput(int count, qreal scroll, qreal viewport = 480) -> CarouselLayoutInput
{
    CarouselLayoutInput in;
    in.layout                = CarouselLayoutId::Hero;
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

    constexpr int count = 10;
    const auto    start = CarouselStrategy::compute(makeInput(count, 0));

    if (start.snap_offsets.isEmpty()) {
        return fail("hero start should produce snap offsets");
    }
    if (start.snap_offsets.first() > 0.5) {
        return fail("first snap offset should be at start");
    }

    const auto* item0 = findItem(start, 0);
    const auto* item1 = findItem(start, 1);
    if (!item0 || item0->size_class != kSizeLarge) {
        return fail("start layout item0 should be large");
    }
    if (!item1 || item1->size >= item0->size) {
        return fail("start layout item1 should be smaller than focal");
    }

    if (start.snap_offsets.size() < 2) {
        return fail("hero start should expose multiple snap offsets");
    }

    const auto mid = CarouselStrategy::compute(makeInput(count, start.snap_offsets.at(1) * 0.5));
    if (mid.items.size() < 2) {
        return fail("mid-scroll should show multiple items");
    }

    std::printf("PASS hero_start_layout\n");
    return EXIT_SUCCESS;
}
