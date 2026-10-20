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
#include "qml_material/control/popup.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/clock.hpp"
#include "qml_material/input/time_state.hpp"

namespace
{
void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}

QQuickItem* visualChild(QQuickItem* item, const QString& name) {
    if (item->objectName() == name) return item;
    for (auto* child : item->childItems()) {
        if (auto* result = visualChild(child, name)) return result;
    }
    return nullptr;
}

void click(QObject* button) {
    QVERIFY(button);
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
}

void verifyText(QQuickItem* item) {
    if (item->isVisible() && item->property("truncated").isValid())
        QVERIFY2(! item->property("truncated").toBool(),
                 qPrintable(item->property("text").toString()));
    for (auto* child : item->childItems()) verifyText(child);
}
} // namespace

class CompositionsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot "
                               "assign|Cannot open|TypeError|ReferenceError|polish.*loop).*"));
    }

    void selectionPopupDismissal_data() {
        QTest::addColumn<QString>("componentName");
        QTest::addColumn<QString>("buttonName");
        QTest::addColumn<QString>("popupName");
        QTest::addColumn<QByteArray>("valueProperty");
        QTest::addColumn<QString>("selection");
        QTest::newRow("wifi") << QStringLiteral("QuickControls") << QStringLiteral("wifiTile")
                              << QStringLiteral("quickDetails") << QByteArray("network")
                              << QStringLiteral("Guest");
        QTest::newRow("bluetooth")
            << QStringLiteral("QuickControls") << QStringLiteral("bluetoothTile")
            << QStringLiteral("quickDetails") << QByteArray("device")
            << QStringLiteral("Desk speaker");
        QTest::newRow("media-output")
            << QStringLiteral("MediaCard") << QStringLiteral("mediaOutput")
            << QStringLiteral("mediaOutputs") << QByteArray("outputDevice")
            << QStringLiteral("This device");
    }

    void selectionPopupDismissal() {
        QFETCH(QString, componentName);
        QFETCH(QString, buttonName);
        QFETCH(QString, popupName);
        QFETCH(QByteArray, valueProperty);
        QFETCH(QString, selection);
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(600, 720);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl("qrc:/Qcm/Material/Example/compositions/" + componentName + ".qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(560);
        root->setHeight(root->implicitHeight());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        QTRY_VERIFY(visualChild(root, buttonName));
        auto button = visualChild(root, buttonName);
        if (componentName == "QuickControls") button = visualChild(button, "tileLabels");
        QVERIFY(button);
        auto popup = root->findChild<qml_material::Popup*>(popupName);
        QVERIFY(popup);
        popup->setEnter(nullptr);
        popup->setExit(nullptr);
        QSignalSpy shown(popup, &qml_material::Popup::aboutToShow);
        const auto point =
            button->mapToScene({ button->width() / 2, button->height() / 2 }).toPoint();
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(popup->isOpened());
        QCOMPARE(shown.count(), 1);
        QVERIFY(! popup->dim());
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(! popup->isVisible());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(! popup->isVisible());
        QCOMPARE(shown.count(), 1);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(popup->isOpened());
        QCOMPARE(shown.count(), 2);
        polish(popup->contentItem());
        QQuickItem* choice = nullptr;
        for (auto item : popup->contentItem()->childItems()) {
            if (qobject_cast<qml_material::ButtonControl*>(item) &&
                item->property("text") == selection) {
                choice = item;
                break;
            }
        }
        QVERIFY(choice);
        QTest::mouseClick(
            &window,
            Qt::LeftButton,
            Qt::NoModifier,
            choice->mapToScene({ choice->width() / 2, choice->height() / 2 }).toPoint());
        QVERIFY(! popup->isVisible());
        QCOMPARE(root->property(valueProperty.constData()).toString(), selection);
        QCOMPARE(shown.count(), 2);
    }

    void page() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(320, 900);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine, QUrl("qrc:/Qcm/Material/Example/Compositions.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setSize({ 320, 900 });
        polish(root);
        QCoreApplication::processEvents();
        polish(root);
        auto* media = root->findChild<QQuickItem*>("standaloneMedia");
        auto* quick = root->findChild<QQuickItem*>("quickControls");
        QVERIFY(media && quick);
        auto* clocks = root->findChild<QQuickItem*>("compositionClocks");
        auto* clock  = clocks->findChild<qml_material::Clock*>("clockPreview");
        QVERIFY(clock);
        auto* alarmTime = root->findChild<qml_material::TimeState*>("pickerTime");
        QVERIFY(alarmTime);
        QCOMPARE(clocks->property("alarmTime").value<qml_material::TimeState*>(), alarmTime);
        alarmTime->setTime(6, 45);
        QCOMPARE(clock->property("alarmText").toString(), alarmTime->displayText());
        clocks->setProperty("showAlarm", false);
        QVERIFY(clock->property("alarmText").toString().isEmpty());
        clocks->setProperty("showAlarm", true);
        click(clocks->findChild<QObject*>("clockFixedChoice"));
        QCOMPARE(clock->source()->mode(), qml_material::ClockSource::Fixed);
        click(clocks->findChild<QObject*>("clockLiveChoice"));
        QCOMPARE(clock->source()->mode(), qml_material::ClockSource::System);
        QVERIFY(media->width() > 0 && media->width() <= 320);
        QVERIFY(quick->width() > 0 && quick->width() <= 320);
        QVERIFY(! root->findChild<QObject*>("systemUiShade"));
        QVERIFY(! root->findChild<QObject*>("editQuickTiles"));
        auto* feed = root->findChild<QObject*>("notificationFeed");
        QVERIFY(feed);
        QCOMPARE(feed->property("count").toInt(), 2);
        QVERIFY(! root->findChild<QObject*>("notificationHistory"));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        click(root->findChild<QObject*>("mediaCompactChoice"));
        QVERIFY(! media->property("expanded").toBool());
        click(root->findChild<QObject*>("mediaExpandedChoice"));
        QVERIFY(media->property("expanded").toBool());
        for (int width : { 320, 900 }) {
            root->setWidth(width);
            polish(root);
            QCoreApplication::processEvents();
            polish(root);
            verifyText(clock);
            for (const char* name : { "quickControls",
                                      "notifications",
                                      "compositionMotion",
                                      "compositionClocks",
                                      "compositionTimePickers",
                                      "compositionDatePickers",
                                      "compositionColorPickers",
                                      "compositionFilePickers" }) {
                auto* section = root->findChild<QQuickItem*>(name);
                QVERIFY(section);
                QVERIFY(section->width() > 0 && section->width() <= root->width());
                QVERIFY(section->height() > 0);
            }
            QCOMPARE(quick->property("columns").toInt(), width >= 712 ? 3 : 2);
        }
    }
};

int run_compositions(int argc, char** argv) {
    CompositionsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "compositions.moc"
