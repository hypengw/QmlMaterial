#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/control/action_command.hpp"
#include "qml_material/control/menu_button.hpp"
#include "qml_material/control/split_button.hpp"
#include "qml_material/style/split_button_indicator_state.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class SplitButtonInputTest : public QObject {
    Q_OBJECT
private:
    std::unique_ptr<QQmlEngine>   m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QPointer<QQuickItem>          m_root;
    QPointer<SplitButton>         m_split;
    QPointer<MenuButton>          m_tail;
    QPointer<Menu>                m_menu, m_second;
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
                    menu: firstMenu
                }
                MD.Menu {
                    id: firstMenu; objectName: "first"
                    width: 180; enter: null; exit: null
                    MD.MenuItem { text: "First" }
                }
                MD.Menu {
                    objectName: "second"
                    width: 180; enter: null; exit: null
                    MD.MenuItem { text: "Second" }
                }
            }
        )",
                          QUrl());
        m_root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(m_root, qPrintable(component.errorString()));
        m_root->setParentItem(m_window->contentItem());
        m_split  = m_root->findChild<SplitButton*>();
        m_tail   = m_root->findChild<MenuButton*>();
        m_menu   = m_root->findChild<Menu*>("first");
        m_second = m_root->findChild<Menu*>("second");
        m_action = m_root->findChild<Action*>();
        QVERIFY(m_split && m_tail && m_menu && m_second && m_action);
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
        QVERIFY(m_tail);
        auto* leading = m_split->leadingControl();
        QVERIFY(leading && state());
        QCOMPARE((m_tail)->metaObject()->indexOfProperty("checkable"), -1);
        QVERIFY(! qmlAttachedPropertiesObject<ActionCommand>(m_tail, false));
        for (const auto name : { "checked", "checkable", "pressed", "pressX", "pressY", "action" })
            QVERIFY(! m_tail->property(name).isValid());
        QSignalSpy clicks(m_split, &SplitButton::clicked), actions(m_action, &Action::triggered),
            tailClicks(m_tail, &CommandButton::clicked);
        activate(trailing ? m_tail.data() : leading, input);
        QCOMPARE(clicks.size(), trailing ? 0 : 1);
        QCOMPARE(actions.size(), trailing ? 0 : 1);
        QCOMPARE(tailClicks.size(), trailing ? 1 : 0);
        QCOMPARE(m_action->isChecked(), ! trailing);
        QCOMPARE(m_tail->isExpanded(), trailing);
        QCOMPARE(state()->isSelected(), trailing);
        if (trailing) {
            QVERIFY(m_menu->isOpened());
            QCOMPARE(m_menu->anchorItem(), m_split);
            QCOMPARE(m_menu->parentItem(), m_root);
        }
    }
    void externalVisibility() {
        create();
        QVERIFY(m_tail && state());
        m_menu->openFor(m_split);
        QVERIFY(m_tail->isExpanded() && state()->isSelected());
        QCOMPARE(state()->innerCorner(), state()->baseCorner());
        m_menu->setDeferredCompletion(true);
        m_menu->close();
        QVERIFY(m_menu->closing());
        QVERIFY(m_tail->isExpanded() && state()->isSelected());
        m_menu->completeExit();
        QVERIFY(! m_tail->isExpanded() && ! state()->isSelected());
        m_tail->click();
        m_menu->completeEnter();
        QVERIFY(m_tail->isExpanded());
        m_tail->click();
        QVERIFY(m_menu->closing());
        m_menu->completeExit();
        QVERIFY(! m_tail->isExpanded());
    }
    void standaloneAnchor() {
        create();
        QVERIFY(m_tail);
        m_tail->setMenuAnchor(nullptr);
        QCOMPARE(m_tail->menuAnchor(), m_tail);
        m_tail->click();
        QCOMPARE(m_menu->anchorItem(), m_tail);
        QVERIFY(m_menu->isOpened());
    }
    void placement_data() {
        QTest::addColumn<bool>("rtl");
        QTest::addColumn<bool>("edge");
        for (bool rtl : { false, true })
            for (bool edge : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(rtl).arg(edge))) << rtl << edge;
    }
    void placement() {
        QFETCH(bool, rtl);
        QFETCH(bool, edge);
        create();
        QVERIFY(m_split);
        qobject_cast<Control*>(m_root)->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        if (edge)
            m_split->setPosition({ 640 - m_split->width() - 12, 480 - m_split->height() - 12 });
        else
            m_split->setX(240);
        m_tail->click();
        QVERIFY(m_menu->isOpened());
        QCOMPARE(m_menu->anchorItem(), m_split);
        QCOMPARE(m_menu->mirrored(), rtl);
        const auto rect   = m_menu->presentationRect();
        const auto anchor = m_split->mapRectToScene(m_split->boundingRect());
        QVERIFY(QRectF(8, 8, 624, 464).contains(rect));
        if (edge)
            QVERIFY(rect.bottom() <= anchor.top());
        else if (rtl)
            QCOMPARE(rect.right(), anchor.right());
        else
            QCOMPARE(rect.left(), anchor.left());
    }
    void pointerClose_data() {
        QTest::addColumn<bool>("touch");
        QTest::addColumn<bool>("deferred");
        for (bool touch : { false, true })
            for (bool deferred : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(touch).arg(deferred)))
                    << touch << deferred;
    }
    void pointerClose() {
        QFETCH(bool, touch);
        QFETCH(bool, deferred);
        create();
        QVERIFY(m_tail);
        m_menu->setDeferredCompletion(deferred);
        QSignalSpy shown(m_menu, &Popup::aboutToShow), clicks(m_tail, &CommandButton::clicked);
        activate(m_tail, touch ? 3 : 1);
        if (deferred) m_menu->completeEnter();
        QVERIFY(m_menu->isOpened());
        activate(m_tail, touch ? 3 : 1);
        QCOMPARE(shown.size(), 1);
        QCOMPARE(clicks.size(), 1);
        if (deferred) {
            QVERIFY(m_menu->closing());
            activate(m_tail, touch ? 3 : 1);
            QCOMPARE(shown.size(), 1);
            m_menu->completeExit();
        }
        QVERIFY(! m_menu->isVisible() && ! m_tail->isExpanded());
    }
    void cancellation_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::addColumn<int>("change");
        for (bool keyboard : { false, true })
            for (int change = 0; change < 7; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(keyboard).arg(change)))
                    << keyboard << change;
    }
    void cancellation() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        create();
        QVERIFY(m_tail);
        auto* anchor = new QQuickItem(m_root);
        anchor->setSize({ 100, 40 });
        m_tail->setMenuAnchor(anchor);
        const auto point =
            m_tail->mapToScene({ m_tail->width() / 2, m_tail->height() / 2 }).toPoint();
        QSignalSpy clicks(m_tail, &CommandButton::clicked);
        if (keyboard) {
            m_tail->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyPress(m_window.get(), Qt::Key_Space);
        } else
            QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
        InteractionSource replacement;
        if (change == 0)
            m_tail->setEnabled(false);
        else if (change == 1)
            m_tail->setVisible(false);
        else if (change == 2)
            m_tail->setInteractionSource(&replacement);
        else if (change == 3)
            m_tail->setMenu(m_second);
        else if (change == 4)
            delete m_menu.data();
        else if (change == 5)
            m_tail->setMenuAnchor(m_root);
        else
            delete anchor;
        if (keyboard)
            QTest::keyRelease(m_window.get(), Qt::Key_Space);
        else
            QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
        flushInteractions();
        QVERIFY(clicks.isEmpty());
        QVERIFY(! m_second->isVisible());
        if (m_menu) QVERIFY(! m_menu->isVisible());
    }
    void redirectInClick_data() {
        QTest::addColumn<int>("change");
        for (int change = 0; change < 5; ++change)
            QTest::newRow(qPrintable(QString::number(change))) << change;
    }
    void redirectInClick() {
        QFETCH(int, change);
        create();
        QVERIFY(m_tail);
        connect(m_tail, &CommandButton::clicked, this, [&] {
            if (change == 0)
                m_tail->setMenu(m_second);
            else if (change == 1)
                m_tail->setMenuAnchor(m_root);
            else if (change == 2)
                delete m_menu.data();
            else if (change == 3)
                m_tail->setEnabled(false);
            else
                delete m_root.data();
        });
        m_tail->click();
        if (! m_root) return;
        if (m_menu) QVERIFY(! m_menu->isVisible());
        QVERIFY(! m_second->isVisible());
    }
    void menuRetargeting() {
        create();
        QVERIFY(m_tail);
        m_tail->setMenu(nullptr);
        bool redirected = false;
        connect(m_tail, &MenuButton::menuChanged, this, [&] {
            if (redirected) return;
            redirected = true;
            m_tail->setMenu(m_second);
        });
        m_tail->setMenu(m_menu);
        QCOMPARE(m_tail->menu(), m_second);
        QSignalSpy expanded(m_tail, &MenuButton::expandedChanged);
        m_menu->openFor(m_root);
        QVERIFY(expanded.isEmpty() && ! m_tail->isExpanded());
        m_menu->dismissImmediately();
        m_tail->click();
        QVERIFY(m_second->isOpened() && m_tail->isExpanded());
        delete m_second.data();
        QVERIFY(! m_tail->menu() && ! m_tail->isExpanded());
    }
    void stateLifecycle() {
        InteractionSource shared, replacement;
        auto*             button = new MenuButton;
        button->setSize({ 100, 40 });
        button->setBackground(new QQuickItem(button));
        button->setInteractionSource(&shared);
        SplitButtonIndicatorState state;
        state.setItem(button);
        state.setSelected(true);
        QCOMPARE(state.baseCorner(), 20);
        button->setHeight(64);
        QCOMPARE(state.baseCorner(), 32);
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
    void leadingRoleDestruction() {
        SplitButton owner;
        auto*       leading = new CommandButton;
        owner.setLeadingControl(leading);
        QSignalSpy roles(&owner, &SplitButton::leadingControlChanged);
        delete leading;
        QVERIFY(! owner.leadingControl());
        QCOMPARE(roles.size(), 1);
    }
    void stateRetargetDuringNotification() {
        MenuButton first, replacement;
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
    void leadingRetargeting() {
        SplitButton   owner;
        CommandButton first, second;
        connect(&first, &CommandButton::clicked, this, [&] {
            owner.setLeadingControl(&second);
        });
        owner.setLeadingControl(&first);
        QSignalSpy clicks(&owner, &SplitButton::clicked);
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
    void deletedInNotification_data() {
        QTest::addColumn<int>("notification");
        for (int notification = 0; notification < 5; ++notification)
            QTest::newRow(qPrintable(QString::number(notification))) << notification;
    }
    void deletedInNotification() {
        QFETCH(int, notification);
        create();
        QVERIFY(m_tail);
        int        calls   = 0;
        const auto destroy = [&] {
            ++calls;
            delete m_root.data();
        };
        if (notification == 0) {
            connect(m_menu, &Popup::aboutToShow, this, destroy, Qt::SingleShotConnection);
            m_tail->click();
        } else if (notification == 1) {
            connect(m_tail, &MenuButton::expandedChanged, this, destroy, Qt::SingleShotConnection);
            m_tail->click();
        } else if (notification == 2) {
            connect(m_tail, &MenuButton::menuChanged, this, destroy, Qt::SingleShotConnection);
            m_tail->setMenu(m_second);
        } else if (notification == 3) {
            connect(
                m_tail, &MenuButton::menuAnchorChanged, this, destroy, Qt::SingleShotConnection);
            m_tail->setMenuAnchor(m_root);
        } else {
            connect(m_split, &SplitButton::clicked, this, destroy, Qt::SingleShotConnection);
            m_split->leadingControl()->click();
        }
        QCOMPARE(calls, 1);
        QVERIFY(! m_root && ! m_split && ! m_tail);
    }
    void nullMenu() {
        create();
        QVERIFY(m_tail);
        m_tail->setMenu(nullptr);
        QSignalSpy clicks(m_tail, &CommandButton::clicked);
        m_tail->click();
        QCOMPARE(clicks.size(), 1);
        QVERIFY(! m_tail->isExpanded() && ! state()->isSelected());
        QVERIFY(! qmlAttachedPropertiesObject<ActionCommand>(m_tail, false));
    }
};

int run_split_button_input(int argc, char** argv) {
    SplitButtonInputTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "split_button_input.moc"
