#include "qml_material/view/lazy_masonry_grid.hpp"
#include "qml_material/model/lazy_row.hpp"
#include "masonry_layout_p.hpp"
#include <QQmlContext>
#include <QQmlIncubationController>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QtTest>

using namespace qml_material;

namespace
{
QVariantList rows(int count) {
    QVariantList values;
    for (int i = 0; i < count; ++i)
        values.append(QVariantMap { { "id", QString::number(i) }, { "size", 40 } });
    return values;
}
void frames(QQuickRenderControl& render, int count = 12) {
    for (int i = 0; i < count; ++i) {
        QCoreApplication::processEvents();
        render.polishItems();
    }
}
struct Scene {
    QQmlEngine          engine;
    QQuickRenderControl render;
    QQuickWindow        window { &render };
    QQmlComponent       delegate { &engine };
    ListSnapshotSource  source;
    LazyMasonryGrid     view { window.contentItem() };
    explicit Scene(QVariantList values = rows(1000), QByteArray code = {},
                   QQmlIncubationController* controller = nullptr) {
        if (controller) engine.setIncubationController(controller);
        delegate.setData(
            code.isEmpty()
                ? QByteArray("import QtQuick\nItem { required property QtObject row; "
                             "implicitHeight: row.value.size; property int marker: row.index }")
                : code,
            {});
        source.setKeyRole("id");
        source.setItems(values);
        window.setGeometry(0, 0, 1000, 800);
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({ 300, 160 });
        view.setMinimumCellExtent(100);
        view.setEstimatedItemExtent(40);
        view.setCacheExtent(0);
        view.setSource(&source);
        view.setDelegate(&delegate);
        if (controller)
            window.setVisible(true);
        else
            frames(render);
    }
    QQuickItem* item(const QString& key) {
        for (auto* host : view.contentItem()->childItems())
            for (auto* child : host->childItems()) {
                auto* row = child->property("row").value<LazyRow*>();
                if (row && row->active() && row->key() == key) return child;
            }
        return nullptr;
    }
};
} // namespace

