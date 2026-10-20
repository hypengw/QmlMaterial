#include <QtTest>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "../input_test.hpp"
#include "qml_material/control/combined_command_button.hpp"

using namespace qml_material;
using qmtest::events;
using qmtest::key;
using qmtest::mouse;

class InputHostTest : public QObject {
    Q_OBJECT

private slots:
    void qmlKeysPriority_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::newRow("button") << QByteArray("CombinedCommandButtonBase");
        QTest::newRow("clickable") << QByteArray("Clickable");
    }

    void qmlKeysPriority() {
        QFETCH(QByteArray, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport Qcm.Material as MD\nMD." + type + R"(
            {
                property bool intercept: true
                property int calls: 0
                Keys.onPressed: event => { event.accepted = intercept }
                Keys.onReleased: event => { event.accepted = intercept }
                onClicked: calls++
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(object.get());
        key(*item, QEvent::KeyPress, Qt::Key_Space);
        key(*item, QEvent::KeyRelease, Qt::Key_Space);
        QCOMPARE(object->property("calls").toInt(), 0);
        object->setProperty("intercept", false);
        key(*item, QEvent::KeyPress, Qt::Key_Space);
        key(*item, QEvent::KeyRelease, Qt::Key_Space);
        QCOMPARE(object->property("calls").toInt(), 1);
    }

    void buttonPointerFocusPolicy() {
        QQuickWindow window;
        window.resize(240, 120);
        CombinedCommandButton button(window.contentItem());
        button.setSize({ 100, 40 });
        button.setFocusPolicy(Qt::StrongFocus);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QVERIFY(button.hasActiveFocus());
        QVERIFY(! button.focusVisible());
    }

    void standardButtonSharedSource() {
        InteractionSource     shared, replacement;
        CombinedCommandButton first, second;
        first.setSize({ 100, 40 });
        second.setSize({ 100, 40 });
        first.setInteractionSource(&shared);
        second.setInteractionSource(&shared);
        InteractionAccumulator input(this);
        input.setSource(&shared);
        QSignalSpy firstClicks(&first, &CommandButton::clicked);
        QSignalSpy secondClicks(&second, &CommandButton::clicked);
        mouse(first, QEvent::MouseButtonPress, { 10, 10 });
        mouse(second, QEvent::MouseButtonPress, { 10, 10 });
        QCOMPARE(input.active().size(), 2);
        first.setInteractionSource(&replacement);
        flushInteractions();
        QCOMPARE(input.active().size(), 1);
        mouse(first, QEvent::MouseButtonRelease, { 10, 10 });
        mouse(second, QEvent::MouseButtonRelease, { 10, 10 });
        QCOMPARE(firstClicks.size(), 0);
        QCOMPARE(secondClicks.size(), 1);
        QVERIFY(input.active().isEmpty());
    }

    void standardButtonExplicitCombinedClick() {
        CombinedCommandButton button;
        button.setSize({ 100, 40 });
        QSignalSpy clicks(&button, &CommandButton::clicked);
        QSignalSpy doubles(&button, &CombinedCommandButton::doubleClicked);
        QSignalSpy holds(&button, &CombinedCommandButton::longClicked);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 }, 100);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 }, 120);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 }, 180);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 }, 200);
        QCOMPARE(clicks.size(), 2);
        QVERIFY(doubles.isEmpty() && holds.isEmpty());
    }

    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }

    void qmlInputNotifications() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"QML(
            import QtQuick
            import Qcm.Material as MD
            MD.CombinedCommandButtonBase {
                id: button
                width: 100; height: 50
                MD.InteractionObserver { id: feedback; source: button.interactionSource }
                property int activeCount: feedback.pressed ? 1 : 0
                property int begins: 0
                property int ends: 0
                Connections {
                  target: button.interactionSource
                  function onInteraction(event) {
                    if (event.kind !== MD.Interaction.Press) return;
                    if (event.phase === MD.Interaction.Begin) ++begins;
                    if (event.phase === MD.Interaction.Cancel) ++ends;
                  }
                }
            }
        )QML",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<CombinedCommandButton*>(object.get());
        QVERIFY(button);
        setButtonPressed(button, true);
        QCOMPARE(object->property("activeCount").toInt(), 1);
        setButtonPressed(button, false);
        QCOMPARE(object->property("activeCount").toInt(), 0);
        QCOMPARE(object->property("begins").toInt(), 1);
        QCOMPARE(object->property("ends").toInt(), 1);
    }

    void ownerNotifications() {
        CombinedCommandButton control;
        auto*                 source = control.interactionSource();
        QSignalSpy            first(source, &InteractionSource::interaction),
            second(source, &InteractionSource::interaction);
        auto       emitter = source->emitter(&control);
        const auto token   = emitter->begin(Interaction::Press, Interaction::Touch, 7, { 3, 4 });
        emitter->end(token);
        flushInteractions();
        QCOMPARE(first.count(), 2);
        QCOMPARE(second.count(), 2);
        QCOMPARE(events(first, Interaction::Press)[0].position, QPointF(3, 4));
        QCOMPARE(events(first, Interaction::Press)[1].id, token);
    }

    void staleIdentity() {
        CombinedCommandButton  control;
        InteractionAccumulator input(this);
        input.setSource(control.interactionSource());
        auto       emitter = control.interactionSource()->emitter(&control);
        const auto first   = emitter->begin(Interaction::Press);
        emitter->end(first, Interaction::Cancel);
        const auto second = emitter->begin(Interaction::Press);
        emitter->end(first);
        flushInteractions();
        QVERIFY(input.values().pressed);
        emitter->end(second);
        flushInteractions();
        QVERIFY(! input.values().pressed);
    }

    void reentrantCancellationOrder() {
        CombinedCommandButton     control;
        auto*                     source  = control.interactionSource();
        auto                      emitter = source->emitter(&control);
        QList<Interaction::Phase> phases;
        connect(source, &InteractionSource::interaction, &control, [&](const Interaction& event) {
            if (event.phase == Interaction::Begin) emitter->cancel();
        });
        connect(source, &InteractionSource::interaction, &control, [&](const Interaction& event) {
            phases.append(event.phase);
        });
        emitter->begin(Interaction::Press);
        flushInteractions();
        QCOMPARE(phases, QList<Interaction::Phase>({ Interaction::Begin, Interaction::Cancel }));
    }

    void deletedInSourceObserver() {
        auto*                           control = new CombinedCommandButton;
        QPointer<CombinedCommandButton> guard(control);
        auto                            emitter = control->interactionSource()->emitter(control);
        connect(control->interactionSource(), &InteractionSource::interaction, control, [control] {
            delete control;
        });
        emitter->begin(Interaction::Hover);
        flushInteractions();
        QVERIFY(! guard);
    }

    void pointerCancellation() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        QSignalSpy clicks(&button, &CommandButton::clicked),
            input(button.interactionSource(), &InteractionSource::interaction);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 });
        QVERIFY(buttonInput.values().pressed);
        mouse(button, QEvent::MouseMove, { 110, 10 });
        QVERIFY(! buttonInput.values().pressed);
        mouse(button, QEvent::MouseMove, { 10, 10 });
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 });
        QCOMPARE(clicks.count(), 0);
        const auto presses = events(input, Interaction::Press);
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[1].phase, Interaction::Cancel);
    }

    void multipleKeys() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        QSignalSpy clicks(&button, &CommandButton::clicked),
            input(button.interactionSource(), &InteractionSource::interaction);
        key(button, QEvent::KeyPress, Qt::Key_Space);
        key(button, QEvent::KeyPress, Qt::Key_Return);
        key(button, QEvent::KeyPress, Qt::Key_Space, true);
        QCOMPARE(events(input, Interaction::Press).size(), 2);
        key(button, QEvent::KeyRelease, Qt::Key_Space);
        QVERIFY(buttonInput.values().pressed);
        QCOMPARE(clicks.count(), 1);
        key(button, QEvent::KeyRelease, Qt::Key_Return);
        QVERIFY(! buttonInput.values().pressed);
        QCOMPARE(clicks.count(), 2);
        key(button, QEvent::KeyRelease, Qt::Key_Return);
        QCOMPARE(clicks.count(), 2);
    }

    void disabledInPressObserver() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        QSignalSpy clicks(&button, &CommandButton::clicked);
        connect(button.interactionSource(),
                &InteractionSource::interaction,
                &button,
                [&](const auto& event) {
                    if (event.kind == Interaction::Press && event.phase == Interaction::Begin)
                        button.setEnabled(false);
                });
        mouse(button, QEvent::MouseButtonPress, { 10, 10 });
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 });
        QVERIFY(! buttonInput.values().pressed);
        QCOMPARE(clicks.count(), 0);
    }

    void deletedInPressBinding() {
        auto* button = new CombinedCommandButton;
        button->setSize({ 100, 50 });
        QPointer<CombinedCommandButton> guard(button);
        InteractionAccumulator          input(this);
        input.setSource(button->interactionSource());
        auto observer = input.bindableValues().addNotifier([button] {
            delete button;
        });
        mouse(*button, QEvent::MouseButtonPress, { 10, 10 });
        QVERIFY(! guard);
    }

    void programmaticActivation() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        QSignalSpy clicks(&button, &CommandButton::clicked),
            input(button.interactionSource(), &InteractionSource::interaction);
        button.click();
        QCOMPARE(clicks.count(), 1);
        QVERIFY(events(input, Interaction::Press).isEmpty());
    }

    void longPress() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        button.setLongClickEnabled(true);
        button.setLongClickInterval(1);
        QSignalSpy holds(&button, &CombinedCommandButton::longClicked),
            clicks(&button, &CommandButton::clicked);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 });
        QTRY_COMPARE(holds.count(), 1);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 });
        QCOMPARE(clicks.count(), 0);
        QVERIFY(! buttonInput.values().pressed);
    }

    void doubleClickConsumesBothTaps() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        button.setDoubleClickEnabled(true);
        QSignalSpy doubleClicks(&button, &CombinedCommandButton::doubleClicked),
            clicks(&button, &CommandButton::clicked);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 }, 100);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 }, 120);
        QCOMPARE(clicks.count(), 0);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 }, 180);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 }, 200);
        QCOMPARE(doubleClicks.count(), 1);
        QCOMPARE(clicks.count(), 0);
        QVERIFY(! buttonInput.values().pressed);
    }

    void tooSoonTapIsIgnored() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        button.setDoubleClickEnabled(true);
        QSignalSpy doubleClicks(&button, &CombinedCommandButton::doubleClicked),
            clicks(&button, &CommandButton::clicked),
            input(button.interactionSource(), &InteractionSource::interaction);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 }, 100);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 }, 120);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 }, 130);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 }, 140);
        QCOMPARE(doubleClicks.count(), 0);
        QCOMPARE(events(input, Interaction::Press).size(), 2);
        QTRY_COMPARE(clicks.count(), 1);
    }

    void keyboardDoubleClick() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setDoubleClickEnabled(true);
        QSignalSpy doubleClicks(&button, &CombinedCommandButton::doubleClicked),
            clicks(&button, &CommandButton::clicked),
            input(button.interactionSource(), &InteractionSource::interaction);
        key(button, QEvent::KeyPress, Qt::Key_Space, false, 100);
        key(button, QEvent::KeyRelease, Qt::Key_Space, false, 120);
        key(button, QEvent::KeyPress, Qt::Key_Space, false, 180);
        key(button, QEvent::KeyRelease, Qt::Key_Space, false, 200);
        QCOMPARE(doubleClicks.count(), 1);
        QCOMPARE(clicks.count(), 0);
        QCOMPARE(events(input, Interaction::Press).size(), 4);
        QVERIFY(! buttonInput.values().pressed);
    }

    void tooSoonKeyStartsSeparateClick() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setDoubleClickEnabled(true);
        QSignalSpy doubleClicks(&button, &CombinedCommandButton::doubleClicked),
            clicks(&button, &CommandButton::clicked),
            input(button.interactionSource(), &InteractionSource::interaction);
        key(button, QEvent::KeyPress, Qt::Key_Space, false, 100);
        key(button, QEvent::KeyRelease, Qt::Key_Space, false, 120);
        key(button, QEvent::KeyPress, Qt::Key_Space, false, 130);
        QCOMPARE(clicks.count(), 1);
        QVERIFY(buttonInput.values().pressed);
        key(button, QEvent::KeyRelease, Qt::Key_Space, false, 140);
        QCOMPARE(doubleClicks.count(), 0);
        QCOMPARE(events(input, Interaction::Press).size(), 4);
        QTRY_COMPARE(clicks.count(), 2);
    }

    void focusLossDuringKeyBegin() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        QSignalSpy clicks(&button, &CommandButton::clicked),
            input(button.interactionSource(), &InteractionSource::interaction);
        connect(button.interactionSource(),
                &InteractionSource::interaction,
                &button,
                [&](const Interaction& event) {
                    if (event.kind != Interaction::Press || event.phase != Interaction::Begin)
                        return;
                    QFocusEvent blur(QEvent::FocusOut, Qt::TabFocusReason);
                    QCoreApplication::sendEvent(&button, &blur);
                });
        key(button, QEvent::KeyPress, Qt::Key_Space);
        key(button, QEvent::KeyRelease, Qt::Key_Space);
        QVERIFY(! buttonInput.values().pressed);
        QCOMPARE(clicks.count(), 0);
        const auto presses = events(input, Interaction::Press);
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[0].id, presses[1].id);
        QCOMPARE(presses[1].phase, Interaction::Cancel);
    }

    void actualFocusIndependentOfVisualFocus() {
        QQuickWindow window;
        window.resize(240, 120);
        CombinedCommandButton  control(window.contentItem());
        InteractionAccumulator controlInput(this);
        controlInput.setSource(control.interactionSource());
        control.setSize({ 100, 50 });
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        control.forceActiveFocus(Qt::MouseFocusReason);
        QTRY_VERIFY(control.hasActiveFocus());
        QVERIFY(! control.focusVisible());
        QTRY_VERIFY(controlInput.values().focused);
        control.setFocus(false);
        QTRY_VERIFY(! controlInput.values().focused);
    }

    void delayedShortTap() {
        QQuickWindow window;
        window.resize(240, 200);
        Flickable view(window.contentItem());
        view.setSize({ 240, 200 });
        view.setContentHeight(800);
        view.setContentWidth(240);
        auto* policy =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&view, true));
        QVERIFY(policy);
        policy->setPressDelay(2000);
        CombinedCommandButton  button(view.contentItem());
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 200, 160 });
        QSignalSpy input(button.interactionSource(), &InteractionSource::interaction),
            clicks(&button, &CommandButton::clicked);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 80, 80 }, 0);
        QVERIFY(events(input, Interaction::Press).isEmpty());
        QVERIFY(! buttonInput.values().pressed);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 80, 80 }, 0);
        QCOMPARE(clicks.count(), 1);
        const auto presses = events(input, Interaction::Press);
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[0].phase, Interaction::Begin);
        QCOMPARE(presses[1].phase, Interaction::End);
        QCOMPARE(presses[0].id, presses[1].id);
        QVERIFY(! buttonInput.values().pressed);
    }

    void longPressBeforeFeedbackDelay() {
        Flickable view;
        view.setSize({ 240, 200 });
        view.setContentHeight(800);
        view.setContentWidth(240);
        auto* policy =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&view, true));
        QVERIFY(policy);
        policy->setPressDelay(2000);
        CombinedCommandButton  button(view.contentItem());
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 200, 160 });
        button.setLongClickEnabled(true);
        button.setLongClickInterval(1);
        QSignalSpy input(button.interactionSource(), &InteractionSource::interaction),
            clicks(&button, &CommandButton::clicked),
            holds(&button, &CombinedCommandButton::longClicked);
        mouse(button, QEvent::MouseButtonPress, { 80, 80 });
        QTRY_COMPARE(holds.count(), 1);
        QVERIFY(events(input, Interaction::Press).isEmpty());
        mouse(button, QEvent::MouseButtonRelease, { 80, 80 });
        QCOMPARE(clicks.count(), 0);
        const auto presses = events(input, Interaction::Press);
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[0].id, presses[1].id);
        QCOMPARE(presses[1].phase, Interaction::End);
        QVERIFY(! buttonInput.values().pressed);
    }

    void scrollingCancelsPendingPress() {
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
        CombinedCommandButton  button(view.contentItem());
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 200, 160 });
        QSignalSpy input(button.interactionSource(), &InteractionSource::interaction),
            clicks(&button, &CommandButton::clicked);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
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
        QVERIFY(! buttonInput.values().pressed);
        QVERIFY(events(input, Interaction::Press).isEmpty());
        touch.release(0, { 80, 40 }, &window).commit();
        flush();
        QCOMPARE(clicks.count(), 0);
    }

    void delayedPressExpires() {
        QQuickWindow window;
        window.resize(240, 200);
        Flickable view(window.contentItem());
        view.setSize({ 240, 200 });
        view.setContentHeight(800);
        view.setContentWidth(240);
        auto* policy =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&view, true));
        QVERIFY(policy);
        policy->setPressDelay(1);
        CombinedCommandButton  button(view.contentItem());
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 200, 160 });
        QSignalSpy input(button.interactionSource(), &InteractionSource::interaction),
            clicks(&button, &CommandButton::clicked);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 80, 80 }, 0);
        QTRY_VERIFY(buttonInput.values().pressed);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 80, 80 }, 0);
        QCOMPARE(events(input, Interaction::Press).size(), 2);
        QCOMPARE(clicks.count(), 1);
    }

    void lostFocusCancelsKeysNotPointer() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        mouse(button, QEvent::MouseButtonPress, { 10, 10 });
        key(button, QEvent::KeyPress, Qt::Key_Space);
        QSignalSpy input(button.interactionSource(), &InteractionSource::interaction),
            clicks(&button, &CommandButton::clicked);
        QFocusEvent blur(QEvent::FocusOut, Qt::TabFocusReason);
        QCoreApplication::sendEvent(&button, &blur);
        QVERIFY(buttonInput.values().pressed);
        key(button, QEvent::KeyRelease, Qt::Key_Space);
        QCOMPARE(clicks.count(), 0);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 });
        QCOMPARE(clicks.count(), 1);
        QCOMPARE(events(input, Interaction::Press).size(), 2);
    }

    void pointerCancellationPreservesKeys() {
        CombinedCommandButton  button;
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        QSignalSpy clicks(&button, &CommandButton::clicked);
        key(button, QEvent::KeyPress, Qt::Key_Space);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 });
        mouse(button, QEvent::MouseMove, { 120, 10 });
        QVERIFY(buttonInput.values().pressed);
        key(button, QEvent::KeyRelease, Qt::Key_Space);
        QCOMPARE(clicks.count(), 1);
        QVERIFY(! buttonInput.values().pressed);
    }

    void multiTouchCompletesOnce() {
        QQuickWindow window;
        window.resize(240, 200);
        CombinedCommandButton  button(window.contentItem());
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 200, 160 });
        QSignalSpy input(button.interactionSource(), &InteractionSource::interaction),
            clicks(&button, &CommandButton::clicked);
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
        const auto presses = events(input, Interaction::Press);
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[0].identity, 0);
        QCOMPARE(presses[0].id, presses[1].id);
        QVERIFY(! buttonInput.values().pressed);
    }

    void reparentCancelsInput() {
        QQuickItem             first, second;
        CombinedCommandButton  button(&first);
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        QSignalSpy clicks(&button, &CommandButton::clicked);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 });
        button.setParentItem(&second);
        QTRY_VERIFY(! buttonInput.values().pressed);
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 });
        QCOMPARE(clicks.count(), 0);
        mouse(button, QEvent::MouseButtonPress, { 10, 10 });
        mouse(button, QEvent::MouseButtonRelease, { 10, 10 });
        QCOMPARE(clicks.count(), 1);
    }

    void windowPointer() {
        QQuickWindow window;
        window.resize(240, 120);
        CombinedCommandButton  button(window.contentItem());
        InteractionAccumulator buttonInput(this);
        buttonInput.setSource(button.interactionSource());
        button.setSize({ 100, 50 });
        QSignalSpy clicks(&button, &CommandButton::clicked);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QVERIFY(buttonInput.values().pressed);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QCOMPARE(clicks.count(), 1);
        QVERIFY(! buttonInput.values().pressed);
    }
};

int run_input_host(int argc, char** argv) {
    InputHostTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "input_host.moc"
