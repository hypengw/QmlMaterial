#include "ordered_extent_index_p.hpp"
#include <QtTest>
#include <limits>
#include <optional>
#include <random>
#include <vector>

using namespace qml_material;

namespace
{
struct Row {
    int    key;
    double start;
    double extent;
};

std::vector<Row> rows(const std::vector<std::optional<double>>& values, double spacing) {
    std::vector<Row> result;
    double           offset = 0;
    for (int key = 0; key < int(values.size()); ++key) {
        if (! values[key]) continue;
        result.push_back({ key, offset, *values[key] });
        offset += *values[key] + spacing;
    }
    return result;
}

std::vector<int> visible(const std::vector<Row>& values, double begin, double end,
                         size_t limit = std::numeric_limits<size_t>::max(), bool reverse = false) {
    std::vector<int> result;
    if (begin >= end) return result;
    for (const auto& row : values)
        if (row.extent > 0 && row.start < end && row.start + row.extent > begin)
            result.push_back(row.key);
    if (reverse) std::reverse(result.begin(), result.end());
    if (result.size() > limit) result.resize(limit);
    return result;
}

int nearest(const std::vector<Row>& values, double position) {
    int    result   = -1;
    double distance = std::numeric_limits<double>::infinity();
    for (const auto& row : values) {
        if (row.extent <= 0) continue;
        const double next = std::abs(row.start + row.extent / 2 - position);
        if (next < distance) {
            result   = row.key;
            distance = next;
        }
    }
    return result;
}

int at(const std::vector<Row>& values, double position) {
    int last = -1;
    for (const auto& row : values) {
        if (row.extent <= 0) continue;
        last = row.key;
        if (row.start + row.extent > position) return row.key;
    }
    return last;
}
} // namespace

