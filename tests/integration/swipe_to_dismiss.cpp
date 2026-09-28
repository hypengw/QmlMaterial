#include "qml_material/input/swipe_to_dismiss_state.hpp"
#include <QtTest>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <limits>

using namespace qml_material;
using Swipe = SwipeToDismissState;

class SwipeToDismissTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|TypeError|ReferenceError|Unable to assign).*"));
    }
    void nestedInput_data() {
        QTest::addColumn<QString>("viewType");
        QTest::addColumn<bool>("nested");
        QTest::addColumn<bool>("touch");
        for (const auto& viewType :
             { QString("Flickable"), QString("MD.Scrollable"), QString("ListView") })
            for (bool nested : { false, true })
                for (bool touch : { false, true })
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(viewType).arg(nested).arg(touch)))
                        << viewType << nested << touch;
    }
    void nestedInput() {
        QFETCH(QString, viewType);
        QFETCH(bool, nested);
        QFETCH(bool, touch);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        auto          source = QString(R"(
import QtQuick
import Qcm.Material as MD
%1 {
    width: 320; height: 400; contentHeight: 1000
    MD.NestedScroll.enabled: %2
    MD.NestedScroll.axes: Qt.Vertical
    MD.SwipeToDismiss {
        id: swipe; objectName: "swipe"; width: 300; height: 100; y: 100
        property int clicks: 0
        animationsEnabled: false
        MD.Button { width: 300; height: 100; text: "Click"; onClicked: swipe.clicks++ }
    }
})")
                                   .arg(viewType, nested ? "true" : "false");
        if (viewType == "ListView") {
            source.replace("MD.SwipeToDismiss {",
                           "model: 1\nheader: Item { height: 100 }\nfooter: Item { height: 800 "
                           "}\ndelegate: MD.SwipeToDismiss {");
            source.replace("; y: 100", "");
        }
        component.setData(source.toUtf8(), QUrl("qrc:/swipe-nested.qml"));
        QQuickWindow window;
        window.resize(320, 400);
        std::unique_ptr<QQuickItem> view(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(view, qPrintable(component.errorString()));
        view->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (viewType == "ListView") QTRY_VERIFY(view->property("currentItem").value<QQuickItem*>());
        auto* swipe = viewType == "ListView" ? view->property("currentItem").value<QQuickItem*>()
                                             : view->findChild<QQuickItem*>("swipe");
        QVERIFY(swipe);
        auto* state = swipe->property("dismissState").value<Swipe*>();
        QVERIFY(state);
        const auto initialY = view->property("contentY").toReal();
        auto       send     = [&](QEvent::Type type, QPoint position, ulong time) {
            if (touch) {
                static auto* device   = QTest::createTouchDevice();
                auto         sequence = QTest::touchEvent(&window, device);
                if (type == QEvent::MouseButtonPress)
                    sequence.press(0, position);
                else if (type == QEvent::MouseButtonRelease)
                    sequence.release(0, position);
                else
                    sequence.move(0, position);
                sequence.commit();
                QQuickWindowPrivate::get(&window)
                    ->deliveryAgentPrivate()
                    ->flushFrameSynchronousEvents(&window);
            } else {
                QMouseEvent event(type,
                                  position,
                                  window.mapToGlobal(position),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                                  type == QEvent::MouseButtonRelease ? Qt::NoButton
                                                                     : Qt::LeftButton,
                                  Qt::NoModifier);
                event.setTimestamp(time);
                QCoreApplication::sendEvent(&window, &event);
            }
        };
        send(QEvent::MouseButtonPress, { 60, 150 }, 1000);
        send(QEvent::MouseButtonRelease, { 60, 150 }, 1020);
        QCOMPARE(swipe->property("clicks").toInt(), 1);
        send(QEvent::MouseButtonPress, { 60, 150 }, 1100);
        send(QEvent::MouseMove, { 100, 145 }, 1120);
        send(QEvent::MouseMove, { 140, 140 }, 1140);
        QVERIFY(state->dragging());
        QVERIFY(state->offset() > 0);
        QCOMPARE(view->property("contentY").toReal(), initialY);
        swipe->setProperty("gesturesEnabled", false);
        QVERIFY(! state->dragging());
        QCOMPARE(state->offset(), 0);
        send(QEvent::MouseButtonRelease, { 140, 140 }, 1160);
        QCOMPARE(swipe->property("clicks").toInt(), 1);
        swipe->setProperty("gesturesEnabled", true);
        send(QEvent::MouseButtonPress, { 60, 150 }, 1200);
        send(QEvent::MouseMove, { 65, 110 }, 1220);
        send(QEvent::MouseMove, { 70, 70 }, 1240);
        send(QEvent::MouseMove, { 75, 30 }, 1260);
        QVERIFY(! state->dragging());
        QVERIFY(view->property("contentY").toReal() > initialY);
        send(QEvent::MouseButtonRelease, { 75, 30 }, 1280);
        QCOMPARE(swipe->property("clicks").toInt(), 1);
        QVERIFY(QMetaObject::invokeMethod(view.get(), "cancelFlick"));
        view->setProperty("contentY", initialY);
        QSignalSpy dismissed(state, &Swipe::dismissed);
        send(QEvent::MouseButtonPress, { 60, 150 }, 1400);
        send(QEvent::MouseMove, { 100, 150 }, 1420);
        send(QEvent::MouseMove, { 200, 150 }, 1440);
        send(QEvent::MouseMove, { 290, 150 }, 1460);
        QVERIFY(state->dragging());
        QVERIFY(state->offset() > 150);
        send(QEvent::MouseButtonRelease, { 290, 150 }, 1480);
        QVERIFY(! state->dragging());
        QCOMPARE(state->settledValue(), Swipe::StartToEnd);
        QCOMPARE(dismissed.size(), 1);
        QCOMPARE(swipe->property("clicks").toInt(), 1);
        state->reset();
        QCOMPARE(state->settledValue(), Swipe::Settled);
        send(QEvent::MouseButtonPress, { 60, 150 }, 1600);
        send(QEvent::MouseMove, { 100, 150 }, 1620);
        send(QEvent::MouseMove, { 180, 150 }, 1640);
        send(QEvent::MouseMove, { 120, 150 }, 1660);
        send(QEvent::MouseMove, { 70, 150 }, 1680);
        send(QEvent::MouseButtonRelease, { 70, 150 }, 1700);
        QCOMPARE(state->settledValue(), Swipe::Settled);
        QCOMPARE(dismissed.size(), 1);
        send(QEvent::MouseButtonPress, { 60, 150 }, 1800);
        send(QEvent::MouseMove, { 100, 150 }, 1820);
        send(QEvent::MouseMove, { 180, 150 }, 1840);
        QVERIFY(state->dragging());
        window.hide();
        QCoreApplication::processEvents();
        QVERIFY(! state->dragging());
        QCOMPARE(state->settledValue(), Swipe::Settled);
        QCOMPARE(dismissed.size(), 1);
    }
    void presentation() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.SwipeToDismiss {
    width: 300; height: 80
    background: Rectangle { color: "red" }
    Rectangle { objectName: "foreground"; width: parent.width; height: 80; color: "white" }
})",
                          QUrl("qrc:/swipe-presentation.qml"));
        std::unique_ptr<QQuickItem> item(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(item, qPrintable(component.errorString()));
        auto* state = item->property("dismissState").value<Swipe*>();
        QVERIFY(state);
        auto* foreground = item->findChild<QQuickItem*>("foreground");
        QVERIFY(foreground);
        auto* animation = item->findChild<QQuickNumberAnimation*>();
        QVERIFY(animation);
        QSignalSpy dismissed(state, &Swipe::dismissed);
        QCOMPARE(item->property("presentedOffset").toReal(), 0);
        QCOMPARE(state->distance(), 300);
        QCOMPARE(foreground->width(), 300);
        state->dismiss(Swipe::StartToEnd);
        QVERIFY(animation->isRunning());
        QCOMPARE(dismissed.size(), 0);
        animation->setCurrentTime(animation->duration() / 2);
        const auto offset = item->property("presentedOffset").toReal();
        QVERIFY(offset > 0 && offset < 300);
        QCOMPARE(foreground->width(), 300);
        animation->setCurrentTime(animation->duration());
        QCOMPARE(item->property("presentedOffset").toReal(), 300);
        QCOMPARE(dismissed.size(), 1);
        state->reset();
        animation->setCurrentTime(animation->duration() / 2);
        item->setProperty("animationsEnabled", false);
        QCOMPARE(item->property("presentedOffset").toReal(), 0);
        QCOMPARE(state->settledValue(), Swipe::Settled);
        QVERIFY(! animation->isRunning());
        item->setProperty("layoutDirection", Qt::RightToLeft);
        state->dismiss(Swipe::StartToEnd);
        QCOMPARE(item->property("presentedOffset").toReal(), -300);
        QCOMPARE(dismissed.size(), 2);
        item->setWidth(400);
        QCOMPARE(foreground->width(), 400);
        QCOMPARE(item->property("presentedOffset").toReal(), -400);
        QCOMPARE(dismissed.size(), 2);
        state->setStartToEndEnabled(false);
        QCOMPARE(state->settledValue(), Swipe::Settled);
        QCOMPARE(item->property("presentedOffset").toReal(), 0);
        item->setProperty("animationsEnabled", true);
        state->dismiss(Swipe::EndToStart);
        animation->setCurrentTime(animation->duration() / 2);
        item->setWidth(200);
        animation->setCurrentTime(animation->duration());
        QCOMPARE(item->property("presentedOffset").toReal(), 200);
        QCOMPARE(foreground->width(), 200);
        QCOMPARE(dismissed.size(), 3);
        state->snapTo(Swipe::Settled);
        QCOMPARE(item->property("presentedOffset").toReal(), 0);
        QVERIFY(! animation->isRunning());
        QCOMPARE(dismissed.size(), 3);
    }
    void targets_data() {
        QTest::addColumn<qreal>("offset");
        QTest::addColumn<qreal>("velocity");
        QTest::addColumn<int>("target");
        QTest::newRow("slow-before") << 55.0 << 10.0 << int(Swipe::Settled);
        QTest::newRow("slow-threshold") << 56.0 << 10.0 << int(Swipe::StartToEnd);
        QTest::newRow("stationary-near-start") << 100.0 << 0.0 << int(Swipe::Settled);
        QTest::newRow("stationary-near-end") << 200.0 << 0.0 << int(Swipe::StartToEnd);
        QTest::newRow("fast-forward") << 10.0 << 125.0 << int(Swipe::StartToEnd);
        QTest::newRow("fast-reverse") << 200.0 << -125.0 << int(Swipe::Settled);
        QTest::newRow("slow-reverse") << 200.0 << -10.0 << int(Swipe::Settled);
        QTest::newRow("reverse-near-end") << 270.0 << -10.0 << int(Swipe::StartToEnd);
        QTest::newRow("negative-threshold") << -56.0 << -10.0 << int(Swipe::EndToStart);
        QTest::newRow("negative-fast") << -10.0 << -125.0 << int(Swipe::EndToStart);
        QTest::newRow("negative-reverse") << -200.0 << 125.0 << int(Swipe::Settled);
        QTest::newRow("exact-start") << 0.0 << 125.0 << int(Swipe::Settled);
    }
    void targets() {
        QFETCH(qreal, offset);
        QFETCH(qreal, velocity);
        QFETCH(int, target);
        Swipe state;
        state.setDistance(300);
        QSignalSpy transitions(&state, &Swipe::transitionRequested);
        QSignalSpy dismissed(&state, &Swipe::dismissed);
        QVERIFY(state.begin(0));
        state.dragBy(offset);
        QCOMPARE(state.offset(), offset);
        state.release(velocity);
        QCOMPARE(int(state.targetValue()), target);
        QCOMPARE(state.settledValue(), Swipe::Settled);
        QCOMPARE(dismissed.size(), 0);
        QVERIFY(state.settling());
        const auto revision = transitions.last().at(0).toUInt();
        state.complete(revision);
        QCOMPARE(int(state.settledValue()), target);
        QCOMPARE(dismissed.size(), target == Swipe::Settled ? 0 : 1);
        state.complete(revision);
        QCOMPARE(dismissed.size(), target == Swipe::Settled ? 0 : 1);
    }
    void anchorsAndInvalidInput() {
        Swipe state;
        QVERIFY(! state.begin(0));
        state.setDistance(40);
        state.setEndToStartEnabled(false);
        QVERIFY(state.begin(0));
        state.dragBy(-100);
        QCOMPARE(state.offset(), 0);
        state.dragBy(100);
        QCOMPARE(state.offset(), 40);
        QCOMPARE(state.progress(), 1);
        state.release(1);
        QCOMPARE(state.targetValue(), Swipe::StartToEnd);
        state.setStartToEndEnabled(false);
        QCOMPARE(state.targetValue(), Swipe::Settled);
        QCOMPARE(state.offset(), 0);
        QVERIFY(! state.begin(0));
        state.setDistance(std::numeric_limits<qreal>::quiet_NaN());
        state.setDistance(-1);
        QCOMPARE(state.distance(), 40);
        state.setStartToEndEnabled(true);
        state.snapTo(Swipe::StartToEnd);
        QCOMPARE(state.settledValue(), Swipe::StartToEnd);
        state.setDistance(80);
        QCOMPARE(state.offset(), 80);
        state.setDistance(0);
        QCOMPARE(state.settledValue(), Swipe::Settled);
        QCOMPARE(state.progress(), 0);
    }
    void interruption() {
        Swipe state;
        state.setDistance(300);
        QSignalSpy transitions(&state, &Swipe::transitionRequested);
        QSignalSpy dismissed(&state, &Swipe::dismissed);
        state.dismiss(Swipe::StartToEnd);
        const auto obsolete = transitions.last().at(0).toUInt();
        state.reset();
        state.complete(obsolete);
        QCOMPARE(dismissed.size(), 0);
        QVERIFY(state.settling());
        state.complete(transitions.last().at(0).toUInt());
        QVERIFY(! state.settling());
        state.dismiss(Swipe::EndToStart);
        const auto interrupted = transitions.last().at(0).toUInt();
        QVERIFY(state.begin(-70));
        QCOMPARE(state.offset(), -70);
        state.complete(interrupted);
        QVERIFY(state.dragging());
        state.dragBy(100);
        QCOMPARE(state.offset(), 30);
        state.cancel();
        QCOMPARE(state.targetValue(), Swipe::Settled);
        state.complete(transitions.last().at(0).toUInt());
        QCOMPARE(dismissed.size(), 0);
    }
    void callbackReentry() {
        Swipe state;
        state.setDistance(300);
        QSignalSpy transitions(&state, &Swipe::transitionRequested);
        connect(&state, &Swipe::dismissed, &state, &Swipe::reset);
        state.dismiss(Swipe::StartToEnd);
        state.complete(transitions.last().at(0).toUInt());
        QCOMPARE(state.targetValue(), Swipe::Settled);
        QVERIFY(state.settling());
        state.complete(transitions.last().at(0).toUInt());
        QCOMPARE(state.settledValue(), Swipe::Settled);
        QPointer<Swipe> dying = new Swipe;
        dying->setDistance(300);
        connect(dying, &Swipe::motionChanged, dying, [dying] {
            delete dying;
        });
        dying->dismiss(Swipe::StartToEnd);
        QVERIFY(! dying);
    }
};

int run_swipe_to_dismiss(int argc, char** argv) {
    SwipeToDismissTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "swipe_to_dismiss.moc"
