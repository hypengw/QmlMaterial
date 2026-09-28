#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QStyleHints>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <cmath>
#include <numbers>
#include "qml_material/input/time_state.hpp"
#include "qml_material/input/time_dial.hpp"
#include "qml_material/control/dialog.hpp"
#include "qml_material/control/abstract_button.hpp"
#include "qml_material/control/text_field.hpp"

using namespace qml_material;
class TestDial : public TimeDial {
public:
    using TimeDial::keyPressEvent;
    using TimeDial::mouseUngrabEvent;
    void mouse(QEvent::Type type, QPointF point) {
        QMouseEvent event(type,
                          point,
                          point,
                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                          Qt::NoModifier);
        if (type == QEvent::MouseButtonPress)
            mousePressEvent(&event);
        else if (type == QEvent::MouseMove)
            mouseMoveEvent(&event);
        else
            mouseReleaseEvent(&event);
    }
};
class TimeInputTest : public QObject {
    Q_OBJECT
private slots:
    void values() {
        TimeState state;
        state.setHourFormat(TimeState::Hour12);
        QCOMPARE(state.hourText(), "12");
        state.setPeriod(1);
        QCOMPARE(state.hour(), 12);
        QCOMPARE(state.hourText(), "12");
        state.editHour("1");
        QCOMPARE(state.hour(), 13);
        state.setPeriod(0);
        QCOMPARE(state.hour(), 1);
        state.setHourFormat(TimeState::Hour24);
        state.setPeriod(1);
        QCOMPARE(state.hourText(), "13");
        QVERIFY(state.setTime(23, 59));
        QCOMPARE(state.value(), 1439);
        QVERIFY(! state.setTime(24, 0));
        QVERIFY(! state.setTime(0, 60));
        state.setValue(-1);
        state.setValue(1440);
        QCOMPARE(state.value(), 1439);
        state.setSelection(TimeState::Minutes);
        state.step(1);
        QCOMPARE(state.value(), 23 * 60);
        state.step(-1);
        QCOMPARE(state.minute(), 59);
    }
    void localeRoundTrip_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("h24");
        for (const auto* name : { "en_US", "de_DE", "ar_EG", "fa_IR", "zh_CN" })
            for (bool h24 : { false, true })
                QTest::newRow(qPrintable(QString(name) + (h24 ? "24" : "12")))
                    << QString(name) << h24;
    }
    void localeRoundTrip() {
        QFETCH(QString, name);
        QFETCH(bool, h24);
        TimeState state;
        state.setLocale(QLocale(name));
        state.setHourFormat(h24 ? TimeState::Hour24 : TimeState::Hour12);
        for (int value = 0; value < 1440; ++value) {
            state.setValue(value);
            QCOMPARE(state.parseText(state.displayText()), value);
            state.editHour(state.hourText());
            state.editMinute(state.minuteText());
            QVERIFY(state.acceptableInput());
            QCOMPARE(state.value(), value);
        }
        QVERIFY(state.parseText("25:00") < 0);
        QVERIFY(state.parseText("10:60") < 0);
        QVERIFY(state.parseText("") < 0);
    }
    void drafts() {
        TimeState state;
        state.setLocale(QLocale("en_US"));
        QVERIFY(! state.is24Hour());
        state.setLocale(QLocale("de_DE"));
        QVERIFY(state.is24Hour());
        state.setTime(9, 30);
        state.setDisplayMode(TimeState::Input);
        state.editHour("");
        state.editMinute("60");
        QVERIFY(! state.acceptableInput());
        QCOMPARE(state.value(), 570);
        QVERIFY(! state.commitInput());
        state.setDisplayMode(TimeState::Clock);
        QCOMPARE(state.displayMode(), TimeState::Input);
        QCOMPARE(state.hourText(), "");
        state.editHour("10");
        state.editMinute("5");
        QCOMPARE(state.value(), 605);
        QCOMPARE(state.minuteText(), "5");
        QVERIFY(state.commitInput());
        QCOMPARE(state.minuteText(), "05");
        state.editHour("+1");
        QVERIFY(! state.hourAcceptable());
        state.discardInput();
        QVERIFY(state.acceptableInput());
        state.setDisplayMode(TimeState::Clock);
        QCOMPARE(state.displayMode(), TimeState::Clock);
        QProperty<int> bound;
        bound.setBinding([&] {
            return state.value();
        });
        state.setTime(1, 2);
        QCOMPARE(bound.value(), 62);
    }
    void validatorAndLifetime() {
        auto*         state = new TimeState;
        TimeValidator validator;
        state->setHourFormat(TimeState::Hour24);
        validator.setTime(state);
        QCOMPARE(validator.parse("23:59"), 1439);
        state->setHourFormat(TimeState::Hour12);
        QCOMPARE(validator.parse("23:59"), -1);
        delete state;
        QCOMPARE(validator.parse("23:59"), -1);
        QCOMPARE(validator.time(), nullptr);
        QPointer<TimeState> dying = new TimeState;
        connect(dying, &TimeState::changed, this, [dying] {
            delete dying;
        });
        dying->editMinute("10");
        QVERIFY(dying.isNull());
    }
    void dial() {
        TimeState state;
        state.setHourFormat(TimeState::Hour24);
        TestDial dial;
        dial.setSize({ 256, 256 });
        dial.setTime(&state);
        QCOMPARE(dial.labels().size(), 24);
        dial.mouse(QEvent::MouseButtonPress, { 232, 128 });
        QVERIFY(dial.pressed());
        dial.mouse(QEvent::MouseButtonRelease, { 232, 128 });
        QCOMPARE(state.hour(), 3);
        QCOMPARE(state.selection(), TimeState::Minutes);
        QVERIFY(! dial.pressed());
        QCOMPARE(dial.labels().size(), 12);
        state.setSelection(TimeState::Hours);
        dial.mouse(QEvent::MouseButtonPress, { 195, 128 });
        dial.mouse(QEvent::MouseButtonRelease, { 195, 128 });
        QCOMPARE(state.hour(), 15);
        auto point = [](int minute) {
            const qreal a = minute * std::numbers::pi / 30;
            return QPointF(128 + 104 * std::sin(a), 128 - 104 * std::cos(a));
        };
        dial.mouse(QEvent::MouseButtonPress, point(17));
        dial.mouse(QEvent::MouseButtonRelease, point(17));
        QCOMPARE(state.minute(), 15);
        dial.mouse(QEvent::MouseButtonPress, point(0));
        dial.mouse(QEvent::MouseMove, point(17));
        dial.mouse(QEvent::MouseButtonRelease, point(17));
        QCOMPARE(state.minute(), 17);
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
        dial.keyPressEvent(&key);
        QCOMPARE(state.minute(), 18);
        state.setSelection(TimeState::Hours);
        dial.mouse(QEvent::MouseButtonPress, { 232, 128 });
        dial.setEnabled(false);
        QVERIFY(! dial.pressed());
        dial.mouse(QEvent::MouseButtonRelease, { 232, 128 });
        QCOMPARE(state.selection(), TimeState::Hours);
        dial.setEnabled(true);
        dial.mouse(QEvent::MouseButtonPress, { 232, 128 });
        dial.setTime(nullptr);
        QVERIFY(! dial.pressed());
        QVERIFY(dial.labels().isEmpty());
    }
    void windowTouch() {
        QQuickWindow window;
        window.resize(300, 300);
        TimeState state;
        state.setHourFormat(TimeState::Hour24);
        TimeDial dial;
        dial.setParentItem(window.contentItem());
        dial.setSize({ 256, 256 });
        dial.setTime(&state);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        static auto device = QTest::createTouchDevice();
        auto        touch  = QTest::touchEvent(&window, device, false);
        auto        flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 232, 128 }, &window).commit();
        flush();
        QVERIFY(dial.pressed());
        touch.release(0, { 232, 128 }, &window).commit();
        flush();
        QVERIFY(! dial.pressed());
        QCOMPARE(state.hour(), 3);
        QCOMPARE(state.selection(), TimeState::Minutes);
        state.setSelection(TimeState::Hours);
        touch.press(0, { 128, 24 }, &window).commit();
        flush();
        touch.move(0, { 24, 128 }, &window).commit();
        flush();
        QCOMPARE(state.hour(), 9);
        dial.setVisible(false);
        QVERIFY(! dial.pressed());
        touch.release(0, { 128, 232 }, &window).commit();
        flush();
        QCOMPARE(state.hour(), 9);
        QCOMPARE(state.selection(), TimeState::Hours);
        dial.setParentItem(nullptr);
    }
    void dialReentry() {
        TimeState state, replacement;
        state.setHourFormat(TimeState::Hour24);
        TestDial dial;
        dial.setSize({ 256, 256 });
        dial.setTime(&state);
        dial.mouse(QEvent::MouseButtonPress, { 232, 128 });
        auto connection = connect(&dial, &TimeDial::pressedChanged, this, [&] {
            if (! dial.pressed()) dial.setTime(&replacement);
        });
        dial.mouse(QEvent::MouseButtonRelease, { 232, 128 });
        QCOMPARE(dial.time(), &replacement);
        QCOMPARE(replacement.value(), 0);
        QCOMPARE(state.value(), 0);
        disconnect(connection);
        QPointer<TestDial> dying = new TestDial;
        dying->setSize({ 256, 256 });
        dying->setTime(&state);
        connect(&state, &TimeState::changed, this, [dying] {
            delete dying;
        });
        dying->mouse(QEvent::MouseButtonPress, { 232, 128 });
        dying->mouse(QEvent::MouseMove, { 24, 128 });
        QVERIFY(dying.isNull());
    }
    void qmlViews() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.TimePicker { time.hourFormat: MD.TimeState.Hour24; time.hour: 18; time.minute: 45 }
)",
                          QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* state = object->property("time").value<TimeState*>();
        QVERIFY(state);
        auto* hour   = object->findChild<TextField*>("timeHour");
        auto* minute = object->findChild<TextField*>("timeMinute");
        QVERIFY(hour && minute);
        QCOMPARE(hour->text(), "18");
        QCOMPARE(minute->text(), "45");
        state->setDisplayMode(TimeState::Input);
        hour->setText("25");
        QMetaObject::invokeMethod(hour, "textEdited");
        QCOMPARE(hour->text(), "25");
        QVERIFY(! state->acceptableInput());
        state->setDisplayMode(TimeState::Clock);
        QCOMPARE(state->displayMode(), TimeState::Input);
        state->setTime(12, 30);
        QCOMPARE(hour->text(), "12");
        state->setDisplayMode(TimeState::Clock);
        QCOMPARE(state->displayMode(), TimeState::Clock);
        auto* picker = qobject_cast<QQuickItem*>(object.data());
        picker->setWidth(250);
        state->setHourFormat(TimeState::Hour12);
        state->setLocale(QLocale("ar_EG"));
        QVERIFY(hour->height() >= hour->contentHeight());
        QVERIFY(minute->height() >= minute->contentHeight());
        QVERIFY(hour->x() >= 0);
        QVERIFY(minute->x() + minute->width() <= hour->parentItem()->width());
    }
    void dialogTransaction() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.TimePickerDialog { value: 570; hourFormat: MD.TimeState.Hour24; enter: null; exit: null }
)",
                          QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* dialog = qobject_cast<Dialog*>(object.data());
        QVERIFY(dialog);
        QQuickWindow window;
        window.resize(400, 600);
        dialog->setParentItem(window.contentItem());
        auto* draft = object->property("time").value<TimeState*>();
        QVERIFY(draft);
        QSignalSpy accepted(dialog, &Dialog::accepted);
        QSignalSpy acceptedTime(object.data(), SIGNAL(acceptedTime(int, int)));
        dialog->open();
        QCOMPARE(draft->value(), 570);
        draft->editHour("25");
        QVERIFY(! dialog->acceptEnabled());
        auto* ok = dialog->standardButton(Dialog::Ok);
        QVERIFY(ok);
        QVERIFY(! ok->isEnabled());
        dialog->accept();
        dialog->done(Dialog::Accepted);
        QCOMPARE(accepted.size(), 0);
        QVERIFY(dialog->isVisible());
        draft->setTime(12, 30);
        dialog->reject();
        QCOMPARE(object->property("value").toInt(), 570);
        dialog->open();
        QCOMPARE(draft->value(), 570);
        draft->setTime(23, 59);
        dialog->accept();
        QCOMPARE(accepted.size(), 1);
        QCOMPARE(acceptedTime.size(), 1);
        QCOMPARE(object->property("value").toInt(), 1439);
    }
    void textField() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.TimeTextField { value: 570; hourFormat: MD.TimeState.Hour24; locale: Qt.locale("en_US") }
)",
                          QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* field = qobject_cast<TextField*>(object.data());
        QVERIFY(field);
        QCOMPARE(field->text(), "09:30");
        QSignalSpy modified(object.data(), SIGNAL(modified(int)));
        field->setText("25:30");
        QVERIFY(! field->hasAcceptableInput());
        QMetaObject::invokeMethod(field, "editingFinished");
        QCOMPARE(modified.size(), 0);
        QCOMPARE(object->property("value").toInt(), 570);
        field->setText("23:59");
        QMetaObject::invokeMethod(field, "editingFinished");
        QCOMPARE(modified.size(), 1);
        QCOMPARE(object->property("value").toInt(), 1439);
        object->setProperty("value", 0);
        QCOMPARE(field->text(), "00:00");
    }
};
int run_time_input(int argc, char** argv) {
    TimeInputTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "time_input.moc"
