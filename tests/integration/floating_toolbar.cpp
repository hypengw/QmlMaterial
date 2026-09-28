#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include "qml_material/control/control.hpp"
#include "qml_material/input/floating_toolbar_scroll.hpp"
#include "qml_material/input/floating_toolbar_exit.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include <limits>

class FloatingToolbarTest : public QObject {
    Q_OBJECT
    QQmlEngine m_engine;
private slots:
    void initTestCase() { m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH)); }
    void scrollThresholds() {
        qml_material::FloatingToolbarScroll behavior;
        QSignalSpy collapse(&behavior, &qml_material::FloatingToolbarScroll::collapseRequested);
        QSignalSpy expand(&behavior, &qml_material::FloatingToolbarScroll::expandRequested);
        connect(&behavior, &qml_material::FloatingToolbarScroll::collapseRequested, &behavior, [&] {
            behavior.setExpanded(false);
        });
        connect(&behavior, &qml_material::FloatingToolbarScroll::expandRequested, &behavior, [&] {
            behavior.setExpanded(true);
        });
        behavior.scrollBy({ 100, 0 });
        behavior.scrollBy({ 0, 39 });
        QCOMPARE(collapse.size(), 0);
        behavior.scrollBy({ 0, 1 });
        QCOMPARE(collapse.size(), 1);
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, 300 });
        QCOMPARE(collapse.size(), 1);
        behavior.scrollBy({ 0, -20 });
        behavior.scrollBy({ 0, 10 });
        behavior.scrollBy({ 0, -39 });
        QCOMPARE(expand.size(), 0);
        behavior.scrollBy({ 0, -1 });
        QCOMPARE(expand.size(), 1);
        behavior.setCollapseScrollThreshold(60);
        behavior.setExpandScrollThreshold(20);
        behavior.scrollBy({ 0, 59 });
        behavior.setEnabled(false);
        behavior.scrollBy({ 0, 100 });
        behavior.setEnabled(true);
        behavior.scrollBy({ 0, 59 });
        QVERIFY(behavior.expanded());
        behavior.scrollBy({ 0, 1 });
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, -20 });
        QVERIFY(behavior.expanded());
        behavior.setReverseLayout(true);
        behavior.scrollBy({ 0, -60 });
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, 20 });
        QVERIFY(behavior.expanded());
        behavior.scrollBy({ 0, -59 });
        behavior.reset();
        behavior.scrollBy({ 0, -1 });
        QVERIFY(behavior.expanded());
        behavior.setExpanded(false);
        behavior.scrollBy({ 0, 19 });
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, std::numeric_limits<qreal>::quiet_NaN() });
        behavior.setExpandScrollThreshold(-1);
        QCOMPARE(behavior.expandScrollThreshold(), 20.);
        behavior.scrollBy({ 0, 1 });
        QVERIFY(behavior.expanded());
    }
    void exitState() {
        qml_material::FloatingToolbarExit state;
        state.setDistance(100);
        state.scrollBy({ 0, 49 });
        QVERIFY(state.active());
        state.settle();
        QCOMPARE(state.offset(), 0.);
        QVERIFY(! state.active());
        state.scrollBy({ 0, 50 });
        state.settle();
        QCOMPARE(state.offset(), 100.);
        state.setDistance(200);
        QCOMPARE(state.offset(), 200.);
        state.scrollBy({ 0, -100 });
        QCOMPARE(state.offset(), 100.);
        state.setDistance(100);
        QCOMPARE(state.offset(), 50.);
        state.setEnabled(false);
        QCOMPARE(state.offset(), 0.);
        state.scrollBy({ 0, 100 });
        QVERIFY(! state.active());
        state.setEnabled(true);
        state.setReverseLayout(true);
        state.scrollBy({ 0, -30 });
        QCOMPARE(state.offset(), 30.);
        state.scrollBy({ 100, 0 });
        state.scrollBy({ 0, std::numeric_limits<qreal>::quiet_NaN() });
        QCOMPARE(state.offset(), 30.);
        state.reset();
        QCOMPARE(state.offset(), 0.);
    }
    void exitFling() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    exitBehavior: MD.FloatingToolbarExit { distance: 100 }
    mainContent: Item { implicitWidth: 48; implicitHeight: 48 }
})",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* state =
            qvariant_cast<qml_material::FloatingToolbarExit*>(object->property("exitBehavior"));
        QVERIFY(state);
        QQuickNumberAnimation* fling = nullptr;
        for (auto* animation : object->findChildren<QQuickNumberAnimation*>())
            if (animation->property() == "__flingOffset") fling = animation;
        QVERIFY(fling);
        state->begin();
        state->setOffset(30);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "__releaseExit", Q_ARG(QVariant, 300.)));
        QVERIFY(fling->isRunning());
        QCOMPARE(fling->duration(), 200);
        fling->setCurrentTime(100);
        QCOMPARE(state->offset(), 52.5);
        QCOMPARE(object->property("presentedExitOffset").toReal(), 52.5);
        state->begin();
        QVERIFY(! fling->isRunning());
        QCOMPARE(state->offset(), 52.5);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "__releaseExit", Q_ARG(QVariant, 1000.)));
        fling->setCurrentTime(200);
        QVERIFY(! fling->isRunning());
        QCOMPARE(state->offset(), 100.);
        QVERIFY(! state->active());
        state->begin();
        state->setOffset(30);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "__releaseExit", Q_ARG(QVariant, -150.)));
        fling->setCurrentTime(fling->duration());
        QTRY_VERIFY(! object->property("exitTransitioning").toBool());
        QCOMPARE(state->offset(), 0.);
        state->begin();
        state->setOffset(30);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "__releaseExit", Q_ARG(QVariant, 300.)));
        state->setEnabled(false);
        QVERIFY(! fling->isRunning());
        QCOMPARE(object->property("presentedExitOffset").toReal(), 0.);
    }
    void exitDrag_data() {
        QTest::addColumn<int>("edge");
        QTest::addColumn<bool>("touch");
        for (bool touch : { false, true }) {
            QTest::newRow(touch ? "touch-left" : "mouse-left") << int(Qt::LeftEdge) << touch;
            QTest::newRow(touch ? "touch-right" : "mouse-right") << int(Qt::RightEdge) << touch;
            QTest::newRow(touch ? "touch-top" : "mouse-top") << int(Qt::TopEdge) << touch;
            QTest::newRow(touch ? "touch-bottom" : "mouse-bottom") << int(Qt::BottomEdge) << touch;
        }
    }
    void exitDrag() {
        QFETCH(int, edge);
        QFETCH(bool, touch);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    id: bar; x: 100; y: 100
    property int clicks: 0
    animationsEnabled: false
    exitBehavior: MD.FloatingToolbarExit { distance: 200 }
    mainContent: MD.IconButton { icon.name: "edit"; onClicked: bar.clicks++ }
})",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<QQuickItem*>(object.get());
        bar->setProperty("exitEdge", edge);
        auto* state =
            qvariant_cast<qml_material::FloatingToolbarExit*>(bar->property("exitBehavior"));
        auto* handler = bar->findChild<QQuickDragHandler*>();
        QVERIFY(state && handler);
        QQuickWindow window;
        window.resize(500, 500);
        bar->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QPointF start(132, 132);
        const QPointF direction = edge == Qt::LeftEdge    ? QPointF(-1, 0)
                                  : edge == Qt::RightEdge ? QPointF(1, 0)
                                  : edge == Qt::TopEdge   ? QPointF(0, -1)
                                                          : QPointF(0, 1);
        auto          mouse     = [&](QEvent::Type type, QPointF position, ulong time) {
            if (touch) {
                static auto* device   = QTest::createTouchDevice();
                auto         sequence = QTest::touchEvent(&window, device);
                if (type == QEvent::MouseButtonPress)
                    sequence.press(0, position.toPoint());
                else if (type == QEvent::MouseButtonRelease)
                    sequence.release(0, position.toPoint());
                else
                    sequence.move(0, position.toPoint());
                sequence.commit();
                QQuickWindowPrivate::get(&window)
                    ->deliveryAgentPrivate()
                    ->flushFrameSynchronousEvents(&window);
                return;
            }
            QMouseEvent event(type,
                              position,
                              window.mapToGlobal(position.toPoint()),
                              type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                              type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                              Qt::NoModifier);
            event.setTimestamp(time);
            QCoreApplication::sendEvent(&window, &event);
        };
        mouse(QEvent::MouseButtonPress, start, 1000);
        mouse(QEvent::MouseButtonRelease, start, 1020);
        QCOMPARE(bar->property("clicks").toInt(), 1);
        mouse(QEvent::MouseButtonPress, start, 1100);
        mouse(QEvent::MouseMove, start + direction * 40, 1120);
        mouse(QEvent::MouseMove, start + direction * 60, 1140);
        QVERIFY(handler->active());
        QVERIFY(state->offset() > 0);
        const auto previous = state->offset();
        mouse(QEvent::MouseMove, start + direction * 40, 1160);
        QCOMPARE(state->offset(), previous - 20);
        mouse(QEvent::MouseButtonRelease, start + direction * 40, 1400);
        QVERIFY(! handler->active());
        QVERIFY(! state->active());
        QCOMPARE(state->offset(), 0.);
        QCOMPARE(bar->property("clicks").toInt(), 1);
        mouse(QEvent::MouseButtonPress, start, 1500);
        mouse(QEvent::MouseMove, start + direction * 40, 1520);
        mouse(QEvent::MouseMove, start + direction * 60, 1540);
        bar->setProperty("dragToHide", false);
        QVERIFY(! state->active());
        QCOMPARE(state->offset(), 0.);
        mouse(QEvent::MouseButtonRelease, start + direction * 60, 1800);
        bar->setParentItem(nullptr);
    }
    void exitPresentation_data() {
        QTest::addColumn<int>("edge");
        QTest::newRow("left") << int(Qt::LeftEdge);
        QTest::newRow("right") << int(Qt::RightEdge);
        QTest::newRow("top") << int(Qt::TopEdge);
        QTest::newRow("bottom") << int(Qt::BottomEdge);
    }
    void exitInitiallyHidden() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    exitBehavior: MD.FloatingToolbarExit { distance: 100; offset: 100 }
    mainContent: Item { implicitWidth: 48; implicitHeight: 48 }
})",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("presentedExitOffset").toReal(), 100.);
        QVERIFY(! object->property("exitTransitioning").toBool());
        object->setProperty("exitBehavior",
                            QVariant::fromValue<qml_material::FloatingToolbarExit*>(nullptr));
        QCOMPARE(object->property("presentedExitOffset").toReal(), 0.);
        QVERIFY(! object->property("exitTransitioning").toBool());
    }
    void exitPresentation() {
        QFETCH(int, edge);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    width: 300; height: 300
    MD.FloatingToolbar {
        id: bar; objectName: "bar"
        x: parent.width / 10; y: parent.height / 10
        animationsEnabled: false
        exitBehavior: MD.FloatingToolbarExit { distance: bar.exitDistance }
        mainContent: Item { implicitWidth: 48; implicitHeight: 48 }
    }
})",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = object->findChild<QQuickItem*>("bar");
        QVERIFY(bar);
        bar->setProperty("exitEdge", edge);
        auto* state =
            qvariant_cast<qml_material::FloatingToolbarExit*>(bar->property("exitBehavior"));
        QVERIFY(state);
        const bool negative   = edge == Qt::LeftEdge || edge == Qt::TopEdge;
        const bool horizontal = edge == Qt::LeftEdge || edge == Qt::RightEdge;
        QCOMPARE(state->distance(), negative ? 94. : 270.);
        state->scrollBy({ 0, state->distance() / 2 });
        auto point = bar->mapToItem(bar->parentItem(), QPointF());
        QCOMPARE(horizontal ? point.x() : point.y(), 30 + (negative ? -1 : 1) * state->offset());
        QCOMPARE(bar->position(), QPointF(30, 30));
        state->settle();
        QCOMPARE(state->offset(), state->distance());
        object->setProperty("width", 400);
        QCOMPARE(bar->x(), 40.);
        QCOMPARE(state->offset(), state->distance());
        bar->setProperty("animationsEnabled", true);
        state->reset();
        // Seek the actual animation rather than guessing a frame with a timed wait.
        QQuickNumberAnimation* animation = nullptr;
        for (auto* candidate : bar->findChildren<QQuickNumberAnimation*>())
            if (candidate->property() == "__exitOffset") animation = candidate;
        QVERIFY(animation);
        animation->setCurrentTime(bar->property("duration").toInt() / 2);
        const auto presented = bar->property("presentedExitOffset").toReal();
        state->begin();
        QCOMPARE(state->offset(), presented);
        QVERIFY(! bar->property("exitTransitioning").toBool());
        state->scrollBy({ 0, -10 });
        QCOMPARE(bar->property("presentedExitOffset").toReal(), std::max(0., presented - 10));
        state->settle();
        QTRY_VERIFY(! bar->property("exitTransitioning").toBool());
        QCOMPARE(bar->property("presentedExitOffset").toReal(), state->offset());
        state->setEnabled(false);
        QCOMPARE(bar->mapToItem(bar->parentItem(), QPointF()), bar->position());
    }
    void geometry_data() {
        QTest::addColumn<bool>("vertical");
        QTest::addColumn<bool>("mirrored");
        QTest::newRow("horizontal") << false << false;
        QTest::newRow("rtl") << false << true;
        QTest::newRow("vertical") << true << false;
    }
    void scrollBinding() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    id: bar
    animationsEnabled: false
    property MD.FloatingToolbarScroll behavior: MD.FloatingToolbarScroll {
        expanded: bar.expanded
        onCollapseRequested: bar.expanded = false
        onExpandRequested: bar.expanded = true
    }
    property MD.Scrollable scrollSource: MD.Scrollable {
        MD.NestedScroll.enabled: true
        MD.NestedScroll.onScrollConsumed: delta => bar.behavior.scrollBy(delta)
    }
    mainContent: Item { implicitWidth: 48; implicitHeight: 48 }
    trailingContent: Item { implicitWidth: 48; implicitHeight: 48 }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* behavior =
            qvariant_cast<qml_material::FloatingToolbarScroll*>(object->property("behavior"));
        QVERIFY(behavior);
        auto* source = qvariant_cast<QObject*>(object->property("scrollSource"));
        QVERIFY(source);
        auto* nested = qobject_cast<qml_material::NestedScroll*>(
            qmlAttachedPropertiesObject<qml_material::NestedScroll>(source));
        QVERIFY(nested);
        emit nested->scrollConsumed({ 0, 40 });
        QVERIFY(! object->property("expanded").toBool());
        QVERIFY(! behavior->expanded());
        QCOMPARE(object->property("width").toReal(), 64.);
        object->setProperty("expanded", true);
        QVERIFY(behavior->expanded());
        behavior->scrollBy({ 0, 40 });
        QVERIFY(! behavior->expanded());
        behavior->scrollBy({ 0, -40 });
        QVERIFY(object->property("expanded").toBool());
        QCOMPARE(object->property("width").toReal(), 112.);
    }
    void geometry() {
        QFETCH(bool, vertical);
        QFETCH(bool, mirrored);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    animationsEnabled: false
    leadingContent: Item { implicitWidth: 48; implicitHeight: 48 }
    mainContent: Item { implicitWidth: 48; implicitHeight: 48 }
    trailingContent: Item { implicitWidth: 48; implicitHeight: 48 }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<qml_material::Control*>(object.get());
        QVERIFY(bar);
        bar->setProperty("orientation", vertical ? Qt::Vertical : Qt::Horizontal);
        bar->setLayoutDirection(mirrored ? Qt::RightToLeft : Qt::LeftToRight);
        auto* main     = qvariant_cast<QQuickItem*>(bar->property("mainContent"));
        auto* leading  = qvariant_cast<QQuickItem*>(bar->property("leadingContent"));
        auto* trailing = qvariant_cast<QQuickItem*>(bar->property("trailingContent"));
        QVERIFY(main && leading && trailing);
        QCOMPARE(bar->size(), vertical ? QSizeF(64, 160) : QSizeF(160, 64));
        auto position = [&](QQuickItem* item) {
            const auto point = item->mapToItem(bar, QPointF());
            return vertical ? point.y() : point.x();
        };
        QCOMPARE(position(main), 56.);
        QCOMPARE(position(leading), mirrored ? 104. : 8.);
        QCOMPARE(position(trailing), mirrored ? 8. : 104.);
        bar->setProperty("expanded", false);
        QCOMPARE(bar->size(), QSizeF(64, 64));
        QCOMPARE(main->size(), QSizeF(48, 48));
        QVERIFY(! leading->isVisible());
        QVERIFY(! leading->isEnabled());
        QVERIFY(main->isVisible());
        bar->setProperty("expanded", true);
        bar->setProperty("expansionProgress", .5);
        QCOMPARE(bar->size(), vertical ? QSizeF(64, 112) : QSizeF(112, 64));
        QCOMPARE(leading->size(), QSizeF(48, 48));
        QCOMPARE(main->size(), QSizeF(48, 48));
        leading->setImplicitWidth(72);
        leading->setImplicitHeight(72);
        QCOMPARE(vertical ? bar->height() : bar->width(), 124.);
        if (vertical)
            bar->setHeight(48);
        else
            bar->setWidth(48);
        QCOMPARE(vertical ? main->height() : main->width(), 32.);
        QVERIFY(position(main) >= 8.);
        QVERIFY(! leading->isVisible());
        QVERIFY(! trailing->isVisible());
        bar->setProperty("leadingContent", QVariant::fromValue<QQuickItem*>(nullptr));
        QVERIFY(! leading->parentItem());
        QCOMPARE(qvariant_cast<QQuickItem*>(bar->property("mainContent")), main);
        delete trailing;
        QVERIFY(! qvariant_cast<QQuickItem*>(bar->property("trailingContent")));
    }
    void actionToolbar() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    width: 112
    mainContent: MD.ActionToolBar {
        actions: [
            MD.Action { icon.name: "edit" },
            MD.Action { icon.name: "share" },
            MD.Action { icon.name: "download" }
        ]
    }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* main = qvariant_cast<QQuickItem*>(object->property("mainContent"));
        QVERIFY(main);
        auto polish = [](auto&& self, QQuickItem* item) -> void {
            item->ensurePolished();
            for (auto* child : item->childItems()) self(self, child);
        };
        polish(polish, qobject_cast<QQuickItem*>(object.get()));
        QCoreApplication::processEvents();
        QCOMPARE(main->width(), 96.);
        QVERIFY(main->property("maximumContentWidth").toReal() > main->width());
        QVERIFY(main->property("visibleWidth").toReal() <= main->width());
        object->setProperty("expanded", false);
        QCOMPARE(qvariant_cast<QQuickItem*>(object->property("mainContent")), main);
        object->setProperty("mainContent", QVariant::fromValue<QQuickItem*>(nullptr));
        object->setProperty("trailingContent", QVariant::fromValue(main));
        object->setProperty("animationsEnabled", false);
        object->setProperty("width", 300.);
        object->setProperty("expanded", true);
        polish(polish, qobject_cast<QQuickItem*>(object.get()));
        QTRY_VERIFY(main->isVisible());
        QVERIFY(main->implicitWidth() >= 144.);
        object->setProperty("expanded", false);
        QVERIFY(! main->isVisible());
        object->setProperty("expanded", true);
        polish(polish, qobject_cast<QQuickItem*>(object.get()));
        QVERIFY(main->isVisible());
        QVERIFY(main->implicitWidth() >= 144.);
    }

    void transitions() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    expanded: false
    mainContent: MD.IconButton { icon.name: "edit" }
    trailingContent: MD.IconButton { icon.name: "share" }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("expansionProgress").toReal(), 0.);
        QVERIFY(! object->property("transitioning").toBool());
        object->setProperty("expanded", true);
        object->setProperty("expanded", false);
        QTRY_VERIFY(! object->property("transitioning").toBool());
        QCOMPARE(object->property("expansionProgress").toReal(), 0.);
        object->setProperty("expanded", true);
        QTRY_VERIFY(! object->property("transitioning").toBool());
        QCOMPARE(object->property("expansionProgress").toReal(), 1.);
    }
};

int run_floating_toolbar(int argc, char** argv) {
    FloatingToolbarTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "floating_toolbar.moc"
