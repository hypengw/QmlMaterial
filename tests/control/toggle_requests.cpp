#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/check_box.hpp"
#include "qml_material/control/switch.hpp"
#include "qml_material/control/toggle_button.hpp"
#include "qml_material/style/check_box_state.hpp"
#include "qml_material/style/switch_state.hpp"
#include "qml_material/token/color.hpp"
#include "../input_test.hpp"

using namespace qml_material;
using qmtest::events;
using qmtest::mouse;
namespace
{
class CheckBoxAppearanceProbe : public CheckBoxState {
public:
    using ButtonInteractionState::checked;
};
std::unique_ptr<ButtonControl> controlFor(int kind) {
    if (kind == 0) return std::make_unique<Switch>();
    if (kind == 3) return std::make_unique<ToggleButton>();
    auto box = std::make_unique<CheckBox>();
    box->setTristate(kind == 2);
    return box;
}
int committed(ButtonControl* control) {
    if (auto* box = qobject_cast<CheckBox*>(control)) return box->checkState();
    return static_cast<ToggleButton*>(control)->isChecked() ? Qt::Checked : Qt::Unchecked;
}
void commit(ButtonControl* control, int value) {
    if (auto* box = qobject_cast<CheckBox*>(control))
        box->setCheckState(Qt::CheckState(value));
    else
        static_cast<ToggleButton*>(control)->setChecked(value == Qt::Checked);
}
void observeRequests(ButtonControl* control, QObject* context, std::function<void(int)> requested) {
    if (auto* box = qobject_cast<CheckBox*>(control))
        QObject::connect(box, &CheckBox::checkStateChangeRequested, context, requested);
    else
        QObject::connect(static_cast<ToggleButton*>(control),
                         &ToggleButton::checkedChangeRequested,
                         context,
                         [requested](bool value) {
                             requested(value ? Qt::Checked : Qt::Unchecked);
                         });
}
} // namespace

class ToggleRequestsTest : public QObject {
    Q_OBJECT

private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*")); }

