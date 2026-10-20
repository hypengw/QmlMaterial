#include <QtTest>
#include "qml_material/carousel/carousel_strategy.hpp"

using namespace qml_material;
#include "qml_material/token/token.hpp"

namespace
{
constexpr auto carouselTokens = token::Carousel {};
static_assert(CarouselEngineDefaults::min_peek_px == carouselTokens.min_peek_px);
static_assert(CarouselEngineDefaults::parallax_ratio == carouselTokens.parallax_ratio);
static_assert(CarouselEngineDefaults::parallax_ratio_uncontained ==
              carouselTokens.parallax_ratio_uncontained);
static_assert(CarouselEngineDefaults::snap_duration == carouselTokens.snap_duration);
static_assert(CarouselEngineDefaults::min_item_aspect == carouselTokens.min_item_aspect);
static_assert(CarouselEngineDefaults::max_item_aspect == carouselTokens.max_item_aspect);
} // namespace

class CarouselKeylineMappingTest : public QObject {
    Q_OBJECT

private slots:
    void focalTargets_data() {
        QTest::addColumn<int>("layout");
        QTest::addColumn<int>("count");
        QTest::addColumn<int>("width");
        for (const auto layout : { CarouselLayoutId::MultiBrowse,
                                   CarouselLayoutId::Hero,
                                   CarouselLayoutId::HeroCenter })
            for (int count : { 1, 2, 3, 10 })
                for (int width : { 80, 320, 480, 1024 })
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(int(layout)).arg(count).arg(width)))
                        << int(layout) << count << width;
    }

    void focalTargets() {
        QFETCH(int, layout);
        QFETCH(int, count);
        QFETCH(int, width);
        CarouselLayoutInput input;
        input.layout                = layout;
        input.count                 = count;
        input.viewport_size         = width;
        input.cross_size            = 180;
        input.content_padding_start = 16;
        input.content_padding_end   = 16;
        input.spacing               = 8;
        const auto initial          = CarouselStrategy::compute(input);
        QCOMPARE(initial.snap_offsets.size(), count);
        for (int index = 0; index < count; ++index) {
            input.scroll_offset = initial.snap_offsets[index];
            const auto output   = CarouselStrategy::compute(input);
            QCOMPARE(output.snap_offsets, initial.snap_offsets);
            QCOMPARE(output.max_scroll_offset, initial.max_scroll_offset);
            const CarouselItemGeometry* target  = nullptr;
            qreal                       largest = 0;
            for (const auto& item : output.items) {
                if (item.index == index) target = &item;
                largest = std::max(largest, item.size);
            }
            QVERIFY2(
                target,
                qPrintable(QString("target %1 missing at %2").arg(index).arg(input.scroll_offset)));
            QCOMPARE(target->size_class, 2);
            QVERIFY(target->size >= largest - 1);
            QVERIFY(target->mask_start < 1e-6);
            QVERIFY(target->mask_end < 1e-6);
            QVERIFY(target->position >= input.scroll_offset - 1e-6);
            QVERIFY(target->position + target->size <= input.scroll_offset + width + 1e-6);
        }
        for (int step = 0; step < 21; ++step) {
            input.scroll_offset = initial.max_scroll_offset * step / 20;
            const auto output   = CarouselStrategy::compute(input);
            QCOMPARE(output.snap_offsets, initial.snap_offsets);
            QCOMPARE(output.max_scroll_offset, initial.max_scroll_offset);
        }
    }
};

int run_carousel_keyline_mapping(int argc, char** argv) {
    CarouselKeylineMappingTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "carousel_keyline_mapping.moc"
