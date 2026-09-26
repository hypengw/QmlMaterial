#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "qml_material/control/popup.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStyleHints>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtTest>

using namespace qml_material;

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
    void sheetListPriority() {
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
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        auto* list   = sheet->findChild<QQuickFlickable*>("sheetList");
        auto* scroll = sheet->findChild<Flickable*>();
        QVERIFY(list && scroll);
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
        mouse(QEvent::MouseMove, start + QPointF(0, 70), 1080);
        QCOMPARE(scroll->contentY(), -20);
        QCOMPARE(list->contentY(), 0);
        QVERIFY(sheet->isOpened());
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
        mouse(QEvent::MouseButtonRelease, handle + QPointF(0, 120), 2240);
        QTRY_VERIFY(! sheet->isVisible());
    }
};

int run_nested_scroll(int argc, char** argv) {
    NestedScrollTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "nested_scroll.moc"
