#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/input/date_validator.hpp"
#include "qml_material/model/calendar_month_model.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/control/dialog.hpp"
#include "qml_material/control/abstract_button.hpp"

using namespace qml_material;

class DateInputTest : public QObject {
    Q_OBJECT
private slots:
    void formats_data() {
        QTest::addColumn<QString>("localeName");
        QTest::addColumn<QString>("format");
        QTest::addColumn<QDate>("date");
        QTest::newRow("iso-leap") << "en_US" << "yyyy-MM-dd" << QDate(2024, 2, 29);
        QTest::newRow("day-first") << "de_DE" << "dd.MM.yyyy" << QDate(2026, 9, 28);
        QTest::newRow("month-first") << "en_US" << "MM/dd/yyyy" << QDate(2026, 9, 28);
        QTest::newRow("month-name") << "fr_FR" << "d MMMM yyyy" << QDate(2026, 9, 28);
        QTest::newRow("locale-default") << "de_DE" << "" << QDate(2026, 9, 28);
        QTest::newRow("early-year") << "en_US" << "yyyy-MM-dd" << QDate(99, 3, 1);
        QTest::newRow("arabic") << "ar_EG" << "yyyy-MM-dd" << QDate(2026, 9, 28);
        QTest::newRow("dst-day") << "en_US" << "yyyy-MM-dd" << QDate(2024, 3, 10);
    }
    void formats() {
        QFETCH(QString, localeName);
        QFETCH(QString, format);
        QFETCH(QDate, date);
        DateValidator validator;
        validator.setLocale(QLocale(localeName));
        validator.setDateFormat(format);
        const auto value = date.startOfDay().addSecs(3600);
        auto       text  = validator.formatDate(value);
        QVERIFY(! text.isEmpty());
        int cursor = text.size();
        QCOMPARE(validator.validate(text, cursor), QValidator::Acceptable);
        QCOMPARE(validator.parse(text).toLocalTime().date(), date);
        QCOMPARE(validator.parse(text), date.startOfDay());
    }
    void validityAndCache() {
        DateValidator validator;
        QVERIFY(! validator.parse("2023-02-29").isValid());
        QVERIFY(! validator.parse("2024-04-31").isValid());
        QVERIFY(! validator.parse("2024-13-01").isValid());
        QVERIFY(! validator.parse("").isValid());
        QVERIFY(! validator.parse("2024-01-01 extra").isValid());
        const auto day = QDate(2024, 2, 29).startOfDay();
        validator.setMinDate(day.addSecs(12 * 3600));
        validator.setMaxDate(day.addSecs(23 * 3600));
        QCOMPARE(validator.parse("2024-02-29"), day);
        QVERIFY(! validator.parse("2024-02-28").isValid());
        validator.setMinDate(day.addDays(1));
        QVERIFY(! validator.parse("2024-02-29").isValid());
        validator.setMinDate({});
        validator.setMaxDate({});
        QCOMPARE(validator.parse("2024-02-29"), day);
        validator.setDateFormat("dd/MM/yyyy");
        QVERIFY(! validator.parse("2024-02-29").isValid());
        QCOMPARE(validator.parse("29/02/2024"), day);
        validator.setDateFormat("d MMMM yyyy");
        validator.setLocale(QLocale("fr_FR"));
        QCOMPARE(validator.parse("29 février 2024"), day);
        validator.setLocale(QLocale("en_US"));
        QVERIFY(! validator.parse("29 février 2024").isValid());
        QVERIFY(validator.sameDay(day, day.addSecs(3600)));
        QVERIFY(validator.rangeValid(day.addSecs(3600), day, {}, {}));
        QVERIFY(! validator.rangeValid(day.addDays(1), day, {}, {}));
        QVERIFY(! validator.rangeValid(day, {}, {}, {}));
        QVERIFY(validator.insideRange(day, day.addDays(-1), day.addDays(1)));
        QVERIFY(! validator.insideRange(day, day, day.addDays(1)));
        QVERIFY(validator.monthEnabled(2024, 1, day, day));
        QVERIFY(! validator.monthEnabled(2024, 0, day, day));
        QVERIFY(! validator.monthEnabled(2024, 2, day, day));
        QVERIFY(! validator.monthEnabled(2024, 1, day.addDays(1), day));
        validator.setDateFormat("'yy' dd/MM/yyyy");
        QCOMPARE(validator.parse("yy 29/02/2024"), day);
    }
    void field() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.DateTextField {
    width: 240
    value: new Date(2024, 1, 29)
    locale: Qt.locale("de_DE")
    dateFormat: "dd.MM.yyyy"
}
)",
                          QUrl());
        QQuickWindow             window;
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* field = qobject_cast<TextField*>(object.get());
        QVERIFY(field);
        field->setParentItem(window.contentItem());
        QCOMPARE(field->text(), QStringLiteral("29.02.2024"));
        QSignalSpy modified(field, SIGNAL(modified(QDateTime)));
        QVERIFY(modified.isValid());
        field->setText("31.02.2024");
        QVERIFY(! field->hasAcceptableInput());
        QMetaObject::invokeMethod(field, "editingFinished");
        QCOMPARE(modified.size(), 0);
        QCOMPARE(field->text(), QStringLiteral("31.02.2024"));
        field->setText("28.02.2024");
        QVERIFY(field->hasAcceptableInput());
        QMetaObject::invokeMethod(field, "editingFinished");
        QCOMPARE(modified.size(), 1);
        QCOMPARE(field->property("value").toDateTime().toLocalTime().date(), QDate(2024, 2, 28));
        field->setProperty("value", QDate(2026, 9, 28).startOfDay());
        QCOMPARE(field->text(), QStringLiteral("28.09.2026"));
        field->setProperty("dateFormat", "d MMMM yyyy");
        field->setProperty("locale", QLocale("fr_FR"));
        QCOMPARE(field->text(), QStringLiteral("28 septembre 2026"));
        field->setProperty("minDate", QDate(2027, 1, 1).startOfDay());
        QVERIFY(! field->hasAcceptableInput());
        field->setProperty("minDate", QDateTime());
        QVERIFY(field->hasAcceptableInput());
    }
    void pickerAndDialog() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.DatePickerDialog {
    locale: Qt.locale("de_DE")
    selectedDate: new Date(2024, 1, 29)
    minDate: new Date(2024, 1, 29, 12)
    maxDate: new Date(2024, 1, 29, 23)
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* dialog = qobject_cast<Dialog*>(object.get());
        QVERIFY(dialog);
        auto* ok = dialog->standardButton(Dialog::Ok);
        QVERIFY(ok);
        QVERIFY(object->property("selectionValid").toBool());
        QVERIFY(ok->isEnabled());
        object->setProperty("displayMode", 1);
        auto* input = object->findChild<TextField*>("datePickerStartInput");
        QVERIFY(input);
        input->setText("30.02.2024");
        QVERIFY(QMetaObject::invokeMethod(input, "textEdited"));
        QVERIFY(! ok->isEnabled());
        input->setText("29.02.2024");
        QVERIFY(QMetaObject::invokeMethod(input, "textEdited"));
        QVERIFY(ok->isEnabled());
        object->setProperty("displayMode", 0);
        auto* calendar = object->findChild<CalendarMonthModel*>();
        QVERIFY(calendar);
        QCOMPARE(calendar->locale(), QLocale("de_DE"));
        QCOMPARE(calendar->weekDays().first().toMap().value("day").toInt(), 1);
        QCOMPARE(calendar->firstDate().toLocalTime().date(), QDate(2024, 2, 1));
        dialog->setLocale(QLocale("en_US"));
        QCOMPARE(calendar->weekDays().first().toMap().value("day").toInt(), 0);
        object->setProperty("selectionMode", 1);
        QVERIFY(! object->property("selectionValid").toBool());
        QVERIFY(! ok->isEnabled());
        object->setProperty("rangeStart", QDate(2024, 2, 29).startOfDay().addSecs(3600));
        object->setProperty("rangeEnd", QDate(2024, 2, 29).startOfDay());
        QVERIFY(object->property("selectionValid").toBool());
        QVERIFY(ok->isEnabled());
        object->setProperty("rangeEnd", QDate(2024, 2, 28).startOfDay());
        QVERIFY(! object->property("selectionValid").toBool());
        QVERIFY(! ok->isEnabled());
        QSignalSpy acceptedRange(object.get(), SIGNAL(acceptedRange(QDateTime, QDateTime)));
        QVERIFY(acceptedRange.isValid());
        dialog->accept();
        QCOMPARE(acceptedRange.size(), 0);
    }
    void pickerInputModes() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.DatePicker {
    width: 360
    displayMode: MD.DatePicker.Input
    locale: Qt.locale("de_DE")
    selectedDate: new Date(2024, 1, 29)
}
)",
                          QUrl());
        QQuickWindow             window;
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* picker = qobject_cast<QQuickItem*>(object.get());
        picker->setParentItem(window.contentItem());
        auto* start    = picker->findChild<TextField*>("datePickerStartInput");
        auto* end      = picker->findChild<TextField*>("datePickerEndInput");
        auto* calendar = picker->findChild<CalendarMonthModel*>();
        QVERIFY(start && end && calendar);
        QCOMPARE(start->text(), "29.02.2024");
        QCOMPARE(calendar->monthNames().at(1), "Februar");
        auto edit = [](TextField* field, const QString& text) {
            field->setText(text);
            return QMetaObject::invokeMethod(field, "textEdited");
        };
        QVERIFY(edit(start, "31.02.2024"));
        QVERIFY(! picker->property("selectionValid").toBool());
        QCOMPARE(start->text(), "31.02.2024");
        QVERIFY(edit(start, "15.03.2025"));
        QVERIFY(picker->property("selectionValid").toBool());
        QCOMPARE(picker->property("selectedDate").toDateTime().date(), QDate(2025, 3, 15));
        picker->setProperty("displayMode", 0);
        QCOMPARE(picker->property("year").toInt(), 2025);
        QCOMPARE(picker->property("month").toInt(), 2);
        picker->setProperty("year", 2030);
        picker->setProperty("month", 8);
        QCOMPARE(picker->property("selectedDate").toDateTime().date(), QDate(2025, 3, 15));
        picker->setProperty("displayMode", 1);
        QCOMPARE(start->text(), "15.03.2025");
        picker->setProperty("inputDateFormat", "yyyy-MM-dd");
        QCOMPARE(start->text(), "2025-03-15");
        picker->setProperty("selectionMode", 1);
        QCOMPARE(start->text(), "");
        QVERIFY(edit(start, "2025-03-15"));
        QVERIFY(edit(end, "2025-03-14"));
        QVERIFY(! picker->property("selectionValid").toBool());
        QVERIFY(edit(end, "2025-03-16"));
        QVERIFY(picker->property("selectionValid").toBool());
        picker->setProperty("maxDate", QDate(2025, 3, 15).startOfDay());
        QVERIFY(! picker->property("selectionValid").toBool());
        QVERIFY(edit(end, "2025-03-15"));
        QVERIFY(picker->property("selectionValid").toBool());
        QVERIFY(edit(start, ""));
        QVERIFY(! picker->property("selectionValid").toBool());
        picker->setProperty("rangeStart", QDate(2025, 3, 14).startOfDay());
        QCOMPARE(start->text(), "2025-03-14");
        QVERIFY(picker->property("selectionValid").toBool());
    }
    void pickerNavigation() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.DatePicker {
    width: 360
    selectedDate: new Date(2024, 1, 29)
    minDate: new Date(2024, 1, 1)
    maxDate: new Date(2026, 2, 31)
}
)",
                          QUrl());
        QQuickWindow             window;
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* picker = qobject_cast<QQuickItem*>(object.get());
        picker->setParentItem(window.contentItem());
        picker->setProperty("__navigation", 1);
        auto locate = [](auto&& self, QQuickItem* root, const QString& name) -> QQuickItem* {
            if (root->objectName() == name) return root;
            for (auto* child : root->childItems())
                if (auto* found = self(self, child, name)) return found;
            return nullptr;
        };
        QTRY_VERIFY(locate(locate, picker, "datePickerYear2026"));
        auto* year = locate(locate, picker, "datePickerYear2026");
        QVERIFY(QMetaObject::invokeMethod(year, "clicked"));
        QCOMPARE(picker->property("year").toInt(), 2026);
        QCOMPARE(picker->property("__navigation").toInt(), 2);
        auto* march = locate(locate, picker, "datePickerMonth2");
        auto* april = locate(locate, picker, "datePickerMonth3");
        QVERIFY(march && april);
        QVERIFY(march->isEnabled());
        QVERIFY(! april->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(march, "clicked"));
        QCOMPARE(picker->property("month").toInt(), 2);
        QCOMPARE(picker->property("__navigation").toInt(), 0);
        QCOMPARE(picker->property("selectedDate").toDateTime().date(), QDate(2024, 2, 29));
    }
};

int run_date_input(int argc, char** argv) {
    DateInputTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "date_input.moc"
