#include <QtTest/QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QAbstractItemModel>
#include <QJSValue>
#include <algorithm>
#include <cmath>
#include <limits>
#include <QDir>
#include <QMutex>
#include <private/qquickanimation_p.h>
#include "qml_material/input/time_state.hpp"
#include "qml_material/control/dialog.hpp"

namespace
{
void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}

void click(QObject* button) {
    QVERIFY(button);
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
}
} // namespace

class ExamplePickersTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot "
                               "assign|Cannot open|TypeError|ReferenceError|polish.*loop).*"));
    }

    void pickers() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(440, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/TimePickers.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(400);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* time   = root->findChild<qml_material::TimeState*>("pickerTime");
        auto* picker = root->findChild<QQuickItem*>("exampleTimePicker");
        auto* dialog = root->findChild<qml_material::Dialog*>();
        QVERIFY(time && picker && dialog);
        QCOMPARE(picker->property("time").value<qml_material::TimeState*>(), time);
        QCOMPARE(time->value(), 10 * 60 + 30);
        click(root->findChild<QObject*>("picker24HourChoice"));
        QCOMPARE(time->hourFormat(), qml_material::TimeState::Hour24);
        click(picker->findChild<QObject*>("timeModeToggle"));
        QCOMPARE(time->displayMode(), qml_material::TimeState::Input);
        click(picker->findChild<QObject*>("timeModeToggle"));
        QCOMPARE(time->displayMode(), qml_material::TimeState::Clock);
        dialog->setEnter(nullptr);
        dialog->setExit(nullptr);
        QVERIFY(QMetaObject::invokeMethod(root, "openDialog"));
        QTRY_VERIFY(dialog->isOpened());
        auto* draft = dialog->property("time").value<qml_material::TimeState*>();
        QVERIFY(draft && draft != time);
        QCOMPARE(draft->value(), time->value());
        QCOMPARE(draft->hourFormat(), time->hourFormat());
        draft->setTime(18, 45);
        QCOMPARE(time->value(), 10 * 60 + 30);
        QVERIFY(QMetaObject::invokeMethod(dialog, "reject"));
        QTRY_VERIFY(! dialog->isVisible());
        QCOMPARE(time->value(), 10 * 60 + 30);
        QVERIFY(QMetaObject::invokeMethod(root, "openDialog"));
        QTRY_VERIFY(dialog->isOpened());
        QCOMPARE(draft->value(), time->value());
        draft->setTime(18, 45);
        QVERIFY(QMetaObject::invokeMethod(dialog, "accept"));
        QTRY_VERIFY(! dialog->isVisible());
        QCOMPARE(time->value(), 18 * 60 + 45);
        time->setTime(7, 15);
        QCOMPARE(root->findChild<QObject*>("exampleTimeField")->property("value").toInt(),
                 time->value());
        root->setWidth(288);
        QCOMPARE(time->displayMode(), qml_material::TimeState::Clock);
        QCOMPARE(picker->width(), root->width());
        auto* field = root->findChild<QObject*>("exampleTimeField");
        field->setProperty("readOnly", true);
        QVERIFY(QMetaObject::invokeMethod(root, "openDialog"));
        QVERIFY(! dialog->isVisible());
        std::unique_ptr<QObject> narrow(
            component.createWithInitialProperties({ { "width", 288 } }));
        QVERIFY2(narrow, qPrintable(component.errorString()));
        QCOMPARE(narrow->findChild<qml_material::TimeState*>("pickerTime")->displayMode(),
                 qml_material::TimeState::Input);
    }

    void pickerLayout() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQuickWindow window;
        window.resize(440, 900);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        for (bool time : { true, false }) {
            const auto    url = time ? "qrc:/Qcm/Material/Example/compositions/TimePickers.qml"
                                     : "qrc:/Qcm/Material/Example/compositions/DatePickers.qml";
            QQmlComponent component(&engine, QUrl(QString::fromLatin1(url)));
            std::unique_ptr<QObject> object(component.create());
            QVERIFY2(object, qPrintable(component.errorString()));
            auto* root = qobject_cast<QQuickItem*>(object.get());
            QVERIFY(root);
            root->setParentItem(window.contentItem());
            auto* picker =
                root->findChild<QQuickItem*>(time ? "exampleTimePicker" : "exampleDatePicker");
            auto* field =
                root->findChild<QQuickItem*>(time ? "exampleTimeField" : "exampleDateField");
            auto* choice = root->findChild<QQuickItem*>(time ? "pickerHourFormatChoice"
                                                             : "pickerDateSelectionChoice");
            auto* viewport =
                time ? picker : root->findChild<QQuickItem*>("exampleDatePickerViewport");
            auto* background = root->findChild<QQuickItem*>(time ? "exampleTimePickerBackground"
                                                                 : "exampleDatePickerBackground");
            QVERIFY(picker && field && choice && viewport && background);
            for (int width : { 288, 360, 400 }) {
                root->setWidth(width);
                polish(root);
                QCoreApplication::processEvents();
                polish(root);
                for (auto* child : { field, choice, viewport }) {
                    const auto bounds =
                        child->mapRectToItem(root, QRectF(QPointF(), child->size()));
                    QVERIFY(qAbs(bounds.center().x() - width / 2.0) < .01);
                    QVERIFY(bounds.left() >= -.01 && bounds.right() <= width + .01);
                }
                QVERIFY(picker->width() <= picker->implicitWidth());
                QCOMPARE(background->size(), picker->size());
                QVERIFY(background->property("color").value<QColor>().alpha() == 255);
                QVERIFY(background->property("radius").toReal() > 0);
                const auto pickerBounds =
                    viewport->mapRectToItem(root, QRectF(QPointF(), viewport->size()));
                const auto choiceBounds =
                    choice->mapRectToItem(root, QRectF(QPointF(), choice->size()));
                QVERIFY(pickerBounds.top() >= choiceBounds.bottom() + 15.99);
            }
            if (! time) {
                root->setWidth(288);
                QVERIFY(picker->setProperty("displayMode", 0));
                polish(root);
                QCOMPARE(viewport->width(), 288.0);
                QCOMPARE(picker->width(), picker->implicitWidth());
                QCOMPARE(viewport->property("contentWidth").toReal(), picker->width());
            }
        }
    }

    void pickerSynchronization() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent dates(&engine,
                            QUrl("qrc:/Qcm/Material/Example/compositions/DatePickers.qml"));
        std::unique_ptr<QObject> dateRoot(dates.create());
        QVERIFY2(dateRoot, qPrintable(dates.errorString()));
        dateRoot->setProperty("width", 288);
        auto* picker = dateRoot->findChild<QQuickItem*>("exampleDatePicker");
        auto* field  = dateRoot->findChild<QObject*>("exampleDateField");
        QVERIFY(picker && field);
        QCOMPARE(picker->width(), 288);
        QCOMPARE(picker->property("displayMode").toInt(), 1);
        const QDateTime first(QDate(2026, 10, 20), QTime());
        field->setProperty("value", first);
        QVERIFY(QMetaObject::invokeMethod(field, "modified", Q_ARG(QDateTime, first)));
        QCOMPARE(picker->property("selectedDate").toDateTime().date(), first.date());
        const QDateTime second(QDate(2026, 11, 3), QTime());
        picker->setProperty("selectedDate", second);
        QCOMPARE(field->property("value").toDateTime().date(), second.date());
        click(dateRoot->findChild<QObject*>("pickerDateRangeChoice"));
        QCOMPARE(picker->property("selectionMode").toInt(), 1);
        QVERIFY(! qobject_cast<QQuickItem*>(field)->isVisible());
        click(dateRoot->findChild<QObject*>("pickerSingleDateChoice"));
        QCOMPARE(picker->property("selectionMode").toInt(), 0);
        QQmlComponent colors(&engine,
                             QUrl("qrc:/Qcm/Material/Example/compositions/ColorPickers.qml"));
        std::unique_ptr<QObject> colorRoot(colors.create());
        QVERIFY2(colorRoot, qPrintable(colors.errorString()));
        auto* colorPicker = colorRoot->findChild<QObject*>("exampleColorPicker");
        auto* button      = colorRoot->findChild<QObject*>("exampleColorButton");
        QVERIFY(colorPicker && button);
        const QColor blue("#2468ab");
        button->setProperty("color", blue);
        QVERIFY(QMetaObject::invokeMethod(button, "accepted", Q_ARG(QColor, blue)));
        QCOMPARE(colorPicker->property("color").value<QColor>(), blue);
        const QColor green("#25845a");
        colorPicker->setProperty("color", green);
        QCOMPARE(button->property("color").value<QColor>(), green);
    }
};

int run_example_pickers(int argc, char** argv) {
    ExamplePickersTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "example_pickers.moc"
