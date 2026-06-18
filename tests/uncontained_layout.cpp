#include "qml_material/carousel/carousel_strategy.hpp"

#include <QCoreApplication>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace qml_material;

namespace
{

constexpr int kLayoutUncontained            = 0;
constexpr int kLayoutUncontainedMultiAspect = 5;

auto makeUncontainedInput(qreal scroll) -> CarouselLayoutInput
{
    CarouselLayoutInput in;
    in.layout                = kLayoutUncontained;
    in.viewport_size         = 480;
    in.cross_size            = 196;
    in.scroll_offset         = scroll;
    in.item_extent           = 180;
    in.spacing               = 8;
    in.content_padding_start = 16;
    in.count                 = 5;
    return in;
}

auto makeMultiAspectInput(qreal scroll) -> CarouselLayoutInput
{
    CarouselLayoutInput in;
    in.layout                = kLayoutUncontainedMultiAspect;
    in.viewport_size         = 480;
    in.cross_size            = 196;
    in.scroll_offset         = scroll;
    in.spacing               = 8;
    in.content_padding_start = 16;
    in.count                 = 4;
    in.item_aspects          = { 16.0 / 9.0, 1.0, 9.0 / 16.0, 2.0 };
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

    const auto uncontained = CarouselStrategy::compute(makeUncontainedInput(0));
    if (uncontained.items.isEmpty()) {
        return fail("uncontained should layout visible items");
    }
    if (!nearEqual(uncontained.items.first().size, 180, 2.0)) {
        return fail("uncontained item size should match item_extent");
    }
    if (uncontained.snap_offsets.size() < 2) {
        return fail("uncontained should expose multiple snap offsets");
    }

    const auto multi_aspect = CarouselStrategy::compute(makeMultiAspectInput(0));
    if (multi_aspect.items.size() < 2) {
        return fail("multi-aspect should layout multiple items");
    }
    qreal prev = -1;
    bool varied = false;
    for (const auto& g : multi_aspect.items) {
        if (prev > 0 && !nearEqual(g.size, prev, 0.5)) {
            varied = true;
        }
        prev = g.size;
    }
    if (!varied) {
        return fail("multi-aspect items should have different widths");
    }

    std::printf("PASS uncontained_layout\n");
    return EXIT_SUCCESS;
}
