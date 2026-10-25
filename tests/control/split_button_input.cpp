#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/control/action_command.hpp"
#include "qml_material/control/menu.hpp"
#include "qml_material/control/split_button.hpp"
#include "qml_material/style/split_button_indicator_state.hpp"
#include "qml_material/anim/spring_motion.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class SplitButtonInputTest : public QObject {
    Q_OBJECT
private:
    std::unique_ptr<QQmlEngine>   m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QPointer<QQuickItem>          m_root;
    QPointer<SplitButton>         m_split;
    QPointer<CommandButton>       m_tail;
    QPointer<Menu>                m_menu;
    QPointer<Action>              m_action;

    void create() {
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Control {
                width: 640; height: 480
                MD.Action { id: command; objectName: "command"; text: "Save"; checkable: true }
                MD.SplitButton {
                    objectName: "split"; x: 40; y: 40
                    action: command
                    expanded: firstMenu.visible
                }
                MD.Menu {
                    id: firstMenu; objectName: "first"
                    width: 180; enter: null; exit: null
                    MD.MenuItem { text: "First" }
                }
            }
        )",
                          QUrl());
        m_root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(m_root, qPrintable(component.errorString()));
        m_root->setParentItem(m_window->contentItem());
        m_split  = m_root->findChild<SplitButton*>();
        m_tail   = m_split->trailingControl();
        m_menu   = m_root->findChild<Menu*>("first");
        m_action = m_root->findChild<Action*>();
        QVERIFY(m_split && m_tail && m_menu && m_action);
        m_window->show();
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
    }
    SplitButtonIndicatorState* state() const {
        return m_tail->property("mdState").value<SplitButtonIndicatorState*>();
    }
    void flushPointerEvents() {
        QQuickWindowPrivate::get(m_window.get())
            ->deliveryAgentPrivate()
            ->flushFrameSynchronousEvents(m_window.get());
        flushInteractions();
    }
    void activate(CommandButton* button, int input) {
        const auto point =
            button->mapToScene({ button->width() / 2, button->height() / 2 }).toPoint();
        if (input == 0)
            button->click();
        else if (input == 1)
            QTest::mouseClick(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
        else if (input == 2) {
            button->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(m_window.get(), Qt::Key_Space);
        } else {
            static auto* device = QTest::createTouchDevice();
            QTest::touchEvent(m_window.get(), device).press(0, point, m_window.get()).commit();
            flushPointerEvents();
            QTest::touchEvent(m_window.get(), device).release(0, point, m_window.get()).commit();
            flushPointerEvents();
        }
        flushInteractions();
    }
    static void addRoles() {
        QTest::addColumn<bool>("trailing");
        QTest::newRow("leading") << false;
        QTest::newRow("trailing") << true;
    }
private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(".*"));
        m_engine = std::make_unique<QQmlEngine>();
        m_engine->addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window = std::make_unique<QQuickWindow>();
        m_window->resize(640, 480);
    }
    void cleanup() {
        delete m_root.data();
        flushInteractions();
        m_window.reset();
        m_engine.reset();
    }
    void shapeFeedback_data() {
        QTest::addColumn<bool>("rtl");
        QTest::newRow("ltr") << false;
        QTest::newRow("rtl") << true;
    }
    void shapeFeedback() {
        QFETCH(bool, rtl);
        create();
        m_split->setProperty("animationsEnabled", false);
        m_split->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        auto* leading = m_split->leadingControl();
        QCOMPARE(leading->x() > m_tail->x(), rtl);
        m_split->setProperty("animationsEnabled", true);
        for (auto* control : { leading, static_cast<CommandButton*>(m_tail.data()) }) {
            auto* values       = control->property("mdState").value<CommonState*>();
            auto* presentation = control->property("_motion").value<QObject*>();
            QVERIFY(values && presentation);
            QTRY_VERIFY(presentation->property("_animate").toBool());
            auto* shape = presentation->findChild<CornerSpringMotion*>();
            QVERIFY(shape);
            shape->runner().setManualClock(true);
            QCOMPARE(shape->spec(), (token::SpringSpec { 1600, 1 }));
            const auto normal = values->corners();
            QCOMPARE(normal.topLeft() > normal.topRight(), control == leading ? ! rtl : rtl);
            TestInteractionEmitter input(control->interactionSource(), control);
            input.set(Interaction::Hover, true);
            QCOMPARE(values->corners(), normal);
            input.set(Interaction::Press, true);
            QVERIFY(values->corners() != normal);
            QCOMPARE(shape->corners(), normal);
            shape->runner().setPlayTime(40);
            const auto middle = shape->corners();
            QVERIFY(middle != normal && middle != values->corners());
            input.clear();
            QCOMPARE(shape->corners(), middle);
            shape->complete();
            QCOMPARE(shape->corners(), normal);
            QVERIFY(! shape->isRunning());
        }
    }
    void commands_data() {
        QTest::addColumn<bool>("trailing");
        QTest::addColumn<int>("input");
        for (bool trailing : { false, true })
            for (int input = 0; input < 4; ++input)
                QTest::newRow(qPrintable(QString("%1-%2").arg(trailing).arg(input)))
                    << trailing << input;
    }
    void commands() {
        QFETCH(bool, trailing);
        QFETCH(int, input);
        create();
        auto* leading = m_split->leadingControl();
        QVERIFY(leading && state());
        QVERIFY(! qmlAttachedPropertiesObject<ActionCommand>(m_tail, false));
        for (const auto name : { "checked",
                                 "checkable",
                                 "pressed",
                                 "pressX",
                                 "pressY",
                                 "action",
                                 "popupMenu",
                                 "menuAnchor",
                                 "menuExpanded" })
            QVERIFY(! m_tail->property(name).isValid());
        QVERIFY(! m_split->property("menu").isValid());
        QSignalSpy clicks(m_split, &SplitButton::clicked), actions(m_action, &Action::triggered),
            trailingClicks(m_split, &SplitButton::trailingClicked);
        activate(trailing ? m_tail.data() : leading, input);
        QCOMPARE(clicks.size(), trailing ? 0 : 1);
        QCOMPARE(actions.size(), trailing ? 0 : 1);
        QCOMPARE(trailingClicks.size(), trailing ? 1 : 0);
        QCOMPARE(m_action->isChecked(), ! trailing);
        QVERIFY(! m_menu->isVisible());
        QVERIFY(! state()->isSelected());
    }
    void externalVisibility() {
        create();
        m_menu->openFor(m_split);
        QVERIFY(m_split->property("expanded").toBool() && state()->isSelected());
        QCOMPARE(state()->innerCorner(), state()->baseCorner());
        m_menu->setDeferredCompletion(true);
        m_menu->close();
        QVERIFY(m_menu->closing());
        QVERIFY(m_split->property("expanded").toBool() && state()->isSelected());
        m_menu->completeExit();
        QVERIFY(! m_split->property("expanded").toBool() && ! state()->isSelected());
        m_tail->click();
        QVERIFY(! m_menu->isVisible() && ! state()->isSelected());
    }
    void cancellation_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::addColumn<int>("change");
        for (bool keyboard : { false, true })
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(keyboard).arg(change)))
                    << keyboard << change;
    }
    void cancellation() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        create();
        const auto point =
            m_tail->mapToScene({ m_tail->width() / 2, m_tail->height() / 2 }).toPoint();
        QSignalSpy clicks(m_split, &SplitButton::trailingClicked);
        if (keyboard) {
            m_tail->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyPress(m_window.get(), Qt::Key_Space);
        } else
            QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
        InteractionSource replacement;
        if (change == 0)
            m_split->setEnabled(false);
        else if (change == 1)
            m_split->setVisible(false);
        else
            m_tail->setInteractionSource(&replacement);
        if (keyboard)
            QTest::keyRelease(m_window.get(), Qt::Key_Space);
        else
            QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
        flushInteractions();
        QVERIFY(clicks.isEmpty());
    }
    void stateLifecycle() {
        InteractionSource shared, replacement;
        auto*             button = new CommandButton;
        button->setSize({ 100, 40 });
        button->setBackground(new QQuickItem(button));
        button->setInteractionSource(&shared);
        SplitButtonIndicatorState state;
        state.setItem(button);
        state.setSelected(true);
        QCOMPARE(state.baseCorner(), 20);
        button->setHeight(64);
        QCOMPARE(state.baseCorner(), 32);
        QSignalSpy corners(&state, &SplitButtonIndicatorState::baseCornerChanged);
        button->setWidth(24);
        QCOMPARE(state.baseCorner(), 12);
        QVERIFY(! corners.isEmpty());
        button->setWidth(100);
        auto       emitter = shared.emitter(button);
        const auto press   = emitter->begin(Interaction::Press, Interaction::Mouse, -1, { 20, 20 });
        flushInteractions();
        QVERIFY(state.interactions().pressed);
        QCOMPARE(state.innerCorner(), state.sizeToken().inner_corner_pressed_size);
        button->setInteractionSource(&replacement);
        QVERIFY(! state.interactions().pressed);
        QCOMPARE(state.innerCorner(), state.baseCorner());
        delete button;
        QVERIFY(! state.item() && ! state.interactionSource());
        emitter->end(press);
        flushInteractions();
        QVERIFY(! state.interactions().pressed);
    }
    void roleDestruction_data() { addRoles(); }
    void roleDestruction() {
        QFETCH(bool, trailing);
        SplitButton owner;
        auto*       button = new CommandButton;
        if (trailing)
            owner.setTrailingControl(button);
        else
            owner.setLeadingControl(button);
        QSignalSpy roles(&owner,
                         trailing ? &SplitButton::trailingControlChanged
                                  : &SplitButton::leadingControlChanged);
        delete button;
        QVERIFY(! (trailing ? owner.trailingControl() : owner.leadingControl()));
        QCOMPARE(roles.size(), 1);
    }
    void stateRetargetDuringNotification() {
        CommandButton first, replacement;
        first.setSize({ 100, 40 });
        replacement.setSize({ 100, 64 });
        first.setBackground(new QQuickItem(&first));
        replacement.setBackground(new QQuickItem(&replacement));
        SplitButtonIndicatorState state;
        state.setSelected(true);
        state.setItem(&first);
        connect(&state, &SplitButtonIndicatorState::baseCornerChanged, this, [&] {
            if (state.item() == &first) state.setItem(&replacement);
        });
        first.setHeight(60);
        QCOMPARE(state.item(), &replacement);
        QCOMPARE(state.interactionSource(), replacement.interactionSource());
        QCOMPARE(state.baseCorner(), 32);
        first.setHeight(90);
        QCOMPARE(state.baseCorner(), 32);
    }
    void roleRetargeting_data() { addRoles(); }
    void roleRetargeting() {
        QFETCH(bool, trailing);
        SplitButton   owner;
        CommandButton first, second;
        const auto    assign = [&](CommandButton* button) {
            if (trailing)
                owner.setTrailingControl(button);
            else
                owner.setLeadingControl(button);
        };
        connect(&first, &CommandButton::clicked, this, [&] {
            assign(&second);
        });
        assign(&first);
        QSignalSpy clicks(&owner, trailing ? &SplitButton::trailingClicked : &SplitButton::clicked);
        first.click();
        QVERIFY(clicks.isEmpty());
        second.click();
        QCOMPARE(clicks.size(), 1);
        owner.setEnabled(false);
        second.click();
        QCOMPARE(clicks.size(), 1);
    }
    void deleteInLeadingAction() {
        create();
        QVERIFY(m_split);
        int calls = 0;
        connect(m_action, &Action::triggered, this, [&] {
            ++calls;
            delete m_root.data();
        });
        m_split->leadingControl()->click();
        QCOMPARE(calls, 1);
        QVERIFY(! m_root && ! m_split && ! m_tail);
    }
    void deleteInTrailingCommand() {
        create();
        int calls = 0;
        connect(m_split, &SplitButton::trailingClicked, this, [&] {
            ++calls;
            delete m_root.data();
        });
        m_tail->click();
        QCOMPARE(calls, 1);
        QVERIFY(! m_root && ! m_split && ! m_tail);
    }
    void deletedInRoleNotification_data() { addRoles(); }
    void deletedInRoleNotification() {
        QFETCH(bool, trailing);
        QPointer<SplitButton> owner = new SplitButton;
        CommandButton         button;
        connect(owner,
                trailing ? &SplitButton::trailingControlChanged
                         : &SplitButton::leadingControlChanged,
                this,
                [&] {
                    delete owner.data();
                });
        if (trailing)
            owner->setTrailingControl(&button);
        else
            owner->setLeadingControl(&button);
        QVERIFY(! owner);
    }
};

int run_split_button_input(int argc, char** argv) {
    SplitButtonInputTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "split_button_input.moc"
