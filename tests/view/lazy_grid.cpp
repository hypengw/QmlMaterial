#include "qml_material/view/lazy_grid.hpp"
#include "qml_material/view/lazy_list.hpp"
#include "qml_material/model/lazy_row.hpp"
#include "qml_material/model/item_model_source.hpp"
#include "grid_layout_p.hpp"
#include <QQmlContext>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QKeyEvent>
#include <QtTest>
#include <functional>

using namespace qml_material;

namespace
{
QVariantList rows(int count) {
    QVariantList values;
    for (int i = 0; i < count; ++i)
        values.append(
            QVariantMap { { "id", QString::number(i) }, { "title", QString::number(i) } });
    return values;
}
void frames(QQuickRenderControl& render, int count = 8) {
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
    LazyGrid            view { window.contentItem() };
    Scene(int count = 100) {
        delegate.setData("import QtQuick\nItem { required property QtObject row; property string "
                         "title: row.value.title }",
                         {});
        source.setKeyRole("id");
        source.setItems(rows(count));
        window.setGeometry(0, 0, 1200, 800);
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({ 300, 160 });
        view.setMinimumCellExtent(100);
        view.setItemExtent(40);
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

class GridCallback : public QObject {
    Q_OBJECT
public:
    std::function<void()> action;
    Q_INVOKABLE void      fire() {
        auto callback = std::exchange(action, {});
        if (callback) callback();
    }
};

class LazyGridTest : public QObject {
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
            MD.LazyGrid {
                width: 350; height: 120
                leftMargin: 10; rightMargin: 20; topMargin: 5; bottomMargin: 15
                minimumCellExtent: 100; crossSpacing: 10; mainSpacing: 8
                itemExtent: cellCrossExtent + 20
                source: MD.ListSnapshotSource { keyRole: "id"; items: [{id:"a"},{id:"b"},{id:"c"},{id:"d"}] }
                delegate: Item { required property MD.LazyRow row }
                MD.ScrollBarBase.vertical: MD.ScrollBar { objectName: "bar" }
            }
        )",
                          {});
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* view = qobject_cast<LazyGrid*>(object.get());
        QVERIFY(view);
        view->setParentItem(window.contentItem());
        frames(render);
        QCOMPARE(view->slotCount(), 3);
        QCOMPARE(view->cellCrossExtent(), 100.0);
        QCOMPARE(view->itemExtent(), 120.0);
        QCOMPARE(view->contentHeight(), 248.0);
        QVERIFY(view->findChild<QObject*>("bar"));
        QVERIFY(! view->estimatedContentHeight());
        QVERIFY(view->errorString().isEmpty());
    }
    void geometry_data() {
        QTest::addColumn<bool>("horizontal");
        QTest::newRow("vertical") << false;
        QTest::newRow("horizontal") << true;
    }
    void geometry() {
        QFETCH(bool, horizontal);
        Scene s(7);
        s.view.setOrientation(horizontal ? Qt::Horizontal : Qt::Vertical);
        s.view.setSize(horizontal ? QSizeF(120, 350) : QSizeF(350, 120));
        s.view.setLeftMargin(horizontal ? 5 : 10);
        s.view.setRightMargin(horizontal ? 15 : 20);
        s.view.setTopMargin(horizontal ? 10 : 5);
        s.view.setBottomMargin(horizontal ? 20 : 15);
        s.view.setCrossSpacing(10);
        s.view.setMainSpacing(8);
        s.view.setItemExtent(70);
        QVERIFY(s.view.positionAtKey("0"));
        frames(s.render);
        QCOMPARE(s.view.slotCount(), 3);
        QCOMPARE(s.view.cellCrossExtent(), 100.0);
        QCOMPARE(horizontal ? s.view.contentWidth() : s.view.contentHeight(), 226.0);
        QCOMPARE(s.view.firstVisibleIndex(), 0);
        QCOMPARE(s.view.lastVisibleIndex(), 5);
        for (int i = 0; i < 6; ++i) {
            auto* item = s.item(QString::number(i));
            QVERIFY(item);
            QCOMPARE(item->size(), horizontal ? QSizeF(70, 100) : QSizeF(100, 70));
            const auto rect = s.view.itemRect(i);
            QCOMPARE(horizontal ? rect.x() : rect.y(), (i / 3) * 78.0 + 5);
            QCOMPARE(horizontal ? rect.y() : rect.x(), (i % 3) * 110.0 + 10);
        }
        QVERIFY(s.view.positionAtKey("6", 1));
        frames(s.render);
        QVERIFY(horizontal ? s.view.isAtXEnd() : s.view.isAtYEnd());
        QVERIFY(s.view.currentDemand().reachesEnd());
    }
    void rangeAndRounding() {
        GridLayout layout;
        QVERIFY(layout.reset(7, 320, 3, 100, 70, 8, 10, false));
        QCOMPARE(layout.visibleRange(70, 78).first, -1);
        QCOMPARE(layout.visibleRange(0, 70).last, 2);
        QCOMPARE(layout.visibleRange(78, 148).first, 3);
        QCOMPARE(layout.visibleRange(156, 226).last, 6);
        QCOMPARE(layout.visibleRange(226, 300).first, -1);
        QVERIFY(layout.reset(20, 405, 0, 130, 40, 0, 5, false));
        QCOMPARE(layout.fixedSlotCount(), 3);
        QVERIFY(qAbs(layout.rect(2).right() - 405) < 0.00001);
        QVERIFY(layout.reset(1, 1, 0, 140, 40, 0, 12, false));
        QCOMPARE(layout.fixedSlotCount(), 1);
        QCOMPARE(layout.cellExtent(), 1.0);
        QVERIFY(layout.reset(0, 0, 0, 140, 40, 0, 12, false));
        QCOMPARE(layout.totalExtent(), 0.0);
        QCOMPARE(layout.visibleRange(0, 100).first, -1);
    }
    void identitiesAndResize() {
        Scene s(10000);
        s.view.setWidth(500);
        frames(s.render);
        QVERIFY(s.view.positionAtKey("5000", 0, 7));
        frames(s.render);
        QCOMPARE(s.view.itemRect(5000).y(), -7.0);
        auto* item = s.item("5000");
        QVERIFY(item);
        s.view.setCurrentKey("5000");
        s.view.setWidth(450);
        frames(s.render);
        QCOMPARE(s.view.slotCount(), 4);
        QCOMPARE(s.view.itemRect(5000).y(), -7.0);
        QCOMPARE(s.item("5000"), item);
        auto values = s.source.items();
        for (int i = 99; i >= 0; --i)
            values.prepend(QVariantMap { { "id", QString("new%1").arg(i) } });
        s.source.setItems(values);
        frames(s.render);
        QCOMPARE(s.view.currentKey(), QString("5000"));
        QCOMPARE(s.view.currentIndex(), 5100);
        QCOMPARE(s.view.itemRect(5100).y(), -7.0);
        values[5100] = QVariantMap { { "id", "5000" }, { "title", "changed" } };
        s.source.setItems(values);
        frames(s.render);
        QCOMPARE(s.item("5000")->property("title").toString(), QString("changed"));
        values.removeAt(5100);
        s.source.setItems(values);
        frames(s.render);
        QCOMPARE(s.view.currentIndex(), -1);
        QVERIFY(s.view.currentKey().isEmpty());
        QVERIFY(s.view.liveCount() < 100);
        QVERIFY2(s.view.errorString().isEmpty(), qPrintable(s.view.errorString()));
    }
    void visibleBudgetAndCache() {
        Scene s(10000);
        s.view.setSize({ 300, 100 });
        s.view.setMinimumCellExtent(10);
        s.view.setItemExtent(10);
        frames(s.render);
        QCOMPARE(s.view.lastVisibleIndex(), 299);
        QCOMPARE(s.view.liveCount(), 300);
        QVERIFY(s.view.errorString().isEmpty());
        s.view.setCacheExtent(100000);
        frames(s.render);
        QVERIFY(s.view.liveCount() > 300);
        QVERIFY(s.view.liveCount() <= 600);
        s.view.setWidth(0);
        frames(s.render);
        QCOMPARE(s.view.liveCount(), 0);
        QCOMPARE(s.view.firstVisibleIndex(), -1);
        QVERIFY(! s.view.currentDemand().valid());
        s.view.setWidth(300);
        frames(s.render);
        QCOMPARE(s.view.lastVisibleIndex(), 299);
    }
    void rowUsageAndPooling() {
        Scene s(100);
        s.view.setSize({ 300, 80 });
        s.view.setCacheExtent(40);
        frames(s.render);
        auto* visible = s.item("0")->property("row").value<LazyRow*>();
        auto* cached  = s.item("6")->property("row").value<LazyRow*>();
        QCOMPARE(visible->usage(), LazyRow::Visible);
        QCOMPARE(cached->usage(), LazyRow::Cached);
        QCOMPARE(visible->view(), &s.view);
        QVERIFY(visible->active() && cached->active());
        QPointer<LazyRow> old = visible;
        s.view.setPoolLimit(100);
        s.view.setHeight(0);
        frames(s.render);
        QVERIFY(old);
        QCOMPARE(old->usage(), LazyRow::Pooled);
        QVERIFY(! old->active() && ! old->view());
        s.view.setHeight(80);
        frames(s.render);
        auto* reused = s.item("0")->property("row").value<LazyRow*>();
        QCOMPARE(reused->usage(), LazyRow::Visible);
        QCOMPARE(reused->view(), &s.view);
    }
    void usageCallbackReentry() {
        Scene s(100);
        s.view.setSize({ 300, 80 });
        s.view.setCacheExtent(40);
        frames(s.render);
        auto* row = s.item("6")->property("row").value<LazyRow*>();
        QCOMPARE(row->usage(), LazyRow::Cached);
        QObject::connect(row, &LazyRow::usageChanged, &s.view, [&] {
            if (row->usage() == LazyRow::Visible) s.view.setDelegate(nullptr);
        });
        s.view.setContentY(40);
        frames(s.render);
        QCOMPARE(s.view.liveCount(), 0);
        s.view.setDelegate(&s.delegate);
        frames(s.render);
        QVERIFY(s.item("6"));
        QCOMPARE(s.item("6")->property("row").value<LazyRow*>()->usage(), LazyRow::Visible);
    }
    void keyboardAndMirroring() {
        Scene s(10);
        s.view.setHeight(70);
        frames(s.render);
        auto key = [&](int code) {
            QKeyEvent event(QEvent::KeyPress, code, Qt::NoModifier);
            QCoreApplication::sendEvent(&s.view, &event);
            frames(s.render);
        };
        s.view.setCurrentIndex(1);
        key(Qt::Key_Down);
        QCOMPARE(s.view.currentIndex(), 4);
        key(Qt::Key_Down);
        QCOMPARE(s.view.currentIndex(), 7);
        key(Qt::Key_Down);
        QCOMPARE(s.view.currentIndex(), 9);
        key(Qt::Key_Left);
        QCOMPARE(s.view.currentIndex(), 9);
        key(Qt::Key_Home);
        QCOMPARE(s.view.currentIndex(), 0);
        s.view.setLayoutDirection(Qt::RightToLeft);
        frames(s.render);
        QCOMPARE(s.view.itemRect(0).x(), 200.0);
        s.view.setCurrentIndex(1);
        key(Qt::Key_Right);
        QCOMPARE(s.view.currentIndex(), 0);
        key(Qt::Key_End);
        QCOMPARE(s.view.currentIndex(), 9);
        QVERIFY(s.view.itemRect(9).bottom() <= s.view.height());
    }
    void cancellationAndDemand() {
        Scene      s;
        QSignalSpy done(&s.view, &LazyGrid::positioningFinished);
        auto       oldDemand = s.view.currentDemand();
        QVERIFY(oldDemand.valid());
        QVERIFY(s.view.positionAtKey("99", 0, 0, LazyGrid::Smooth));
        s.view.setSource(nullptr);
        frames(s.render);
        QCOMPARE(done.size(), 1);
        QCOMPARE(done.first().at(1).toBool(), false);
        QVERIFY(! oldDemand.valid());
        QCOMPARE(s.view.liveCount(), 0);
        QCOMPARE(s.view.firstVisibleIndex(), -1);
    }
    void horizontalRtl() {
        Scene s(20);
        s.view.setOrientation(Qt::Horizontal);
        s.view.setFixedSlotCount(2);
        s.view.setLayoutDirection(Qt::RightToLeft);
        s.view.setSize({ 120, 200 });
        frames(s.render);
        QCOMPARE(s.view.contentWidth(), 400.0);
        QCOMPARE(s.view.contentX(), 280.0);
        QCOMPARE(s.view.itemRect(0).x(), 80.0);
        QCOMPARE(s.view.itemRect(0).y(), 0.0);
        QCOMPARE(s.view.itemRect(1).y(), 100.0);
        QCOMPARE(s.view.firstVisibleIndex(), 0);
        QCOMPARE(s.view.lastVisibleIndex(), 5);
        s.view.setCurrentIndex(0);
        QKeyEvent left(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
        QCoreApplication::sendEvent(&s.view, &left);
        QCOMPARE(s.view.currentIndex(), 2);
        QVERIFY(s.view.positionAtKey("19", 0));
        frames(s.render);
        QCOMPARE(s.view.contentX(), 0.0);
        QCOMPARE(s.view.firstVisibleIndex(), 14);
        QCOMPARE(s.view.lastVisibleIndex(), 19);
    }
    void sharedDragInput() {
        Scene s;
        s.view.setSynchronousDrag(true);
        frames(s.render);
        QTest::mousePress(&s.window, Qt::LeftButton, Qt::NoModifier, { 120, 120 });
        QTest::mouseMove(&s.window, { 120, 70 });
        frames(s.render);
        QVERIFY(s.view.contentY() > 0);
        QVERIFY(s.view.isDragging());
        QCOMPARE(s.view.contentX(), 0.0);
        QTest::mouseRelease(&s.window, Qt::LeftButton, Qt::NoModifier, { 120, 70 });
        QVERIFY(! s.view.isDragging());
    }
    void callbackReentry_data() {
        QTest::addColumn<int>("event");
        QTest::addColumn<bool>("destroy");
        for (int e = 0; e < 4; ++e)
            for (bool destroy : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(e).arg(destroy))) << e << destroy;
    }
    void callbackReentry() {
        QFETCH(int, event);
        QFETCH(bool, destroy);
        Scene        s;
        GridCallback hook;
        s.engine.rootContext()->setContextProperty("hook", &hook);
        s.engine.rootContext()->setContextProperty("callbackEvent", event);
        QQmlComponent delegate(&s.engine);
        delegate.setData(R"(
            import QtQuick
            Item {
                required property QtObject row
                Component.onCompleted: { if (callbackEvent === 0) hook.fire() }
                Connections {
                    target: row
                    function onChanged() { if (callbackEvent === 1) hook.fire() }
                    function onPooled() { if (callbackEvent === 2) hook.fire() }
                    function onReused() { if (callbackEvent === 3) hook.fire() }
                }
            }
        )",
                         {});
        QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));
        QPointer<LazyGrid> view = new LazyGrid(s.window.contentItem());
        QQmlEngine::setContextForObject(view, s.engine.rootContext());
        view->setSize({ 300, 120 });
        view->setItemExtent(40);
        view->setCacheExtent(0);
        view->setSource(&s.source);
        view->setDelegate(&delegate);
        if (event) frames(s.render);
        ListSnapshotSource replacement;
        replacement.setKeyRole("id");
        replacement.setItems(rows(1));
        int calls   = 0;
        hook.action = [&] {
            ++calls;
            if (destroy)
                delete view.data();
            else
                view->setSource(&replacement);
        };
        if (event == 1) {
            auto values = s.source.items();
            values[0]   = QVariantMap { { "id", "0" }, { "title", "updated" } };
            s.source.setItems(values);
        }
        if (event >= 2) view->setContentY(1000);
        frames(s.render, 20);
        QCOMPARE(calls, 1);
        if (destroy)
            QVERIFY(! view);
        else {
            QVERIFY(view);
            QCOMPARE(view->source(), &replacement);
            QCOMPARE(view->liveCount(), 1);
            QVERIFY(view->errorString().isEmpty());
            delete view.data();
        }
    }
};

int run_lazy_grid(int argc, char** argv) {
    LazyGridTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "lazy_grid.moc"
