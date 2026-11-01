#include <QtTest>
#include <QQuickWindow>
#include <QTimer>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QSGRendererInterface>
#include "qml_material/anim/shimmer.hpp"
#include "qml_material/anim/motion_runner.hpp"
#include "qml_material/item/skeleton.hpp"
#include "qml_material/core/item_activity.hpp"
#include "qml_material/view/lazy_grid.hpp"
#include "../../src/item/shimmer_geometry_p.hpp"
#include "../../src/anim/shimmer_subscription_p.hpp"
#include <cmath>
#include <limits>

using namespace qml_material;

class ShimmerTest : public QObject {
    Q_OBJECT
private slots:
    void parameters() {
        Shimmer group;
        QCOMPARE(group.boundsMode(), Shimmer::View);
        QCOMPARE(group.duration(), 800);
        QCOMPARE(group.pauseDuration(), 1500);
        QCOMPARE(group.angle(), 15);
        QCOMPARE(group.bandWidth(), 400);
        QCOMPARE(group.minimumOpacity(), .25);
        QCOMPARE(group.maximumOpacity(), 1);
        QVERIFY(group.running() && ! group.animating());
        const auto nan = std::numeric_limits<qreal>::quiet_NaN();
        group.setDuration(0);
        group.setPauseDuration(-1);
        group.setBandWidth(nan);
        group.setBandWidth(0);
        group.setAngle(-15);
        group.setMinimumOpacity(2);
        group.setMaximumOpacity(.1);
        QCOMPARE(group.duration(), 800);
        QCOMPARE(group.pauseDuration(), 1500);
        QCOMPARE(group.angle(), 15);
        QCOMPARE(group.bandWidth(), 400);
        QCOMPARE(group.minimumOpacity(), .25);
        QCOMPARE(group.maximumOpacity(), 1);
        group.setAngle(375);
        QCOMPARE(group.angle(), 15);
        group.seek(.5);
        QCOMPARE(group.phase(), 0);
        group.setRunning(false);
        group.seek(.5);
        QCOMPARE(group.phase(), .5);
        group.seek(nan);
        QCOMPARE(group.phase(), .5);
        group.restart();
        QCOMPARE(group.phase(), 0);
        QVERIFY(! group.running());
    }
    void projection_data() {
        QTest::addColumn<qreal>("angle");
        for (qreal angle : { 0, 15, 90, 180, 270 })
            QTest::newRow(qPrintable(QString::number(angle))) << angle;
    }
    void projection() {
        QFETCH(qreal, angle);
        const QRectF region(0, 0, 320, 180);
        const auto   area = shimmer::project(region.size(), {}, region, angle, 80);
        QVERIFY(area.valid);
        for (const auto p : { region.topLeft(),
                              region.topRight(),
                              region.bottomLeft(),
                              region.bottomRight(),
                              region.center() }) {
            QCOMPARE(shimmer::mask(area.at(p), area.travel, { 0, .25f, 1 }), .25f);
            QCOMPARE(shimmer::mask(area.at(p), area.travel, { 1, .25f, 1 }), .25f);
        }
        QCOMPARE(shimmer::mask(area.at(region.center()), area.travel, { .5f, .25f, 1 }), 1.f);
        QVERIFY(! shimmer::project({}, {}, region, angle, 80).valid);
        QTransform singular;
        singular.scale(0, 1);
        QVERIFY(! shimmer::project(region.size(), singular, region, angle, 80).valid);
    }
    void coordinates() {
        QQuickWindow window;
        window.resize(640, 400);
        QQuickItem reference(window.contentItem());
        reference.setSize({ 300, 200 });
        reference.setPosition({ 50, 60 });
        QQuickItem first(&reference), second(&reference);
        first.setPosition({ 20, 30 });
        first.setSize({ 100, 40 });
        second.setPosition({ 30, 50 });
        second.setSize({ 140, 80 });
        Shimmer group;
        group.setBoundsMode(Shimmer::Item);
        group.setBoundsItem(&reference);
        group.setAngle(15);
        const auto a = shimmer::areaFor(&first, &group), b = shimmer::areaFor(&second, &group);
        QVERIFY(a.valid && b.valid);
        QCOMPARE(a.at({ 20, 30 }), b.at({ 10, 10 }));
        reference.setClip(true);
        first.setX(-30);
        const auto clipped = shimmer::areaFor(&first, &group);
        QCOMPARE(clipped.travel, a.travel);
        reference.setRotation(30);
        QCOMPARE(shimmer::areaFor(&first, &group), clipped);
        group.setBoundsMode(Shimmer::Window);
        const auto rotated = shimmer::areaFor(&first, &group);
        QVERIFY(rotated.valid);
        QVERIFY(rotated.projection != clipped.projection);
        QQuickWindow other;
        QQuickItem   foreign(other.contentItem());
        foreign.setSize({ 200, 200 });
        group.setBoundsItem(&foreign);
        group.setBoundsMode(Shimmer::Item);
        QVERIFY(! shimmer::areaFor(&first, &group).valid);
    }
    void sharedClock() {
        Shimmer group;
        group.setPauseDuration(100000);
        auto* clock = group.findChild<MotionRunner*>();
        QVERIFY(clock);
        clock->setManualClock(true);
        QQuickItem          first, second;
        ShimmerSubscription firstLink(&first), secondLink(&second);
        firstLink.setGroup(&group);
        firstLink.setActive(true);
        QVERIFY(group.animating());
        clock->advance(200);
        QCOMPARE(group.phase(), .25);
        secondLink.setGroup(&group);
        secondLink.setActive(true);
        QCOMPARE(group.phase(), .25);
        firstLink.setActive(false);
        QVERIFY(group.animating());
        secondLink.setActive(false);
        QVERIFY(! group.animating());
        clock->advance(2000);
        QCOMPARE(group.phase(), .25);
        secondLink.setActive(true);
        QVERIFY(group.animating());
        clock->advance(600);
        QCOMPARE(group.phase(), 1);
        QVERIFY(! group.animating());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        auto* rest = group.findChild<QTimer*>();
        QVERIFY(rest && rest->isActive());
        group.setRunning(false);
        QVERIFY(! rest->isActive());
        group.setDuration(1600);
        QCOMPARE(group.phase(), 1);
        group.seek(.5);
        group.setRunning(true);
        clock->advance(400);
        QCOMPARE(group.phase(), .75);
        group.setAnimationsEnabled(false);
        QVERIFY(! group.animating());
        clock->advance(400);
        QCOMPARE(group.phase(), .75);
        group.setAnimationsEnabled(true);
        QVERIFY(group.animating());
        group.setMinimumOpacity(1);
        QVERIFY(! group.animating());
        group.setMinimumOpacity(.25);
        QVERIFY(group.animating());
        secondLink.setActive(false);
        QVERIFY(! group.animating());
    }
    void zeroRest() {
        Shimmer group;
        group.setPauseDuration(0);
        auto* clock = group.findChild<MotionRunner*>();
        clock->setManualClock(true);
        QQuickItem          item;
        ShimmerSubscription link(&item);
        link.setGroup(&group);
        link.setActive(true);
        QVERIFY(group.animating());
        clock->advance(800);
        QCOMPARE(group.phase(), 1);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(group.phase(), 0);
        QVERIFY(group.animating());
        group.setRunning(false);
        group.seek(.5);
        group.setRunning(true);
        clock->advance(400);
        group.restart();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(group.phase(), 0);
        QVERIFY(group.animating());
    }
    void ownerLifetime() {
        Skeleton item;
        item.setSize({ 80, 40 });
        item.setColor(Qt::red);
        QPointer<Shimmer> group = new Shimmer;
        auto*             clock = group->findChild<MotionRunner*>();
        clock->setManualClock(true);
        item.setShimmer(group);
        ShimmerSubscription link(&item);
        link.setGroup(group);
        link.setActive(true);
        QVERIFY(group->animating());
        QSignalSpy changed(&item, &Skeleton::shimmerChanged);
        connect(group, &Shimmer::phaseChanged, this, [&] {
            delete group.data();
        });
        clock->advance(100);
        QVERIFY(! group);
        QCOMPARE(item.shimmer(), nullptr);
        QCOMPARE(changed.size(), 1);
    }
    void clientActivity() {
        QQuickWindow window;
        window.resize(320, 240);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        Skeleton first(window.contentItem());
        if (! first.supported()) QSKIP("Requires an RHI backend; software must remain static");
        first.setSize({ 100, 40 });
        first.setColor(Qt::red);
        Shimmer group;
        auto*   clock = group.findChild<MotionRunner*>();
        clock->setManualClock(true);
        first.setShimmer(&group);
        QVERIFY(group.animating());
        clock->advance(200);
        first.setY(260);
        QVERIFY(! group.animating());
        QCOMPARE(group.phase(), .25);
        first.setY(0);
        QVERIFY(group.animating());
        QCOMPARE(group.phase(), .25);
        first.setVisible(false);
        QVERIFY(! group.animating());
        first.setVisible(true);
        QVERIFY(group.animating());
        first.setColor(Qt::transparent);
        QVERIFY(! group.animating());
    }
    void gridReuse() {
        QQuickWindow window;
        window.resize(320, 200);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (window.rendererInterface()->graphicsApi() == QSGRendererInterface::Software)
            QSKIP("Requires RHI activity; software is tested separately");
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQuick
            import Qcm.Material as MD
            Item { id:rootItem; width:320; height:200; property int count:100
                MD.Shimmer {id:g; objectName:"group"; boundsMode:MD.Shimmer.Item; boundsItem:view}
                MD.LazyGrid {id:view; objectName:"view"; anchors.fill:parent; fixedSlotCount:2; itemExtent:80
                    source:MD.ListSnapshotSource { keyRole:"id"; items: {
                        let result = []; for(let i=0;i<rootItem.count;++i) result.push({id:"item-"+i}); return result;
                    }}
                    delegate:Item {required property MD.LazyRow row; implicitHeight:80
                        MD.Skeleton {anchors.fill:parent; anchors.margins:4; color:"gray"; shimmer:g}
                    }
                }
            }
        )",
                          QUrl("qrc:/tests/shimmer-reuse.qml"));
        std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(root, qPrintable(component.errorString()));
        root->setParentItem(window.contentItem());
        auto* group = root->findChild<Shimmer*>("group");
        auto* view  = root->findChild<LazyGrid*>("view");
        QVERIFY(group && view);
        auto* clock = group->findChild<MotionRunner*>();
        clock->setManualClock(true);
        QTRY_VERIFY(group->animating());
        clock->advance(400);
        QCOMPARE(group->phase(), .5);
        for (int i = 0; i < 3; ++i) {
            QVERIFY(view->positionAtIndex(80));
            QTRY_VERIFY(view->firstVisibleIndex() >= 78);
            QVERIFY(group->animating());
            QCOMPARE(group->phase(), .5);
            QVERIFY(view->positionAtIndex(0));
            QTRY_COMPARE(view->firstVisibleIndex(), 0);
            QVERIFY(group->animating());
            QCOMPARE(group->phase(), .5);
        }
        root->setProperty("count", 112);
        QTRY_COMPARE(view->count(), 112);
        QCOMPARE(group->phase(), .5);
        root->setVisible(false);
        QVERIFY(! group->animating());
        root->setVisible(true);
        QVERIFY(group->animating());
        QCOMPARE(group->phase(), .5);
        QCOMPARE(group->findChildren<MotionRunner*>().size(), 1);
    }
    void softwareFallback() {
        QQuickWindow window;
        window.resize(160, 80);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (window.rendererInterface()->graphicsApi() != QSGRendererInterface::Software)
            QSKIP("Run with QT_QUICK_BACKEND=software");
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Rectangle {width:160; height:80; color:"white"
                MD.Shimmer {id:g; objectName:"group"; minimumOpacity:0}
                MD.Skeleton {width:60; height:60; color:"red"; radius:12; shimmer:g}
                Item {x:80; width:60; height:60; layer.enabled:true
                    layer.effect:MD.ShimmerEffect {shimmer:g}
                    Rectangle {anchors.fill:parent; radius:30; color:"blue"}
                }
            }
        )",
                          QUrl("qrc:/tests/shimmer-software.qml"));
        std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(root, qPrintable(component.errorString()));
        root->setParentItem(window.contentItem());
        QImage image;
        QTRY_VERIFY(! (image = window.grabWindow()).isNull());
        QTRY_VERIFY((image = window.grabWindow()).pixelColor(20, 20).red() > 245);
        QCOMPARE(image.pixelColor(20, 20), QColor(Qt::red));
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::white));
        QTRY_COMPARE((image = window.grabWindow()).pixelColor(110, 30), QColor(Qt::blue));
        QCOMPARE(image.pixelColor(80, 0), QColor(Qt::white));
        QVERIFY(! root->findChild<Shimmer*>("group")->animating());
    }
    void example() {
        QTest::failOnWarning(QRegularExpression(
            ".*(TypeError|ReferenceError|Unable to assign|Binding loop|Cannot assign).*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/Skeletons.qml"));
        std::unique_ptr<QQuickItem> root(
            qobject_cast<QQuickItem*>(component.createWithInitialProperties({ { "width", 320 } })));
        QVERIFY2(root, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(360, 900);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* group = root->findChild<Shimmer*>("skeletonShimmer");
        auto* view  = root->findChild<LazyGrid*>("skeletonGrid");
        QVERIFY(group && view);
        auto* clock = group->findChild<MotionRunner*>();
        clock->setManualClock(true);
        QTRY_COMPARE(view->count(), 12);
        QVERIFY(QMetaObject::invokeMethod(root.get(), "appendItems"));
        QTRY_COMPARE(view->count(), 24);
        auto* play = root->findChild<QObject*>("shimmerPlay");
        QVERIFY(play);
        QVERIFY(QMetaObject::invokeMethod(play, "click"));
        QVERIFY(! group->running());
        group->seek(.4);
        QCOMPARE(group->phase(), .4);
        auto* loading = root->findChild<QObject*>("skeletonLoading");
        QVERIFY(loading);
        QVERIFY(QMetaObject::invokeMethod(loading, "click"));
        QVERIFY(! root->property("loading").toBool());
        QCOMPARE(group->phase(), .4);
        QVERIFY(QMetaObject::invokeMethod(loading, "click"));
        QVERIFY(root->property("loading").toBool());
        QCOMPARE(group->phase(), .4);
        QVERIFY(QMetaObject::invokeMethod(play, "click"));
        QVERIFY(group->running());
        QTRY_VERIFY(group->animating());
    }
    void geometryNotification() {
        QQuickWindow window;
        window.resize(200, 200);
        QQuickItem parent(window.contentItem()), item(&parent);
        item.setSize({ 40, 40 });
        ItemActivity activity(&item);
        QSignalSpy   refreshed(&activity, &ItemActivity::refreshed);
        parent.setX(10);
        QVERIFY(! refreshed.isEmpty());
        refreshed.clear();
        parent.setRotation(15);
        QVERIFY(! refreshed.isEmpty());
    }
};
int run_shimmer(int argc, char** argv) {
    ShimmerTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "shimmer.moc"
