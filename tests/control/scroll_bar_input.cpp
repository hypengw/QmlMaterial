#include <QtTest>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>
#include "qml_material/control/scroll_bar.hpp"
#include "qml_material/input/clickable.hpp"

using namespace qml_material;

namespace
{
void mouse(QQuickItem* item, QEvent::Type type, QPointF position) {
    QMouseEvent event(type,
                      position,
                      position,
                      type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(item, &event);
}
void prepare(ScrollBar& bar) {
    bar.setWidth(20);
    bar.setHeight(200);
    bar.setSize(.2);
}
} // namespace

class ScrollBarInputTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void qmlAppearanceAndControlledBinding() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ScrollBar {
                height: 200
                size: .2
                policy: MD.ScrollBarBase.AlwaysOn
                property real committed: .1
                property bool acceptRequest: false
                position: committed
                onPositionChangeRequested: (value) => { if (acceptRequest) committed = value }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<ScrollBar*>(object.get());
        QVERIFY(bar);
        QCOMPARE(bar->width(), 8.0);
        const auto height = bar->availableHeight();
        mouse(bar, QEvent::MouseButtonPress, { 4, 30 });
        QTRY_VERIFY(bar->interactionValues().dragged);
        QVERIFY(bar->property("expanded").toBool());
        QCOMPARE(bar->width(), 8.0);
        QCOMPARE(bar->availableHeight(), height);
        QCOMPARE(bar->position(), .1);
        mouse(bar, QEvent::MouseMove, { 4, 100 });
        QCOMPARE(bar->position(), .1);
        bar->setProperty("acceptRequest", true);
        mouse(bar, QEvent::MouseButtonRelease, { 4, 100 });
        QTRY_VERIFY(! bar->interactionValues().dragged);
        QVERIFY(bar->position() > .4);
        bar->setProperty("committed", .3);
        QCOMPARE(bar->position(), .3);
        QHoverEvent enter(QEvent::HoverEnter, { 4, 30 }, { 4, 30 }, { 4, 30 });
        QCoreApplication::sendEvent(bar, &enter);
        QTRY_VERIFY(bar->interactionValues().hovered);
        QVERIFY(bar->active());
        bar->setHoverEnabled(false);
        QTRY_VERIFY(! bar->interactionValues().hovered);
        QVERIFY(! bar->active());
    }
    void controlledPosition() {
        ScrollBar bar;
        prepare(bar);
        QSignalSpy requests(&bar, &ScrollBar::positionChangeRequested);
        QSignalSpy events(bar.interactionSource(), &InteractionSource::interaction);
        mouse(&bar, QEvent::MouseButtonPress, { 10, 10 });
        QTRY_VERIFY(bar.interactionValues().dragged);
        QVERIFY(! bar.interactionValues().pressed);
        mouse(&bar, QEvent::MouseMove, { 10, 110 });
        QCOMPARE(bar.position(), 0.0);
        QCOMPARE(requests.last()[0].toReal(), .5);
        mouse(&bar, QEvent::MouseButtonRelease, { 10, 130 });
        QCOMPARE(requests.last()[0].toReal(), .6);
        QTRY_VERIFY(! bar.interactionValues().dragged);
        QTRY_COMPARE(events.size(), 2);
        QCOMPARE(events[0][0].value<Interaction>().kind, Interaction::Drag);
        QCOMPARE(events[1][0].value<Interaction>().phase, Interaction::End);
        QCOMPARE(events[0][0].value<Interaction>().id, events[1][0].value<Interaction>().id);

        const auto accepted =
            connect(&bar, &ScrollBar::positionChangeRequested, &bar, &ScrollBar::setPosition);
        bar.increase();
        QCOMPARE(bar.position(), .1);
        bar.setStepSize(.2);
        bar.decrease();
        QCOMPARE(bar.position(), 0.0);
        bar.requestPosition(5);
        QCOMPARE(bar.position(), .8);
        disconnect(accepted);
        bar.requestPosition(.3);
        QCOMPARE(bar.position(), .8);
        const auto count = requests.size();
        bar.setInteractive(false);
        bar.increase();
        bar.requestPosition(.5);
        QCOMPARE(requests.size(), count);
        QCOMPARE(bar.metaObject()->indexOfProperty("pressed"), -1);
    }
    void snapAndMinimumThumb_data() {
        QTest::addColumn<bool>("horizontal");
        QTest::addColumn<int>("snap");
        for (const bool horizontal : { false, true })
            for (int snap = ScrollBar::NoSnap; snap <= ScrollBar::SnapOnRelease; ++snap)
                QTest::newRow(qPrintable(QString("%1-%2").arg(horizontal).arg(snap)))
                    << horizontal << snap;
    }
    void snapAndMinimumThumb() {
        QFETCH(bool, horizontal);
        QFETCH(int, snap);
        ScrollBar bar;
        prepare(bar);
        if (horizontal) {
            bar.setOrientation(Qt::Horizontal);
            bar.setWidth(200);
            bar.setHeight(20);
        }
        bar.setMinimumSize(.4);
        bar.setStepSize(.25);
        bar.setSnapMode(ScrollBar::SnapMode(snap));
        connect(&bar, &ScrollBar::positionChangeRequested, &bar, &ScrollBar::setPosition);
        const auto point = [horizontal](qreal p) {
            return horizontal ? QPointF(p, 10) : QPointF(10, p);
        };
        mouse(&bar, QEvent::MouseButtonPress, point(20));
        mouse(&bar, QEvent::MouseMove, point(100));
        QCOMPARE(bar.position(), snap == ScrollBar::SnapAlways ? .6 : 8.0 / 15.0);
        mouse(&bar, QEvent::MouseButtonRelease, point(100));
        QCOMPARE(bar.position(), snap == ScrollBar::NoSnap ? 8.0 / 15.0 : .6);
    }
    void cancellation_data() {
        QTest::addColumn<QString>("reason");
        for (auto reason : { "disabled",
                             "hidden",
                             "interactive",
                             "orientation",
                             "source",
                             "destroyed",
                             "parent" })
            QTest::newRow(reason) << QString::fromLatin1(reason);
    }
    void cancellation() {
        QFETCH(QString, reason);
        QQuickItem        parent;
        auto              source = std::make_unique<InteractionSource>();
        InteractionSource next;
        ScrollBar         bar;
        prepare(bar);
        bar.setInteractionSource(source.get());
        QSignalSpy events(source.get(), &InteractionSource::interaction);
        QSignalSpy requests(&bar, &ScrollBar::positionChangeRequested);
        mouse(&bar, QEvent::MouseButtonPress, { 10, 10 });
        QTRY_VERIFY(bar.interactionValues().dragged);
        if (reason == "disabled")
            bar.setEnabled(false);
        else if (reason == "hidden")
            bar.setVisible(false);
        else if (reason == "interactive")
            bar.setInteractive(false);
        else if (reason == "orientation")
            bar.setOrientation(Qt::Horizontal);
        else if (reason == "source")
            bar.setInteractionSource(&next);
        else if (reason == "destroyed")
            source.reset();
        else if (reason == "parent")
            bar.setParentItem(&parent);
        QTRY_VERIFY(! bar.interactionValues().dragged);
        const auto count = requests.size();
        mouse(&bar, QEvent::MouseButtonRelease, { 10, 150 });
        QCOMPARE(requests.size(), count);
        QVERIFY(! bar.keepMouseGrab());
        QVERIFY(! bar.keepTouchGrab());
        if (source) {
            QTRY_COMPARE(events.size(), 2);
            QCOMPARE(events[1][0].value<Interaction>().phase, Interaction::Cancel);
        }
    }
    void sharedSourceDoesNotControlGesture() {
        InteractionSource source;
        ScrollBar         first, second;
        prepare(first);
        prepare(second);
        first.setInteractionSource(&source);
        second.setInteractionSource(&source);
        QSignalSpy requests(&second, &ScrollBar::positionChangeRequested);
        mouse(&first, QEvent::MouseButtonPress, { 10, 10 });
        QTRY_VERIFY(second.interactionValues().dragged);
        mouse(&second, QEvent::MouseMove, { 10, 150 });
        QCOMPARE(requests.size(), 0);
        mouse(&second, QEvent::MouseButtonPress, { 10, 10 });
        first.setInteractive(false);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(second.interactionValues().dragged);
        mouse(&second, QEvent::MouseButtonRelease, { 10, 150 });
        QTRY_VERIFY(! second.interactionValues().dragged);
    }
    void deletionInRequest() {
        InteractionSource source;
        auto              bar = std::make_unique<ScrollBar>();
        prepare(*bar);
        bar->setInteractionSource(&source);
        QSignalSpy events(&source, &InteractionSource::interaction);
        connect(bar.get(), &ScrollBar::positionChangeRequested, this, [&] {
            bar.reset();
        });
        mouse(bar.get(), QEvent::MouseButtonPress, { 10, 80 });
        QVERIFY(! bar);
        QTRY_COMPARE(events.size(), 2);
        QCOMPARE(events[1][0].value<Interaction>().phase, Interaction::Cancel);
    }
    void nativeGrabAndTouch() {
        QQuickWindow window;
        window.resize(240, 240);
        ScrollBar bar(window.contentItem());
        prepare(bar);
        connect(&bar, &ScrollBar::positionChangeRequested, &bar, &ScrollBar::setPosition);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 10, 10 });
        QTRY_VERIFY(bar.interactionValues().dragged);
        QCOMPARE(window.mouseGrabberItem(), &bar);
        bar.ungrabMouse();
        QTRY_VERIFY(! bar.interactionValues().dragged);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 10, 150 });
        QCOMPARE(bar.position(), 0.0);
        auto* device = QTest::createTouchDevice();
        auto  touch  = QTest::touchEvent(&window, device, false);
        touch.press(0, { 10, 10 }, &window).commit();
        QTRY_VERIFY(bar.interactionValues().dragged);
        touch.move(0, { 80, 150 }, &window).commit();
        QTRY_COMPARE(bar.position(), .7);
        touch.release(0, { 80, 180 }, &window).commit();
        QTRY_VERIFY(! bar.interactionValues().dragged);
        QCOMPARE(bar.position(), .8);
    }
};
int run_scroll_bar_input(int argc, char** argv) {
    ScrollBarInputTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "scroll_bar_input.moc"