class LazyMasonryTest : public QObject {
    Q_OBJECT
private slots:
    void incubationFillsVisibleBeforeNotification_data() {
        QTest::addColumn<qreal>("budget");
        QTest::newRow("no-cache") << qreal(0);
        QTest::newRow("default-budget") << qreal(2);
        QTest::newRow("large-budget") << qreal(1000);
    }
    void incubationFillsVisibleBeforeNotification() {
        QFETCH(qreal, budget);
        auto values = rows(5000);
        for (int i = 0; i < values.size(); ++i) {
            auto value    = values[i].toMap();
            value["size"] = i == 0 ? 6000 : 120 + i % 5 * 31;
            values[i]     = value;
        }
        QQmlIncubationController controller;
        Scene                    s(values, {}, &controller);
        s.view.setSize({ 900, 900 });
        s.view.setMinimumCellExtent(160);
        s.view.setCrossSpacing(12);
        s.view.setMainSpacing(12);
        s.view.setEstimatedItemExtent(160);
        s.view.setCacheExtent(160);
        s.view.setCachePreparationBudget(budget);
        QString failure;
        int     checked = 0;
        connect(&s.view, &LazyView::layoutChanged, &s.view, [&] {
            for (int index : s.view.currentDemand().indices()) {
                if (! s.view.itemRect(index).intersects(s.view.boundingRect())) continue;
                ++checked;
                const auto key = s.source.snapshot()->descriptorAt(index).key;
                if (! s.item(key) && failure.isEmpty()) failure = QString::number(index);
            }
        });
        for (int step = 0; step <= 80; ++step) {
            s.view.setContentY(step * 480);
            for (int frame = 0; frame < 3; ++frame) {
                QCoreApplication::processEvents();
                s.render.polishItems();
                QVERIFY2(failure.isEmpty(), qPrintable(failure));
                controller.incubateFor(100);
            }
        }
        QVERIFY(checked > 0);
        QVERIFY(s.view.errorString().isEmpty());
    }
    void incubationSparseItemsAndPositioning() {
        auto values = rows(1000);
        for (int i = 1; i < values.size(); ++i) {
            auto value    = values[i].toMap();
            value["size"] = 40 + i % 3 * 20;
            values[i]     = value;
        }
        auto longRow    = values[0].toMap();
        longRow["size"] = 10000;
        values[0]       = longRow;
        QQmlIncubationController controller;
        Scene                    s(values, {}, &controller);
        s.view.setPoolLimit(0);
        s.view.setCachePreparationBudget(1000);
        s.view.setCacheExtent(160);
        frames(s.render);
        QVERIFY(s.view.pendingCount() > 0);
        QCOMPARE(controller.incubatingObjectCount(), s.view.pendingCount());
        s.view.setContentY(2000);
        frames(s.render);
        QVERIFY(s.item("0"));
        QCOMPARE(s.view.firstVisibleIndex(), 0);
        QVERIFY(s.view.lastVisibleIndex() > 30);
        const auto needed = s.view.currentDemand().indices();
        QVERIFY(needed.contains(0));
        QVERIFY(! needed.contains(1));
        QVERIFY(needed.size() < 40);
        int target = -1;
        for (int index : needed) {
            const auto key = s.source.snapshot()->descriptorAt(index).key;
            if (s.view.itemRect(index).intersects(s.view.boundingRect())) {
                QVERIFY2(s.item(key), qPrintable(key));
                QCOMPARE(s.item(key)->property("row").value<LazyRow*>()->usage(), LazyRow::Visible);
            } else if (! s.item(key))
                target = index;
        }
        QVERIFY(target >= 0);
        const auto key = s.source.snapshot()->descriptorAt(target).key;
        QSignalSpy finished(&s.view, &LazyView::positioningFinished);
        QVERIFY(s.view.positionAtKey(key));
        frames(s.render);
        QCOMPARE(finished.size(), 1);
        QVERIFY(finished.first().at(1).toBool());
        QVERIFY(s.item(key));
        QCOMPARE(s.item(key)->property("row").value<LazyRow*>()->usage(), LazyRow::Visible);
        QVERIFY(qAbs(s.view.itemRect(target).y()) <= 1);
        QCOMPARE(s.view.itemRect(target).height(),
                 s.source.valueAt(target).toMap().value("size").toReal());
        for (int i = 0; i < 32 && s.view.pendingCount(); ++i) {
            controller.incubateFor(100);
            frames(s.render);
        }
        QCOMPARE(s.view.pendingCount(), 0);
        for (int index : s.view.currentDemand().indices())
            QVERIFY(s.item(s.source.snapshot()->descriptorAt(index).key));
        QVERIFY(s.view.errorString().isEmpty());
        s.view.setCacheExtent(1600);
        s.view.setContentY(8000);
        frames(s.render);
        QVERIFY(s.view.pendingCount() > 0);
        s.view.setVisible(false);
        QCOMPARE(s.view.pendingCount(), 0);
        QCOMPARE(controller.incubatingObjectCount(), 0);
        s.view.setCachePreparationBudget(0);
        s.view.setVisible(true);
        frames(s.render);
        QCOMPARE(s.view.pendingCount(), 0);
        QVERIFY(s.item("0"));
    }
    void incubationSourceChanges() {
        QQmlIncubationController controller;
        Scene                    s(rows(100), {}, &controller);
        s.view.setHeight(120);
        s.view.setPoolLimit(0);
        s.view.setCachePreparationBudget(1000);
        s.view.setCacheExtent(160);
        frames(s.render);
        QVERIFY(s.view.pendingCount() > 0);
        QVERIFY(! s.item("10"));
        auto values = s.source.items();
        values.swapItemsAt(10, 11);
        auto changed    = values[12].toMap();
        changed["size"] = 90;
        values[12]      = changed;
        values.removeAt(13);
        s.source.setItems(values);
        frames(s.render);
        for (int i = 0; i < 32 && s.view.pendingCount(); ++i) {
            controller.incubateFor(100);
            frames(s.render);
        }
        QCOMPARE(s.view.pendingCount(), 0);
        QVERIFY(s.item("10"));
        QCOMPARE(s.item("10")->property("row").value<LazyRow*>()->index(), 11);
        QVERIFY(s.item("12"));
        QCOMPARE(s.view.itemRect(s.source.indexOfKey("12")).height(), 90.0);
        QVERIFY(! s.item("13"));
        QCOMPARE(s.view.currentDemand().snapshot(), s.source.snapshot());
        QVERIFY(s.view.errorString().isEmpty());
    }
    void incubationColumnChanges() {
        QQmlIncubationController controller;
        Scene                    s(rows(100),
                                   R"(
            import QtQuick
            Item {
                required property QtObject row
                implicitHeight: row.value.size + width / 10
                property real initialWidth
                Component.onCompleted: initialWidth = width
            }
        )",
                                   &controller);
        s.source.setTypeRole("id");
        s.view.setPoolLimit(0);
        s.view.setCachePreparationBudget(1000);
        s.view.setCacheExtent(200);
        frames(s.render);
        QVERIFY(s.view.pendingCount() > 0);
        int target = -1;
        for (int index : s.view.currentDemand().indices())
            if (! s.item(s.source.snapshot()->descriptorAt(index).key)) target = index;
        QVERIFY(target >= 0);
        const auto key = s.source.snapshot()->descriptorAt(target).key;
        s.view.setFixedSlotCount(2);
        frames(s.render);
        QVERIFY(s.view.positionAtKey(key));
        frames(s.render);
        auto* item = s.item(key);
        QVERIFY(item);
        QCOMPARE(item->property("initialWidth").toReal(), 150.0);
        QCOMPARE(item->width(), 150.0);
        QCOMPARE(s.view.itemRect(s.source.indexOfKey(key)).height(), 55.0);
        QVERIFY(s.view.errorString().isEmpty());
    }
    void measurementValidity() {
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(6));
        MasonryViewLayout layout;
        layout.fixedSlotCount = 2;
        layout.estimate       = 40;
        QVERIFY(layout.apply(source.snapshot(), 200, 0, 100).accepted);
        for (int i = 0; i < 6; ++i) {
            QVERIFY(layout.needsMeasurement(i));
            QVERIFY(layout.measure(i, 40, 0).accepted);
            QVERIFY(! layout.needsMeasurement(i));
        }
        QVERIFY(layout.apply(source.snapshot(), 200, 0, 100).accepted);
        for (int i = 0; i < 6; ++i) QVERIFY(! layout.needsMeasurement(i));
        auto values = source.items();
        values.swapItemsAt(2, 3);
        source.setItems(values);
        QVERIFY(layout.apply(source.snapshot(), 200, 0, 100).accepted);
        QVERIFY(! layout.needsMeasurement(0));
        QVERIFY(! layout.needsMeasurement(1));
        for (int i = 2; i < 6; ++i) {
            QVERIFY(layout.needsMeasurement(i));
            QVERIFY(layout.measure(i, 40, 0).accepted);
            QVERIFY(! layout.needsMeasurement(i));
        }
        auto changed    = values[0].toMap();
        changed["size"] = 90;
        values[0]       = changed;
        source.setItems(values);
        QVERIFY(layout.apply(source.snapshot(), 200, 0, 100).accepted);
        QVERIFY(layout.needsMeasurement(0));
        QVERIFY(! layout.needsMeasurement(1));
        QVERIFY(layout.apply(source.snapshot(), 300, 0, 100).accepted);
        for (int i = 0; i < 6; ++i) QVERIFY(layout.needsMeasurement(i));
    }
    void qmlConstruction() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        QQmlComponent       component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.LazyMasonryGrid {
                width: 320; height: 180
                minimumCellExtent: 100; crossSpacing: 10; mainSpacing: 8
                estimatedItemExtent: 50
                source: MD.ListSnapshotSource { keyRole: "id"; items: [{id:"a",h:80},{id:"b",h:40},{id:"c",h:120},{id:"d",h:60}] }
                delegate: Item { required property MD.LazyRow row; implicitHeight: row.value.h }
                MD.ScrollBarBase.vertical: MD.ScrollBar {}
            }
        )",
                          {});
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* view = qobject_cast<LazyMasonryGrid*>(object.get());
        QVERIFY(view);
        view->setParentItem(window.contentItem());
        frames(render);
        QCOMPARE(view->slotCount(), 3);
        QCOMPARE(view->cellCrossExtent(), 100.0);
        QVERIFY(view->errorString().isEmpty());
        QCOMPARE(view->itemRect(3).x(), 110.0);
        QCOMPARE(view->itemRect(3).y(), 48.0);
        QCOMPARE(view->itemRect(3).height(), 60.0);
        view->setOrientation(Qt::Horizontal);
        QCOMPARE(view->orientation(), Qt::Vertical);
    }
    void sparseVisibleItems() {
        auto values     = rows(20000);
        auto longRow    = values[0].toMap();
        longRow["size"] = 100000;
        values[0]       = longRow;
        Scene s(values);
        s.view.setContentY(4000);
        frames(s.render);
        QVERIFY(s.item("0"));
        QVERIFY(s.view.firstVisibleIndex() == 0);
        QVERIFY(s.view.lastVisibleIndex() > 100);
        QVERIFY(s.view.liveCount() < 30);
        QVERIFY(s.view.currentDemand().valid());
        const auto needed = s.view.currentDemand().indices();
        QVERIFY(needed.contains(0));
        QVERIFY(needed.size() < 30);
        QVERIFY(! needed.contains(1));
        QVERIFY(s.view.currentDemand().count() > needed.size());
        s.view.setContentY(12000);
        frames(s.render);
        QVERIFY(s.view.liveCount() < 30);
    }
    void delayedMeasurementAndAnchor() {
        Scene s;
        QVERIFY(s.view.positionAtKey("150"));
        frames(s.render);
        const int   anchor = s.view.firstVisibleIndex();
        const auto  key    = s.view.keyAtIndex(anchor);
        const qreal before = s.view.itemRect(anchor).y();
        auto*       first  = s.item(key);
        QVERIFY(first);
        first->setImplicitHeight(90);
        frames(s.render);
        QCOMPARE(s.view.itemRect(anchor).height(), 90.0);
        QCOMPARE(s.view.itemRect(anchor).y(), before);
        auto values = s.source.items();
        values.prepend(QVariantMap { { "id", "inserted" }, { "size", 75 } });
        s.source.setItems(values);
        frames(s.render);
        const int next = s.source.indexOfKey(key);
        QVERIFY(qAbs(s.view.itemRect(next).y() - before) < 1);
        s.view.setWidth(510);
        frames(s.render);
        QCOMPARE(s.view.slotCount(), 5);
        QVERIFY(qAbs(s.view.itemRect(next).y() - before) < 1);
        values.removeAt(next);
        s.source.setItems(values);
        frames(s.render);
        QVERIFY(s.view.errorString().isEmpty());
    }
    void budgetMakesProgress() {
        Scene s(rows(10000));
        s.view.setSize({ 300, 15000 });
        frames(s.render, 24);
        QVERIFY2(s.view.errorString().isEmpty(), qPrintable(s.view.errorString()));
        QCOMPARE(s.view.liveCount(), 1125);
        QVERIFY(s.item("1124"));
        s.view.setCacheExtent(1e12);
        frames(s.render, 24);
        QVERIFY(s.view.liveCount() <= 1125 + 256);
    }
    void zeroAndInvalidMeasurements() {
        auto values = rows(10000);
        for (auto& value : values) {
            auto row    = value.toMap();
            row["size"] = 0;
            value       = row;
        }
        Scene s(values);
        frames(s.render, 12);
        QVERIFY(s.view.liveCount() <= 256);
        QVERIFY(! s.view.errorString().isEmpty());
        Scene invalid(rows(4),
                      "import QtQuick\nItem { required property QtObject row; height: -1 }");
        QVERIFY(! invalid.view.errorString().isEmpty());
    }
    void laneGeometryAndEnd() {
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(6));
        MasonryViewLayout layout;
        layout.fixedSlotCount = 2;
        layout.estimate       = 40;
        QVERIFY(layout.apply(source.snapshot(), 200, 0, 100).accepted);
        QVERIFY(layout.measure(0, 1000, 0).accepted);
        for (int i = 1; i < 6; ++i) QVERIFY(layout.measure(i, 40, 0).accepted);
        QCOMPARE(layout.rect(2).x(), 100.0);
        QCOMPARE(layout.rect(2).y(), 40.0);
        QCOMPARE(layout.totalExtent(), 1000.0);
        QVERIFY(layout.reachesEnd(100, 100, 0));
        QVERIFY(! layout.reachesEnd(0, 100, 0));
        QCOMPARE(layout.neighbor(1, 1, 0), 2);
        QCOMPARE(layout.neighbor(2, -1, 0), 1);
        QCOMPARE(layout.neighbor(2, 0, -1), 0);
        layout.mirrored = true;
        QVERIFY(layout.apply(source.snapshot(), 200, 0, 100).accepted);
        QCOMPARE(layout.rect(0).x(), 100.0);
        QCOMPARE(layout.neighbor(2, 0, 1), 0);
        const auto oldLane = layout.rect(1).x();
        QVERIFY(layout.measure(1, 100, 0).accepted);
        QCOMPARE(layout.rect(1).x(), oldLane);
    }
    void anchorInLeadingSpacing() {
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(7));
        MasonryViewLayout layout;
        layout.fixedSlotCount = 3;
        layout.estimate       = 40;
        layout.mainSpacing    = 12;
        QVERIFY(layout.apply(source.snapshot(), 300, 0, 100).accepted);
        QVERIFY(layout.measure(0, 1000, 0).accepted);
        for (int i = 1; i < 7; ++i) QVERIFY(layout.measure(i, 40, 0).accepted);
        QCOMPARE(layout.offset(3), 52.0);
        QCOMPARE(layout.indexAt(46), 3);
        QCOMPARE(layout.indexAt(20), 0);
        QCOMPARE(layout.indexAt(500), 0);
        QCOMPARE(layout.indices(46, 50), QVector<int> { 0 });
    }
    void demandLifetime() {
        Scene      s;
        const auto demand = s.view.currentDemand();
        QVERIFY(demand.valid());
        s.view.setContentY(300);
        QVERIFY(! demand.valid());
        frames(s.render);
        const auto next = s.view.currentDemand();
        QVERIFY(next.valid());
        s.view.setCacheExtent(200);
        QVERIFY(! next.valid());
        frames(s.render);
        const auto resized = s.view.currentDemand();
        QVERIFY(resized.valid());
        QVERIFY(s.item(s.view.keyAtIndex(s.view.anchorIndex())));
        s.item(s.view.keyAtIndex(s.view.anchorIndex()))->setImplicitHeight(90);
        QVERIFY(! resized.valid());
        frames(s.render);
        const auto changed = s.view.currentDemand();
        QVERIFY(changed.valid());
        s.view.setSource(nullptr);
        QVERIFY(! changed.valid());
    }
    void positioningAndReuse() {
        Scene s(rows(10000));
        s.view.setCurrentKey("5000");
        QVERIFY(s.view.positionAtKey("5000", 0, 7));
        frames(s.render);
        QVERIFY(! s.view.positioning());
        QCOMPARE(s.view.currentKey(), QString("5000"));
        QVERIFY(qAbs(s.view.itemRect(5000).y() + 7) < 1);
        QVERIFY(s.item("5000"));
        s.view.setContentY(0);
        frames(s.render);
        QVERIFY(s.view.liveCount() < 30);
        s.view.setSource(nullptr);
        frames(s.render);
        QCOMPARE(s.view.liveCount(), 0);
    }
    void appendAndReorder() {
        Scene      s(rows(100));
        const auto before = s.view.itemRect(3);
        auto       values = s.source.items();
        values.append(QVariantMap { { "id", "new" }, { "size", 90 } });
        s.source.setItems(values);
        frames(s.render);
        QCOMPARE(s.view.itemRect(3), before);
        QVERIFY(s.view.positionAtKey("45"));
        frames(s.render);
        const auto  key = s.view.keyAtIndex(s.view.anchorIndex());
        const qreal y   = s.view.itemRect(s.source.indexOfKey(key)).y();
        std::rotate(values.begin(), values.begin() + 10, values.begin() + 20);
        s.source.setItems(values);
        frames(s.render);
        QVERIFY(qAbs(s.view.itemRect(s.source.indexOfKey(key)).y() - y) < 1);
        QVERIFY(s.view.errorString().isEmpty());
    }
};

int run_lazy_masonry(int argc, char** argv) {
    LazyMasonryTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "lazy_masonry.moc"