    void inputRequests_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<int>("input");
        for (int kind = 0; kind < 4; ++kind)
            for (bool accept : { false, true })
                for (int input = 0; input < 4; ++input)
                    if (kind == 3 || input == 1 || input == 2)
                        QTest::newRow(
                            qPrintable(QString("%1-%2-%3").arg(kind).arg(accept).arg(input)))
                            << kind << accept << input;
    }

    void inputRequests() {
        QFETCH(int, kind);
        QFETCH(bool, accept);
        QFETCH(int, input);
        auto         control = controlFor(kind);
        QQuickWindow window;
        window.resize(200, 100);
        control->setParentItem(window.contentItem());
        control->setSize({ 100, 50 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QList<int> requests;
        observeRequests(control.get(), this, [&](int value) {
            requests.append(value);
            QCOMPARE(committed(control.get()),
                     requests.size() == 1 || ! accept ? 0
                     : kind == 2                      ? 1
                                                      : 2);
            if (accept) commit(control.get(), value);
        });
        InteractionAccumulator feedback(this);
        feedback.setSource(control->interactionSource());
        const auto activate = [&] {
            if (input == 0)
                control->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
            else if (input == 2) {
                control->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, { 20, 20 }, &window).commit();
                QTest::touchEvent(&window, device).release(0, { 20, 20 }, &window).commit();
            }
            flushInteractions();
        };
        activate();
        const int first = kind == 2 ? Qt::PartiallyChecked : Qt::Checked;
        QCOMPARE(requests, QList<int> { first });
        QCOMPARE(committed(control.get()), accept ? first : int(Qt::Unchecked));
        activate();
        QCOMPARE(requests.size(), 2);
        QCOMPARE(requests.last(),
                 accept ? kind == 2 ? int(Qt::Checked) : int(Qt::Unchecked) : first);
        QVERIFY(! feedback.values().pressed && ! feedback.values().dragged);
    }

    void qmlOwner_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::newRow("switch") << QByteArray("Switch");
        QTest::newRow("checkbox") << QByteArray("CheckBox");
        QTest::newRow("toggle") << QByteArray("ToggleButtonBase");
        QTest::newRow("toggle-fab") << QByteArray("ToggleFAB");
        QTest::newRow("button") << QByteArray("ToggleButton");
        QTest::newRow("icon") << QByteArray("IconToggleButton");
        QTest::newRow("segmented") << QByteArray("SegmentedButton");
    }

    void qmlOwner() {
        QFETCH(QByteArray, kind);
        const bool toggle = kind != "CheckBox";
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                property bool accepted: false
                property int ownerValue: 0
                property int requests: 0
                MD.)" + kind + R"( {
                    objectName: "control"
                    width: 120; height: 48
                    )" +
                              (toggle ? QByteArray(R"(
                    checked: parent.ownerValue === Qt.Checked
                    onCheckedChangeRequested: value => {
                        parent.requests++
                        if (parent.accepted) parent.ownerValue = value ? Qt.Checked : Qt.Unchecked
                    }
                    )")
                                      : QByteArray(R"(
                    checkState: parent.ownerValue
                    onCheckStateChangeRequested: value => {
                        parent.requests++
                        if (parent.accepted) parent.ownerValue = value
                    }
                    )")) + R"(
                }
            }
        )",
                          QUrl("qrc:/tests/toggle-owner.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = object->findChild<ButtonControl*>("control");
        QVERIFY(control);
        control->click();
        control->click();
        QCOMPARE(committed(control), 0);
        QCOMPARE(object->property("requests").toInt(), 2);
        object->setProperty("accepted", true);
        control->click();
        QCOMPARE(committed(control), 2);
        object->setProperty("accepted", false);
        control->click();
        QCOMPARE(committed(control), 2);
        object->setProperty("accepted", true);
        object->setProperty("ownerValue", 0);
        QCOMPARE(committed(control), 0);
        control->click();
        QCOMPARE(committed(control), 2);
        auto* state = control->findChild<CommonState*>();
        if (kind == "ToggleButtonBase") {
            QVERIFY(! state);
            return;
        }
        QVERIFY(state);
        QCOMPARE(state->interactionSource(), control->interactionSource());
        if (! toggle) {
            object->setProperty("ownerValue", int(Qt::PartiallyChecked));
            QCOMPARE(committed(control), 1);
            QCOMPARE(state->indication().color, state->colors()->primary());
            control->setEnabled(false);
            auto* appearance  = static_cast<CheckBoxState*>(state);
            auto  disabledInk = state->colors()->on_surface();
            disabledInk.setAlphaF(state->stateTokens().disabled_content);
            QCOMPARE(appearance->iconColor(), state->colors()->surface());
            QCOMPARE(appearance->iconBackgroundColor(), disabledInk);
            QCOMPARE(appearance->outlineColor(), disabledInk);
            QCOMPARE(control->contentItem()->opacity(), state->contentOpacity());
        } else if (kind == "Switch") {
            auto* item = static_cast<Switch*>(control);
            QCOMPARE(item->position(), 1);
            item->setLayoutDirection(Qt::RightToLeft);
            QCOMPARE(item->visualPosition(), 0);
        }
    }

    void cancelled_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("change");
        for (int kind = 0; kind < 4; ++kind)
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(kind).arg(change))) << kind << change;
    }

    void cancelled() {
        QFETCH(int, kind);
        QFETCH(int, change);
        auto control = controlFor(kind);
        control->setSize({ 100, 50 });
        int requests = 0;
        observeRequests(control.get(), this, [&](int) {
            ++requests;
        });
        InteractionAccumulator feedback(this);
        feedback.setSource(control->interactionSource());
        QMouseEvent press(QEvent::MouseButtonPress,
                          { 20, 20 },
                          { 20, 20 },
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &press);
        flushInteractions();
        QVERIFY(feedback.values().pressed);
        InteractionSource next;
        if (change == 0)
            control->setEnabled(false);
        else if (change == 1)
            control->setVisible(false);
        else
            control->setInteractionSource(&next);
        QMouseEvent release(QEvent::MouseButtonRelease,
                            { 20, 20 },
                            { 20, 20 },
                            Qt::LeftButton,
                            Qt::NoButton,
                            Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &release);
        flushInteractions();
        QCOMPARE(requests, 0);
        QCOMPARE(committed(control.get()), 0);
        QVERIFY(! feedback.values().pressed);
    }

    void appearanceTargetReentry() {
        auto     first = std::make_unique<CheckBox>();
        CheckBox second;
        second.setCheckState(Qt::PartiallyChecked);
        CheckBoxAppearanceProbe state;
        connect(&state, &CommonState::targetChanged, this, [&] {
            if (first && state.item() == first.get()) state.setItem(&second);
        });
        state.setItem(first.get());
        QCOMPARE(state.item(), &second);
        QVERIFY(state.checked());
        first.reset();
        QCOMPARE(state.item(), &second);
        second.setCheckState(Qt::Unchecked);
        QVERIFY(! state.checked());
        state.setItem(nullptr);
        QVERIFY(! state.checked());
        QCOMPARE(state.interactionSource(), nullptr);
    }

    void switchWithoutIndicatorUsesClick() {
        Switch control;
        control.setSize({ 100, 50 });
        connect(&control, &Switch::checkedChangeRequested, &control, &Switch::setChecked);
        QSignalSpy input(control.interactionSource(), &InteractionSource::interaction);
        mouse(control, QEvent::MouseButtonPress, { 10, 10 });
        mouse(control, QEvent::MouseMove, { 80, 10 });
        mouse(control, QEvent::MouseButtonRelease, { 80, 10 });
        QVERIFY(control.isChecked());
        QVERIFY(events(input, Interaction::Drag).isEmpty());
        mouse(control, QEvent::MouseButtonPress, { 10, 10 });
        mouse(control, QEvent::MouseMove, { 110, 10 });
        mouse(control, QEvent::MouseButtonRelease, { 10, 10 });
        QVERIFY(control.isChecked());
    }

    void toggleBinding() {
        QProperty<bool> value(false);
        ToggleButton    control;
        control.bindableChecked().setBinding([&] {
            return value.value();
        });
        QSignalSpy changes(&control, &ToggleButton::checkedChanged);
        control.click();
        QVERIFY(! control.isChecked());
        QVERIFY(changes.isEmpty());
        connect(&control, &ToggleButton::checkedChangeRequested, this, [&](bool checked) {
            value = checked;
        });
        control.click();
        QVERIFY(control.isChecked());
        QCOMPARE(changes.size(), 1);
        control.click();
        QVERIFY(! control.isChecked());
        QCOMPARE(changes.size(), 2);
        value = true;
        QVERIFY(control.isChecked());
        QVERIFY(control.bindableChecked().hasBinding());
    }

    void switchDeletedInNotification_data() {
        QTest::addColumn<int>("notification");
        for (int i = 0; i < 3; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
    }

    void switchDragRequests_data() {
        QTest::addColumn<bool>("rtl");
        QTest::addColumn<bool>("touch");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("initial");
        for (bool rtl : { false, true })
            for (bool touch : { false, true })
                for (bool accept : { false, true })
                    for (bool initial : { false, true })
                        QTest::newRow(qPrintable(
                            QString("%1-%2-%3-%4").arg(rtl).arg(touch).arg(accept).arg(initial)))
                            << rtl << touch << accept << initial;
    }

    void switchDragRequests() {
        QFETCH(bool, rtl);
        QFETCH(bool, touch);
        QFETCH(bool, accept);
        QFETCH(bool, initial);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD; MD.Switch { width: 120; height: 48 }",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = qobject_cast<Switch*>(object.get());
        QVERIFY(control && control->indicator());
        QProperty<bool> value(initial);
        control->bindableChecked().setBinding([&] {
            return value.value();
        });
        control->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        QSignalSpy requests(control, &Switch::checkedChangeRequested);
        if (accept)
            connect(control, &Switch::checkedChangeRequested, this, [&](bool checked) {
                value = checked;
            });
        InteractionAccumulator feedback(this);
        feedback.setSource(control->interactionSource());
        QQuickWindow window;
        window.resize(160, 80);
        control->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto point = [control, rtl](int x) {
            auto* track = control->indicator();
            return track->mapToScene({ rtl ? track->width() - x : qreal(x), track->height() / 2 })
                .toPoint();
        };
        const auto start = point(initial ? 42 : 10), end = point(initial ? 10 : 42);
        const auto flush = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
            flushInteractions();
        };
        static auto* device   = QTest::createTouchDevice();
        auto         sequence = QTest::touchEvent(&window, device, false);
        if (touch)
            sequence.press(0, start, &window).commit();
        else
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        flush();
        QVERIFY(! control->isDragging());
        if (touch)
            sequence.move(0, end, &window).commit();
        else
            QTest::mouseMove(&window, end);
        flush();
        QVERIFY(control->isDragging());
        QCOMPARE(control->position(), (initial ? 10. : 42.) / 52);
        QCOMPARE(control->visualPosition(), rtl ? 1 - control->position() : control->position());
        QCOMPARE(control->isChecked(), initial);
        QVERIFY(requests.isEmpty());
        QVERIFY(feedback.values().dragged && ! feedback.values().pressed);
        if (touch)
            sequence.release(0, end, &window).commit();
        else
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, end);
        flush();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toBool(), ! initial);
        QCOMPARE(control->isChecked(), accept ? ! initial : initial);
        QCOMPARE(control->position(), control->isChecked() ? 1 : 0);
        QVERIFY(! control->isDragging());
        QVERIFY(! feedback.values().pressed && ! feedback.values().dragged);
        QVERIFY(control->bindableChecked().hasBinding());
    }

    void switchDragCancellation_data() {
        QTest::addColumn<int>("reason");
        for (int reason = 0; reason < 9; ++reason)
            QTest::newRow(qPrintable(QString::number(reason))) << reason;
    }

    void switchDragCancellation() {
        QFETCH(int, reason);
        Switch control;
        control.setSize({ 120, 48 });
        auto* track = new QQuickItem(&control);
        track->setSize({ 52, 32 });
        control.setIndicator(track);
        QQuickWindow window;
        window.resize(160, 80);
        control.setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        InteractionSource replacement;
        QSignalSpy        requests(&control, &Switch::checkedChangeRequested);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 10, 16 });
        QTest::mouseMove(&window, { 42, 16 });
        QVERIFY(control.isDragging());
        switch (reason) {
        case 0: control.setEnabled(false); break;
        case 1: control.setVisible(false); break;
        case 2: control.setInteractionSource(&replacement); break;
        case 3: control.setIndicator(nullptr); break;
        case 4: track->setWidth(60); break;
        case 5: track->setHeight(36); break;
        case 6: control.setLayoutDirection(Qt::RightToLeft); break;
        case 7: control.setParentItem(nullptr); break;
        case 8: window.hide(); break;
        }
        QVERIFY(! control.isDragging());
        QCOMPARE(control.position(), 0);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 42, 16 });
        QVERIFY(requests.isEmpty());
        QVERIFY(! control.isChecked());
        control.setParentItem(nullptr);
    }

    void switchLabelDrag() {
        Switch control;
        control.setSize({ 180, 48 });
        auto* track = new QQuickItem(&control);
        track->setSize({ 52, 32 });
        control.setIndicator(track);
        QQuickWindow window;
        window.resize(200, 80);
        control.setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy requests(&control, &Switch::checkedChangeRequested);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 150, 16 });
        QTest::mouseMove(&window, { 120, 16 });
        QVERIFY(! control.isDragging());
        QTest::mouseMove(&window, { 30, 16 });
        QVERIFY(control.isDragging());
        QCOMPARE(control.position(), 30. / 52);
        QTest::mouseMove(&window, { 5, 16 });
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 5, 16 });
        QVERIFY(requests.isEmpty());
        QVERIFY(! control.isDragging());
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 150, 16 });
        QTest::mouseMove(&window, { 190, 16 });
        QTest::mouseMove(&window, { 150, 16 });
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 150, 16 });
        QVERIFY(requests.isEmpty());
        control.setParentItem(nullptr);
    }

    void switchReleaseRetargeting_data() {
        QTest::addColumn<int>("change");
        for (int change = 0; change < 5; ++change)
            QTest::newRow(qPrintable(QString::number(change))) << change;
    }

    void switchReleaseRetargeting() {
        QFETCH(int, change);
        Switch control;
        control.setSize({ 52, 32 });
        auto* track = new QQuickItem(&control);
        track->setSize({ 52, 32 });
        control.setIndicator(track);
        InteractionSource replacement;
        const auto        send = [&](QEvent::Type type, int x) {
            QMouseEvent event(type,
                              QPointF(x, 16),
                              QPointF(x, 16),
                              type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                              type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                              Qt::NoModifier);
            QCoreApplication::sendEvent(&control, &event);
        };
        send(QEvent::MouseButtonPress, 10);
        send(QEvent::MouseMove, 42);
        QVERIFY(control.isDragging());
        bool changed = false;
        connect(&control, &Switch::positionChanged, this, [&] {
            if (std::exchange(changed, true)) return;
            switch (change) {
            case 0: control.setIndicator(nullptr); break;
            case 1: control.setInteractionSource(&replacement); break;
            case 2:
                control.setVisible(false);
                control.setVisible(true);
                break;
            case 3: control.cancelActivation(); break;
            case 4:
                send(QEvent::MouseButtonPress, 10);
                send(QEvent::MouseMove, 26);
                break;
            }
        });
        QSignalSpy requests(&control, &Switch::checkedChangeRequested);
        send(QEvent::MouseButtonRelease, 52);
        QVERIFY(changed);
        QVERIFY(requests.isEmpty());
        QVERIFY(! control.isChecked());
        QCOMPARE(control.isDragging(), change == 4);
        if (change == 4) {
            QCOMPARE(control.position(), .5);
            send(QEvent::MouseButtonRelease, 10);
            QVERIFY(! control.isDragging());
            QVERIFY(requests.isEmpty());
        }
        QCOMPARE(control.position(), 0);
    }

    void switchDragDeletion_data() {
        QTest::addColumn<int>("notification");
        for (int notification = 0; notification < 4; ++notification)
            QTest::newRow(qPrintable(QString::number(notification))) << notification;
    }

    void switchDragDeletion() {
        QFETCH(int, notification);
        auto control = std::make_unique<Switch>();
        control->setSize({ 52, 32 });
        auto* track = new QQuickItem(control.get());
        track->setSize({ 52, 32 });
        control->setIndicator(track);
        if (notification == 0)
            connect(control.get(), &Switch::draggingChanged, this, [&] {
                control.reset();
            });
        if (notification == 1)
            connect(control.get(), &Switch::positionChanged, this, [&] {
                control.reset();
            });
        if (notification == 2)
            connect(control.get(), &Switch::visualPositionChanged, this, [&] {
                control.reset();
            });
        if (notification == 3)
            connect(control.get(), &Switch::checkedChangeRequested, this, [&](bool) {
                control.reset();
            });
        QMouseEvent press(QEvent::MouseButtonPress,
                          { 10, 16 },
                          { 10, 16 },
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &press);
        QMouseEvent move(QEvent::MouseMove,
                         { 42, 16 },
                         { 42, 16 },
                         Qt::NoButton,
                         Qt::LeftButton,
                         Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &move);
        if (control) {
            QMouseEvent release(QEvent::MouseButtonRelease,
                                { 42, 16 },
                                { 42, 16 },
                                Qt::LeftButton,
                                Qt::NoButton,
                                Qt::NoModifier);
            QCoreApplication::sendEvent(control.get(), &release);
        }
        QVERIFY(! control);
    }

    void switchDeletedInNotification() {
        QFETCH(int, notification);
        auto        control = std::make_unique<Switch>();
        QStringList notifications;
        const auto  observe = [&](auto signal, int index, const QString& name) {
            connect(control.get(), signal, this, [&, index, name] {
                QVERIFY(control->isChecked());
                QCOMPARE(control->position(), 1);
                QCOMPARE(control->visualPosition(), 1);
                notifications.append(name);
                if (notification == index) control.reset();
            });
        };
        observe(&ToggleButton::checkedChanged, 0, "checked");
        observe(&Switch::positionChanged, 1, "position");
        observe(&Switch::visualPositionChanged, 2, "visual");
        control->setChecked(true);
        QVERIFY(! control);
        QCOMPARE(notifications,
                 QStringList({ "checked", "position", "visual" }).mid(0, notification + 1));
    }

    void toggleFABInput_data() {
        QTest::addColumn<int>("input");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("initial");
        for (int input = 0; input < 4; ++input)
            for (bool accept : { false, true })
                for (bool initial : { false, true })
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(input).arg(accept).arg(initial)))
                        << input << accept << initial;
    }

    void toggleFABInput() {
        QFETCH(int, input);
        QFETCH(bool, accept);
        QFETCH(bool, initial);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                width: 160; height: 100
                property bool value: false
                property bool accept: false
                property int requests: 0
                property bool requested: false
                MD.ToggleFAB {
                    objectName: "toggle"
                    animationsEnabled: false
                    checked: owner.value
                    onCheckedChangeRequested: value => {
                        owner.requests++
                        owner.requested = value
                        if (owner.accept) owner.value = value
                    }
                }
            }
        )",
                          QUrl("qrc:/tests/toggle-fab-owner.qml"));
        QQuickWindow             window;
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "accept", accept }, { "value", initial } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root    = qobject_cast<QQuickItem*>(object.get());
        auto* control = object->findChild<ToggleButton*>("toggle");
        QVERIFY(root && control);
        root->setParentItem(window.contentItem());
        window.resize(160, 100);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (input == 0)
            control->click();
        else if (input == 1)
            QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        else if (input == 2) {
            control->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(&window, Qt::Key_Space);
        } else {
            static auto* device = QTest::createTouchDevice();
            QTest::touchEvent(&window, device).press(0, { 20, 20 }, &window).commit();
            QTest::touchEvent(&window, device).release(0, { 20, 20 }, &window).commit();
        }
        flushInteractions();
        QCOMPARE(object->property("requests").toInt(), 1);
        QCOMPARE(object->property("requested").toBool(), ! initial);
        QCOMPARE(control->isChecked(), accept ? ! initial : initial);
        QCOMPARE(control->property("checkedProgress").toReal(), control->isChecked() ? 1. : 0.);
        object->setProperty("value", ! control->isChecked());
        QCOMPARE(control->isChecked(), accept ? initial : ! initial);
        QCOMPARE(control->property("checkedProgress").toReal(), control->isChecked() ? 1. : 0.);
        auto* state = control->findChild<CommonState*>();
        QVERIFY(state);
        QCOMPARE(state->interactionSource(), control->interactionSource());
    }

    void requestValidation() {
        CheckBox   box;
        QSignalSpy requests(&box, &CheckBox::checkStateChangeRequested);
        box.setCheckState(Qt::CheckState(-1));
        box.requestCheckState(Qt::CheckState(3));
        box.requestCheckState(Qt::Unchecked);
        QCOMPARE(box.checkState(), Qt::Unchecked);
        QVERIFY(requests.isEmpty());
        box.setCheckState(Qt::PartiallyChecked);
        box.click();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests.last().first().value<Qt::CheckState>(), Qt::Checked);
        QCOMPARE(box.checkState(), Qt::PartiallyChecked);
        box.setTristate(true);
        box.setCheckState(Qt::Checked);
        box.click();
        QCOMPARE(requests.last().first().value<Qt::CheckState>(), Qt::Unchecked);
        QCOMPARE(box.checkState(), Qt::Checked);
        Switch     toggle;
        QSignalSpy changes(&toggle, &Switch::checkedChangeRequested);
        toggle.requestChecked(false);
        toggle.setEnabled(false);
        toggle.requestChecked(true);
        QVERIFY(changes.isEmpty());
    }

    void deleteInRequest_data() {
        QTest::addColumn<int>("kind");
        for (int kind = 0; kind < 4; ++kind)
            QTest::newRow(qPrintable(QString::number(kind))) << kind;
    }

    void deleteInRequest() {
        QFETCH(int, kind);
        auto                    control = controlFor(kind);
        QPointer<ButtonControl> guard(control.get());
        observeRequests(control.get(), this, [&](int) {
            control.reset();
        });
        control->click();
        QVERIFY(! guard);
    }
};

int run_toggle_requests(int argc, char** argv) {
    ToggleRequestsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "toggle_requests.moc"