class OrderedExtentIndexTest : public QObject {
    Q_OBJECT
private slots:
    void randomizedUpdates() {
        for (double spacing : { 0.0, 3.0 }) {
            OrderedExtentIndex                 index(spacing);
            std::vector<std::optional<double>> values(129);
            std::mt19937                       random(9182);
            for (int iteration = 0; iteration < 400; ++iteration) {
                const int key = int(random() % values.size());
                if (random() % 4 == 0) {
                    QCOMPARE(index.erase(key), values[key].has_value());
                    values[key].reset();
                } else {
                    const double extent = random() % 7 == 0 ? 0 : random() % 151;
                    QVERIFY(index.set(key, extent));
                    values[key] = extent;
                }
                const auto oracle = rows(values, spacing);
                double     prefix = 0;
                int        first = -1, last = -1;
                for (int i = 0; i < int(values.size()); ++i) {
                    QCOMPARE(index.prefix(i), prefix);
                    QCOMPARE(index.extent(i), values[i].value_or(0));
                    if (values[i]) {
                        prefix += *values[i] + spacing;
                        if (*values[i] > 0) {
                            if (first < 0) first = i;
                            last = i;
                        }
                    }
                }
                QCOMPARE(index.prefix(int(values.size())), prefix);
                QCOMPARE(index.totalExtent(), oracle.empty() ? 0 : prefix - spacing);
                QCOMPARE(index.first(), first);
                QCOMPARE(index.last(), last);

                int before = -1, after = -1;
                for (const auto& row : oracle) {
                    if (row.extent <= 0) continue;
                    if (row.key < key) before = row.key;
                    if (row.key > key && after < 0) after = row.key;
                }
                QCOMPARE(index.previous(key), before);
                QCOMPARE(index.next(key), after);
                for (int query = 0; query < 5; ++query) {
                    const double begin = double(random() % (int(prefix) + 41)) - 20;
                    const double end   = begin + random() % 101;
                    const size_t limit = random() % 9;
                    QCOMPARE(index.indexAt(begin), at(oracle, begin));
                    QCOMPARE(index.nearestCenter(begin), nearest(oracle, begin));
                    QVERIFY(index.intersecting(begin, end) == visible(oracle, begin, end));
                    QVERIFY(index.intersecting(begin, end, limit) ==
                            visible(oracle, begin, end, limit));
                    QVERIFY(index.intersecting(begin, end, limit, true) ==
                            visible(oracle, begin, end, limit, true));
                }
            }
        }
    }
    void middleErase() {
        OrderedExtentIndex index(5);
        for (int key : { 10, 20, 30, 40, 50 }) QVERIFY(index.set(key, key));
        QCOMPARE(index.totalExtent(), 170.0);
        QCOMPARE(index.prefix(40), 75.0);
        QVERIFY(index.erase(30));
        QCOMPARE(index.totalExtent(), 135.0);
        QCOMPARE(index.prefix(40), 40.0);
        QCOMPARE(index.extent(30), 0.0);
        QCOMPARE(index.previous(40), 20);
        QCOMPARE(index.next(20), 40);
        QVERIFY(! index.erase(30));
        QVERIFY(index.intersecting(0, 1000) == (std::vector<int> { 10, 20, 40, 50 }));
        QVERIFY(index.erase(10));
        QVERIFY(index.erase(50));
        QCOMPARE(index.first(), 20);
        QCOMPARE(index.last(), 40);
        QCOMPARE(index.prefix(40), 25.0);
        QVERIFY(index.erase(20));
        QVERIFY(index.erase(40));
        QCOMPARE(index.totalExtent(), 0.0);
        QCOMPARE(index.indexAt(0), -1);
        QVERIFY(index.set(17, 7));
        QCOMPARE(index.prefix(17), 0.0);
        QCOMPARE(index.totalExtent(), 7.0);
    }
    void spacingAndBoundaries() {
        OrderedExtentIndex index(5);
        QVERIFY(index.set(0, 10));
        QVERIFY(index.set(4, 10));
        QCOMPARE(index.totalExtent(), 25.0);
        QCOMPARE(index.indexAt(-10), 0);
        QCOMPARE(index.indexAt(9), 0);
        QCOMPARE(index.indexAt(10), 4);
        QCOMPARE(index.indexAt(15), 4);
        QCOMPARE(index.indexAt(25), 4);
        QCOMPARE(index.nearestCenter(12.5), 0);
        QVERIFY(index.intersecting(10, 15).empty());
        QVERIFY(index.intersecting(9, 16) == (std::vector<int> { 0, 4 }));
        QVERIFY(index.intersecting(9, 16, 1, true) == (std::vector<int> { 4 }));
        QVERIFY(index.intersecting(9, 16, 0).empty());
        QVERIFY(index.intersecting(16, 9).empty());
    }
    void zeroSizedRows() {
        OrderedExtentIndex spaced(5);
        for (int key : { 1, 2, 3 }) QVERIFY(spaced.set(key, 0));
        QCOMPARE(spaced.totalExtent(), 10.0);
        QCOMPARE(spaced.first(), -1);
        QCOMPARE(spaced.last(), -1);
        QCOMPARE(spaced.indexAt(0), -1);
        QCOMPARE(spaced.nearestCenter(0), -1);
        QCOMPARE(spaced.next(0), -1);
        QCOMPARE(spaced.previous(4), -1);
        QVERIFY(spaced.intersecting(-1, 100).empty());
        QVERIFY(spaced.set(2, 8));
        QCOMPARE(spaced.prefix(2), 5.0);
        QCOMPARE(spaced.totalExtent(), 18.0);
        QVERIFY(spaced.intersecting(0, 5).empty());
        QVERIFY(spaced.intersecting(5, 13) == (std::vector<int> { 2 }));
        QVERIFY(spaced.intersecting(13, 20).empty());
        QCOMPARE(spaced.next(1), 2);
        QCOMPARE(spaced.previous(3), 2);

        OrderedExtentIndex sparse;
        for (int key = 0; key < 20000; ++key) QVERIFY(sparse.set(key, 0));
        QVERIFY(sparse.set(19998, 20));
        QCOMPARE(sparse.totalExtent(), 20.0);
        QCOMPARE(sparse.indexAt(0), 19998);
        QCOMPARE(sparse.nearestCenter(10), 19998);
        QVERIFY(sparse.intersecting(0, 20) == (std::vector<int> { 19998 }));
        QVERIFY(sparse.set(19998, 0));
        QCOMPARE(sparse.indexAt(0), -1);
        QVERIFY(sparse.intersecting(0, 20).empty());
    }
    void rejectsInvalidExtentWithoutMutation() {
        const double       maximum  = std::numeric_limits<double>::max();
        const double       infinity = std::numeric_limits<double>::infinity();
        const double       nan      = std::numeric_limits<double>::quiet_NaN();
        OrderedExtentIndex index;
        QVERIFY(index.set(1, maximum / 2));
        QVERIFY(index.set(3, maximum / 2));
        QCOMPARE(index.totalExtent(), maximum);
        QVERIFY(! index.set(2, maximum / 2));
        QVERIFY(! index.set(1, maximum));
        QVERIFY(! index.set(1, infinity));
        QVERIFY(! index.set(1, nan));
        QVERIFY(! index.set(1, -1));
        QVERIFY(! index.set(-1, 1));
        QCOMPARE(index.totalExtent(), maximum);
        QCOMPARE(index.prefix(3), maximum / 2);
        QCOMPARE(index.extent(1), maximum / 2);
        QCOMPARE(index.extent(2), 0.0);
        QCOMPARE(index.next(1), 3);
        QVERIFY(index.intersecting(0, maximum) == (std::vector<int> { 1, 3 }));
        QCOMPARE(index.indexAt(infinity), -1);
        QCOMPARE(index.nearestCenter(nan), -1);
        QVERIFY(index.intersecting(0, infinity).empty());
        QVERIFY(index.intersecting(nan, 1).empty());
        for (double spacing : { -1.0, infinity, nan }) {
            OrderedExtentIndex invalid(spacing);
            QVERIFY(! invalid.set(0, 1));
            QCOMPARE(invalid.totalExtent(), 0.0);
        }
    }
};

int run_ordered_extent_index(int argc, char** argv) {
    OrderedExtentIndexTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "ordered_extent_index.moc"
