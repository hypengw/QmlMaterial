#include "qml_material/carousel/carousel_strategy.hpp"
#include "qml_material/carousel/carousel_view.hpp"

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickWindow>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace qml_material;

namespace
{

constexpr int kLayoutUncontained = 0;

struct TestCarouselView : CarouselView
{
    using CarouselView::CarouselView;

    void complete()
    {
        componentComplete();
    }
};

auto fail(const char* msg) -> int
{
    std::fprintf(stderr, "FAIL: %s\n", msg);
    return EXIT_FAILURE;
}

auto nearEqual(qreal a, qreal b, qreal eps = 1.0) -> bool
{
    return std::fabs(a - b) <= eps;
}

auto flickContentX(QQuickItem* flickable) -> qreal
{
    return flickable ? flickable->property("contentX").toDouble() : 0;
}

auto expectedSnapOffset(int index, int count) -> qreal
{
    CarouselLayoutInput in;
    in.layout                = kLayoutUncontained;
    in.viewport_size         = 480;
    in.cross_size            = 196;
    in.scroll_offset         = 0;
    in.item_extent           = 140;
    in.spacing               = 8;
    in.content_padding_start = 16;
    in.count                 = count;
    const auto out           = CarouselStrategy::compute(in);
    if (index <= 0) {
        return 0;
    }
    if (index >= count - 1) {
        return out.end_snap_offset;
    }
    if (index < out.snap_offsets.size()) {
        return out.snap_offsets.at(index);
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QQmlEngine engine;
    QQuickWindow window;
    window.resize(480, 196);

    TestCarouselView view;
    view.setParentItem(window.contentItem());
    view.setWidth(480);
    view.setHeight(196);
    view.setLayout(kLayoutUncontained);
    view.setModel(10);
    view.setItemExtent(140);
    view.setSpacing(8);
    view.setContentPaddingStart(16);
    view.complete();

    view.setCurrentIndex(9);
    if (view.currentIndex() != 9) {
        return fail("expected current index 9 before shrink");
    }

    view.setModel(5);
    if (view.currentIndex() != 4) {
        std::fprintf(stderr, "FAIL: shrink model currentIndex=%d expected=4\n", view.currentIndex());
        return EXIT_FAILURE;
    }

    auto* flick = view.flickable();
    const qreal expected = expectedSnapOffset(4, 5);
    if (!nearEqual(flickContentX(flick), expected)) {
        std::fprintf(stderr, "FAIL: shrink scroll contentX=%.2f expected=%.2f\n",
                     flickContentX(flick),
                     expected);
        return EXIT_FAILURE;
    }

    view.setModel(0);
    if (view.currentIndex() != 0) {
        std::fprintf(stderr, "FAIL: empty model currentIndex=%d expected=0\n", view.currentIndex());
        return EXIT_FAILURE;
    }

    std::printf("PASS model_shrink\n");
    return EXIT_SUCCESS;
}
