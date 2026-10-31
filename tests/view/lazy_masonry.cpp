#include "qml_material/view/lazy_masonry_grid.hpp"
#include "qml_material/model/lazy_row.hpp"
#include "masonry_layout_p.hpp"
#include <QQmlContext>
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
    explicit Scene(QVariantList values = rows(1000), QByteArray code = {}) {
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
