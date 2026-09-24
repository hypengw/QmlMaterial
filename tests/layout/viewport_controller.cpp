#include "viewport_controller.hpp"
#include <QtTest>

using namespace qml_material;

static ItemSnapshotPtr snapshot(QVector<ItemRecord> rows, bool stable = true) {
    auto result        = std::make_shared<ItemSnapshot>();
    result->rows       = std::move(rows);
    result->stableKeys = stable;
    for (int i = 0; i < result->rows.size(); ++i) result->indexes.insert(result->rows[i].key, i);
    return result;
}

class ViewportControllerTest : public QObject {
    Q_OBJECT
private slots:
    void structuralAnchors() {
        ViewportController view;
        auto               original =
            snapshot({ { "a", {}, 1 }, { "b", {}, 2 }, { "c", {}, 3 }, { "d", {}, 4 } });
        QVERIFY(view.apply(original, 300, 40, 0, 0).accepted);
        auto offset = view.apply(snapshot({ { "new", {}, 0 },
                                            { "a", {}, 1 },
                                            { "b", {}, 2 },
                                            { "c", {}, 3 },
                                            { "d", {}, 4 } }),
                                 300,
                                 40,
                                 0,
                                 45)
                          .offset;
        QCOMPARE(offset, 85.0);
        offset =
            view.apply(snapshot({ { "b", {}, 2 }, { "a", {}, 1 }, { "c", {}, 3 }, { "d", {}, 4 } }),
                       300,
                       40,
                       0,
                       offset)
                .offset;
        QCOMPARE(offset, 5.0);
        offset = view.apply(snapshot({ { "a", {}, 1 }, { "c", {}, 3 }, { "d", {}, 4 } }),
                            300,
                            40,
                            0,
                            offset)
                     .offset;
        QCOMPARE(offset, -35.0);
        QCOMPARE(view.apply(snapshot({}), 300, 40, 0, offset).offset, 0.0);
    }
    void measurementAndInvalidation() {
        ViewportController view;
        auto               rows = snapshot({ { "a", {}, 1 }, { "b", {}, 2 }, { "c", {}, 3 } });
        view.apply(rows, 300, 40, 0, 0);
        QCOMPARE(view.measure(0, 80, 45).offset, 85.0);
        QCOMPARE(view.layout().offset(1), 80.0);
        view.apply(rows, 300, 40, 0, 85);
        QCOMPARE(view.layout().extent(0), 80.0);
        QCOMPARE(view.invalidate("a", 85).offset, 45.0);
        view.measure(0, 80, 45);
        QCOMPARE(view.apply(rows, 400, 40, 0, 85).offset, 45.0);
        QCOMPARE(view.layout().extent(0), 40.0);
        view.measure(0, 80, 45);
        view.apply(snapshot({ { "a", {}, 99 }, { "b", {}, 2 }, { "c", {}, 3 } }), 400, 40, 0, 85);
        QCOMPARE(view.layout().extent(0), 40.0);
        QVERIFY(view.estimated());
        for (int i = 0; i < 3; ++i) QVERIFY(view.measure(i, 50, 0).accepted);
        QVERIFY(! view.estimated());
        QVERIFY(! view.positionAt("missing", 100, 0));
        QCOMPARE(*view.positionAt("b", 100, 0.5), 25.0);
    }
    void largePrepend() {
        QVector<ItemRecord> rows;
        for (int i = 0; i < 100000; ++i) rows.append({ QString::number(i), {}, i });
        ViewportController view;
        view.apply(snapshot(rows), 300, 40, 1, 0);
        const double offset = view.layout().offset(50000) + 7;
        for (int i = 0; i < 100; ++i) rows.prepend({ QStringLiteral("new%1").arg(i), {}, i });
        const auto result = view.apply(snapshot(rows), 300, 40, 1, offset);
        QVERIFY(result.accepted);
        QCOMPARE(result.offset, offset + 4100);
        const int index = view.layout().indexAt(result.offset);
        QCOMPARE(view.snapshot()->rows[index].key, QStringLiteral("50000"));
        QCOMPARE(result.offset - view.layout().offset(index), 7.0);
    }
    void predecessorAndUnstableIdentity() {
        ViewportController view;
        auto               rows = snapshot({ { "a", {}, 1 }, { "b", {}, 2 }, { "c", {}, 3 } });
        view.apply(rows, 300, 40, 0, 0);
        QCOMPARE(view.apply(snapshot({ { "a", {}, 1 }, { "b", {}, 2 } }), 300, 40, 0, 85).offset,
                 85.0);
        view.measure(0, 80, 0);
        view.apply(snapshot({ { "a", "changed", 1 }, { "b", {}, 2 } }), 300, 40, 0, 0);
        QCOMPARE(view.layout().extent(0), 40.0);
        view.clear();
        auto unstable = snapshot({ { "a", {}, 1 } }, false);
        view.apply(unstable, 300, 40, 0, 0);
        view.measure(0, 80, 0);
        view.apply(unstable, 300, 40, 0, 0);
        QCOMPARE(view.layout().extent(0), 80.0);
        view.apply(snapshot({ { "a", {}, 1 } }, false), 300, 40, 0, 0);
        QCOMPARE(view.layout().extent(0), 40.0);
        QVERIFY(! view.apply(rows, -1, 40, 0, 0).accepted);
        QCOMPARE(view.layout().count(), 1);
    }
};

QTEST_APPLESS_MAIN(ViewportControllerTest)
#include "viewport_controller.moc"
