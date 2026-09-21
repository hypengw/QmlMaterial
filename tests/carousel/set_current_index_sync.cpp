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

void setFlickContentX(QQuickItem* flickable, qreal value)
{
    if (flickable) {
        flickable->setProperty("contentX", value);
    }
}

auto expectedSnapOffset(int index, int count) -> qreal
{
    CarouselLayoutInput in;
    in.layout                = CarouselLayoutId::Uncontained;
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
    view.setLayout(CarouselLayoutId::Uncontained);
    view.setModel(10);
    view.setItemExtent(140);
    view.setSpacing(8);
    view.setContentPaddingStart(16);
    view.complete();

    auto* flick = view.flickable();
    if (!flick) {
        return fail("carousel view should expose flickable");
    }

    constexpr int target_index = 3;
    const qreal expected       = expectedSnapOffset(target_index, 10);
    view.setCurrentIndex(target_index);
    if (!nearEqual(flickContentX(flick), expected)) {
        std::fprintf(stderr, "FAIL: setCurrentIndex(%d) contentX=%.2f expected=%.2f\n",
                     target_index,
                     flickContentX(flick),
                     expected);
        return EXIT_FAILURE;
    }

    setFlickContentX(flick, 200);
    view.setCurrentIndex(0);
    if (!nearEqual(flickContentX(flick), 0)) {
        std::fprintf(stderr, "FAIL: setCurrentIndex(0) contentX=%.2f expected=0\n", flickContentX(flick));
        return EXIT_FAILURE;
    }

    const qreal end_expected = expectedSnapOffset(9, 10);
    view.setCurrentIndex(9);
    if (!nearEqual(flickContentX(flick), end_expected)) {
        std::fprintf(stderr, "FAIL: setCurrentIndex(9) contentX=%.2f expected=%.2f\n",
                     flickContentX(flick),
                     end_expected);
        return EXIT_FAILURE;
    }

    std::printf("PASS set_current_index_sync\n");
    return EXIT_SUCCESS;
}
