#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QMouseEvent>
#include "qml_material/control/number_input.hpp"
#include "qml_material/control/repeat_button.hpp"
#include "qml_material/control/page_indicator.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/control/action_command.hpp"
#include "qml_material/input/selectable.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/control/color_input.hpp"
#include "qml_material/control/time_field.hpp"
#include "qml_material/input/date_picker_state.hpp"
#include "qml_material/input/swipe_to_dismiss_state.hpp"
#include "qml_material/input/toggleable.hpp"
#include <QAccessible>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "../input_test.hpp"

using namespace qml_material;
namespace
{
class DragTriangle : public ColorTriangle {
public:
    using InteractiveItem::inputHost;
};
void mouse(QQuickItem* item, QEvent::Type type, QPointF point = { 8, 8 }) {
    QMouseEvent event(type,
                      point,
                      point,
                      Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(item, &event);
    flushInteractions();
}
} // namespace
class ValueInputTest : public QObject {
    Q_OBJECT
private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*")); }
    void creation_data() {
        QTest::addColumn<QByteArray>("type");
        for (const auto* name : { "NumberStepper",
                                  "StepperInput",
                                  "ActionLabel",
                                  "ColorRadio",
                                  "PageIndicator",
                                  "ChannelSlider",
                                  "HueChannelSlider",
                                  "ColorWheel",
                                  "ColorTriangle",
                                  "ColorPicker",
                                  "DatePicker",
                                  "TimeInput" })
            QTest::newRow(name) << QByteArray(name);
    }
    void creation() {
        QFETCH(QByteArray, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick; import Qcm.Material as MD; MD." + type + " {}", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        if (type == "StepperInput") {
            const auto* input = qobject_cast<NumberInput*>(object.get());
            QVERIFY(input && input->editor());
            QVERIFY(input->editor()->width() > 0);
            QCOMPARE(input->editor()->height(), input->height());
            QCOMPARE(input->editor()->text(), input->formattedValue());
        }
    }
    void repeatReleaseAndCancellation_data() {
        QTest::addColumn<int>("cancel");
        for (int i = 0; i < 6; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void repeatReleaseAndCancellation() {
        QFETCH(int, cancel);
        RepeatButton button;
        button.setSize({ 40, 40 });
        button.setRepeatDelay(1);
        button.setRepeatInterval(10000);
        InteractionSource replacement;
        QSignalSpy        clicked(&button, &CommandButton::clicked);
        mouse(&button, QEvent::MouseButtonPress);
        QTRY_COMPARE(clicked.size(), 1);
        if (cancel == 0) mouse(&button, QEvent::MouseButtonRelease);
        if (cancel == 1) button.setEnabled(false);
        if (cancel == 2) button.setVisible(false);
        if (cancel == 3) button.setInteractionSource(&replacement);
        if (cancel == 4) mouse(&button, QEvent::MouseMove, { 50, 50 });
        if (cancel == 5) button.setRepeatInterval(2);
        mouse(&button, QEvent::MouseButtonRelease);
        QCoreApplication::processEvents();
        QCOMPARE(clicked.size(), 1);
    }
    void repeatKeyboardAndReentry() {
        auto button = std::make_unique<RepeatButton>();
        button->setSize({ 40, 40 });
        button->setRepeatDelay(1);
        button->setRepeatInterval(10000);
        int count = 0;
        connect(button.get(), &CommandButton::clicked, this, [&] {
            ++count;
            button.reset();
        });
        QKeyEvent press(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
        QCoreApplication::sendEvent(button.get(), &press);
        QTRY_COMPARE(count, 1);
        QVERIFY(! button);
    }
    void numberRequests() {
        NumberInput  input;
        RepeatButton up, down;
        input.setIncreaseButton(&up);
        input.setDecreaseButton(&down);
        input.setFrom(0);
        input.setTo(10);
        input.setValue(10);
        QSignalSpy requested(&input, &NumberInput::valueChangeRequested);
        up.click();
        QCOMPARE(requested.size(), 0);
        input.setWrap(true);
        up.click();
        QCOMPARE(requested.size(), 1);
        QCOMPARE(requested[0][0].toReal(), 1);
        QCOMPARE(input.value(), 10);
        input.setTo(0);
        up.click();
        QCOMPARE(requested.size(), 2);
        QCOMPARE(requested[1][0].toReal(), 0);
        input.setStep(std::numeric_limits<qreal>::infinity());
        QCOMPARE(input.step(), 1);
        input.setTo(10);
        input.setValue(5);
        connect(&input, &NumberInput::valueChangeRequested, &input, &NumberInput::setValue);
        down.click();
        QCOMPARE(input.value(), 4);
        input.setIncreaseButton(nullptr);
        up.click();
        QCOMPARE(input.value(), 4);
    }
    void repeatFocusLoss() {
        RepeatButton button;
        button.setRepeatDelay(1);
        button.setRepeatInterval(10000);
        int count = 0;
        connect(&button, &CommandButton::clicked, this, [&] {
            ++count;
            QFocusEvent blur(QEvent::FocusOut);
            QCoreApplication::sendEvent(&button, &blur);
        });
        for (int key : { Qt::Key_Space, Qt::Key_Return }) {
            QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
            QCoreApplication::sendEvent(&button, &press);
        }
        QTRY_COMPARE(count, 1);
        for (int key : { Qt::Key_Space, Qt::Key_Return }) {
            QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
            QCoreApplication::sendEvent(&button, &release);
        }
        QCOMPARE(count, 1);
    }
    void dateReentry() {
        DatePickerState state;
        TextField       start, end;
        state.setSelectionMode(1);
        state.setStartInput(&start);
        state.setEndInput(&end);
        const auto first    = QDate(2024, 2, 1).startOfDay();
        const auto last     = QDate(2024, 3, 1).startOfDay();
        bool       replaced = false;
        connect(&start, &QQuickTextInput::textChanged, this, [&] {
            if (std::exchange(replaced, true)) return;
            state.setRange(last, last);
        });
        state.setRange(first, first);
        QCOMPARE(start.text(), state.rules()->formatDate(last));
        QCOMPARE(end.text(), state.rules()->formatDate(last));
        bool navigated = false;
        connect(&state, &DatePickerState::viewChanged, this, [&] {
            if (std::exchange(navigated, true)) return;
            state.setYear(2025);
            state.setMonth(7);
        });
        state.setSelectedDate(first);
        QCOMPARE(state.year(), 2025);
        QCOMPARE(state.month(), 7);
    }
    void numberEditor() {
        NumberInput input;
        TextField   editor;
        input.setEditor(&editor);
        input.setFrom(0);
        input.setTo(10);
        input.setValue(5);
        QSignalSpy requested(&input, &NumberInput::valueChangeRequested);
        editor.setText("9");
        Q_EMIT editor.editingFinished();
        QCOMPARE(requested.size(), 1);
        QCOMPARE(input.value(), 5);
        QCOMPARE(editor.text(), QString("5"));
        editor.setText("9junk");
        Q_EMIT editor.editingFinished();
        QCOMPARE(requested.size(), 1);
        QCOMPARE(editor.text(), QString("5"));
        connect(&input, &NumberInput::valueChangeRequested, &input, &NumberInput::setValue);
        editor.setText("12");
        Q_EMIT editor.editingFinished();
        QCOMPARE(input.value(), 10);
        QCOMPARE(editor.text(), QString("10"));
    }
    void indicatorRequests() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick; import Qcm.Material as MD; MD.PageIndicator { count: 3; "
                          "interactive: true; width: 100; height: 32 }",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* indicator = qobject_cast<PageIndicator*>(object.get());
        QVERIFY(indicator);
        QVERIFY(indicator->itemAt(2));
        // Explicit geometry keeps this recognizer test independent of the Row polish pass.
        for (int i = 0; i < 3; ++i) indicator->itemAt(i)->setX(i * 20);
        const auto point = indicator->itemAt(2)->mapToItem(indicator, { 4, 4 });
        QSignalSpy requested(indicator, &PageIndicator::currentIndexChangeRequested);
        mouse(indicator, QEvent::MouseButtonPress, point);
        QCOMPARE(indicator->highlightedIndex(), 2);
        mouse(indicator, QEvent::MouseButtonRelease, point);
        QCOMPARE(requested.size(), 1);
        QCOMPARE(requested[0][0].toInt(), 2);
        QCOMPARE(indicator->currentIndex(), 0);
        QCOMPARE(indicator->highlightedIndex(), -1);
        connect(indicator,
                &PageIndicator::currentIndexChangeRequested,
                indicator,
                &PageIndicator::setCurrentIndex);
        QKeyEvent key(QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
        QCoreApplication::sendEvent(indicator, &key);
        QCOMPARE(indicator->currentIndex(), 2);
    }
    void triangleGeometryAndRequests() {
        ColorTriangle triangle;
        triangle.setSize({ 180, 180 });
        triangle.setSaturation(.5);
        triangle.setValue(.5);
        QSignalSpy requested(&triangle, &ColorInput::svChangeRequested);
        QVERIFY(triangle.contains(triangle.hueVertex()));
        QVERIFY(! triangle.contains({ 0, 0 }));
        mouse(&triangle, QEvent::MouseButtonPress, triangle.hueVertex());
        QCOMPARE(requested.size(), 1);
        QVERIFY(qAbs(requested[0][0].toReal() - 1) < 1e-6);
        QVERIFY(qAbs(requested[0][1].toReal() - 1) < 1e-6);
        QCOMPARE(triangle.saturation(), .5);
        QCOMPARE(triangle.value(), .5);
        mouse(&triangle, QEvent::MouseButtonRelease, triangle.hueVertex());
        QCOMPARE(requested.size(), 1);
        connect(&triangle, &ColorInput::svChangeRequested, &triangle, [&](qreal s, qreal v) {
            triangle.setSaturation(s);
            triangle.setValue(v);
        });
        mouse(&triangle, QEvent::MouseButtonPress, triangle.whiteVertex());
        QVERIFY(qAbs(triangle.saturation()) < 1e-6);
        QVERIFY(qAbs(triangle.value() - 1) < 1e-6);
        mouse(&triangle, QEvent::MouseMove, { -100, -100 });
        QVERIFY(std::isfinite(triangle.saturation()) && std::isfinite(triangle.value()));
        const auto count = requested.size();
        triangle.setEnabled(false);
        mouse(&triangle, QEvent::MouseButtonRelease, triangle.hueVertex());
        QCOMPARE(requested.size(), count);
        triangle.setSize({ 0, 0 });
        QVERIFY(! triangle.contains({ 0, 0 }));
    }
    void wheelRequestsAndRoles() {
        ColorWheel wheel;
        wheel.setSize({ 240, 240 });
        QSignalSpy    requested(&wheel, &ColorWheel::hueChangeRequested);
        const QPointF top { 120, 12 }, left { 12, 120 };
        QVERIFY(wheel.contains(top));
        QVERIFY(! wheel.contains({ 120, 120 }));
        connect(&wheel, &ColorWheel::hueChangeRequested, &wheel, &ColorInput::setHue);
        mouse(&wheel, QEvent::MouseButtonPress, top);
        QCOMPARE(wheel.hue(), .25);
        mouse(&wheel, QEvent::MouseMove, left);
        QCOMPARE(wheel.hue(), .5);
        mouse(&wheel, QEvent::MouseButtonRelease, left);
        QCOMPARE(requested.size(), 2);
        auto triangle = std::make_unique<ColorTriangle>();
        triangle->setSize({ 100, 100 });
        wheel.setTriangle(triangle.get());
        QSignalSpy sv(&wheel, &ColorInput::svChangeRequested);
        mouse(triangle.get(), QEvent::MouseButtonPress, triangle->hueVertex());
        QCOMPARE(sv.size(), 1);
        wheel.setTriangle(nullptr);
        mouse(triangle.get(), QEvent::MouseButtonRelease, triangle->whiteVertex());
        QCOMPARE(sv.size(), 1);
        wheel.setTriangle(triangle.get());
        triangle.reset();
        QCOMPARE(wheel.triangle(), nullptr);
    }
    void dateRequestsAndKeyboard() {
        DatePickerState state;
        state.setSelectedDate(QDate(2024, 2, 29).startOfDay());
        state.setMinDate(QDate(2024, 2, 1).startOfDay());
        state.setMaxDate(QDate(2024, 3, 31).startOfDay());
        const auto march = QDate(2024, 3, 2).startOfDay();
        QSignalSpy date(&state, &DatePickerState::selectedDateChangeRequested);
        state.requestDate(march);
        QCOMPARE(date.size(), 1);
        QCOMPARE(state.selectedDate().date(), QDate(2024, 2, 29));
        state.requestDate(QDate(2024, 4, 1).startOfDay());
        QCOMPARE(date.size(), 1);
        state.setSelectionMode(1);
        QSignalSpy range(&state, &DatePickerState::rangeChangeRequested);
        state.requestDate(march);
        QCOMPARE(range.size(), 1);
        QVERIFY(! state.rangeStart().isValid());
        connect(&state, &DatePickerState::rangeChangeRequested, &state, &DatePickerState::setRange);
        state.requestDate(march);
        state.requestDate(QDate(2024, 3, 1).startOfDay());
        QCOMPARE(state.rangeStart().date(), QDate(2024, 3, 1));
        QVERIFY(! state.rangeEnd().isValid());
        state.requestDate(march);
        QCOMPARE(state.rangeEnd().date(), QDate(2024, 3, 2));
        state.setSelectionMode(0);
        CalendarDay day;
        day.setDateState(&state);
        int index = -1;
        for (int i = 0; i < 42; ++i)
            if (state.calendar()->dateAt(i).date() == QDate(2024, 2, 29)) index = i;
        QVERIFY(index >= 0);
        day.setDayIndex(index);
        QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
        QCoreApplication::sendEvent(&day, &right);
        QCOMPARE(state.month(), 2);
        QCOMPARE(state.focusedDate().date(), QDate(2024, 3, 1));
        QCOMPARE(state.selectedDate().date(), QDate(2024, 2, 29));
        const auto requests = date.size();
        state.shiftMonth(100000);
        QCOMPARE(state.month(), 2);
        QCOMPARE(date.size(), requests);
    }
    void timeReadOnlyAndEditor() {
        TimeState time;
        TimeField field;
        field.setTime(&time);
        field.setSelection(TimeState::Minutes);
        field.setReadOnly(true);
        field.setSize({ 100, 60 });
        mouse(&field, QEvent::MouseButtonPress);
        QCOMPARE(time.selection(), TimeState::Minutes);
        field.setReadOnly(false);
        mouse(&field, QEvent::MouseButtonRelease);
        field.setText("12");
        QCOMPARE(field.text(), QString("12"));
        field.setReadOnly(true);
        QSignalSpy requested(&field, &TimeField::inputRequested);
        for (const auto [type, time] : { std::pair { QEvent::MouseButtonPress, 1000UL },
                                         { QEvent::MouseButtonRelease, 1020UL },
                                         { QEvent::MouseButtonPress, 1100UL },
                                         { QEvent::MouseButtonRelease, 1120UL } }) {
            QMouseEvent event(type,
                              { 10, 10 },
                              { 10, 10 },
                              Qt::LeftButton,
                              type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                              Qt::NoModifier);
            event.setTimestamp(time);
            QCoreApplication::sendEvent(&field, &event);
        }
        QCOMPARE(requested.size(), 1);
        field.setTime(nullptr);
        QCOMPARE(field.time(), nullptr);
    }
    void accessibleRequests_data() {
        QTest::addColumn<QByteArray>("type");
        for (const auto* name : { "NumberStepper", "Slider", "ColorWheel" })
            QTest::newRow(name) << QByteArray(name);
    }
    void accessibleRequests() {
        QFETCH(QByteArray, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        const bool    wheel = type == "ColorWheel";
        component.setData(
            "import QtQuick; import Qcm.Material as MD; MD." + type +
                (wheel ? " { hue: 0.2; value: 0.7 }" : " { from: 0; to: 1; value: 0.2 }"),
            QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* accessible = QAccessible::queryAccessibleInterface(object.get());
        QVERIFY(accessible && accessible->valueInterface());
        auto* value = accessible->valueInterface();
        QCOMPARE(value->currentValue().toReal(), .2);
        QCOMPARE(value->minimumValue().toReal(), 0);
        QCOMPARE(value->maximumValue().toReal(), 1);
        QSignalSpy requests(object.get(),
                            wheel ? SIGNAL(hueChangeRequested(qreal))
                                  : SIGNAL(valueChangeRequested(qreal)));
        QVERIFY(requests.isValid());
        value->setCurrentValue(.6);
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toReal(), .6);
        QCOMPARE(value->currentValue().toReal(), .2);
        accessible->actionInterface()->doAction(QAccessibleActionInterface::decreaseAction());
        QCOMPARE(requests.size(), 2);
        QCOMPARE(value->currentValue().toReal(), .2);
    }
    void selectionBindings() {
        QProperty<bool> value(false);
        Selectable      selection;
        Toggleable      toggle;
        selection.bindableSelected().setBinding([&] {
            return value.value();
        });
        toggle.bindableChecked().setBinding([&] {
            return value.value();
        });
        selection.setSelected(true);
        toggle.setChecked(true);
        QVERIFY(selection.bindableSelected().hasBinding());
        QVERIFY(toggle.bindableChecked().hasBinding());
        value = true;
        value = false;
        QVERIFY(! selection.isSelected() && ! toggle.isChecked());
    }
    void dragTargetDestruction() {
        QQuickWindow window;
        window.resize(240, 100);
        SwipeToDismissState state;
        state.setDistance(200);
        auto item = std::make_unique<Control>(window.contentItem());
        item->setSize({ 240, 100 });
        state.setInputItem(item.get());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 20, 30 });
        QTest::mouseMove(&window, { 100, 30 });
        QTRY_VERIFY(state.dragging());
        item.reset();
        QVERIFY(! state.inputItem());
        QVERIFY(! state.dragging());
        QCOMPARE(state.offset(), 0);
    }
    void immediateDragQualification() {
        DragTriangle triangle;
        triangle.setSize({ 120, 120 });
        bool accepted                        = false;
        triangle.inputHost().drag().canStart = [&](QPointF, QPointF) {
            return accepted;
        };
        QSignalSpy requests(&triangle, &ColorInput::svChangeRequested);
        mouse(&triangle, QEvent::MouseButtonPress, { 60, 60 });
        QVERIFY(requests.isEmpty());
        accepted = true;
        mouse(&triangle, QEvent::MouseMove, { 61, 60 });
        QCOMPARE(requests.size(), 1);
        mouse(&triangle, QEvent::MouseButtonRelease, { 61, 60 });
        QCOMPARE(requests.size(), 1);
    }
    void deletedInDragQualification() {
        auto triangle = std::make_unique<DragTriangle>();
        triangle->setSize({ 120, 120 });
        triangle->inputHost().drag().canStart = [&](QPointF, QPointF) {
            triangle.reset();
            return true;
        };
        mouse(triangle.get(), QEvent::MouseButtonPress, { 60, 60 });
        QVERIFY(! triangle);
    }
    void dragTracksOriginalTouch() {
        QQuickWindow window;
        window.resize(240, 100);
        Control item(window.contentItem());
        item.setSize({ 240, 100 });
        SwipeToDismissState state;
        state.setDistance(200);
        state.setInputItem(&item);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        static auto* device   = QTest::createTouchDevice();
        auto         sequence = QTest::touchEvent(&window, device, false);
        const auto   flush    = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
            flushInteractions();
        };
        sequence.press(0, { 20, 30 }, &window).commit();
        flush();
        sequence.move(0, { 100, 30 }, &window).commit();
        flush();
        QVERIFY(state.dragging());
        const auto offset = state.offset();
        sequence.stationary(0).press(1, { 200, 30 }, &window).commit();
        flush();
        QCOMPARE(state.offset(), offset);
        sequence.move(0, { 120, 30 }, &window).stationary(1).commit();
        flush();
        QVERIFY(state.dragging());
        QVERIFY(state.offset() > offset);
        sequence.release(0, { 120, 30 }, &window).stationary(1).commit();
        flush();
        QVERIFY(! state.dragging());
        sequence.release(1, { 200, 30 }, &window).commit();
        flush();
        QVERIFY(! state.dragging());
    }
};
int run_value_input(int argc, char** argv) {
    ValueInputTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "value_input.moc"
