#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "qml_material/input/app_bar_scroll.hpp"
#include "qml_material/input/floating_toolbar_scroll.hpp"
#include "qml_material/control/popup.hpp"
#include "qml_material/util/qml_util.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStyleHints>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtTest>

using namespace qml_material;

class TestScrollConnection : public NestedScrollConnection {
public:
    std::function<QPointF(QPointF)> pre = [](QPointF) {
        return QPointF();
    };
    std::function<QPointF(QPointF, QPointF)> post = [](QPointF, QPointF) {
        return QPointF();
    };
    int                             starts    = 0;
    std::function<QPointF(QPointF)> onRelease = [](QPointF) {
        return QPointF();
    };
    QPointF     release(QPointF velocity) override { return onRelease(velocity); }
    QList<bool> endings;
    QPointF     preScroll(QPointF available, Source) override { return pre(available); }
    QPointF     postScroll(QPointF consumed, QPointF available, Source) override {
        return post(consumed, available);
    }
    bool canConsume(QPointF, Source) const override { return true; }
    void begin(Source) override { ++starts; }
    void end(bool cancelled) override { endings.append(cancelled); }
};

class NestedScrollTest : public QObject {
    Q_OBJECT
    QQmlEngine                  engine;
    QQuickWindow                window;
    std::unique_ptr<QQuickItem> root;
    Flickable*                  outer = nullptr;
    QQuickFlickable*            inner = nullptr;
    void                        wheel(QPoint pixels, Qt::ScrollPhase phase = Qt::ScrollUpdate) {
        QWheelEvent event({ 100, 100 },
                          window.mapToGlobal(QPoint(100, 100)),
                          pixels,
                          pixels,
                          Qt::NoButton,
                          Qt::NoModifier,
                          phase,
                          false);
        QCoreApplication::sendEvent(&window, &event);
    }
    void mouse(QEvent::Type type, QPointF point, ulong timestamp) {
        QMouseEvent event(type,
                          point,
                          window.mapToGlobal(point.toPoint()),
                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                          Qt::NoModifier);
        event.setTimestamp(timestamp);
        QCoreApplication::sendEvent(&window, &event);
    }
private slots:
    void connectionReleaseVelocity() {
        TestScrollConnection child, parent;
        auto*                childConfig =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        auto* parentConfig =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(outer));
        childConfig->setConnection(&child);
        parentConfig->setConnection(&parent);
        QStringList    calls;
        QList<QPointF> velocities;
        parent.onRelease = [&](QPointF velocity) {
            calls << "parent";
            velocities << velocity;
            return QPointF(100, 500);
        };
        child.onRelease = [&](QPointF velocity) {
            calls << "child";
            velocities << velocity;
            return QPointF(0, 3000);
        };
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        mouse(QEvent::MouseButtonRelease, { 100, 110 }, 1025);
        QCOMPARE(calls, QStringList({ "parent", "child" }));
        QCOMPARE(velocities, QList<QPointF>({ { 0, 2000 }, { 0, 1500 } }));
        QCOMPARE(child.endings, QList<bool>({ false }));
        QCOMPARE(parent.endings, QList<bool>({ false }));
        QVERIFY(! inner->isMoving());
        QVERIFY(! outer->isMoving());
    }
    void connectionReleaseOnlyForDrag() {
        TestScrollConnection connection;
        auto*                config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&connection);
        int releases         = 0;
        connection.onRelease = [&](QPointF velocity) {
            ++releases;
            return velocity;
        };
        wheel({ 0, -20 }, Qt::ScrollBegin);
        wheel({}, Qt::ScrollEnd);
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseButtonRelease, { 100, 150 }, 1020);
        QCOMPARE(releases, 0);
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 2000);
        mouse(QEvent::MouseMove, { 100, 110 }, 2020);
        config->setEnabled(false);
        mouse(QEvent::MouseButtonRelease, { 100, 110 }, 2025);
        QCOMPARE(releases, 0);
    }
    void connectionReleaseReplacement() {
        TestScrollConnection first, second;
        auto*                config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&first);
        first.onRelease = [&](QPointF) {
            config->setConnection(&second);
            return QPointF();
        };
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        mouse(QEvent::MouseButtonRelease, { 100, 110 }, 1025);
        QCOMPARE(first.endings, QList<bool>({ true }));
        QCOMPARE(second.starts, 0);
        QVERIFY(! inner->isMoving());
        QVERIFY(! outer->isMoving());
    }
    void appBarInitialGeometry() {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.AppBar {
    width: 320
    type: MD.Enum.AppBarLarge
    title: "A long app bar title"
    animationsEnabled: false
    scrollBehavior: MD.AppBarScroll {
        heightOffset: -44
        collapseDistance: 88
    }
})",
                          QUrl("qrc:/app-bar-scroll-test.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QQuickItem> bar(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(bar, qPrintable(component.errorString()));
        QCOMPARE(bar->implicitHeight(), 108);
        QCOMPARE(bar->property("collapsedFraction").toReal(), 0.5);
        auto* behavior = bar->property("scrollBehavior").value<AppBarScroll*>();
        QVERIFY(behavior);
        behavior->begin(NestedScrollConnection::Drag);
        behavior->preScroll({ 0, 44 }, NestedScrollConnection::Drag);
        QCOMPARE(bar->implicitHeight(), 64);
        behavior->end(false);
        behavior->reset();
        QCOMPARE(bar->implicitHeight(), 152);
    }
    void appBarConsumption() {
        AppBarScroll bar;
        bar.setAnimationsEnabled(false);
        bar.setCollapseDistance(88);
        bar.begin(NestedScrollConnection::Drag);
        QVERIFY(bar.active());
        QCOMPARE(bar.preScroll({ 10, 60 }, NestedScrollConnection::Drag), QPointF(0, 60));
        QCOMPARE(bar.heightOffset(), -60);
        QCOMPARE(bar.preScroll({ 0, 50 }, NestedScrollConnection::Drag), QPointF(0, 28));
        QCOMPARE(bar.collapsedFraction(), 1);
        QCOMPARE(bar.preScroll({ 0, -40 }, NestedScrollConnection::Drag), QPointF());
        QCOMPARE(bar.postScroll({ 0, -100 }, { 0, -40 }, NestedScrollConnection::Drag),
                 QPointF(0, -40));
        QCOMPARE(bar.heightOffset(), -48);
        bar.end(false);
        QCOMPARE(bar.heightOffset(), -88);
        QVERIFY(! bar.active());
        bar.setMode(AppBarScroll::EnterAlways);
        QCOMPARE(bar.preScroll({ 0, -60 }, NestedScrollConnection::Drag), QPointF(0, -60));
        bar.end(false);
        QCOMPARE(bar.heightOffset(), 0);
        bar.setMode(AppBarScroll::Pinned);
        QCOMPARE(bar.preScroll({ 0, 100 }, NestedScrollConnection::Drag), QPointF());
        bar.setContentAtStart(false);
        QVERIFY(bar.overlapped());
        bar.setEnabled(false);
        QVERIFY(! bar.overlapped());
    }
    void appBarSnapInterrupted() {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.AppBar {
    width: 320
    type: MD.Enum.AppBarLarge
    scrollBehavior: MD.AppBarScroll { collapseDistance: 88 }
})",
                          QUrl("qrc:/app-bar-snap.qml"));
        std::unique_ptr<QQuickItem> bar(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(bar, qPrintable(component.errorString()));
        auto* behavior = bar->property("scrollBehavior").value<AppBarScroll*>();
        QVERIFY(behavior);
        auto* snap = behavior->findChild<QVariantAnimation*>();
        QVERIFY(snap);
        behavior->begin(NestedScrollConnection::Drag);
        behavior->preScroll({ 0, 60 }, NestedScrollConnection::Drag);
        behavior->end(false);
        QVERIFY(behavior->settling());
        QCOMPARE(behavior->heightOffset(), -60);
        snap->setCurrentTime(snap->duration() / 2);
        const auto presented = behavior->heightOffset();
        QVERIFY(presented < -60 && presented > -88);
        QCOMPARE(bar->implicitHeight(), 152 + presented);
        QCOMPARE(bar->property("collapsedFraction").toReal(), behavior->collapsedFraction());
        behavior->begin(NestedScrollConnection::Drag);
        QVERIFY(! behavior->settling());
        QCOMPARE(behavior->heightOffset(), presented);
        behavior->postScroll({}, { 0, -10 }, NestedScrollConnection::Drag);
        QCOMPARE(bar->property("__offset").toReal(), presented + 10);
        behavior->end(false);
        QVERIFY(behavior->settling());
        behavior->setEnabled(false);
        QVERIFY(! behavior->settling());
        QCOMPARE(bar->implicitHeight(), 152);
        behavior->setEnabled(true);
        behavior->setHeightOffset(-44);
        QVERIFY(! behavior->settling());
        QCOMPARE(bar->implicitHeight(), 108);
        behavior->end(false);
        QVERIFY(behavior->settling());
        QVERIFY(bar->setProperty("animationsEnabled", false));
        QVERIFY(! behavior->settling());
        QCOMPARE(behavior->heightOffset(), -88);
        QVERIFY(bar->setProperty("animationsEnabled", true));
        behavior->setHeightOffset(-44);
        behavior->end(false);
        QVERIFY(behavior->settling());
        delete behavior;
        QCOMPARE(bar->implicitHeight(), 152);
    }
    void appBarScrollAppearance_data() {
        QTest::addColumn<int>("type");
        QTest::addColumn<qreal>("fraction");
        QTest::addColumn<qreal>("topAlpha");
        QTest::addColumn<qreal>("gray");
        const qreal fractions[] = { 0, .25, .5, .75, 1 };
        const qreal alphas[]    = { 0, .007071738, .045670218, .212760247, 1 };
        const qreal grays[]     = { 0, .012394933, .203700284, .537245349, 1 };
        for (auto type : { Enum::AppBarType::AppBarMedium, Enum::AppBarType::AppBarLarge })
            for (int i = 0; i < 5; ++i)
                QTest::newRow(qPrintable(QString("%1-%2").arg(int(type)).arg(i)))
                    << int(type) << fractions[i] << alphas[i] << grays[i];
    }
    void appBarScrollAppearance() {
        QFETCH(int, type);
        QFETCH(qreal, fraction);
        QFETCH(qreal, topAlpha);
        QFETCH(qreal, gray);
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.AppBar {
    id: bar
    width: 320
    title: "Library"
    collapsedHeight: 60
    backgroundColor: "black"
    scrolledBackgroundColor: "white"
    scrollBehavior: MD.AppBarScroll {
        collapseDistance: bar.expandedHeight - bar.collapsedHeight
    }
})",
                          QUrl("qrc:/app-bar-appearance.qml"));
        std::unique_ptr<QQuickItem> bar(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(bar, qPrintable(component.errorString()));
        QVERIFY(bar->setProperty("type", type));
        auto* behavior = bar->property("scrollBehavior").value<AppBarScroll*>();
        behavior->setHeightOffset(-behavior->collapseDistance() * fraction);
        auto* background = bar->property("background").value<QQuickItem*>();
        QVERIFY(background);
        const auto color = background->property("color").value<QColor>();
        QVERIFY(std::abs(color.redF() - gray) < .0003);
        QVERIFY(std::abs(color.greenF() - gray) < .0003);
        QVERIFY(std::abs(color.blueF() - gray) < .0003);
        auto titles = bar->findChildren<QQuickText*>();
        titles.removeIf([](QQuickText* text) {
            return text->text() != "Library";
        });
        QCOMPARE(titles.size(), 2);
        for (auto* title : titles) {
            const bool expanded = title->parentItem()->y() == 60;
            QVERIFY(std::abs(title->opacity() - (expanded ? 1 - fraction : topAlpha)) < .00001);
            if (expanded && fraction == 0) {
                const auto baseline = title->y() + title->baselineOffset();
                const auto expected = type == int(Enum::AppBarType::AppBarMedium) ? 24 : 28;
                QCOMPARE(title->parentItem()->height() - baseline, expected);
            }
        }
    }
    void appBarContentOverlap_data() {
        QTest::addColumn<bool>("reverse");
        QTest::newRow("normal") << false;
        QTest::newRow("reverse") << true;
    }
    void appBarContentOverlap() {
        QFETCH(bool, reverse);
        AppBarScroll bar;
        bar.setCollapseDistance(100);
        bar.setMode(AppBarScroll::Pinned);
        bar.setReverseLayout(reverse);
        const qreal direction = reverse ? -1 : 1;
        QCOMPARE(bar.postScroll(
                     { 0, 40 * direction }, { 0, 60 * direction }, NestedScrollConnection::Drag),
                 QPointF());
        QCOMPARE(bar.contentOffset(), 40);
        QCOMPARE(bar.overlappedFraction(), .4);
        QCOMPARE(bar.heightOffset(), 0);
        bar.postScroll({ 0, -10 * direction }, {}, NestedScrollConnection::Drag);
        QCOMPARE(bar.contentOffset(), 30);
        bar.setContentAtStart(false);
        bar.setContentAtStart(true);
        QCOMPARE(bar.contentOffset(), 0);
        bar.postScroll({ 0, -40 * direction }, {}, NestedScrollConnection::Drag);
        QCOMPARE(bar.overlappedFraction(), 0);
        bar.setContentAtStart(false);
        QCOMPARE(bar.overlappedFraction(), 1);
        bar.setCollapseDistance(0);
        bar.postScroll({ 0, 20 * direction }, {}, NestedScrollConnection::Drag);
        QCOMPARE(bar.overlappedFraction(), 1);
        bar.setEnabled(false);
        QCOMPARE(bar.overlappedFraction(), 0);
    }
    void appBarSingleRowBackground() {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.AppBar {
    type: MD.Enum.AppBarSmall
    animationsEnabled: false
    backgroundColor: "black"
    scrolledBackgroundColor: "white"
    scrollBehavior: MD.AppBarScroll { mode: MD.AppBarScroll.Pinned; collapseDistance: 64 }
})",
                          QUrl("qrc:/app-bar-single-row.qml"));
        std::unique_ptr<QQuickItem> bar(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(bar, qPrintable(component.errorString()));
        auto* behavior   = bar->property("scrollBehavior").value<AppBarScroll*>();
        auto* background = bar->property("background").value<QQuickItem*>();
        QVERIFY(background);
        behavior->setContentOffset(.5);
        QCOMPARE(background->property("color").value<QColor>(), QColor("black"));
        behavior->setContentOffset(1);
        QCOMPARE(background->property("color").value<QColor>(), QColor("white"));
        behavior->reset();
        QCOMPARE(background->property("color").value<QColor>(), QColor("black"));
        QVERIFY(bar->setProperty("animationsEnabled", true));
        behavior->setContentOffset(64);
        auto* transition = background->findChild<QQuickNumberAnimation*>();
        QVERIFY(transition);
        QVERIFY(transition->isRunning());
        QTRY_VERIFY(background->property("color").value<QColor>() != QColor("black"));
        QVERIFY(bar->setProperty("type", int(Enum::AppBarType::AppBarMedium)));
        QCOMPARE(background->property("color").value<QColor>(), QColor("black"));
    }
    void appBarSnapCancelledByStateChange() {
        AppBarScroll bar;
        bar.setCollapseDistance(100);
        bar.begin(NestedScrollConnection::Drag);
        bar.setHeightOffset(-60);
        connect(&bar, &AppBarScroll::activeChanged, &bar, [&] {
            if (! bar.active()) bar.reset();
        });
        bar.end(false);
        QVERIFY(! bar.settling());
        QCOMPARE(bar.heightOffset(), 0);

        AppBarScroll stopped;
        stopped.setCollapseDistance(100);
        stopped.setHeightOffset(-60);
        stopped.end(false);
        QVERIFY(stopped.settling());
        connect(&stopped, &AppBarScroll::settlingChanged, &stopped, [&] {
            if (! stopped.settling()) stopped.setEnabled(false);
        });
        stopped.begin(NestedScrollConnection::Drag);
        QVERIFY(! stopped.enabled());
        QVERIFY(! stopped.active());
        QVERIFY(! stopped.settling());
    }
    void oklabInterpolation() {
        const auto middle =
            Util::mixColorOklab(QColor::fromRgbF(0, 0, 0, .2), QColor::fromRgbF(1, 1, 1, .8), .5);
        // Neutral Oklab L=.5 corresponds to linear RGB=.125, not sRGB=.5.
        QVERIFY(std::abs(middle.redF() - .388572859) < .0001);
        QVERIFY(std::abs(middle.greenF() - .388572859) < .0001);
        QVERIFY(std::abs(middle.blueF() - .388572859) < .0001);
        QVERIFY(std::abs(middle.alphaF() - .5) < .0001);
        const auto chromatic = Util::mixColorOklab(Qt::red, Qt::blue, .5);
        QVERIFY(std::abs(chromatic.redF() - .550441) < .001);
        QVERIFY(std::abs(chromatic.greenF() - .325621) < .001);
        QVERIFY(std::abs(chromatic.blueF() - .636501) < .001);
        QCOMPARE(Util::mixColorOklab(Qt::red, Qt::blue, -1), QColor(Qt::red));
        QCOMPARE(Util::mixColorOklab(Qt::red, Qt::blue, 2), QColor(Qt::blue));
        QCOMPARE(Util::mixColorOklab(Qt::red, Qt::blue, qQNaN()), QColor(Qt::red));
    }
    void appBarReverseAndCancellation() {
        AppBarScroll bar;
        bar.setCollapseDistance(80);
        bar.setReverseLayout(true);
        QCOMPARE(bar.preScroll({ 0, -50 }, NestedScrollConnection::Drag), QPointF(0, -50));
        bar.end(true);
        QCOMPARE(bar.heightOffset(), -50);
        bar.setCollapseDistance(160);
        QCOMPARE(bar.heightOffset(), -100);
        QCOMPARE(bar.postScroll({}, { 0, 40 }, NestedScrollConnection::Drag), QPointF(0, 40));
        QCOMPARE(bar.heightOffset(), -60);
        bar.setCollapseDistance(0);
        QCOMPARE(bar.collapsedFraction(), 0);
        QVERIFY(! bar.canConsume({ 0, -100 }, NestedScrollConnection::Drag));
    }
    void appBarWheelBeforeContent() {
        AppBarScroll bar;
        bar.setAnimationsEnabled(false);
        bar.setCollapseDistance(40);
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&bar);
        wheel({ 0, -60 }, Qt::ScrollBegin);
        QCOMPARE(bar.heightOffset(), -40);
        QCOMPARE(inner->contentY(), 20);
        wheel({ 0, 30 });
        QCOMPARE(inner->contentY(), 0);
        QCOMPARE(bar.heightOffset(), -30);
        wheel({}, Qt::ScrollEnd);
        QCOMPARE(bar.heightOffset(), -40);
        QVERIFY(! bar.active());
    }
    void appBarWheelModes_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<qreal>("offset");
        QTest::addColumn<qreal>("position");
        QTest::newRow("pinned") << int(AppBarScroll::Pinned) << 0. << 80.;
        QTest::newRow("enter-always") << int(AppBarScroll::EnterAlways) << -20. << 100.;
        QTest::newRow("exit-until-collapsed")
            << int(AppBarScroll::ExitUntilCollapsed) << -40. << 80.;
    }
    void appBarWheelModes() {
        QFETCH(int, mode);
        QFETCH(qreal, offset);
        QFETCH(qreal, position);
        AppBarScroll bar;
        bar.setMode(AppBarScroll::Mode(mode));
        bar.setCollapseDistance(40);
        bar.setHeightOffset(-40);
        inner->setContentY(100);
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&bar);
        wheel({ 0, 20 }, Qt::ScrollBegin);
        QCOMPARE(bar.heightOffset(), offset);
        QCOMPARE(inner->contentY(), position);
        wheel({ 20, 0 });
        QCOMPARE(bar.heightOffset(), offset);
        wheel({}, Qt::ScrollEnd);
    }
    void connectionWheelBarrier() {
        AppBarScroll bar;
        bar.setCollapseDistance(80);
        bar.setHeightOffset(-30);
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&bar);
        config->setProperty("wheelEnabled", false);
        wheel({ 0, -40 }, Qt::ScrollBegin);
        QVERIFY(! bar.active());
        wheel({}, Qt::ScrollEnd);
        QCOMPARE(bar.heightOffset(), -30);
        QCOMPARE(inner->contentY(), 0);
        QCOMPARE(outer->contentY(), 0);
    }
    void connectionHasOneAttachment() {
        TestScrollConnection connection;
        auto* child = qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        auto* parent =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(outer));
        child->setConnection(&connection);
        QTest::ignoreMessage(QtWarningMsg,
                             "NestedScrollConnection is already attached to another viewport");
        parent->setConnection(&connection);
        QCOMPARE(parent->connection(), nullptr);
        QCOMPARE(child->connection(), &connection);
        child->setConnection(nullptr);
        parent->setConnection(&connection);
        QCOMPARE(parent->connection(), &connection);
    }
    void appBarShortContent_data() {
        QTest::addColumn<bool>("owned");
        QTest::addColumn<bool>("touch");
        QTest::newRow("qt-mouse") << false << false;
        QTest::newRow("qt-touch") << false << true;
        QTest::newRow("owned-mouse") << true << false;
        QTest::newRow("owned-touch") << true << true;
    }
    void appBarTransformedDrag() {
        AppBarScroll bar;
        bar.setCollapseDistance(80);
        inner->setTransformOrigin(QQuickItem::TopLeft);
        inner->setScale(2);
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&bar);
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QCOMPARE(bar.heightOffset(), -20);
        QCOMPARE(inner->contentY(), 0);
        mouse(QEvent::MouseButtonRelease, { 100, 110 }, 1220);
        QVERIFY(bar.settling());
        auto* snap = bar.findChild<QVariantAnimation*>();
        QVERIFY(snap);
        snap->setCurrentTime(snap->duration());
        QCOMPARE(bar.heightOffset(), 0);
        QVERIFY(! bar.settling());
    }
    void appBarShortContent() {
        QFETCH(bool, owned);
        QFETCH(bool, touch);
        AppBarScroll bar;
        bar.setCollapseDistance(80);
        outer->setContentHeight(outer->height());
        inner->setProperty("model", 0);
        QMetaObject::invokeMethod(inner, "forceLayout");
        if (owned) inner->setVisible(false);
        auto* item   = owned ? static_cast<QQuickItem*>(outer) : static_cast<QQuickItem*>(inner);
        auto* config = qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(item));
        config->setConnection(&bar);
        if (touch) {
            static auto* device = QTest::createTouchDevice();
            QTest::touchEvent(&window, device).press(0, { 100, 150 }).commit();
            QTest::touchEvent(&window, device).move(0, { 100, 110 }).commit();
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        } else {
            mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
            mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        }
        QCOMPARE(bar.heightOffset(), -40);
        QCOMPARE(inner->contentY(), 0);
        QCOMPARE(outer->contentY(), 0);
        config->setEnabled(false);
        QVERIFY(! bar.active());
        QCOMPARE(bar.heightOffset(), -40);
    }
    void connectionConsumption() {
        TestScrollConnection child, parent;
        auto*                childConfig =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        auto* parentConfig =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(outer));
        childConfig->setConnection(&child);
        parentConfig->setConnection(&parent);
        QStringList    calls;
        QList<QPointF> values;
        parent.pre = [&](QPointF delta) {
            calls << "parent-pre";
            values << delta;
            return QPointF(0, 10);
        };
        child.pre = [&](QPointF delta) {
            calls << "child-pre";
            values << delta;
            return QPointF(0, 20);
        };
        child.post = [&](QPointF consumed, QPointF delta) {
            calls << "child-post";
            values << consumed << delta;
            return QPointF(0, 30);
        };
        parent.post = [&](QPointF consumed, QPointF delta) {
            calls << "parent-post";
            values << consumed << delta;
            return QPointF();
        };
        inner->setContentY(390);
        QSignalSpy childConsumed(childConfig, &NestedScroll::scrollConsumed);
        wheel({ 0, -100 }, Qt::ScrollBegin);
        QCOMPARE(calls, QStringList({ "parent-pre", "child-pre", "child-post", "parent-post" }));
        QCOMPARE(
            values,
            QList<QPointF>({ { 0, 100 }, { 0, 90 }, { 0, 10 }, { 0, 60 }, { 0, 90 }, { 0, 0 } }));
        QCOMPARE(inner->contentY(), 400);
        QCOMPARE(outer->contentY(), 30);
        QCOMPARE(childConsumed.size(), 1);
        QCOMPARE(childConsumed.first().first().toPointF(), QPointF(0, 10));
        wheel({}, Qt::ScrollEnd);
        QCOMPARE(child.starts, 1);
        QCOMPARE(child.endings, QList<bool>({ false }));
        QCOMPARE(parent.endings, QList<bool>({ false }));
    }
    void connectionReplacementCancels() {
        TestScrollConnection first, second;
        auto*                config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&first);
        wheel({ 0, -10 }, Qt::ScrollBegin);
        config->setConnection(&second);
        QCOMPARE(first.endings, QList<bool>({ true }));
        QCOMPARE(second.starts, 0);
        wheel({ 0, -10 }, Qt::ScrollBegin);
        QCOMPARE(second.starts, 1);
        config->setConnection(nullptr);
        QCOMPARE(second.endings, QList<bool>({ true }));
    }
    void connectionConsumptionBounds() {
        TestScrollConnection connection;
        auto*                config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&connection);
        connection.pre = [](QPointF) {
            return QPointF(100, -100);
        };
        wheel({ 0, -20 }, Qt::ScrollBegin);
        QCOMPARE(inner->contentY(), 20);
        connection.pre = [](QPointF) {
            return QPointF(0, 100);
        };
        wheel({ 0, -20 });
        QCOMPARE(inner->contentY(), 20);
        wheel({}, Qt::ScrollEnd);
    }
    void connectionReplacementDuringConsumption() {
        TestScrollConnection first, second;
        auto*                config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        config->setConnection(&first);
        first.pre = [&](QPointF delta) {
            config->setConnection(&second);
            return delta;
        };
        wheel({ 0, -20 }, Qt::ScrollBegin);
        QCOMPARE(first.endings, QList<bool>({ true }));
        QCOMPARE(second.starts, 0);
        QCOMPARE(inner->contentY(), 0);
        QCOMPARE(outer->contentY(), 0);
        wheel({ 0, -20 }, Qt::ScrollBegin);
        QCOMPARE(second.starts, 1);
        QCOMPARE(inner->contentY(), 20);
        wheel({}, Qt::ScrollEnd);
    }
    void connectionDestroyedDuringSession() {
        auto                 child = std::make_unique<TestScrollConnection>();
        TestScrollConnection parent;
        auto*                childConfig =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        auto* parentConfig =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(outer));
        childConfig->setConnection(child.get());
        parentConfig->setConnection(&parent);
        wheel({ 0, -20 }, Qt::ScrollBegin);
        child.reset();
        QCOMPARE(childConfig->connection(), nullptr);
        QCOMPARE(parent.endings, QList<bool>({ true }));
        wheel({ 0, -20 }, Qt::ScrollBegin);
        QCOMPARE(parent.starts, 2);
        wheel({}, Qt::ScrollEnd);
    }
    void initTestCase() {
        QTest::failOnWarning(QRegularExpression(".*(Binding loop|TypeError|ReferenceError).*"));
    }
    void init() {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.Scrollable {
    id: view
    width: 300; height: 300; contentHeight: 900
    flickableDirection: MD.Scrollable.VerticalFlick
    MD.NestedScroll.enabled: true
    property int clicks: 0
    property bool blockDrag: false
    ListView {
        objectName: "inner"; width: 300; height: 200
        MD.NestedScroll.enabled: true
        boundsBehavior: Flickable.StopAtBounds
        model: 12
        delegate: Rectangle {
            width: 300; height: 50
            MouseArea { anchors.fill: parent; preventStealing: view.blockDrag; onClicked: view.clicks++ }
        }
    }
})",
                          QUrl("qrc:/nested-scroll-test.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY(root);
        outer = qobject_cast<Flickable*>(root.get());
        inner = root->findChild<QQuickFlickable*>("inner");
        QVERIFY(outer && inner);
        window.setGeometry(0, 0, 300, 300);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(QMetaObject::invokeMethod(inner, "forceLayout"));
        QSignalSpy frames(&window, &QQuickWindow::afterAnimating);
        window.update();
        QTRY_VERIFY(! frames.isEmpty());
    }
    void cleanup() {
        root.reset();
        window.hide();
    }
    void wheelRemainder() {
        inner->setContentY(390);
        wheel({ 0, -40 }, Qt::ScrollBegin);
        QCOMPARE(inner->contentY(), 400);
        QCOMPARE(outer->contentY(), 30);
        wheel({ 0, 20 });
        QCOMPARE(inner->contentY(), 380);
        QCOMPARE(outer->contentY(), 30);
        wheel({}, Qt::ScrollEnd);
        QVERIFY(! inner->isMoving());
        QVERIFY(! outer->isMoving());
    }
    void consumedNotification() {
        auto* child = qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        auto* parent =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(outer));
        QSignalSpy childScroll(child, &NestedScroll::scrollConsumed);
        QSignalSpy parentScroll(parent, &NestedScroll::scrollConsumed);
        inner->setContentY(390);
        QCOMPARE(childScroll.size(), 0);
        wheel({ 0, -40 }, Qt::ScrollBegin);
        QCOMPARE(childScroll.size(), 1);
        QCOMPARE(childScroll.at(0).at(0).toPointF(), QPointF(0, 10));
        QCOMPARE(parentScroll.size(), 1);
        QCOMPARE(parentScroll.at(0).at(0).toPointF(), QPointF(0, 30));
        wheel({}, Qt::ScrollEnd);
        inner->setContentY(400);
        outer->setContentY(600);
        wheel({ 0, -40 }, Qt::ScrollBegin);
        QCOMPARE(childScroll.size(), 1);
        QCOMPARE(parentScroll.size(), 1);
        wheel({}, Qt::ScrollEnd);
    }
    void sessionNotifications() {
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        QSignalSpy started(config, &NestedScroll::scrollStarted);
        QSignalSpy finished(config, &NestedScroll::scrollFinished);
        wheel({ 0, -40 }, Qt::ScrollBegin);
        QCOMPARE(started.size(), 1);
        QVERIFY(finished.isEmpty());
        wheel({}, Qt::ScrollEnd);
        QCOMPARE(finished.size(), 1);
        QCOMPARE(finished.last().first().toBool(), false);
        wheel({ 0, -40 }, Qt::ScrollBegin);
        QCOMPARE(started.size(), 2);
        config->setEnabled(false);
        QCOMPARE(finished.size(), 2);
        QCOMPARE(finished.last().first().toBool(), true);
    }
    void sessionFinishedReentry() {
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        QSignalSpy started(config, &NestedScroll::scrollStarted);
        QSignalSpy finished(config, &NestedScroll::scrollFinished);
        const auto connection = connect(config, &NestedScroll::scrollFinished, this, [&] {
            wheel({ 0, -10 }, Qt::ScrollBegin);
        });
        wheel({ 0, -10 }, Qt::ScrollBegin);
        wheel({}, Qt::ScrollEnd);
        QCOMPARE(started.size(), 2);
        QCOMPARE(finished.size(), 1);
        disconnect(connection);
        wheel({}, Qt::ScrollEnd);
        QCOMPARE(finished.size(), 2);
    }
    void sessionStartedDestroysView() {
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        connect(config, &NestedScroll::scrollStarted, this, [&] {
            root.reset();
        });
        wheel({ 0, -10 }, Qt::ScrollBegin);
        QVERIFY(! root);
    }
    void consumedNotificationDestroysView() {
        auto* child = qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(inner));
        connect(child, &NestedScroll::scrollConsumed, this, [&] {
            root.reset();
        });
        wheel({ 0, -40 }, Qt::ScrollBegin);
        QVERIFY(! root);
    }
    void singleParticipantNotification() {
        inner->setParentItem(nullptr);
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(outer));
        FloatingToolbarScroll behavior;
        connect(config, &NestedScroll::scrollConsumed, &behavior, &FloatingToolbarScroll::scrollBy);
        connect(&behavior, &FloatingToolbarScroll::collapseRequested, &behavior, [&] {
            behavior.setExpanded(false);
        });
        connect(&behavior, &FloatingToolbarScroll::expandRequested, &behavior, [&] {
            behavior.setExpanded(true);
        });
        wheel({ 0, -39 }, Qt::ScrollBegin);
        QVERIFY(behavior.expanded());
        wheel({ 0, -1 });
        QVERIFY(! behavior.expanded());
        wheel({ 0, 40 });
        QVERIFY(behavior.expanded());
        QCOMPARE(outer->contentY(), 0.);
        wheel({}, Qt::ScrollEnd);
    }
    void smoothWheelRemainder() {
        inner->setContentY(390);
        QWheelEvent event({ 100, 100 },
                          window.mapToGlobal(QPoint(100, 100)),
                          {},
                          { 0, -120 },
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::NoScrollPhase,
                          false);
        QCoreApplication::sendEvent(&window, &event);
        const qreal expected = 24 * qGuiApp->styleHints()->wheelScrollLines() - 10;
        QTRY_VERIFY(qFuzzyCompare(outer->contentY(), expected));
        QCOMPARE(inner->contentY(), 400);
        QTRY_VERIFY(! inner->isMoving() && ! outer->isMoving());
    }
    void marginsAndOrigin() {
        inner->setTopMargin(20);
        inner->setContentY(-10);
        outer->setTopMargin(100);
        wheel({ 0, 20 }, Qt::ScrollBegin);
        QCOMPARE(inner->contentY(), -20);
        QCOMPARE(outer->contentY(), -10);
        wheel({}, Qt::ScrollEnd);
    }
    void dragRemainderAndReverse() {
        inner->setContentY(390);
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QCOMPARE(inner->contentY(), 400);
        QCOMPARE(outer->contentY(), 30);
        mouse(QEvent::MouseMove, { 100, 130 }, 1040);
        QCOMPARE(inner->contentY(), 380);
        QCOMPARE(outer->contentY(), 30);
        mouse(QEvent::MouseButtonRelease, { 100, 130 }, 1240);
        QVERIFY(! inner->isDragging());
        QVERIFY(! outer->isDragging());
    }
    void transformedDrag() {
        inner->setScale(2);
        inner->setTransformOrigin(QQuickItem::TopLeft);
        inner->setContentY(390);
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QCOMPARE(inner->contentY(), 400);
        QCOMPARE(outer->contentY(), 20);
        mouse(QEvent::MouseMove, { 100, 130 }, 1040);
        QCOMPARE(inner->contentY(), 390);
        QCOMPARE(outer->contentY(), 20);
        mouse(QEvent::MouseButtonRelease, { 100, 130 }, 1240);
    }
    void clickStillWorks() {
        mouse(QEvent::MouseButtonPress, { 100, 125 }, 1000);
        mouse(QEvent::MouseButtonRelease, { 100, 125 }, 1020);
        QCOMPARE(root->property("clicks").toInt(), 1);
        QCOMPARE(inner->contentY(), 0);
        QCOMPARE(outer->contentY(), 0);
    }
    void disabledDuringDrag() {
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QVERIFY(inner->isDragging());
        inner->setInteractive(false);
        QVERIFY(! inner->isDragging());
        QVERIFY(! outer->isDragging());
        const auto position = inner->contentY();
        mouse(QEvent::MouseMove, { 100, 80 }, 1040);
        mouse(QEvent::MouseButtonRelease, { 100, 80 }, 1240);
        QCOMPARE(inner->contentY(), position);
        QCOMPARE(outer->contentY(), 0);
    }
    void childKeepsGrab() {
        root->setProperty("blockDrag", true);
        mouse(QEvent::MouseButtonPress, { 100, 125 }, 1000);
        mouse(QEvent::MouseMove, { 100, 85 }, 1020);
        QCOMPARE(inner->contentY(), 0);
        QCOMPARE(outer->contentY(), 0);
        mouse(QEvent::MouseButtonRelease, { 100, 85 }, 1240);
    }
    void delayedClick() {
        inner->setPressDelay(1000);
        mouse(QEvent::MouseButtonPress, { 100, 125 }, 1000);
        mouse(QEvent::MouseButtonRelease, { 100, 125 }, 1020);
        QCOMPARE(root->property("clicks").toInt(), 1);
    }
    void directPositionStopsSession() {
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        inner->setContentY(100);
        QVERIFY(! inner->isDragging());
        QVERIFY(! outer->isDragging());
        mouse(QEvent::MouseMove, { 100, 80 }, 1040);
        mouse(QEvent::MouseButtonRelease, { 100, 80 }, 1240);
        QCOMPARE(inner->contentY(), 100);
    }
    void touchRemainder() {
        static auto* device = QTest::createTouchDevice();
        inner->setContentY(390);
        QTest::touchEvent(&window, device).press(0, { 100, 150 }).commit();
        QTest::touchEvent(&window, device).move(0, { 100, 110 }).commit();
        QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
            &window);
        QCOMPARE(inner->contentY(), 400);
        QCOMPARE(outer->contentY(), 30);
        inner->setInteractive(false);
        QTest::touchEvent(&window, device).release(0, { 100, 110 }).commit();
        QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
            &window);
        QVERIFY(! inner->isDragging());
        QVERIFY(! outer->isDragging());
    }
    void threeOwnedViews() {
        inner->setVisible(false);
        Flickable middle(outer->contentItem());
        middle.setSize({ 300, 200 });
        middle.setContentHeight(400);
        middle.setFlickableDirection(Flickable::VerticalFlick);
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&middle, true));
        config->setEnabled(true);
        Flickable leaf(middle.contentItem());
        leaf.setSize({ 300, 200 });
        leaf.setContentHeight(400);
        leaf.setFlickableDirection(Flickable::VerticalFlick);
        qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&leaf, true))
            ->setEnabled(true);
        wheel({ 0, -450 }, Qt::ScrollBegin);
        QCOMPARE(leaf.contentY(), 200);
        QCOMPARE(middle.contentY(), 200);
        QCOMPARE(outer->contentY(), 50);
        wheel({}, Qt::ScrollEnd);
        QVERIFY(! leaf.isMoving());
        QVERIFY(! middle.isMoving());
        QVERIFY(! outer->isMoving());
    }
    void crossAxis() {
        outer->setContentWidth(600);
        outer->setFlickableDirection(Flickable::HorizontalFlick);
        wheel({ -30, -40 }, Qt::ScrollBegin);
        QCOMPARE(inner->contentY(), 40);
        QCOMPARE(inner->contentX(), 0);
        QCOMPARE(outer->contentX(), 30);
        QCOMPARE(outer->contentY(), 0);
        wheel({}, Qt::ScrollEnd);
    }
    void reparentCancels() {
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QVERIFY(inner->isDragging());
        inner->setParentItem(window.contentItem());
        QVERIFY(! inner->isDragging());
        QVERIFY(! outer->isDragging());
        mouse(QEvent::MouseButtonRelease, { 100, 110 }, 1240);
    }
    void reparentDuringConsumption() {
        connect(inner, &QQuickFlickable::contentYChanged, inner, [this] {
            inner->setParentItem(window.contentItem());
        });
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QVERIFY(! inner->isDragging());
        QVERIFY(! outer->isDragging());
        mouse(QEvent::MouseButtonRelease, { 100, 110 }, 1240);
    }
    void destructionDuringDrag() {
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QVERIFY(inner->isDragging());
        root.reset();
        mouse(QEvent::MouseButtonRelease, { 100, 110 }, 1240);
        QVERIFY(! window.mouseGrabberItem());
    }
    void flingContinuesAcrossBoundary() {
        inner->setContentY(330);
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        mouse(QEvent::MouseMove, { 100, 90 }, 1040);
        QCOMPARE(inner->contentY(), 390);
        mouse(QEvent::MouseButtonRelease, { 100, 90 }, 1050);
        QTRY_VERIFY(outer->contentY() > 0);
        QCOMPARE(inner->contentY(), 400);
        inner->setInteractive(false);
        QVERIFY(! inner->isMoving());
        QVERIFY(! outer->isMoving());
    }
    void qtAncestorDrag() {
        root->setParentItem(nullptr);
        QQuickFlickable qtOuter(window.contentItem());
        qtOuter.setSize({ 300, 300 });
        qtOuter.setContentHeight(900);
        qtOuter.setFlickableDirection(QQuickFlickable::VerticalFlick);
        qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&qtOuter, true))
            ->setEnabled(true);
        inner->setParentItem(qtOuter.contentItem());
        inner->setContentY(390);
        mouse(QEvent::MouseButtonPress, { 100, 150 }, 1000);
        mouse(QEvent::MouseMove, { 100, 110 }, 1020);
        QCOMPARE(inner->contentY(), 400);
        QCOMPARE(qtOuter.contentY(), 30);
        mouse(QEvent::MouseMove, { 100, 130 }, 1040);
        QCOMPARE(inner->contentY(), 380);
        QCOMPARE(qtOuter.contentY(), 30);
        mouse(QEvent::MouseButtonRelease, { 100, 130 }, 1240);
    }
    void sheetRelease_data() {
        QTest::addColumn<bool>("modal");
        QTest::addColumn<bool>("nested");
        QTest::addColumn<QString>("gesture");
        QTest::addColumn<bool>("dismiss");
        for (bool modal : { true, false }) {
            for (bool nested : { false, true }) {
                for (const auto& gesture : { "fast",
                                             "paused",
                                             "rest",
                                             "reverse",
                                             "unstable",
                                             "distance",
                                             "cancel",
                                             "threshold" }) {
                    const auto name = QByteArray(modal ? "modal-" : "standard-") +
                                      (nested ? "nested-" : "direct-") + gesture;
                    QTest::newRow(name.constData())
                        << modal << nested << QString::fromLatin1(gesture)
                        << (QByteArray(gesture) == "fast" || QByteArray(gesture) == "distance" ||
                            QByteArray(gesture) == "threshold");
                }
            }
        }
    }
    void sheetRelease() {
        QFETCH(bool, modal);
        QFETCH(bool, nested);
        QFETCH(QString, gesture);
        QFETCH(bool, dismiss);
        root.reset();
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: page; width: 300; height: 500
    property MD.BottomSheet sheet: MD.BottomSheet {
        parent: page
        property bool reducedThreshold: false
        dragDismissThreshold: _collapsedHeight * (reducedThreshold ? 0.125 : 0.25)
        animationDuration: 0
        preferredContentHeight: 300
        nestedScrollEnabled: true
        MD.ListView {
            MD.NestedScroll.enabled: true
            objectName: "sheetList"
            width: page.sheet.contentViewportWidth
            height: page.sheet.contentViewportHeight
            model: 20
            delegate: Rectangle { width: 300; height: 50 }
        }
    }
})",
                          QUrl("qrc:/sheet-release-test.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY(root);
        window.resize(300, 500);
        root->setParentItem(window.contentItem());
        auto* sheet = qobject_cast<Popup*>(root->property("sheet").value<QObject*>());
        QVERIFY(sheet);
        QVERIFY(sheet->setProperty("sheetType",
                                   int(modal ? Enum::BottomSheetType::BottomSheetModal
                                             : Enum::BottomSheetType::BottomSheetStandard)));
        QVERIFY(sheet->property("dismissOnDragDown").toBool());
        QCOMPARE(sheet->modal(), modal);
        if (! modal) QVERIFY(sheet->setProperty("lowHeight", 176));
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        auto* list   = sheet->findChild<QQuickFlickable*>("sheetList");
        auto* scroll = sheet->findChild<Flickable*>();
        QVERIFY(list && scroll);
        QVERIFY(QMetaObject::invokeMethod(list, "forceLayout"));
        if (gesture == "threshold") sheet->setProperty("reducedThreshold", true);
        QSignalSpy frames(&window, &QQuickWindow::afterAnimating);
        window.update();
        QTRY_VERIFY(! frames.isEmpty());
        const auto start = list->mapToScene({ 100, nested ? 40.0 : -24.0 });
        QSignalSpy released(scroll, &Flickable::dragReleased);
        mouse(QEvent::MouseButtonPress, start, 1000);
        const bool far   = gesture == "reverse" || gesture == "distance" || gesture == "cancel";
        auto       moved = start + QPointF(0, far ? 120 : gesture == "threshold" ? 60 : 30);
        mouse(QEvent::MouseMove, moved, 1020);
        QVERIFY(scroll->isDragging());
        if (gesture == "threshold") {
            QCOMPARE(sheet->property("dragDismissThreshold").toReal(),
                     sheet->property("_collapsedHeight").toReal() * 0.125);
            QCOMPARE(scroll->contentY(), -60);
        }
        QVERIFY(scroll->dragVelocity().y() < 0);
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), far || gesture == "threshold" ? 0 : 1);
        if (gesture == "rest") {
            QTRY_COMPARE(scroll->dragVelocity(), QPointF());
            QCOMPARE(sheet->property("_scrimOpacity").toReal(), 1);
            QVERIFY(scroll->isDragging());
        }
        if (gesture == "reverse") {
            moved -= QPointF(0, 10);
            mouse(QEvent::MouseMove, moved, 1040);
            QVERIFY(scroll->dragVelocity().y() > 0);
            QVERIFY(-scroll->contentY() > sheet->property("dragDismissThreshold").toReal());
            QCOMPARE(sheet->property("_scrimOpacity").toReal(), 0);
        }
        if (gesture == "unstable") {
            ulong timestamp = 1040;
            for (qreal distance : { 25, 35, 30 }) {
                moved = start + QPointF(0, distance);
                mouse(QEvent::MouseMove, moved, timestamp);
                timestamp += 20;
                QVERIFY(distance == 35 ? scroll->dragVelocity().y() < 0
                                       : scroll->dragVelocity().y() > 0);
                QCOMPARE(sheet->property("_scrimOpacity").toReal(), 1);
            }
        }
        if (gesture == "cancel") {
            scroll->setInteractive(false);
            QCOMPARE(released.size(), 0);
        } else {
            const bool pause = gesture == "paused" || gesture == "rest" || gesture == "distance" ||
                               gesture == "threshold";
            mouse(QEvent::MouseButtonRelease, moved, pause ? 1240 : 1100);
            QCOMPARE(released.size(), 1);
            const auto velocity = released.first().first().toPointF();
            if (pause)
                QCOMPARE(velocity, QPointF());
            else if (gesture == "reverse" || gesture == "unstable")
                QVERIFY(velocity.y() > 0);
            else
                QVERIFY(velocity.y() < 0);
        }
        if (dismiss)
            QTRY_VERIFY(! sheet->isVisible());
        else {
            QTRY_COMPARE(scroll->contentY(), 0);
            QVERIFY(sheet->isOpened());
            QCOMPARE(sheet->property("_scrimOpacity").toReal(), 1);
        }
        QCOMPARE(scroll->dragVelocity(), QPointF());
    }
    void standardSheetDragDismissDisabled() {
        root.reset();
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: page; width: 300; height: 500
    property MD.BottomSheet sheet: MD.BottomSheet {
        parent: page
        sheetType: MD.Enum.BottomSheetStandard
        dismissOnDragDown: false
        animationDuration: 0
        preferredContentHeight: 300
        collapsedHeight: 200
        Item { width: page.sheet.contentViewportWidth; height: 300 }
    }
})",
                          QUrl("qrc:/standard-sheet-drag-disabled.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY(root);
        window.resize(300, 500);
        root->setParentItem(window.contentItem());
        auto* sheet = qobject_cast<Popup*>(root->property("sheet").value<QObject*>());
        QVERIFY(sheet);
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        auto* scroll = sheet->findChild<Flickable*>();
        QVERIFY(scroll);
        QSignalSpy frames(&window, &QQuickWindow::afterAnimating);
        window.update();
        QTRY_VERIFY(! frames.isEmpty());
        QSignalSpy released(scroll, &Flickable::dragReleased);
        QSignalSpy closed(sheet, &Popup::closed);
        const auto start = sheet->popupItem()->mapToScene({ 100, 24 });
        mouse(QEvent::MouseButtonPress, start, 1000);
        mouse(QEvent::MouseMove, start + QPointF(0, 120), 1020);
        QVERIFY(scroll->isDragging());
        QVERIFY(scroll->contentY() < 0);
        mouse(QEvent::MouseButtonRelease, start + QPointF(0, 120), 1240);
        QCOMPARE(released.size(), 1);
        QTRY_VERIFY(! scroll->isMoving());
        QVERIFY(sheet->isOpened());
        QVERIFY(! sheet->closing());
        QCOMPARE(closed.size(), 0);
        QCOMPARE(sheet->property("_dismissDistance").toReal(), 0);
    }
    void sheetListPriority_data() {
        QTest::addColumn<bool>("modal");
        QTest::newRow("modal") << true;
        QTest::newRow("standard") << false;
    }
    void sheetListPriority() {
        QFETCH(bool, modal);
        root.reset();
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: page; width: 300; height: 500
    property MD.BottomSheet sheet: MD.BottomSheet {
        parent: page
        animationDuration: 0
        preferredContentHeight: 300
        nestedScrollEnabled: true
        MD.ListView {
            MD.NestedScroll.enabled: true
            objectName: "sheetList"
            width: page.sheet.contentViewportWidth
            height: page.sheet.contentViewportHeight
            model: 20
            delegate: Rectangle { width: 300; height: 50 }
        }
    }
})",
                          QUrl("qrc:/nested-sheet-test.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY(root);
        window.resize(300, 500);
        root->setParentItem(window.contentItem());
        auto* sheet = qobject_cast<Popup*>(root->property("sheet").value<QObject*>());
        QVERIFY(sheet);
        QVERIFY(sheet->setProperty("sheetType",
                                   int(modal ? Enum::BottomSheetType::BottomSheetModal
                                             : Enum::BottomSheetType::BottomSheetStandard)));
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        auto* list   = sheet->findChild<QQuickFlickable*>("sheetList");
        auto* scroll = sheet->findChild<Flickable*>();
        QVERIFY(list && scroll);
        QQuickNumberAnimation* dragScrim = nullptr;
        for (auto* animation : sheet->findChildren<QQuickNumberAnimation*>()) {
            if (! animation->group() && animation->property() == QStringLiteral("_scrimOpacity"))
                dragScrim = animation;
        }
        QVERIFY(dragScrim);
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 1);
        QVERIFY(QMetaObject::invokeMethod(list, "forceLayout"));
        const auto  wheelPosition = list->mapToScene({ 100, 100 });
        QWheelEvent down(wheelPosition,
                         window.mapToGlobal(wheelPosition),
                         { 0, 30 },
                         { 0, 30 },
                         Qt::NoButton,
                         Qt::NoModifier,
                         Qt::ScrollBegin,
                         false);
        QCoreApplication::sendEvent(&window, &down);
        QCOMPARE(list->contentY(), 0);
        QCOMPARE(scroll->contentY(), 0);
        QWheelEvent end(wheelPosition,
                        window.mapToGlobal(wheelPosition),
                        {},
                        {},
                        Qt::NoButton,
                        Qt::NoModifier,
                        Qt::ScrollEnd,
                        false);
        QCoreApplication::sendEvent(&window, &end);
        QCOMPARE(scroll->contentY(), 0);
        QVERIFY(sheet->isOpened());
        QWheelEvent notch(wheelPosition,
                          window.mapToGlobal(wheelPosition),
                          {},
                          { 0, 120 },
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::NoScrollPhase,
                          false);
        QCoreApplication::sendEvent(&window, &notch);
        QTRY_VERIFY(! list->isMoving());
        QCOMPARE(scroll->contentY(), 0);
        sheet->setProperty("animationDuration", 40);
        list->setContentY(50);
        const QPointF start = list->mapToScene({ 100, 100 });
        mouse(QEvent::MouseButtonPress, start, 1000);
        mouse(QEvent::MouseMove, start + QPointF(0, 30), 1020);
        QCOMPARE(list->contentY(), 20);
        QCOMPARE(scroll->contentY(), 0);
        mouse(QEvent::MouseMove, start + QPointF(0, 60), 1040);
        QCOMPARE(list->contentY(), 0);
        QCOMPARE(scroll->contentY(), -10);
        mouse(QEvent::MouseMove, start + QPointF(0, 160), 1060);
        QCOMPARE(scroll->contentY(), -110);
        QVERIFY(sheet->isOpened());
        QVERIFY(dragScrim->isRunning());
        QCOMPARE(dragScrim->to(), 0);
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 1);
        dragScrim->complete();
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 0);
        QCOMPARE(sheet->modal(), modal);
        QCOMPARE(sheet->dim(), modal);
        mouse(QEvent::MouseMove, start + QPointF(0, 150), 1070);
        QCOMPARE(scroll->contentY(), -100);
        QVERIFY(scroll->dragVelocity().y() > 0);
        QVERIFY(! dragScrim->isRunning());
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 0);
        mouse(QEvent::MouseMove, start + QPointF(0, 70), 1080);
        QCOMPARE(scroll->contentY(), -20);
        QCOMPARE(list->contentY(), 0);
        QVERIFY(sheet->isOpened());
        QVERIFY(dragScrim->isRunning());
        QCOMPARE(dragScrim->to(), 1);
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 0);
        dragScrim->complete();
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 1);
        mouse(QEvent::MouseMove, start + QPointF(0, 40), 1100);
        QCOMPARE(scroll->contentY(), 0);
        QCOMPARE(list->contentY(), 10);
        mouse(QEvent::MouseMove, start + QPointF(0, 60), 1120);
        QCOMPARE(scroll->contentY(), -10);
        mouse(QEvent::MouseButtonRelease, start + QPointF(0, 60), 1240);
        QTRY_VERIFY(qFuzzyIsNull(scroll->contentY()));
        QVERIFY(sheet->isOpened());
        list->setContentY(50);
        const auto handle = list->mapToScene({ 100, -24 });
        mouse(QEvent::MouseButtonPress, handle, 1500);
        mouse(QEvent::MouseMove, handle + QPointF(0, 30), 1520);
        QVERIFY(scroll->contentY() < 0);
        mouse(QEvent::MouseButtonRelease, handle + QPointF(0, 30), 1740);
        QTRY_VERIFY(qFuzzyIsNull(scroll->contentY()));
        QVERIFY(sheet->isOpened());
        mouse(QEvent::MouseButtonPress, handle, 2000);
        mouse(QEvent::MouseMove, handle + QPointF(0, 120), 2020);
        QCOMPARE(list->contentY(), 50);
        QVERIFY(scroll->contentY() < 0);
        QVERIFY(dragScrim->isRunning());
        QCOMPARE(dragScrim->to(), 0);
        dragScrim->complete();
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 0);
        mouse(QEvent::MouseButtonRelease, handle + QPointF(0, 120), 2240);
        QTRY_VERIFY(! sheet->isVisible());
        QVERIFY(! dragScrim->isRunning());
        sheet->setProperty("animationDuration", 0);
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 1);
    }
};

int run_nested_scroll(int argc, char** argv) {
    NestedScrollTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "nested_scroll.moc"
