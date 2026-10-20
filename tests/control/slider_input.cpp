#include <QtTest>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "../input_test.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/control/range_slider.hpp"

using namespace qml_material;
using qmtest::events;
using qmtest::key;
using qmtest::mouse;

#include "qml_material/control/range_slider.hpp"

class SliderInputTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }

    void sliderTapAndDrag() {
        Slider                 control;
        InteractionAccumulator controlInput(this);
        controlInput.setSource(control.interactionSource());
        connect(&control, &Slider::valueChangeRequested, &control, &Slider::setValue);
        control.setSize({ 200, 40 });
        QSignalSpy input(control.interactionSource(), &InteractionSource::interaction),
            finished(&control, &Slider::interactionFinished);
        mouse(control, QEvent::MouseButtonPress, { 50, 20 });
        QCOMPARE(control.value(), 0);
        mouse(control, QEvent::MouseButtonRelease, { 50, 20 });
        QCOMPARE(control.value(), .25);
        QCOMPARE(finished.count(), 1);
        mouse(control, QEvent::MouseButtonPress, { 50, 20 });
        mouse(control, QEvent::MouseMove, { 150, 20 });
        QVERIFY(controlInput.values().dragged);
        control.setEnabled(false);
        QTRY_COMPARE(events(input, Interaction::Drag).last().phase, Interaction::Cancel);
        QVERIFY(! (controlInput.values().pressed || controlInput.values().dragged));
    }

    void sliderCrossAxisCancellation() {
        Slider                 control;
        InteractionAccumulator controlInput(this);
        controlInput.setSource(control.interactionSource());
        control.setSize({ 200, 40 });
        QSignalSpy finished(&control, &Slider::interactionFinished);
        mouse(control, QEvent::MouseButtonPress, { 50, 20 });
        mouse(control, QEvent::MouseMove, { 50, 100 });
        QVERIFY(! (controlInput.values().pressed || controlInput.values().dragged));
        QCOMPARE(control.value(), 0);
        QCOMPARE(finished.count(), 1);
        QCOMPARE(finished[0][0].toBool(), true);
    }

    void rangeTarget() {
        RangeSlider control;
        control.setSize({ 200, 40 });
        control.setValues(.2, .8);
        QSignalSpy first(control.first()->interactionSource(), &InteractionSource::interaction);
        QSignalSpy second(control.second()->interactionSource(), &InteractionSource::interaction);
        mouse(control, QEvent::MouseButtonPress, { 160, 20 });
        flushInteractions();
        QVERIFY(events(first, Interaction::Press).isEmpty());
        QCOMPARE(events(second, Interaction::Press).size(), 1);
        mouse(control, QEvent::MouseMove, { 120, 20 });
        flushInteractions();
        QCOMPARE(events(second, Interaction::Drag).size(), 1);
        control.setEnabled(false);
        flushInteractions();
        QCOMPARE(events(second, Interaction::Drag).last().phase, Interaction::Cancel);
    }

    void rangeFocusTargets() {
        QQuickWindow window;
        window.resize(300, 100);
        RangeSlider control(window.contentItem());
        control.setSize({ 200, 50 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        InteractionAccumulator first(this), second(this);
        first.setSource(control.first()->interactionSource());
        second.setSource(control.second()->interactionSource());
        control.forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(first.values().focused);
        QVERIFY(control.first()->focusVisible());
        QVERIFY(! second.values().focused);
        control.setFocusedHandle(1);
        flushInteractions();
        QVERIFY(! first.values().focused);
        QVERIFY(! control.first()->focusVisible());
        QVERIFY(second.values().focused);
        QVERIFY(control.second()->focusVisible());
    }

    void sliderInsideScrollable() {
        QQuickWindow window;
        window.resize(400, 300);
        Flickable view(window.contentItem());
        view.setSize({ 400, 300 });
        view.setContentHeight(900);
        view.setContentWidth(400);
        view.setContentY(100);
        Slider                 slider(view.contentItem());
        InteractionAccumulator sliderInput(this);
        sliderInput.setSource(slider.interactionSource());
        connect(&slider, &Slider::valueChangeRequested, &slider, &Slider::setValue);
        slider.setY(100);
        slider.setSize({ 400, 100 });
        slider.setValue(.2);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        static auto* device = QTest::createTouchDevice();
        auto         touch  = QTest::touchEvent(&window, device, false);
        const auto   flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 80, 40 }, &window).commit();
        flush();
        touch.move(0, { 81, 80 }, &window).commit();
        flush();
        touch.move(0, { 82, 120 }, &window).commit();
        flush();
        QVERIFY(view.isDragging());
        QVERIFY(! (sliderInput.values().pressed || sliderInput.values().dragged));
        QCOMPARE(slider.value(), .2);
        touch.release(0, { 82, 120 }, &window).commit();
        flush();
        view.cancelFlick();
        view.setContentY(100);
        touch.press(0, { 80, 40 }, &window).commit();
        flush();
        touch.move(0, { 160, 41 }, &window).commit();
        flush();
        touch.move(0, { 240, 42 }, &window).commit();
        flush();
        QVERIFY((sliderInput.values().pressed || sliderInput.values().dragged));
        QVERIFY(! view.isDragging());
        QCOMPARE(slider.value(), .6);
        touch.release(0, { 240, 42 }, &window).commit();
        flush();
        QVERIFY(! (sliderInput.values().pressed || sliderInput.values().dragged));
    }

    void keyboardSlider() {
        Slider                 control;
        InteractionAccumulator controlInput(this);
        controlInput.setSource(control.interactionSource());
        connect(&control, &Slider::valueChangeRequested, &control, &Slider::setValue);
        control.setSize({ 200, 40 });
        QSignalSpy completed(&control, &Slider::interactionFinished),
            input(control.interactionSource(), &InteractionSource::interaction);
        key(control, QEvent::KeyPress, Qt::Key_Right);
        QCOMPARE(control.value(), .01);
        key(control, QEvent::KeyPress, Qt::Key_PageUp);
        QCOMPARE(control.value(), .11);
        key(control, QEvent::KeyRelease, Qt::Key_Right);
        key(control, QEvent::KeyRelease, Qt::Key_PageUp);
        QCOMPARE(completed.count(), 1);
        key(control, QEvent::KeyPress, Qt::Key_End);
        QCOMPARE(control.value(), 1);
        key(control, QEvent::KeyRelease, Qt::Key_End);
        QVERIFY(events(input, Interaction::Press).isEmpty());
    }

    void sliderDeletedDuringConfiguration() {
        auto* control = new Slider;
        control->setSize({ 200, 40 });
        mouse(*control, QEvent::MouseButtonPress, { 50, 20 });
        QPointer<Slider> guard(control);
        connect(control, &Slider::interactionFinished, control, [control] {
            delete control;
        });
        control->setFrom(-1);
        QVERIFY(! guard);
    }

    void sliderPressed() {
        QQuickWindow window;
        window.resize(240, 120);
        Slider                 slider(window.contentItem());
        InteractionAccumulator input(this);
        input.setSource(slider.interactionSource());
        slider.setSize({ 200, 60 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 30, 30 });
        QTRY_VERIFY(input.values().pressed);
        slider.setEnabled(false);
        QTRY_VERIFY(! input.values().pressed);
    }

    void rangeSources() {
        RangeSlider slider;
        slider.setSize({ 200, 40 });
        slider.setValues(.2, .8);
        InteractionSource shared;
        slider.first()->setInteractionSource(&shared);
        slider.second()->setInteractionSource(&shared);
        InteractionAccumulator input(this);
        input.setSource(&shared);
        auto       other = shared.emitter();
        const auto token = other->begin(Interaction::Press);
        mouse(slider, QEvent::MouseButtonPress, { 160, 20 });
        QVERIFY(input.values().pressed);
        slider.setEnabled(false);
        flushInteractions();
        QVERIFY(input.values().pressed);
        other->end(token);
        flushInteractions();
        QVERIFY(! input.values().pressed);
        slider.setEnabled(true);
        mouse(slider, QEvent::MouseButtonPress, { 40, 20 });
        InteractionSource replacement;
        slider.first()->setInteractionSource(&replacement);
        mouse(slider, QEvent::MouseButtonRelease, { 100, 20 });
        QCOMPARE(slider.first()->value(), .2);
        QCOMPARE(slider.activeHandle(), -1);
    }
};

int run_slider_input(int argc, char** argv) {
    SliderInputTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "slider_input.moc"
