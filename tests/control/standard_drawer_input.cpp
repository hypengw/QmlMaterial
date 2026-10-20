#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/control/action_command.hpp"
#include "qml_material/control/standard_drawer.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class StandardDrawerInputTest : public QObject {
    Q_OBJECT
    std::unique_ptr<QQmlEngine>   m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QPointer<StandardDrawer>      m_control;

    void create(bool business = false) {
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.StandardDrawer {
                id: navigation
                width: 96; height: 400
                property bool useBusiness: false
                property real windowWidth: 840
                property MD.Action businessAction: MD.Action {
                    text: "Settings"
                    icon.name: MD.Token.icon.settings
                }
                MD.MProp.size: MD.ThemeSize { width: navigation.windowWidth; duration: 0 }
                headerAction: useBusiness ? businessAction : defaultHeaderAction
                model: [{name: "One", icon: "home"}, {name: "Two", icon: "settings"}]
            }
        )",
                          QUrl("qrc:/tests/standard-drawer-input.qml"));
        m_control = qobject_cast<StandardDrawer*>(
            component.createWithInitialProperties({ { "useBusiness", business } }));
        QVERIFY2(m_control, qPrintable(component.errorString()));
        m_control->setParentItem(m_window->contentItem());
        m_window->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
        QTRY_VERIFY(m_control->headerControl() && m_control->headerControl()->isVisible());
        QVERIFY(m_control->drawer());
        QVERIFY(! m_control->useEmbed());
    }
    Action* businessAction() const {
        return m_control->property("businessAction").value<Action*>();
    }
    void flushPointerEvents() {
        QQuickWindowPrivate::get(m_window.get())
            ->deliveryAgentPrivate()
            ->flushFrameSynchronousEvents(m_window.get());
        flushInteractions();
    }
    QPoint center(QQuickItem* item) const {
        return item->mapToScene({ item->width() / 2, item->height() / 2 }).toPoint();
    }
    void activate(CommandButton* button, int input) {
        const auto position = center(button);
        if (input == 0)
            button->click();
        else if (input == 1)
            QTest::mouseClick(m_window.get(), Qt::LeftButton, Qt::NoModifier, position);
        else if (input == 2) {
            button->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(m_window.get(), Qt::Key_Space);
        } else {
            static auto* device = QTest::createTouchDevice();
            QTest::touchEvent(m_window.get(), device).press(0, position, m_window.get()).commit();
            flushPointerEvents();
            QTest::touchEvent(m_window.get(), device).release(0, position, m_window.get()).commit();
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
        m_window->resize(840, 500);
    }
    void cleanup() {
        delete m_control.data();
        flushInteractions();
        m_window.reset();
        m_engine.reset();
    }
    void commands_data() {
        QTest::addColumn<int>("input");
        QTest::addColumn<bool>("business");
        for (int input = 0; input < 4; ++input)
            for (bool business : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(input).arg(business)))
                    << input << business;
    }
    void commands() {
        QFETCH(int, input);
        QFETCH(bool, business);
        create(business);
        QVERIFY(m_control && m_control->headerControl());
        auto* button = m_control->headerControl();
        auto* action = business ? businessAction() : m_control->defaultHeaderAction();
        auto* command =
            qobject_cast<ActionCommand*>(qmlAttachedPropertiesObject<ActionCommand>(button, false));
        QVERIFY(command && command->action() == action);
        QVERIFY(! qobject_cast<SelectionButton*>(button));
        for (const auto* name :
             { "action", "selected", "checked", "checkable", "pressed", "pressX", "pressY" })
            QVERIFY(! button->property(name).isValid());
        QSignalSpy calls(action, &Action::triggered);
        QSignalSpy selections(m_control, &NavigationControl::currentIndexChangeRequested);
        activate(button, input);
        QCOMPARE(calls.size(), 1);
        QCOMPARE(calls.first().first().value<QObject*>(), button);
        QVERIFY(selections.isEmpty());
        QCOMPARE(m_control->currentIndex(), 0);
        QCOMPARE(m_control->drawer()->isVisible(), ! business);
        if (! business) QTRY_VERIFY(m_control->property("drawerOpened").toBool());
    }
    void headerGeometry() {
        create(true);
        QVERIFY(m_control && m_control->headerControl());
        auto* button     = m_control->headerControl();
        auto* background = button->background();
        QVERIFY(background);
        QCOMPARE(button->width(), 96.0);
        QCOMPARE(button->implicitWidth(), 96.0);
        QCOMPARE(button->height(), 48.0);
        QCOMPARE(background->width(), 40.0);
        QCOMPARE(background->height(), 40.0);
        QCOMPARE(background->x() + background->width() / 2, button->width() / 2);
        QCOMPARE(button->icon()->width(), 24);
        QCOMPARE(button->icon()->height(), 24);
        QCOMPARE(button->text(), QString("Settings"));
        button->setWidth(120);
        QCOMPARE(background->width(), 40.0);
        QCOMPARE(background->x() + background->width() / 2, button->width() / 2);
        QCOMPARE(button->implicitWidth(), 96.0);
    }
    void embeddedCommands() {
        create();
        QVERIFY(m_control);
        m_control->setProperty("windowWidth", 1280);
        QVERIFY(m_control->useEmbed() && m_control->expanded());
        m_control->open();
        m_control->open();
        QVERIFY(m_control->expanded());
        m_control->close();
        m_control->close();
        QVERIFY(! m_control->expanded());
        QVERIFY(m_control->defaultHeaderAction()->trigger());
        QVERIFY(m_control->expanded());
        QVERIFY(m_control->defaultHeaderAction()->trigger());
        QVERIFY(! m_control->expanded());
        QVERIFY(! m_control->drawer()->isVisible());
        m_control->setProperty("windowWidth", 840);
        QVERIFY(! m_control->useEmbed() && ! m_control->expanded());
        QTRY_VERIFY(m_control->headerControl() && m_control->headerControl()->isVisible());
    }
    void modalCommands() {
        create();
        QVERIFY(m_control);
        auto*      drawer = m_control->drawer();
        QSignalSpy shows(drawer, &Popup::aboutToShow);
        QSignalSpy hides(drawer, &Popup::aboutToHide);
        m_control->open();
        m_control->open();
        QVERIFY(drawer->isVisible());
        QCOMPARE(shows.size(), 1);
        m_control->toggle();
        QVERIFY(drawer->closing() || ! drawer->isVisible());
        QCOMPARE(hides.size(), 1);
        m_control->toggle();
        QVERIFY(drawer->isVisible() && ! drawer->closing());
        QCOMPARE(shows.size(), 2);
        m_control->close();
        m_control->close();
        QCOMPARE(hides.size(), 2);
        QTRY_VERIFY(! drawer->isVisible());
    }
    void modeNotificationReentry_data() {
        QTest::addColumn<bool>("overrideExpanded");
        QTest::newRow("replace-header") << false;
        QTest::newRow("override-expanded") << true;
    }
    void modeNotificationReentry() {
        QFETCH(bool, overrideExpanded);
        create();
        QVERIFY(m_control);
        QSignalSpy changed(m_control, &StandardDrawer::expandedChanged);
        connect(m_control, &StandardDrawer::useEmbedChanged, m_window.get(), [&] {
            if (overrideExpanded)
                m_control->setExpanded(false);
            else
                m_control->setHeaderControl(nullptr);
        });
        m_control->setUseEmbed(true);
        QCOMPARE(m_control->expanded(), ! overrideExpanded);
        QCOMPARE(m_control->property("useLarge").toBool(), ! overrideExpanded);
        QCOMPARE(changed.size(), 1);
    }
    void heldCommandCancelled_data() {
        QTest::addColumn<int>("change");
        QTest::addColumn<bool>("keyboard");
        for (int change = 0; change < 9; ++change)
            for (bool keyboard : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(change).arg(keyboard)))
                    << change << keyboard;
    }
    void heldCommandCancelled() {
        QFETCH(int, change);
        QFETCH(bool, keyboard);
        create(true);
        QVERIFY(m_control && m_control->headerControl());
        QPointer<CommandButton> button(m_control->headerControl());
        auto*                   action = businessAction();
        QSignalSpy              calls(action, &Action::triggered);
        InteractionAccumulator  feedback(this);
        feedback.setSource(button->interactionSource());
        button->forceActiveFocus(Qt::TabFocusReason);
        const auto position = center(button);
        if (keyboard)
            QTest::keyPress(m_window.get(), Qt::Key_Space);
        else
            QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, position);
        flushPointerEvents();
        QVERIFY(feedback.values().pressed);
        if (change == 0) {
            m_control->setEnabled(false);
            m_control->setEnabled(true);
        }
        if (change == 1) {
            m_control->setVisible(false);
            m_control->setVisible(true);
        }
        if (change == 2) {
            action->setEnabled(false);
            action->setEnabled(true);
        }
        if (change == 3) {
            m_control->setProperty("useBusiness", false);
            m_control->setProperty("useBusiness", true);
        }
        if (change == 4) {
            auto* drawer = m_control->drawer();
            m_control->setDrawer(nullptr);
            m_control->setDrawer(drawer);
        }
        if (change == 5) {
            m_control->setHeaderControl(nullptr);
            m_control->setHeaderControl(button);
        }
        if (change == 6)
            m_control->setProperty("header", QVariant::fromValue<QQmlComponent*>(nullptr));
        if (change == 7) m_control->setProperty("windowWidth", 1280);
        if (change == 8) delete action;
        flushInteractions();
        QVERIFY(! feedback.values().pressed);
        if (keyboard)
            QTest::keyRelease(m_window.get(), Qt::Key_Space);
        else
            QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, position);
        flushPointerEvents();
        QVERIFY(calls.isEmpty());
        QVERIFY(! m_control->drawer()->isVisible());
        if (change < 6) {
            QVERIFY(button);
            button->click();
            QCOMPARE(calls.size(), 1);
        }
    }
    void sharedBusinessAction() {
        create(true);
        QVERIFY(m_control);
        auto* action = businessAction();
        action->setCheckable(true);
        QVERIFY(action->trigger());
        QVERIFY(action->isChecked());
        QVERIFY(! m_control->drawer()->isVisible());
        m_control->headerControl()->click();
        QVERIFY(! action->isChecked());
        QCOMPARE(m_control->currentIndex(), 0);
        m_control->setProperty("headerAction", QVariant::fromValue<Action*>(nullptr));
        m_control->headerControl()->click();
        QVERIFY(m_control->drawer()->isVisible());
    }
    void drawerDestroyed() {
        create();
        QVERIFY(m_control);
        delete m_control->drawer();
        QVERIFY(! m_control->drawer());
        m_control->open();
        m_control->close();
        m_control->toggle();
        QVERIFY(m_control->defaultHeaderAction()->trigger());
        QVERIFY(! m_control->property("drawerOpened").toBool());
    }
    void headerRetiredInTrigger_data() {
        QTest::addColumn<bool>("immediate");
        QTest::newRow("loader-retirement") << false;
        QTest::newRow("synchronous-deletion-after-retirement") << true;
    }
    void headerRetiredInTrigger() {
        QFETCH(bool, immediate);
        create(true);
        QVERIFY(m_control && m_control->headerControl());
        QPointer<CommandButton> button(m_control->headerControl());
        auto*                   action = businessAction();
        int                     calls  = 0;
        connect(action, &Action::triggered, m_window.get(), [&] {
            ++calls;
            m_control->setProperty("header", QVariant::fromValue<QQmlComponent*>(nullptr));
            if (immediate) delete button.data();
        });
        button->click();
        QCOMPARE(calls, 1);
        QVERIFY(m_control && ! m_control->headerControl());
        if (immediate)
            QVERIFY(! button);
        else {
            QVERIFY(button && ! button->isVisible());
            button->click();
            QCOMPARE(calls, 1);
            QTRY_VERIFY(! button);
        }
        QVERIFY(! m_control->drawer()->isVisible());
    }
    void deletedInCallback_data() {
        QTest::addColumn<int>("notification");
        for (int notification = 0; notification < 5; ++notification)
            QTest::newRow(qPrintable(QString::number(notification))) << notification;
    }
    void deletedInCallback() {
        QFETCH(int, notification);
        create(notification == 0);
        QVERIFY(m_control);
        int        calls  = 0;
        const auto remove = [&] {
            ++calls;
            delete m_control.data();
        };
        if (notification == 0) {
            connect(businessAction(), &Action::triggered, m_window.get(), remove);
            m_control->headerControl()->click();
        } else if (notification == 1) {
            m_control->setProperty("windowWidth", 1280);
            m_control->close();
            connect(m_control, &StandardDrawer::expandedChanged, m_window.get(), remove);
            m_control->defaultHeaderAction()->trigger();
        } else if (notification == 2) {
            connect(m_control, &StandardDrawer::useEmbedChanged, m_window.get(), remove);
            m_control->setUseEmbed(true);
        } else if (notification == 3) {
            connect(m_control->drawer(), &Popup::aboutToShow, m_window.get(), remove);
            m_control->headerControl()->click();
        } else {
            m_control->open();
            connect(m_control->drawer(), &Popup::aboutToHide, m_window.get(), remove);
            m_control->close();
        }
        QCOMPARE(calls, 1);
        QVERIFY(! m_control);
    }
};

int run_standard_drawer_input(int argc, char** argv) {
    StandardDrawerInputTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "standard_drawer_input.moc"
