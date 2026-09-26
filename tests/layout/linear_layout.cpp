#include "linear_layout.hpp"
#include <QtTest>
#include <limits>
#include <random>

using namespace qml_material;

class LinearLayoutTest : public QObject {
    Q_OBJECT
private slots:
    void boundaries() {
        LinearLayout layout;
        QVERIFY(layout.reset(3, 20, 5));
        QCOMPARE(layout.totalExtent(), 70.0);
        QCOMPARE(layout.offset(2), 50.0);
        QCOMPARE(layout.indexAt(-10), 0);
        QCOMPARE(layout.indexAt(24), 0);
        QCOMPARE(layout.indexAt(25), 1);
        QCOMPARE(layout.indexAt(1000), 2);
        QVERIFY(layout.setExtent(0, 30));
        QCOMPARE(layout.offset(1), 35.0);
        QCOMPARE(layout.totalExtent(), 80.0);
        QVERIFY(layout.reset(0, 20, 5));
        QCOMPARE(layout.indexAt(0), -1);
        QCOMPARE(layout.totalExtent(), 0.0);
        QVERIFY(layout.reset(3, 0, 0));
        QCOMPARE(layout.indexAt(0), 2);
        QVERIFY(! layout.setExtent(0, -1));
        QVERIFY(! layout.reset(3, std::numeric_limits<double>::infinity(), 0));
        QCOMPARE(layout.count(), 3);
    }
    void randomizedUpdates() {
        LinearLayout layout;
        QVERIFY(layout.reset(513, 30, 3));
        std::vector<double> sizes(513, 30);
        std::mt19937        random(42);
        for (int iteration = 0; iteration < 500; ++iteration) {
            const int index = int(random() % sizes.size());
            sizes[index]    = random() % 200;
            QVERIFY(layout.setExtent(index, sizes[index]));
            double prefix = 0;
            for (int i = 0; i < int(sizes.size()); ++i) {
                QCOMPARE(layout.offset(i), prefix);
                QCOMPARE(layout.indexAt(prefix), i);
                prefix += sizes[i] + 3;
            }
            QCOMPARE(layout.totalExtent(), prefix - 3);
        }
    }
    void largeList() {
        LinearLayout layout;
        QVERIFY(layout.reset(100000, 40, 1));
        QVERIFY(layout.setExtent(50000, 100));
        QCOMPARE(layout.offset(90000), 90000.0 * 41 + 60);
        QCOMPARE(layout.indexAt(layout.offset(90000) + 10), 90000);
        QCOMPARE(layout.totalExtent(), 4100059.0);
    }
    void visibleRanges() {
        LinearLayout layout;
        std::mt19937 random(71);
        for (double spacing : {0.0, 5.0}) {
            QVERIFY(layout.reset(40, 0, spacing));
            for (int i = 0; i < layout.count(); ++i)
                QVERIFY(layout.setExtent(i, random() % 3 == 0 ? 0 : random() % 30));
            for (double begin = -10; begin < layout.totalExtent() + 10; begin += 1) {
                for (double size : {0.0, 1.0, 20.0, 100.0}) {
                    int first = -1, last = -1;
                    for (int i = 0; i < layout.count(); ++i) {
                        if (size > 0 && layout.extent(i) > 0 &&
                            layout.offset(i) < begin + size &&
                            layout.offset(i) + layout.extent(i) > begin) {
                            if (first < 0) first = i;
                            last = i;
                        }
                    }
                    const auto actual = layout.visibleRange(begin, begin + size);
                    QCOMPARE(actual.first, first);
                    QCOMPARE(actual.last, last);
                }
            }
        }
        QVERIFY(layout.reset(100000, 0, 0));
        QVERIFY(layout.setExtent(90000, 20));
        QCOMPARE(layout.visibleRange(0, 20).first, 90000);
        QCOMPARE(layout.visibleRange(0, 20).last, 90000);
        QVERIFY(layout.setExtent(90000, 0));
        QCOMPARE(layout.visibleRange(0, 20).first, -1);
    }
};

int run_linear_layout(int argc, char** argv) {
    LinearLayoutTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "linear_layout.moc"
