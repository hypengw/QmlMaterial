#include "../input_test.hpp"
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QMouseEvent>
#include "qml_material/control/action.hpp"
#include "qml_material/control/toggle_button.hpp"
#include "qml_material/control/slider.hpp"

using namespace qml_material;

class ControlBindingsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot assign|TypeError).*"));
    }

    void qmlChecked_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<QByteArray>("operation");
        QTest::addColumn<int>("clicks");
        QTest::newRow("button-click") << QByteArray("ToggleButtonBase") << QByteArray("click") << 1;
        QTest::newRow("action-trigger") << QByteArray("Action") << QByteArray("trigger") << 1;
        QTest::newRow("action-toggle") << QByteArray("Action") << QByteArray("toggle") << 1;
    }

    void qmlChecked() {
        QFETCH(QByteArray, type);
        QFETCH(QByteArray, operation);
        QFETCH(int, clicks);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick; import Qcm.Material as MD; MD." + type + R"(
            {
                property bool sourceValue: false
                checked: sourceValue
                readonly property bool observedChecked: checked
                function assignChecked() { checked = checked }
            })",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        if (auto* button = qobject_cast<ToggleButton*>(object.get()))
            connect(button, &ToggleButton::checkedChangeRequested, button, [button](bool value) {
                button->setProperty("sourceValue", value);
            });
        if (auto* action = qobject_cast<Action*>(object.get())) action->setCheckable(true);
        QSignalSpy changes(object.get(), SIGNAL(checkedChanged()));
        for (int i = 0; i < clicks; ++i)
            QVERIFY(QMetaObject::invokeMethod(object.get(), operation.constData()));
        QVERIFY(object->property("checked").toBool());
        QVERIFY(object->property("observedChecked").toBool());
        QCOMPARE(changes.count(), 1);
        object->setProperty("sourceValue", true);
        object->setProperty("sourceValue", false);
        QVERIFY(! object->property("checked").toBool());
        QVERIFY(! object->property("observedChecked").toBool());
        QCOMPARE(changes.count(), 2);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "assignChecked"));
        object->setProperty("sourceValue", true);
        QVERIFY(! object->property("checked").toBool());
    }

    void cppExplicitSetters() {
        QProperty<bool> source(false);
        ToggleButton    button;
        Action          action;
        button.bindableChecked().setBinding([&] {
            return source.value();
        });
        button.setChecked(false);
        action.setChecked(false);
        QVERIFY(button.bindableChecked().hasBinding());
        button.setChecked(true);
        action.setChecked(true);
        QVERIFY(button.isChecked());
        QVERIFY(action.isChecked());
        source = true;
        source = false;
        QVERIFY(! button.isChecked());
        QVERIFY(action.isChecked());
        button.bindableChecked().setValue(false);
        QVERIFY(! button.bindableChecked().hasBinding());
    }

    void qmlCppSetters_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<QByteArray>("property");
        for (const auto& type : { QByteArray("MD.ToggleButtonBase"), QByteArray("MD.Action") }) {
            const auto properties = type == "MD.ToggleButtonBase"
                                        ? QList<QByteArray> { "checked" }
                                        : QList<QByteArray> { "checked", "checkable" };
            for (const auto& property : properties)
                QTest::newRow((type + '-' + property).constData()) << type << property;
        }
    }

    void qmlCppSetters() {
        QFETCH(QByteArray, type);
        QFETCH(QByteArray, property);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick; import Qcm.Material as MD; " + type +
                              " { property bool sourceValue: false; " + property +
                              ": sourceValue; function assignValue() { " + property +
                              " = false } }",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QVERIFY(object->setProperty(property.constData(), false));
        object->setProperty("sourceValue", true);
        QVERIFY(object->property(property.constData()).toBool());
        QVERIFY(object->setProperty(property.constData(), false));
        QVERIFY(! object->property(property.constData()).toBool());
        object->setProperty("sourceValue", false);
        object->setProperty("sourceValue", true);
        QVERIFY(object->property(property.constData()).toBool());
        QVERIFY(QMetaObject::invokeMethod(object.get(), "assignValue"));
        object->setProperty("sourceValue", false);
        object->setProperty("sourceValue", true);
        QVERIFY(! object->property(property.constData()).toBool());
    }

    void pointerAndKeyboard() {
        QQuickWindow window;
        window.resize(240, 120);
        ToggleButton button(window.contentItem());
        button.setSize(QSizeF(100, 60));
        QProperty<bool> source(false);
        connect(&button, &ToggleButton::checkedChangeRequested, &button, [&source](bool value) {
            source = value;
        });
        button.bindableChecked().setBinding([&] {
            return source.value();
        });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(30, 30));
        QVERIFY(button.isChecked());
        QVERIFY(button.bindableChecked().hasBinding());
        button.forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Space);
        QVERIFY(! button.isChecked());
        QVERIFY(button.bindableChecked().hasBinding());
        source = true;
        QVERIFY(button.isChecked());
    }

    void deletedInBindableObserver_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<QByteArray>("operation");
        for (const auto& type :
             { QByteArray("button"), QByteArray("action"), QByteArray("slider") }) {
            const auto properties = type == "slider" ? QList<QByteArray> { "pressed" }
                                    : type == "button"
                                        ? QList<QByteArray> { "checked" }
                                        : QList<QByteArray> { "checked", "checkable" };
            for (const auto& property : properties) {
                if (type == "action" && property == "checked") continue;
                for (const auto& operation :
                     { QByteArray("internal"), QByteArray("setter"), QByteArray("binding") }) {
                    if (type == "slider" && operation != "internal") continue;
                    const auto name = type + '-' + property + '-' + operation;
                    QTest::newRow(name.constData()) << type << property << operation;
                }
            }
        }
    }

    void deletedInBindableObserver() {
        QFETCH(QByteArray, type);
        QFETCH(QByteArray, property);
        QFETCH(QByteArray, operation);
        QQuickWindow window;
        window.resize(240, 120);
        QProperty<bool>   source(false);
        QObject*          object = type == "button" ? static_cast<QObject*>(new ToggleButton)
                                   : type == "action"
                                       ? static_cast<QObject*>(new Action)
                                       : static_cast<QObject*>(new Slider(window.contentItem()));
        QPointer<QObject> guard(object);
        auto*             button = qobject_cast<ToggleButton*>(object);
        auto*             action = qobject_cast<Action*>(object);
        auto*             slider = qobject_cast<Slider*>(object);
        if (button)
            connect(button, &ToggleButton::checkedChangeRequested, button, [&source](bool value) {
                source = value;
            });
        if (property == "checked") {
            if (action) action->setCheckable(true);
        }
        if (slider) {
            slider->setSize(QSizeF(200, 60));
        }
        InteractionAccumulator input(this);
        if (slider) input.setSource(slider->interactionSource());
        QProperty<bool> pressed;
        pressed.setBinding([&] {
            return input.values().pressed;
        });
        QBindable<bool> bindable = slider   ? QBindable<bool>(&pressed)
                                   : button ? button->bindableChecked()
                                            : action->bindableCheckable();
        QVERIFY(bindable.isValid());
        if (! slider)
            bindable.setBinding([&] {
                return source.value();
            });
        const auto observer = bindable.addNotifier([object] {
            delete object;
        });
        if (operation == "binding") {
            source = true;
        } else if (operation == "setter") {
            QVERIFY(object->setProperty(property.constData(), true));
        } else if (slider) {
            QMouseEvent press(QEvent::MouseButtonPress,
                              QPointF(30, 30),
                              QPointF(30, 30),
                              Qt::LeftButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
            QCoreApplication::sendEvent(slider, &press);
            flushInteractions();
        } else if (property == "checked") {
            if (button) button->requestChecked(true);
            if (action) action->toggle();
        } else {
            action->setCheckable(true);
        }
        QVERIFY(! guard);
    }

    void deletedInPressObserver_data() {
        QTest::addColumn<bool>("deleteOnPress");
        QTest::newRow("pressed-click") << true;
        QTest::newRow("pressed-release") << false;
    }

    void deletedInPressObserver() {
        QFETCH(bool, deleteOnPress);
        auto* button = new ToggleButton;
        button->setSize({ 40, 40 });
        QPointer<ToggleButton> guard(button);
        InteractionAccumulator input(this);
        input.setSource(button->interactionSource());
        const auto observer = input.bindableValues().addNotifier([&] {
            if (input.values().pressed == deleteOnPress && guard) delete button;
        });
        setButtonPressed(button, true);
        if (guard) setButtonPressed(button, false);
        QVERIFY(! guard);
    }

    void deletedDuringChange() {
        QProperty<bool>        source(false);
        auto*                  button = new ToggleButton;
        QPointer<ToggleButton> guard(button);
        button->bindableChecked().setBinding([&] {
            return source.value();
        });
        connect(button, &ToggleButton::checkedChanged, button, [button] {
            delete button;
        });
        button->setChecked(true);
        QVERIFY(! guard);
    }
    void qmlPressState() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"QML(
            import Qcm.Material as MD
            MD.ToggleButtonBase {
                id: control
                width: 100; height: 50
                MD.InteractionObserver { id: feedback; source: control.interactionSource }
                readonly property bool observedPressed: feedback.pressed
            }
        )QML",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto*      button = qobject_cast<ToggleButton*>(object.get());
        QSignalSpy changes(button->interactionSource(), &InteractionSource::interaction);
        button->click();
        QCOMPARE(changes.count(), 0);
        setButtonPressed(button, true);
        QVERIFY(object->property("observedPressed").toBool());
        setButtonPressed(button, false);
        QVERIFY(! object->property("observedPressed").toBool());
        QCOMPARE(changes.count(), 2);
    }
};

int run_control_bindings(int argc, char** argv) {
    ControlBindingsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "control_bindings.moc"
