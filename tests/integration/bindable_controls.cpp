#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QMouseEvent>
#include "qml_material/control/action.hpp"
#include "qml_material/control/action_group.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/control/check_box.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/control/tab_bar.hpp"
#include "qml_material/control/tab_button.hpp"
#include "qml_material/core/state_bindings.hpp"

using namespace qml_material;

class StateBindingInput : public QObject {
public:
    QProperty<int> value { 10 };
    QBindable<int> bindableValue() { return &value; }
};

enum class BindingState
{
    Base,
    Active
};

class SliderInput : public Slider {
public:
    using Slider::mousePressEvent;
    using Slider::Slider;
};

class BindableControlsTest : public QObject {
    Q_OBJECT
private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot assign|TypeError).*"));
    }

    void stateBindingLiveOwner() {
        QProperty<int>                source(30);
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings.base().bind(key, [] {
            return 10;
        }));
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 90;
        }));
        owner.value.setBinding([&] {
            return source.value();
        });
        QVERIFY(bindings.select(BindingState::Active));
        QCOMPARE(owner.value.value(), 90);
        QVERIFY(bindings.select(BindingState::Base));
        QCOMPARE(owner.value.value(), 30);
        source = 40;
        QCOMPARE(owner.value.value(), 40);
        key.reset();
        QCOMPARE(owner.value.value(), 10);
        owner.value.setBinding([&] {
            return source.value();
        });
        QVERIFY(bindings.select(BindingState::Active));
        QVERIFY(bindings.detach());
        source = 50;
        QCOMPARE(owner.value.value(), 50);
        QVERIFY(! key.isValid());
    }

    void stateBindingDeadOwner_data() {
        QTest::addColumn<QString>("operation");
        for (const auto& operation : { "select", "reset", "detach" })
            QTest::newRow(operation) << QString::fromLatin1(operation);
    }
    void stateBindingDeadOwner() {
        QFETCH(QString, operation);
        bool                          alive           = true;
        int                           baseEvaluations = 0, savedEvaluations = 0;
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        QVERIFY(bindings.setOwnerAliveCheck([&] {
            return alive;
        }));
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings.base().bind(key, [&] {
            ++baseEvaluations;
            return 10;
        }));
        QCOMPARE(owner.value.value(), 10);
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 90;
        }));
        owner.value.setBinding([&] {
            ++savedEvaluations;
            return 30;
        });
        QCOMPARE(owner.value.value(), 30);
        QVERIFY(bindings.select(BindingState::Active));
        QCOMPARE(owner.value.value(), 90);
        const int baseBefore = baseEvaluations, savedBefore = savedEvaluations;
        alive = false;
        if (operation == "select")
            QVERIFY(! bindings.select(BindingState::Base));
        else if (operation == "reset")
            key.reset();
        else
            QVERIFY(! bindings.detach());
        QCOMPARE(owner.value.value(), 90);
        QCOMPARE(baseEvaluations, baseBefore);
        QCOMPARE(savedEvaluations, savedBefore);
        QVERIFY(! key.isValid());
        alive = true;
        QVERIFY(! bindings.select(BindingState::Base));
        key.reset();
        QCOMPARE(owner.value.value(), 90);
    }

    void stateBindingDeadOwnerDeclaration() {
        bool                          alive = true;
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        QVERIFY(bindings.setOwnerAliveCheck([&] {
            return alive;
        }));
        const auto key  = bindings.property<&StateBindingInput::bindableValue>(&owner);
        alive           = false;
        int evaluations = 0;
        QVERIFY(! bindings.base().bind(key, [&] {
            ++evaluations;
            return 20;
        }));
        QCOMPARE(owner.value.value(), 10);
        QCOMPARE(evaluations, 0);
        QVERIFY(! bindings.property<&StateBindingInput::bindableValue>(&owner).isValid());
    }

    void stateBindingReentrantAbandon_data() {
        QTest::addColumn<bool>("reset");
        QTest::newRow("restore") << false;
        QTest::newRow("reset") << true;
    }
    void stateBindingReentrantAbandon() {
        QFETCH(bool, reset);
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key       = bindings.property<&StateBindingInput::bindableValue>(&owner);
        bool       terminate = false;
        auto       original  = [&] {
            if (terminate) bindings.abandon();
            return 30;
        };
        QVERIFY(bindings.base().bind(key, original));
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 90;
        }));
        owner.value.setBinding(original);
        const auto observer = owner.value.onValueChanged([] {
        });
        QCOMPARE(owner.value.value(), 30);
        QVERIFY(bindings.select(BindingState::Active));
        QCOMPARE(owner.value.value(), 90);
        terminate = true;
        if (reset)
            key.reset();
        else
            QVERIFY(! bindings.select(BindingState::Base));
        QCOMPARE(owner.value.value(), 30);
        QVERIFY(! key.isValid());
        QVERIFY(! bindings.detach());
    }

    void stateBindingOwnerEndsBetweenProperties() {
        StateBindingInput             first, second;
        bool                          alive = true;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        QVERIFY(bindings.setOwnerAliveCheck([&] {
            if (first.value.value() == 90) alive = false;
            return alive;
        }));
        const auto firstKey  = bindings.property<&StateBindingInput::bindableValue>(&first);
        const auto secondKey = bindings.property<&StateBindingInput::bindableValue>(&second);
        QVERIFY(bindings.state(BindingState::Active).bind(firstKey, [] {
            return 90;
        }));
        QVERIFY(bindings.state(BindingState::Active).bind(secondKey, [] {
            return 80;
        }));
        QVERIFY(! bindings.select(BindingState::Active));
        QCOMPARE(first.value.value(), 90);
        QCOMPARE(second.value.value(), 10);
        QVERIFY(! firstKey.isValid());
        QVERIFY(! secondKey.isValid());
    }

    void qmlChecked_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<QByteArray>("operation");
        QTest::addColumn<int>("clicks");
        QTest::newRow("button-click") << QByteArray("ButtonBase") << QByteArray("click") << 1;
        QTest::newRow("button-toggle") << QByteArray("ButtonBase") << QByteArray("toggle") << 1;
        QTest::newRow("checkbox") << QByteArray("CheckBoxBase") << QByteArray("click") << 1;
        QTest::newRow("tristate") << QByteArray("CheckBoxBase") << QByteArray("click") << 2;
        QTest::newRow("switch") << QByteArray("SwitchBase") << QByteArray("click") << 1;
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
        const bool    tristate = clicks == 2;
        component.setData("import QtQuick; import Qcm.Material as MD; MD." + type + R"(
            {
                property bool sourceValue: false
                checkable: true
                checked: sourceValue
                readonly property bool observedChecked: checked
                function assignChecked() { checked = checked }
            )" + (tristate ? "tristate: true\n" : "") +
                              "}",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
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
        Button          button;
        Action          action;
        Slider          slider;
        button.bindableChecked().setBinding([&] {
            return source.value();
        });
        action.bindableChecked().setBinding([&] {
            return source.value();
        });
        slider.bindablePressed().setBinding([&] {
            return source.value();
        });
        button.setChecked(false);
        action.setChecked(false);
        slider.setPressed(false);
        QVERIFY(button.bindableChecked().hasBinding());
        QVERIFY(action.bindableChecked().hasBinding());
        QVERIFY(slider.bindablePressed().hasBinding());
        button.setChecked(true);
        action.setChecked(true);
        slider.setPressed(true);
        QVERIFY(button.isChecked());
        QVERIFY(action.isChecked());
        QVERIFY(slider.pressed());
        source = true;
        source = false;
        QVERIFY(! button.isChecked());
        QVERIFY(! action.isChecked());
        QVERIFY(! slider.pressed());
        button.bindableChecked().setValue(false);
        action.bindableChecked().setValue(false);
        slider.bindablePressed().setValue(false);
        QVERIFY(! button.bindableChecked().hasBinding());
        QVERIFY(! action.bindableChecked().hasBinding());
        QVERIFY(! slider.bindablePressed().hasBinding());
    }

    void cppDownBinding() {
        Button          button;
        QProperty<bool> source(false);
        button.bindableDown().setBinding([&] {
            return source.value();
        });
        bool expectedDown = false;
        connect(&button, &AbstractButton::pressed, this, [&] {
            QCOMPARE(button.isDown(), expectedDown);
        });
        button.click();
        QVERIFY(button.bindableDown().hasBinding());
        button.setDown(true);
        QVERIFY(button.bindableDown().hasBinding());
        source = true;
        source = false;
        QVERIFY(! button.isDown());
        button.resetDown();
        QVERIFY(! button.bindableDown().hasBinding());
        expectedDown = true;
        button.click();
        QVERIFY(! button.isDown());
    }

    void qmlCppSetters_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<QByteArray>("property");
        for (const auto& type : { QByteArray("QQ.Button"),
                                  QByteArray("MD.ButtonBase"),
                                  QByteArray("QQ.Action"),
                                  QByteArray("MD.Action"),
                                  QByteArray("QQ.Slider"),
                                  QByteArray("MD.SliderBase") }) {
            const auto properties = type.endsWith("Slider") || type.endsWith("SliderBase")
                                        ? QList<QByteArray> { "pressed" }
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
        component.setData(
            "import QtQuick; import QtQuick.Controls as QQ; import Qcm.Material as MD; " + type +
                " { property bool sourceValue: false; " + property +
                ": sourceValue; function assignValue() { " + property + " = false } }",
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

    void buttonGroup() {
        QProperty<bool> first(true), second(false);
        Button          a, b;
        a.setCheckable(true);
        b.setCheckable(true);
        a.bindableChecked().setBinding([&] {
            return first.value();
        });
        b.bindableChecked().setBinding([&] {
            return second.value();
        });
        ButtonGroup group;
        group.addButton(&a);
        group.addButton(&b);
        bool       observedA = true, observedB = false;
        const auto watchA = a.bindableChecked().addNotifier([&] {
            observedA = a.isChecked();
        });
        const auto watchB = b.bindableChecked().addNotifier([&] {
            observedB = b.isChecked();
        });
        b.click();
        QVERIFY(! a.isChecked() && b.isChecked());
        QVERIFY(! observedA && observedB);
        QVERIFY(a.bindableChecked().hasBinding());
        QVERIFY(b.bindableChecked().hasBinding());
        first = false;
        first = true;
        QVERIFY(a.isChecked() && ! b.isChecked());
        QVERIFY(observedA && ! observedB);
        group.setCheckedButton(&b);
        QVERIFY(! a.isChecked() && b.isChecked());
        QVERIFY(! observedA && observedB);
        QVERIFY(a.bindableChecked().hasBinding());
        QVERIFY(b.bindableChecked().hasBinding());
    }

    void autoExclusive() {
        QProperty<bool> first(true), second(false);
        QQuickItem      parent;
        Button          a(&parent), b(&parent);
        a.setCheckable(true);
        b.setCheckable(true);
        a.setAutoExclusive(true);
        b.setAutoExclusive(true);
        a.bindableChecked().setBinding([&] {
            return first.value();
        });
        b.bindableChecked().setBinding([&] {
            return second.value();
        });
        b.click();
        QVERIFY(! a.isChecked() && b.isChecked());
        QVERIFY(a.bindableChecked().hasBinding());
        QVERIFY(b.bindableChecked().hasBinding());
        first = false;
        first = true;
        QVERIFY(a.isChecked() && ! b.isChecked());
    }

    void targetSelection() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                width: 600
                height: 100
                property string selection: "all"
                MD.FilterChip {
                    objectName: "all"
                    text: "All"
                    checked: root.selection === "all"
                    onClicked: root.selection = "all"
                }
                MD.FilterChip {
                    objectName: "a"
                    x: 200
                    text: "A"
                    checked: root.selection === "a"
                    onClicked: root.selection = "a"
                }
                MD.FilterChip {
                    objectName: "b"
                    x: 400
                    text: "B"
                    checked: root.selection === "b"
                    onClicked: root.selection = "b"
                }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(600, 100);
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(root.get());
        QVERIFY(item);
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* all = root->findChild<Button*>("all");
        auto* a   = root->findChild<Button*>("a");
        auto* b   = root->findChild<Button*>("b");
        QVERIFY(all && a && b);
        const auto leading = [](Button* button) {
            return button->contentItem()->childItems().front();
        };
        const auto verifyIcon = [&](Button* selected, Button* deselected) {
            auto* icon  = leading(selected);
            auto* inner = icon->childItems().front();
            QTRY_VERIFY(inner->implicitWidth() > 0);
            QTRY_COMPARE(icon->implicitWidth(), inner->implicitWidth());
            QTRY_COMPARE(icon->opacity(), 1);
            QVERIFY(icon->isVisible());
            QTRY_VERIFY(icon->width() > 0);
            QTRY_COMPARE(leading(deselected)->implicitWidth(), 0);
            QTRY_COMPARE(leading(deselected)->opacity(), 0);
        };
        for (int i = 0; i < 3; ++i) {
            a->click();
            QVERIFY(a->isChecked() && ! b->isChecked() && ! all->isChecked());
            verifyIcon(a, all);
            b->click();
            QVERIFY(! a->isChecked() && b->isChecked() && ! all->isChecked());
            verifyIcon(b, a);
            all->click();
            QVERIFY(! a->isChecked() && ! b->isChecked() && all->isChecked());
            verifyIcon(all, b);
        }
    }

    void tabSelection() {
        QProperty<bool> source(false);
        TabBar          bar;
        TabButton       a, b;
        bar.addItem(&a);
        bar.addItem(&b);
        a.bindableChecked().setBinding([&] {
            return source.value();
        });
        b.bindableChecked().setBinding([&] {
            return source.value();
        });
        bar.setCurrentIndex(0);
        bar.setCurrentIndex(1);
        QVERIFY(! a.isChecked() && b.isChecked());
        QVERIFY(a.bindableChecked().hasBinding());
        QVERIFY(b.bindableChecked().hasBinding());
    }

    void actionGroup() {
        QProperty<bool> first(true), second(false);
        Action          a, b;
        a.setCheckable(true);
        b.setCheckable(true);
        a.bindableChecked().setBinding([&] {
            return first.value();
        });
        b.bindableChecked().setBinding([&] {
            return second.value();
        });
        ActionGroup group;
        group.addAction(&a);
        group.addAction(&b);
        bool       observedA = true, observedB = false;
        const auto watchA = a.bindableChecked().addNotifier([&] {
            observedA = a.isChecked();
        });
        const auto watchB = b.bindableChecked().addNotifier([&] {
            observedB = b.isChecked();
        });
        b.trigger();
        QVERIFY(! a.isChecked() && b.isChecked());
        QVERIFY(! observedA && observedB);
        QVERIFY(a.bindableChecked().hasBinding());
        QVERIFY(b.bindableChecked().hasBinding());
        first = false;
        first = true;
        QVERIFY(a.isChecked() && ! b.isChecked());
        group.setCheckedAction(&b);
        QVERIFY(! a.isChecked() && b.isChecked());
        QVERIFY(! observedA && observedB);
        QVERIFY(a.bindableChecked().hasBinding());
        QVERIFY(b.bindableChecked().hasBinding());
    }

    void actionSynchronization() {
        QProperty<bool> checked(false), checkable(true);
        Action          action;
        Button          button, peer;
        action.bindableChecked().setBinding([&] {
            return checked.value();
        });
        button.bindableChecked().setBinding([&] {
            return checked.value();
        });
        action.bindableCheckable().setBinding([&] {
            return checkable.value();
        });
        button.bindableCheckable().setBinding([&] {
            return checkable.value();
        });
        button.setAction(&action);
        peer.setAction(&action);
        QVERIFY(button.bindableChecked().hasBinding());
        QVERIFY(button.bindableCheckable().hasBinding());
        button.click();
        QVERIFY(action.isChecked() && button.isChecked() && peer.isChecked());
        QVERIFY(action.bindableChecked().hasBinding());
        QVERIFY(button.bindableChecked().hasBinding());
        checked = true;
        checked = false;
        QVERIFY(! action.isChecked() && ! button.isChecked() && ! peer.isChecked());
        peer.setCheckable(false);
        QVERIFY(! action.isCheckable() && ! button.isCheckable());
        QVERIFY(action.bindableCheckable().hasBinding());
        QVERIFY(button.bindableCheckable().hasBinding());
        checkable = false;
        checkable = true;
        QVERIFY(action.isCheckable() && button.isCheckable() && peer.isCheckable());
    }

    void pointerAndKeyboard() {
        QQuickWindow window;
        window.resize(240, 120);
        Button button(window.contentItem());
        button.setSize(QSizeF(100, 60));
        button.setCheckable(true);
        QProperty<bool> source(false);
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

    void sliderPressed() {
        QQuickWindow window;
        window.resize(240, 120);
        Slider slider(window.contentItem());
        slider.setSize(QSizeF(200, 60));
        QProperty<bool> source(false);
        slider.bindablePressed().setBinding([&] {
            return source.value();
        });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(30, 30));
        QVERIFY(slider.pressed());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(30, 30));
        QVERIFY(! slider.pressed());
        QVERIFY(slider.bindablePressed().hasBinding());
        source = true;
        QVERIFY(slider.pressed());
        slider.setEnabled(false);
        QVERIFY(! slider.pressed());
        QVERIFY(slider.bindablePressed().hasBinding());
        source = false;
        source = true;
        QVERIFY(slider.pressed());
    }

    void deletedInBindableObserver_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<QByteArray>("operation");
        for (const auto& type :
             { QByteArray("button"), QByteArray("action"), QByteArray("slider") }) {
            const auto properties = type == "slider" ? QList<QByteArray> { "pressed" }
                                                     : QList<QByteArray> { "checked", "checkable" };
            for (const auto& property : properties) {
                for (const auto& operation :
                     { QByteArray("internal"), QByteArray("setter"), QByteArray("binding") }) {
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
        QProperty<bool> source(false);
        QObject*        object = type == "button" ? static_cast<QObject*>(new Button)
                                 : type == "action"
                                     ? static_cast<QObject*>(new Action)
                                     : static_cast<QObject*>(new SliderInput(window.contentItem()));
        QPointer<QObject> guard(object);
        auto*             button = qobject_cast<Button*>(object);
        auto*             action = qobject_cast<Action*>(object);
        auto*             slider = qobject_cast<Slider*>(object);
        if (property == "checked") {
            if (button) button->setCheckable(true);
            if (action) action->setCheckable(true);
        }
        if (slider) {
            slider->setSize(QSizeF(200, 60));
        }
        QBindable<bool> bindable =
            slider ? slider->bindablePressed()
            : button
                ? (property == "checked" ? button->bindableChecked() : button->bindableCheckable())
                : (property == "checked" ? action->bindableChecked() : action->bindableCheckable());
        QVERIFY(bindable.isValid());
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
            static_cast<SliderInput*>(slider)->mousePressEvent(&press);
        } else if (property == "checked") {
            if (button) button->click();
            if (action) action->toggle();
        } else if (button) {
            Action shared;
            shared.setCheckable(true);
            button->setAction(&shared);
        } else {
            Button peer;
            peer.setAction(action);
            peer.setCheckable(true);
        }
        QVERIFY(! guard);
    }

    void qmlPressState() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ButtonBase {
                readonly property bool observedPressed: pressed
                readonly property bool observedDown: down
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<AbstractButton*>(object.get());
        QVERIFY(button);
        QSignalSpy pressedChanges(button, &AbstractButton::pressedChanged);
        QSignalSpy downChanges(button, &AbstractButton::downChanged);
        bool       expectedDown = true;
        connect(button, &AbstractButton::pressedChanged, this, [&] {
            QCOMPARE(button->isDown(), expectedDown && ! button->isPressed());
            QCOMPARE(object->property("observedDown").toBool(), button->isDown());
        });
        connect(button, &AbstractButton::pressed, this, [&] {
            QVERIFY(button->isPressed());
            QCOMPARE(button->isDown(), expectedDown);
            QVERIFY(object->property("observedPressed").toBool());
            QCOMPARE(object->property("observedDown").toBool(), expectedDown);
        });
        connect(button, &AbstractButton::released, this, [&] {
            QVERIFY(! button->isPressed());
            QVERIFY(! button->isDown());
            QVERIFY(! object->property("observedPressed").toBool());
            QVERIFY(! object->property("observedDown").toBool());
        });
        button->click();
        QCOMPARE(pressedChanges.count(), 2);
        QCOMPARE(downChanges.count(), 2);
        QVERIFY(! button->bindableDown().hasBinding());

        button->setDown(false);
        QVERIFY(! button->bindableDown().hasBinding());
        expectedDown = false;
        button->click();
        QCOMPARE(pressedChanges.count(), 4);
        QCOMPARE(downChanges.count(), 2);

        button->resetDown();
        QVERIFY(! button->bindableDown().hasBinding());
        expectedDown = true;
        button->click();
        QCOMPARE(pressedChanges.count(), 6);
        QCOMPARE(downChanges.count(), 4);
    }

    void downCompatibility_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::newRow("qt") << QByteArray("QQ.Button");
        QTest::newRow("material") << QByteArray("MD.ButtonBase");
    }
    void downCompatibility() {
        QFETCH(QByteArray, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(
            "import QtQuick; import QtQuick.Controls as QQ; import Qcm.Material as MD; " + type +
                R"( {
                property bool sourceValue: false
                property string trace: ""
                down: sourceValue
                onPressedChanged: trace += "changed:" + pressed + ":" + down + ";"
                onPressed: trace += "pressed:" + down + ";"
                onReleased: trace += "released:" + down + ";"
                function resetValue() { down = undefined }
            })",
            QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        const auto down =
            object->metaObject()->property(object->metaObject()->indexOfProperty("down"));
        const auto click = [&] {
            object->setProperty("trace", "");
            QVERIFY(QMetaObject::invokeMethod(object.get(), "click"));
        };
        click();
        QCOMPARE(object->property("trace").toString(),
                 "changed:true:false;pressed:false;changed:false:false;released:false;");

        QVERIFY(down.reset(object.get()));
        click();
        QCOMPARE(object->property("trace").toString(),
                 "changed:true:false;pressed:true;changed:false:true;released:false;");

        object->setProperty("sourceValue", true);
        QVERIFY(object->property("down").toBool());
        QVERIFY(down.reset(object.get()));
        QVERIFY(! object->property("down").toBool());
        object->setProperty("sourceValue", false);
        click();
        QCOMPARE(object->property("trace").toString(),
                 "changed:true:false;pressed:false;changed:false:false;released:false;");

        QVERIFY(object->setProperty("down", false));
        object->setProperty("sourceValue", true);
        QVERIFY(object->property("down").toBool());
        QVERIFY(QMetaObject::invokeMethod(object.get(), "resetValue"));
        object->setProperty("sourceValue", false);
        object->setProperty("sourceValue", true);
        QVERIFY(! object->property("down").toBool());
        click();
        QCOMPARE(object->property("trace").toString(),
                 "changed:true:false;pressed:true;changed:false:true;released:false;");
    }

    void actionToggleCompatibility_data() {
        QTest::addColumn<QByteArray>("prefix");
        QTest::addColumn<bool>("checkable");
        QTest::addColumn<bool>("enabled");
        QTest::addColumn<bool>("exclusive");
        QTest::addColumn<bool>("trigger");
        QTest::addColumn<int>("toggled");
        QTest::addColumn<int>("triggered");
        QTest::addColumn<bool>("checked");
        for (const auto& prefix : { QByteArray("QQ"), QByteArray("MD") }) {
            QTest::newRow((prefix + "-plain-toggle").constData())
                << prefix << false << true << false << false << 1 << 0 << true;
            QTest::newRow((prefix + "-plain-trigger").constData())
                << prefix << false << true << false << true << 0 << 1 << true;
            QTest::newRow((prefix + "-checked-toggle").constData())
                << prefix << true << true << false << false << 1 << 0 << false;
            QTest::newRow((prefix + "-checked-trigger").constData())
                << prefix << true << true << false << true << 1 << 1 << false;
            QTest::newRow((prefix + "-exclusive-toggle").constData())
                << prefix << true << true << true << false << 1 << 0 << false;
            QTest::newRow((prefix + "-exclusive-trigger").constData())
                << prefix << true << true << true << true << 0 << 1 << true;
            QTest::newRow((prefix + "-disabled-toggle").constData())
                << prefix << true << false << false << false << 0 << 0 << true;
            QTest::newRow((prefix + "-disabled-trigger").constData())
                << prefix << true << false << false << true << 0 << 0 << true;
        }
    }
    void actionToggleCompatibility() {
        QFETCH(QByteArray, prefix);
        QFETCH(bool, checkable);
        QFETCH(bool, enabled);
        QFETCH(bool, exclusive);
        QFETCH(bool, trigger);
        QFETCH(int, toggled);
        QFETCH(int, triggered);
        QFETCH(bool, checked);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(
            "import QtQuick; import QtQuick.Controls as QQ; import Qcm.Material as MD; " + prefix +
                R"(.ActionGroup {
                property int toggles: 0
                property int triggers: 0
                property var source: QtObject {}
                property var lastSource: null
                property alias target: action
            )" + prefix +
                R"(.Action {
                    id: action
                    checked: true
                    onToggled: function(source) { toggles++; lastSource = source }
                    onTriggered: function(source) { triggers++; lastSource = source }
                }
                function toggleTarget() { action.toggle(source) }
                function triggerTarget() { action.trigger(source) }
            })",
            QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* target = object->property("target").value<QObject*>();
        QVERIFY(target);
        object->setProperty("exclusive", exclusive);
        target->setProperty("checkable", checkable);
        target->setProperty("enabled", enabled);
        QVERIFY(
            QMetaObject::invokeMethod(object.get(), trigger ? "triggerTarget" : "toggleTarget"));
        QCOMPARE(object->property("toggles").toInt(), toggled);
        QCOMPARE(object->property("triggers").toInt(), triggered);
        QCOMPARE(target->property("checked").toBool(), checked);
        if (toggled || triggered)
            QCOMPARE(object->property("lastSource").value<QObject*>(),
                     object->property("source").value<QObject*>());
    }

    void deletedInPressObserver_data() {
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<QByteArray>("operation");
        QTest::addColumn<bool>("deleteOnPress");
        QTest::newRow("pressed-click") << QByteArray("pressed") << QByteArray("click") << true;
        QTest::newRow("pressed-release") << QByteArray("pressed") << QByteArray("click") << false;
        QTest::newRow("down-click") << QByteArray("down") << QByteArray("click") << true;
        QTest::newRow("down-release") << QByteArray("down") << QByteArray("click") << false;
        QTest::newRow("down-setter") << QByteArray("down") << QByteArray("setter") << true;
        QTest::newRow("down-binding") << QByteArray("down") << QByteArray("binding") << true;
    }
    void deletedInPressObserver() {
        QFETCH(QByteArray, property);
        QFETCH(QByteArray, operation);
        QFETCH(bool, deleteOnPress);
        QProperty<bool>  source(false);
        auto*            button = new Button;
        QPointer<Button> guard(button);
        QBindable<bool>  bindable =
            property == "down" ? button->bindableDown() : button->bindablePressed();
        QVERIFY(bindable.isValid());
        if (operation == "binding") {
            bindable.setBinding([&] {
                return source.value();
            });
        }
        const auto observer = bindable.addNotifier([button, property, deleteOnPress] {
            if (button->property(property.constData()).toBool() == deleteOnPress) delete button;
        });
        if (operation == "binding")
            source = true;
        else if (operation == "setter")
            button->setDown(true);
        else
            button->click();
        QVERIFY(! guard);
    }

    void deletedDuringChange() {
        QProperty<bool>  source(false);
        auto*            button = new Button;
        QPointer<Button> guard(button);
        button->setCheckable(true);
        button->bindableChecked().setBinding([&] {
            return source.value();
        });
        connect(button, &Button::checkedChanged, button, [button] {
            delete button;
        });
        button->click();
        QVERIFY(! guard);
    }
};

int run_bindable_controls(int argc, char** argv) {
    BindableControlsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "bindable_controls.moc"
