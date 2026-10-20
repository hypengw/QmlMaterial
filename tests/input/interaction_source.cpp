#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/input/interaction_source.hpp"
#include "qml_material/input/clickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "qml_material/scrollable/flickable.hpp"
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>

using namespace qml_material;

namespace
{
void mouse(QQuickItem* item, QEvent::Type type, QPointF point = { 20, 20 }, ulong time = 0) {
    QMouseEvent event(type,
                      point,
                      point,
                      type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    event.setTimestamp(time);
    QCoreApplication::sendEvent(item, &event);
}
void key(QQuickItem* item, QEvent::Type type, int key, ulong time = 0) {
    QKeyEvent event(type, key, Qt::NoModifier);
    event.setTimestamp(time);
    QCoreApplication::sendEvent(item, &event);
}
class DragTarget : public QQuickItem {
public:
    InputHost  input { this };
    DragInput& drag = input.drag();
    DragTarget() {
        setSize({ 200, 100 });
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptTouchEvents(true);
    }
    bool event(QEvent* event) override {
        QPointer<DragTarget> guard(this);
        const auto           handled = input.event(event);
        return ! guard || handled || QQuickItem::event(event);
    }
    void touchUngrabEvent() override { input.touchUngrabbed(); }
    void mouseUngrabEvent() override { input.mouseUngrabbed(); }
};
} // namespace

class InteractionSourceTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void immediateDragAndKeyboardPolicy() {
        DragTarget item;
        item.drag.startImmediately         = true;
        item.input.click().keyboardEnabled = false;
        QSignalSpy events(item.input.source(), &InteractionSource::interaction);
        int        starts = 0, moves = 0, finishes = 0, clicks = 0;
        item.drag.started = [&](QPointF point) {
            QCOMPARE(point, QPointF(20, 20));
            ++starts;
        };
        item.drag.moved = [&](QPointF) {
            ++moves;
        };
        item.drag.finished = [&](QPointF point, bool cancelled) {
            QVERIFY(! cancelled);
            ++finishes;
        };
        item.input.click().clicked = [&] {
            ++clicks;
        };
        key(&item, QEvent::KeyPress, Qt::Key_Space);
        key(&item, QEvent::KeyRelease, Qt::Key_Space);
        QCOMPARE(clicks, 0);
        mouse(&item, QEvent::MouseButtonPress);
        QCOMPARE(starts, 1);
        QCOMPARE(moves, 1);
        QVERIFY(item.keepMouseGrab());
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(finishes, 1);
        QCOMPARE(clicks, 0);
        QVERIFY(! item.keepMouseGrab());
        QTRY_COMPARE(events.size(), 2);
        QCOMPARE(events[0][0].value<Interaction>().kind, Interaction::Drag);
        QCOMPARE(events[1][0].value<Interaction>().phase, Interaction::End);
    }
    void immediateDragReentry_data() {
        QTest::addColumn<bool>("destroy");
        QTest::newRow("replace-source") << false;
        QTest::newRow("destroy") << true;
    }
    void immediateDragReentry() {
        QFETCH(bool, destroy);
        InteractionSource first, next;
        auto              item = std::make_unique<DragTarget>();
        item->input.setSource(&first);
        item->drag.startImmediately = true;
        QSignalSpy events(&first, &InteractionSource::interaction);
        int        moves = 0;
        item->drag.moved = [&](QPointF) {
            ++moves;
        };
        item->drag.started = [&](QPointF) {
            if (destroy)
                item.reset();
            else
                item->input.setSource(&next);
        };
        mouse(item.get(), QEvent::MouseButtonPress);
        QCOMPARE(moves, 0);
        QTRY_COMPARE(events.size(), 2);
        QCOMPARE(events[1][0].value<Interaction>().phase, Interaction::Cancel);
    }
    void nativeMouseGrabLoss() {
        QQuickWindow window;
        window.resize(240, 160);
        Clickable item(window.contentItem());
        item.setSize({ 200, 100 });
        InteractionAccumulator state(this);
        state.setSource(item.interactionSource());
        QSignalSpy source(item.interactionSource(), &InteractionSource::interaction);
        QSignalSpy clicks(&item, &Clickable::clicked);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QTRY_VERIFY(state.values().pressed);
        item.ungrabMouse();
        QTRY_VERIFY(! state.values().pressed);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QCOMPARE(clicks.size(), 0);
        QList<Interaction> presses;
        for (const auto& args : source) {
            const auto event = args.front().value<Interaction>();
            if (event.kind == Interaction::Press) presses.append(event);
        }
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[0].id, presses[1].id);
        QCOMPARE(presses[1].phase, Interaction::Cancel);
    }
    void editorDeliveryAcceptanceAndLifetime() {
        auto              item  = std::make_unique<QQuickItem>();
        auto              input = std::make_unique<InputHost>(item.get());
        InteractionSource source;
        input->setSource(&source);
        QSignalSpy  events(&source, &InteractionSource::interaction);
        QMouseEvent press(QEvent::MouseButtonPress,
                          QPointF(10, 10),
                          QPointF(10, 10),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        QVERIFY(! QtInputAdapter::deliverToEditor(*input, &press, [&] {
            press.ignore();
            return false;
        }));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(events.isEmpty());
        QVERIFY(! press.isAccepted());
        QVERIFY(QtInputAdapter::deliverToEditor(*input, &press, [&] {
            press.accept();
            input.reset();
            item.reset();
            return true;
        }));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(events.isEmpty());
    }
    void editorDeliverySourceChangeAndReentry() {
        QQuickItem item;
        item.setSize({ 100, 50 });
        InputHost         input(&item);
        InteractionSource first, second;
        input.setSource(&first);
        QSignalSpy  events(&second, &InteractionSource::interaction);
        QMouseEvent press(QEvent::MouseButtonPress,
                          QPointF(10, 10),
                          QPointF(10, 10),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        QtInputAdapter::deliverToEditor(input, &press, [&] {
            input.setSource(&second);
            return true;
        });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(events.isEmpty());
        QtInputAdapter::deliverToEditor(input, &press, [&] {
            QEvent ungrab(QEvent::UngrabMouse);
            QtInputAdapter::deliverToEditor(input, &ungrab, [] {
                return false;
            });
            return true;
        });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(events.isEmpty());
        input.setSource(nullptr);
        QSignalSpy late(&first, &InteractionSource::interaction);
        QtInputAdapter::deliverToEditor(input, &press, [&] {
            input.setSource(&first);
            return true;
        });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(late.isEmpty());
    }
    void focusOnlyHostDoesNotConsumeEditingInput() {
        QQuickItem item;
        InputHost  input(&item);
        QSignalSpy events(input.source(), &InteractionSource::interaction);
        QKeyEvent  space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier, " ");
        space.setAccepted(false);
        QVERIFY(! input.event(&space));
        QVERIFY(! space.isAccepted());
        QMouseEvent press(QEvent::MouseButtonPress,
                          QPointF(10, 10),
                          QPointF(10, 10),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        press.setAccepted(false);
        QVERIFY(! input.event(&press));
        QVERIFY(! press.isAccepted());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(events.isEmpty());
        input.setHoverEnabled(true);
        QHoverEvent enter(QEvent::HoverEnter, QPointF(10, 10), QPointF(10, 10), QPointF());
        QVERIFY(! input.event(&enter));
        QTRY_COMPARE(events.size(), 1);
        QCOMPARE(events.front().front().value<Interaction>().kind, Interaction::Hover);
        input.setHoverEnabled(false);
        QTRY_COMPARE(events.size(), 2);
        QCOMPARE(events.back().front().value<Interaction>().phase, Interaction::End);
    }
    void editorLateSourceDoesNotRecreatePress() {
        QQuickItem item;
        item.setSize({ 100, 50 });
        InputHost                   input(&item);
        QMouseEvent                 press(QEvent::MouseButtonPress,
                                          QPointF(10, 10),
                                          QPointF(10, 10),
                                          Qt::LeftButton,
                                          Qt::LeftButton,
                                          Qt::NoModifier);
        std::unique_ptr<QSignalSpy> events;
        QtInputAdapter::deliverToEditor(input, &press, [&] {
            events = std::make_unique<QSignalSpy>(input.source(), &InteractionSource::interaction);
            return true;
        });
        QMouseEvent release(QEvent::MouseButtonRelease,
                            QPointF(10, 10),
                            QPointF(10, 10),
                            Qt::LeftButton,
                            Qt::NoButton,
                            Qt::NoModifier);
        QtInputAdapter::deliverToEditor(input, &release, [] {
            return true;
        });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QVERIFY(events->isEmpty());
    }
    void dragThresholdAndOrthogonalCancellation() {
        DragTarget item;
        item.drag.threshold = 5;
        int starts = 0, clicks = 0;
        item.drag.started = [&](QPointF) {
            ++starts;
        };
        item.input.click().clicked = [&] {
            ++clicks;
        };
        mouse(&item, QEvent::MouseButtonPress, { 20, 20 });
        mouse(&item, QEvent::MouseMove, { 23, 24 });
        QCOMPARE(starts, 1);
        mouse(&item, QEvent::MouseButtonRelease, { 23, 24 });
        item.drag.axes = Qt::Horizontal;
        mouse(&item, QEvent::MouseButtonPress, { 20, 20 });
        mouse(&item, QEvent::MouseMove, { 21, 40 });
        mouse(&item, QEvent::MouseButtonRelease, { 21, 40 });
        QCOMPARE(starts, 1);
        QCOMPARE(clicks, 0);
    }
    void lateSourceDoesNotRecreatePress() {
        Clickable item;
        item.setSize({ 100, 50 });
        QSignalSpy clicks(&item, &Clickable::clicked);
        mouse(&item, QEvent::MouseButtonPress);
        QSignalSpy events(item.interactionSource(), &InteractionSource::interaction);
        mouse(&item, QEvent::MouseButtonRelease);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(clicks.count(), 1);
        QCOMPARE(events.count(), 0);
    }
    void deleteInPointerStarted() {
        auto item                          = std::make_unique<DragTarget>();
        item->input.click().pointerStarted = [&](const PointerInput&) {
            item.reset();
        };
        mouse(item.get(), QEvent::MouseButtonPress);
        QVERIFY(! item);
    }
    void doubleClickExpiredBeforeTimerDelivery() {
        Clickable item;
        item.setSize({ 100, 50 });
        item.setDoubleClickEnabled(true);
        QSignalSpy clicks(&item, &Clickable::clicked), doubles(&item, &Clickable::doubleClicked);
        mouse(&item, QEvent::MouseButtonPress, { 20, 20 }, 100);
        mouse(&item, QEvent::MouseButtonRelease, { 20, 20 }, 110);
        mouse(&item, QEvent::MouseButtonPress, { 20, 20 }, 10000);
        QCOMPARE(clicks.count(), 1);
        mouse(&item, QEvent::MouseButtonRelease, { 20, 20 }, 10010);
        QTRY_COMPARE(clicks.count(), 2);
        QCOMPARE(doubles.count(), 0);
    }
    void nativeTouchSequence() {
        QQuickWindow window;
        window.resize(240, 200);
        Clickable item(window.contentItem());
        item.setSize({ 200, 160 });
        QSignalSpy events(item.interactionSource(), &InteractionSource::interaction);
        QSignalSpy clicks(&item, &Clickable::clicked);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        static auto* device = QTest::createTouchDevice();
        auto         touch  = QTest::touchEvent(&window, device, false);
        const auto   flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 40, 40 }, &window).commit();
        flush();
        touch.stationary(0).press(1, { 80, 40 }, &window).commit();
        flush();
        touch.release(0, { 40, 40 }, &window).stationary(1).commit();
        flush();
        QCOMPARE(clicks.count(), 0);
        touch.release(1, { 80, 40 }, &window).commit();
        flush();
        QCOMPARE(clicks.count(), 1);
        QTRY_VERIFY(events.count() >= 2);
        QList<Interaction> presses;
        for (const auto& args : events) {
            auto event = args[0].value<Interaction>();
            if (event.kind == Interaction::Press) presses.append(event);
        }
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[0].identity, 0);
        QCOMPARE(presses[0].id, presses[1].id);
    }
    void nativeScrollCancelsClick() {
        QQuickWindow window;
        window.resize(240, 200);
        Flickable view(window.contentItem());
        view.setSize({ 240, 200 });
        view.setContentHeight(800);
        view.setContentWidth(240);
        view.setFlickableDirection(Flickable::VerticalFlick);
        auto* policy =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&view, true));
        QVERIFY(policy);
        policy->setPressDelay(2000);
        Clickable item(view.contentItem());
        item.setSize({ 200, 160 });
        QSignalSpy events(item.interactionSource(), &InteractionSource::interaction);
        QSignalSpy clicks(&item, &Clickable::clicked);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        static auto* device = QTest::createTouchDevice();
        auto         touch  = QTest::touchEvent(&window, device, false);
        const auto   flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 80, 120 }, &window).commit();
        flush();
        touch.move(0, { 80, 80 }, &window).commit();
        flush();
        touch.move(0, { 80, 40 }, &window).commit();
        flush();
        QVERIFY(view.isDragging());
        touch.release(0, { 80, 40 }, &window).commit();
        flush();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(clicks.count(), 0);
        for (const auto& args : events)
            QVERIFY(args[0].value<Interaction>().kind != Interaction::Press);
    }
    void delayedShortClick() {
        Flickable view;
        view.setSize({ 240, 200 });
        view.setContentHeight(800);
        view.setContentWidth(240);
        auto* policy =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&view, true));
        QVERIFY(policy);
        policy->setPressDelay(2000);
        Clickable item(view.contentItem());
        item.setSize({ 200, 160 });
        QSignalSpy events(item.interactionSource(), &InteractionSource::interaction);
        QSignalSpy clicks(&item, &Clickable::clicked);
        mouse(&item, QEvent::MouseButtonPress);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(events.count(), 0);
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 1);
        QTRY_COMPARE(events.count(), 2);
        QCOMPARE(events[0][0].value<Interaction>().id, events[1][0].value<Interaction>().id);
    }
    void nativeDragOutsideBounds() {
        QQuickWindow window;
        window.resize(400, 200);
        DragTarget item;
        item.setParentItem(window.contentItem());
        item.drag.threshold = 5;
        item.drag.axes      = Qt::Horizontal;
        int starts = 0, finishes = 0;
        item.drag.started = [&](QPointF) {
            ++starts;
        };
        item.drag.finished = [&](QPointF, bool cancelled) {
            QVERIFY(! cancelled);
            ++finishes;
        };
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        static auto* device = QTest::createTouchDevice();
        auto         touch  = QTest::touchEvent(&window, device, false);
        const auto   flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 20, 20 }, &window).commit();
        flush();
        touch.move(0, { 240, 20 }, &window).commit();
        flush();
        QCOMPARE(starts, 1);
        touch.release(0, { 240, 20 }, &window).commit();
        flush();
        QCOMPARE(finishes, 1);
    }
    void frameFeedback() {
        if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
            QSKIP("Requires a presenting window");
        QQuickWindow window;
        window.resize(240, 120);
        Clickable item(window.contentItem());
        item.setSize({ 100, 50 });
        InteractionObserver observer;
        observer.setSource(item.interactionSource());
        connect(&observer, &InteractionObserver::changed, &window, [&] {
            window.setColor(observer.pressed() ? Qt::red : Qt::white);
        });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QSignalSpy frames(&window, &QQuickWindow::frameSwapped);
        window.update();
        QTRY_VERIFY(! frames.isEmpty());
        frames.clear();
        QElapsedTimer elapsed;
        elapsed.start();
        qint64 delivery = -1, presentation = -1;
        connect(&observer, &InteractionObserver::changed, &window, [&] {
            if (observer.pressed() && delivery < 0) delivery = elapsed.nsecsElapsed();
        });
        connect(&window, &QQuickWindow::frameSwapped, &window, [&] {
            if (observer.pressed() && presentation < 0) presentation = elapsed.nsecsElapsed();
        });
        mouse(&item, QEvent::MouseButtonPress);
        QTRY_VERIFY(presentation >= 0);
        QVERIFY(delivery >= 0);
        qInfo("Input feedback delivery %.3f ms, presented %.3f ms",
              delivery / 1e6,
              presentation / 1e6);
        mouse(&item, QEvent::MouseButtonRelease);
    }
    void lightweightClickable() {
        Clickable item;
        item.setSize({ 100, 50 });
        QSignalSpy clicks(&item, &Clickable::clicked);
        mouse(&item, QEvent::MouseButtonPress);
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 1);
        QVERIFY(item.findChildren<InteractionSource*>().isEmpty());
        for (auto property : { "pressed",
                               "pressX",
                               "pressY",
                               "checked",
                               "checkable",
                               "action",
                               "inputInteractions" })
            QCOMPARE(item.metaObject()->indexOfProperty(property), -1);
        InteractionObserver observer;
        observer.setSource(item.interactionSource());
        mouse(&item, QEvent::MouseButtonPress);
        QTRY_VERIFY(observer.pressed());
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 2);
        QTRY_VERIFY(! observer.pressed());
    }
    void sharedClickableCancellation() {
        InteractionSource source;
        Clickable         a, b;
        a.setSize({ 100, 50 });
        b.setSize({ 100, 50 });
        a.setInteractionSource(&source);
        b.setInteractionSource(&source);
        InteractionAccumulator input(this);
        input.setSource(&source);
        mouse(&a, QEvent::MouseButtonPress);
        mouse(&b, QEvent::MouseButtonPress);
        QTRY_COMPARE(input.active().size(), 2);
        a.setEnabled(false);
        QTRY_COMPARE(input.active().size(), 1);
        QCOMPARE(input.active().front().originItem(), &b);
        mouse(&b, QEvent::MouseButtonRelease);
        QTRY_VERIFY(input.active().isEmpty());
    }
    void sourceDestructionCancelsCandidate() {
        Clickable item;
        item.setSize({ 100, 50 });
        auto source = std::make_unique<InteractionSource>();
        item.setInteractionSource(source.get());
        QSignalSpy clicks(&item, &Clickable::clicked);
        mouse(&item, QEvent::MouseButtonPress);
        source.reset();
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 0);
        auto* owned = item.interactionSource();
        mouse(&item, QEvent::MouseButtonPress);
        delete owned;
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 0);
        QVERIFY(item.interactionSource());
        mouse(&item, QEvent::MouseButtonPress);
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 1);
    }
    void sourceReplacementCancelsCandidate() {
        InteractionSource source, next;
        Clickable         item;
        item.setSize({ 100, 50 });
        item.setInteractionSource(&source);
        QSignalSpy events(&source, &InteractionSource::interaction);
        QSignalSpy clicks(&item, &Clickable::clicked);
        mouse(&item, QEvent::MouseButtonPress);
        item.setInteractionSource(&next);
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 0);
        QTRY_COMPARE(events.count(), 2);
        QCOMPARE(events[1][0].value<Interaction>().phase, Interaction::Cancel);
    }
    void independentKeyboardAndPointer() {
        Clickable item;
        item.setSize({ 100, 50 });
        InteractionAccumulator input(this);
        input.setSource(item.interactionSource());
        QSignalSpy clicks(&item, &Clickable::clicked);
        mouse(&item, QEvent::MouseButtonPress);
        key(&item, QEvent::KeyPress, Qt::Key_Space);
        key(&item, QEvent::KeyPress, Qt::Key_Return);
        QTRY_COMPARE(input.active().size(), 3);
        mouse(&item, QEvent::MouseMove, { 110, 60 });
        QTRY_COMPARE(input.active().size(), 2);
        key(&item, QEvent::KeyRelease, Qt::Key_Space);
        QTRY_COMPARE(input.active().size(), 1);
        key(&item, QEvent::KeyRelease, Qt::Key_Return);
        QTRY_VERIFY(input.active().isEmpty());
        QCOMPARE(clicks.count(), 2);
    }
    void doubleClickExplicit() {
        Clickable item;
        item.setSize({ 100, 50 });
        QSignalSpy clicks(&item, &Clickable::clicked), doubles(&item, &Clickable::doubleClicked);
        mouse(&item, QEvent::MouseButtonPress, { 20, 20 }, 100);
        mouse(&item, QEvent::MouseButtonRelease, { 20, 20 }, 110);
        QCOMPARE(clicks.count(), 1);
        item.setDoubleClickEnabled(true);
        mouse(&item, QEvent::MouseButtonPress, { 20, 20 }, 200);
        mouse(&item, QEvent::MouseButtonRelease, { 20, 20 }, 210);
        QCOMPARE(clicks.count(), 1);
        mouse(&item, QEvent::MouseButtonDblClick, { 20, 20 }, 260);
        mouse(&item, QEvent::MouseButtonRelease, { 20, 20 }, 270);
        QCOMPARE(doubles.count(), 1);
        QCOMPARE(clicks.count(), 1);
    }
    void longClickExplicit() {
        Clickable item;
        item.setSize({ 100, 50 });
        item.setLongClickEnabled(true);
        item.setLongClickInterval(1);
        QSignalSpy clicks(&item, &Clickable::clicked), holds(&item, &Clickable::longClicked);
        mouse(&item, QEvent::MouseButtonPress);
        QTRY_COMPARE(holds.count(), 1);
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks.count(), 0);
    }
    void sharedClickDragArbitration() {
        DragTarget          item;
        InteractionObserver observer;
        observer.setSource(item.input.source());
        int clicks = 0, starts = 0, moves = 0, finishes = 0;
        item.input.click().clicked = [&] {
            ++clicks;
        };
        item.drag.threshold = 5;
        item.drag.axes      = Qt::Horizontal;
        item.drag.started   = [&](QPointF origin) {
            QCOMPARE(origin, QPointF(20, 20));
            ++starts;
        };
        item.drag.moved = [&](QPointF) {
            ++moves;
        };
        item.drag.finished = [&](QPointF, bool cancelled) {
            QVERIFY(! cancelled);
            ++finishes;
        };
        mouse(&item, QEvent::MouseButtonPress);
        QTRY_VERIFY(observer.pressed());
        mouse(&item, QEvent::MouseMove, { 40, 20 });
        QTRY_VERIFY(observer.dragged());
        QVERIFY(! observer.pressed());
        QCOMPARE(starts, 1);
        QVERIFY(item.keepMouseGrab());
        mouse(&item, QEvent::MouseButtonRelease, { 45, 20 });
        QTRY_VERIFY(! observer.dragged());
        QCOMPARE(clicks, 0);
        QCOMPARE(finishes, 1);
        QCOMPARE(moves, 1);
        QVERIFY(! item.keepMouseGrab());
        mouse(&item, QEvent::MouseButtonPress);
        mouse(&item, QEvent::MouseButtonRelease);
        QCOMPARE(clicks, 1);
    }
    void dragCancelledAndDeleted() {
        InteractionSource source;
        auto              item = std::make_unique<DragTarget>();
        item->input.setSource(&source);
        item->drag.threshold = 1;
        InteractionObserver observer;
        observer.setSource(&source);
        item->drag.finished = [&](QPointF, bool cancelled) {
            QVERIFY(cancelled);
            item.reset();
        };
        mouse(item.get(), QEvent::MouseButtonPress);
        mouse(item.get(), QEvent::MouseMove, { 50, 20 });
        QTRY_VERIFY(observer.dragged());
        item->setEnabled(false);
        QVERIFY(! item);
        QTRY_VERIFY(! observer.dragged());
    }
    void synchronousClickDeletion() {
        InteractionSource source;
        auto              item = std::make_unique<Clickable>();
        item->setSize({ 100, 50 });
        item->setInteractionSource(&source);
        QSignalSpy events(&source, &InteractionSource::interaction);
        connect(item.get(), &Clickable::clicked, this, [&] {
            item.reset();
        });
        mouse(item.get(), QEvent::MouseButtonPress);
        mouse(item.get(), QEvent::MouseButtonRelease);
        QVERIFY(! item);
        QTRY_COMPARE(events.count(), 2);
        QCOMPARE(events[1][0].value<Interaction>().phase, Interaction::End);
    }
    void nativePointerAndFocus() {
        QQuickWindow window;
        window.resize(240, 120);
        Clickable item(window.contentItem());
        item.setSize({ 100, 50 });
        InteractionObserver observer;
        observer.setSource(item.interactionSource());
        QSignalSpy clicks(&item, &Clickable::clicked);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(QTest::qWaitForWindowActive(&window));
        item.forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(observer.focused());
        QVERIFY(item.focusVisible());
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QTRY_VERIFY(observer.pressed());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QCOMPARE(clicks.count(), 1);
        QTRY_VERIFY(! observer.pressed());
    }
    void sharedProducers() {
        InteractionSource   source;
        InteractionObserver first, second;
        first.setSource(&source);
        second.setSource(&source);
        auto       a = source.emitter(), b = source.emitter();
        QSignalSpy events(&source, &InteractionSource::interaction);
        auto       one = a->begin(Interaction::Press);
        auto       two = b->begin(Interaction::Press);
        QVERIFY(one != two);
        QVERIFY(! first.pressed());
        QTRY_COMPARE(events.count(), 2);
        QVERIFY(first.pressed());
        QVERIFY(second.pressed());
        a->end(two);
        a.reset();
        QCOMPARE(events.count(), 2);
        QTRY_COMPARE(events.count(), 3);
        QVERIFY(first.pressed());
        b->end(two);
        QTRY_VERIFY(! first.pressed());
        QVERIFY(! second.pressed());
        const auto canceled = events[2][0].value<Interaction>();
        QCOMPARE(canceled.id, one);
        QCOMPARE(canceled.phase, Interaction::Cancel);
    }
    void noReplayAndSwitch() {
        InteractionSource first, second;
        auto              a = first.emitter(), b = second.emitter();
        QSignalSpy        delivered(&first, &InteractionSource::interaction);
        auto              old = a->begin(Interaction::Press);
        QTRY_COMPARE(delivered.count(), 1);
        InteractionObserver observer;
        observer.setSource(&first);
        QVERIFY(! observer.pressed());
        a->end(old);
        QTRY_COMPARE(delivered.count(), 2);
        QVERIFY(! observer.pressed());
        a->begin(Interaction::Press);
        QTRY_VERIFY(observer.pressed());
        observer.setSource(&second);
        QVERIFY(! observer.pressed());
        b->begin(Interaction::Drag);
        QTRY_VERIFY(observer.dragged());
        QVERIFY(! observer.pressed());
    }
    void rapidPairs() {
        InteractionSource   source;
        InteractionObserver observer;
        observer.setSource(&source);
        QSignalSpy events(&source, &InteractionSource::interaction);
        QSignalSpy changes(&observer, &InteractionObserver::changed);
        auto       emitter = source.emitter();
        for (int i = 0; i < 100; ++i) emitter->end(emitter->begin(Interaction::Press));
        QCOMPARE(events.count(), 0);
        QTRY_COMPARE(events.count(), 200);
        QCOMPARE(changes.count(), 200);
        QVERIFY(! observer.pressed());
        for (int i = 0; i < events.count(); i += 2) {
            const auto begin = events[i][0].value<Interaction>();
            const auto end   = events[i + 1][0].value<Interaction>();
            QCOMPARE(begin.id, end.id);
            QCOMPARE(begin.phase, Interaction::Begin);
            QCOMPARE(end.phase, Interaction::End);
        }
    }
    void independentPolicies() {
        InteractionSource      source;
        InteractionAccumulator input(this);
        input.setSource(&source);
        auto emitter = source.emitter();
        emitter->begin(Interaction::Hover);
        auto press = emitter->begin(Interaction::Press);
        emitter->begin(Interaction::Focus);
        QTRY_VERIFY(input.values().pressed);
        QCOMPARE(input.latest({ Interaction::Hover, Interaction::Press }), Interaction::Press);
        QCOMPARE(input.latest({ Interaction::Hover, Interaction::Focus }), Interaction::Focus);
        emitter->end(press, Interaction::Cancel);
        emitter->begin(Interaction::Drag);
        QTRY_VERIFY(input.values().dragged);
        QVERIFY(! input.values().pressed);
        QVERIFY(input.values().focused);
        QVERIFY(input.values().hovered);
    }
    void sourceDestroyed() {
        auto                source = std::make_unique<InteractionSource>();
        InteractionObserver observer;
        observer.setSource(source.get());
        QSignalSpy changed(&observer, &InteractionObserver::sourceChanged);
        auto       emitter = source->emitter();
        auto       token   = emitter->begin(Interaction::Press);
        QTRY_VERIFY(observer.pressed());
        source.reset();
        QVERIFY(! observer.source());
        QVERIFY(! observer.pressed());
        QCOMPARE(changed.count(), 1);
        QVERIFY(! emitter->contains(token));
        QCOMPARE(emitter->begin(Interaction::Drag), quint64(0));
        emitter.reset();
    }
    void producerDestroyedBeforeDelivery() {
        InteractionSource source;
        QSignalSpy        events(&source, &InteractionSource::interaction);
        {
            QQuickItem origin;
            auto       emitter = source.emitter(&origin);
            emitter->begin(Interaction::Press);
            emitter->begin(Interaction::Hover);
            emitter->begin(Interaction::Focus);
        }
        QCOMPARE(events.count(), 0);
        QTRY_COMPARE(events.count(), 6);
        for (int i = 0; i < 3; ++i) {
            const auto begin = events[i][0].value<Interaction>();
            const auto end   = events[i + 3][0].value<Interaction>();
            QVERIFY(! begin.originItem());
            QCOMPARE(end.id, begin.id);
            QCOMPARE(end.phase, i == 0 ? Interaction::Cancel : Interaction::End);
        }
    }
    void reentrantDelivery() {
        auto source  = std::make_unique<InteractionSource>();
        auto emitter = source->emitter();
        int  count   = 0;
        connect(source.get(), &InteractionSource::interaction, this, [&](const Interaction& event) {
            if (++count == 1)
                emitter->end(event.id);
            else
                source.reset();
        });
        emitter->begin(Interaction::Press);
        QTRY_COMPARE(count, 2);
        QVERIFY(! source);
    }
    void deleteObserverInCallback() {
        InteractionSource source;
        auto              observer = std::make_unique<InteractionObserver>();
        observer->setSource(&source);
        connect(observer.get(), &InteractionObserver::changed, this, [&] {
            observer.reset();
        });
        auto emitter = source.emitter();
        emitter->end(emitter->begin(Interaction::Press));
        QTRY_VERIFY(! observer);
    }
    void coordinates() {
        QQuickItem root, target(&root);
        target.setPosition({ 30, 40 });
        auto origin = std::make_unique<QQuickItem>(&root);
        origin->setPosition({ 10, 20 });
        origin->setScale(2);
        origin->setTransformOrigin(QQuickItem::TopLeft);
        InteractionSource source;
        QSignalSpy        events(&source, &InteractionSource::interaction);
        auto              emitter = source.emitter(origin.get());
        emitter->begin(Interaction::Press, Interaction::Mouse, 0, { 5, 6 });
        QTRY_COMPARE(events.count(), 1);
        const auto event = events[0][0].value<Interaction>();
        QCOMPARE(event.positionFor(&target), std::optional<QPointF>({ -10, -8 }));
        QQuickItem other;
        QVERIFY(! event.positionFor(&other));
        QQuickWindow window;
        QVERIFY(! event.positionFor(window.contentItem()));
        origin.reset();
        QVERIFY(! event.positionFor(&target));
    }
    void qmlRegistration() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                property int begins: 0
                property int clicks: 0
                MD.InteractionSource {
                    id: source
                    onInteraction: event => {
                        if (event.kind === MD.Interaction.Press && event.phase === MD.Interaction.Begin)
                            ++root.begins
                    }
                }
                MD.InteractionObserver { id: feedback; source: source }
                MD.Clickable {
                    objectName: "clickable"
                    width: 100; height: 50
                    interactionSource: source
                    onClicked: ++root.clicks
                }
                property bool pressed: feedback.pressed
                property int kind: MD.Interaction.Press
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("kind").toInt(), int(Interaction::Press));
        QVERIFY(! object->property("pressed").toBool());
        auto* item = object->findChild<Clickable*>("clickable");
        QVERIFY(item);
        mouse(item, QEvent::MouseButtonPress);
        QTRY_VERIFY(object->property("pressed").toBool());
        QCOMPARE(object->property("begins").toInt(), 1);
        mouse(item, QEvent::MouseButtonRelease);
        QCOMPARE(object->property("clicks").toInt(), 1);
        QTRY_VERIFY(! object->property("pressed").toBool());
    }
};
int run_interaction_source(int argc, char** argv) {
    InteractionSourceTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "interaction_source.moc"
