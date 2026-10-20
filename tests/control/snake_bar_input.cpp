#include <QtTest>
#include <QAbstractItemModelTester>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/control/snake_bar.hpp"
#include "qml_material/control/action_command.hpp"
#include "qml_material/model/snake_model.hpp"
#include "qml_material/style/snake_bar_state.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class SnakeBarInputTest : public QObject {
    Q_OBJECT
private:
    std::unique_ptr<QQmlEngine>   m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QPointer<QQuickItem>          m_root;
    QPointer<SnakeBar>            m_bar;
    QPointer<Action>              m_action;
    QPointer<SnakeModel>          m_model;

    SnakeBar* activeBar(QQuickItem* item) const {
        if (auto* bar = qobject_cast<SnakeBar*>(item); bar && bar->isAvailable()) return bar;
        for (auto* child : item->childItems())
            if (auto* bar = activeBar(child)) return bar;
        return nullptr;
    }

    void create() {
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Control {
                width: 640; height: 420
                MD.Action { id: command; text: "Undo"; checkable: true }
                MD.SnakeModel { objectName: "model" }
                MD.SnakeBar {
                    x: 24; y: 40; width: 420
                    text: "Changes saved"; action: command; showClose: true
                }
            }
        )",
                          QUrl());
        m_root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(m_root, qPrintable(component.errorString()));
        m_bar    = m_root->findChild<SnakeBar*>();
        m_action = m_root->findChild<Action*>();
        m_model  = m_root->findChild<SnakeModel*>();
        QVERIFY(m_bar && m_action && m_model);
        m_root->setParentItem(m_window->contentItem());
        m_window->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
        m_root->ensurePolished();
        m_bar->ensurePolished();
        m_bar->contentItem()->ensurePolished();
        for (auto item : m_bar->contentItem()->childItems()) item->ensurePolished();
    }
    void bind() {
        m_model->show("Changes saved", -1, 1, m_action);
        m_bar->setSnakeId(m_model->currentId());
        m_bar->setPresentationId(m_model->presentationId(m_model->currentId()));
        m_bar->setSnakeModel(m_model);
    }
    void flushPointerEvents() {
        QQuickWindowPrivate::get(m_window.get())
            ->deliveryAgentPrivate()
            ->flushFrameSynchronousEvents(m_window.get());
        flushInteractions();
    }
    void input(CommandButton* button, int mode, bool press) {
        const auto point =
            button->mapToScene({ button->width() / 2, button->height() / 2 }).toPoint();
        if (mode == 0) {
            if (! press) button->click();
        } else if (mode == 1) {
            if (press)
                QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
            else
                QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
        } else if (mode == 2) {
            if (press) {
                button->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyPress(m_window.get(), Qt::Key_Space);
            } else
                QTest::keyRelease(m_window.get(), Qt::Key_Space);
        } else {
            static auto* device = QTest::createTouchDevice();
            if (press)
                QTest::touchEvent(m_window.get(), device).press(0, point, m_window.get()).commit();
            else
                QTest::touchEvent(m_window.get(), device)
                    .release(0, point, m_window.get())
                    .commit();
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
        m_window->resize(640, 420);
    }
    void cleanup() {
        delete m_root.data();
        flushInteractions();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        m_window.reset();
        m_engine.reset();
    }
    void commands_data() {
        QTest::addColumn<bool>("close");
        QTest::addColumn<bool>("associated");
        QTest::addColumn<int>("mode");
        for (bool close : { false, true })
            for (bool associated : { false, true })
                for (int mode = 0; mode < 4; ++mode)
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(close).arg(associated).arg(mode)))
                        << close << associated << mode;
    }
    void commands() {
        QFETCH(bool, close);
        QFETCH(bool, associated);
        QFETCH(int, mode);
        create();
        QVERIFY(m_bar);
        if (associated) bind();
        auto* button = close ? m_bar->iconControl() : m_bar->actionControl();
        QVERIFY(button);
        QCOMPARE((button)->metaObject()->indexOfProperty("checkable"), -1);
        QVERIFY(! qmlAttachedPropertiesObject<ActionCommand>(button, false));
        for (const auto name : { "checked", "checkable", "pressed", "pressX", "pressY", "action" })
            QVERIFY(! button->property(name).isValid());
        QSignalSpy actions(m_action, &Action::triggered),
            dismissed(m_bar, &SnakeBar::dismissRequested);
        input(button, mode, true);
        QCOMPARE(actions.size(), 0);
        input(button, mode, false);
        QCOMPARE(actions.size(), close ? 0 : 1);
        QCOMPARE(dismissed.size(), close ? 1 : 0);
        QCOMPARE(m_action->isChecked(), ! close);
        if (! close) QCOMPARE(actions.at(0).at(0).value<QObject*>(), button);
        if (associated) QCOMPARE(m_model->rowCount(), 0);
    }
    void canceledInput_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<int>("change");
        for (int mode : { 1, 2, 3 })
            for (int change = 0; change < 7; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(mode).arg(change))) << mode << change;
    }
    void canceledInput() {
        QFETCH(int, mode);
        QFETCH(int, change);
        create();
        QVERIFY(m_bar);
        bind();
        auto*      button = m_bar->actionControl();
        QSignalSpy actions(m_action, &Action::triggered);
        input(button, mode, true);
        Action            replacement;
        InteractionSource source;
        if (change == 0)
            m_action->setEnabled(false);
        else if (change == 1)
            m_bar->setVisible(false);
        else if (change == 2)
            button->setInteractionSource(&source);
        else if (change == 3)
            m_bar->setAction(&replacement);
        else if (change == 4)
            m_model->show("New", -1, 1, m_action);
        else if (change == 5)
            m_bar->setSnakeId(-1);
        else
            m_bar->setSnakeModel(nullptr);
        input(button, mode, false);
        QCOMPARE(actions.size(), 0);
        QCOMPARE(m_model->rowCount(), 1);
    }
    void externalActionAndRetiredButtons() {
        create();
        QVERIFY(m_bar);
        bind();
        m_action->trigger();
        QCOMPARE(m_model->rowCount(), 1);
        m_model->show("Other", -1, 1, m_action);
        const auto current = m_model->currentId();
        QVERIFY(! m_bar->isAvailable());
        m_bar->actionControl()->click();
        m_bar->iconControl()->click();
        QCOMPARE(m_model->currentId(), current);
        delete m_model.data();
        QVERIFY(! m_bar->isAvailable());
        QSignalSpy actions(m_action, &Action::triggered);
        m_bar->actionControl()->click();
        QCOMPARE(actions.size(), 0);
    }
    void restoredItemDoesNotReviveOldPresentation() {
        create();
        QVERIFY(m_bar);
        m_model->show("Saved", 0, 1, m_action);
        const auto id = m_model->currentId();
        m_bar->setSnakeId(id);
        m_bar->setPresentationId(m_model->presentationId(id));
        m_bar->setSnakeModel(m_model);
        const auto old = m_bar->presentationId();
        QVERIFY(m_bar->isAvailable());
        m_model->show("Temporary", -1, 1);
        m_model->dismiss(m_model->currentId());
        flushInteractions();
        QCOMPARE(m_model->currentId(), id);
        QVERIFY(m_model->presentationId(id) != old);
        QVERIFY(! m_bar->isAvailable());
        QVERIFY(! m_model->performAction(id, old, m_action, nullptr));
        m_model->dismissPresentation(id, old);
        QCOMPARE(m_model->rowCount(), 1);
        m_bar->setPresentationId(m_model->presentationId(id));
        QVERIFY(m_bar->isAvailable());
        m_bar->actionControl()->click();
        QCOMPARE(m_model->rowCount(), 0);
    }
    void roleReplacement() {
        create();
        QVERIFY(m_bar);
        auto*         old = m_bar->actionControl();
        CommandButton replacement(m_root);
        replacement.setSize({ 80, 40 });
        m_bar->setActionControl(&replacement);
        QSignalSpy actions(m_action, &Action::triggered);
        old->click();
        QCOMPARE(actions.size(), 0);
        replacement.click();
        QCOMPARE(actions.size(), 1);
        m_bar->setActionControl(nullptr);
        replacement.click();
        QCOMPARE(actions.size(), 1);
    }
    void closeIsIndependentOfAction() {
        create();
        QVERIFY(m_bar);
        auto*      close = m_bar->iconControl();
        QSignalSpy dismissed(m_bar, &SnakeBar::dismissRequested);
        input(close, 2, true);
        m_action->setEnabled(false);
        input(close, 2, false);
        QCOMPARE(dismissed.size(), 1);
    }
    void dismissReentryKeepsNewAssociation() {
        create();
        QVERIFY(m_bar);
        bind();
        QSignalSpy dismissed(m_bar, &SnakeBar::dismissRequested);
        connect(
            m_model,
            &SnakeModel::currentIdChanged,
            this,
            [this] {
                m_model->show("Next", -1, 1, m_action);
                m_bar->setSnakeId(m_model->currentId());
                m_bar->setPresentationId(m_model->presentationId(m_model->currentId()));
            },
            Qt::SingleShotConnection);
        m_bar->iconControl()->click();
        QCOMPARE(m_model->rowCount(), 1);
        QCOMPARE(m_model->data(m_model->index(0), Qt::UserRole + 1).toString(), "Next");
        QVERIFY(m_bar->isAvailable());
        QCOMPARE(dismissed.size(), 0);
    }
    void actionOutcome() {
        Action action;
        action.setEnabled(false);
        QVERIFY(! action.trigger());
        action.setEnabled(true);
        action.setCheckable(true);
        connect(
            &action,
            &Action::checkedChanged,
            &action,
            [&] {
                action.setEnabled(false);
            },
            Qt::SingleShotConnection);
        QVERIFY(! action.trigger());
        auto* dying = new Action;
        connect(
            dying,
            &Action::triggered,
            this,
            [dying] {
                delete dying;
            },
            Qt::SingleShotConnection);
        QVERIFY(dying->trigger());
        Action recursive;
        connect(&recursive, &Action::triggered, this, [&] {
            QVERIFY(! recursive.trigger());
        });
        QVERIFY(recursive.trigger());
    }
    void modelIdentityAndBounds() {
        SnakeModel               model;
        QAbstractItemModelTester tester(&model,
                                        QAbstractItemModelTester::FailureReportingMode::QtTest);
        auto                     item = model.createSnake();
        QCOMPARE(item.id, 0);
        for (int i = 0; i < 1100; ++i) QVERIFY(model.createSnake().id > item.id);
        item.duration = -1;
        model.showSnake(item);
        model.showSnake(item);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.rowCount(model.index(0)), 0);
        QVERIFY(! model.data({}).isValid());
        QVERIFY(! model.removeRows(-1, 1));
        QVERIFY(! model.removeRows(0, 2));
        QVERIFY(! model.removeRows(0, 0));
        QVERIFY(! model.removeRows(0, 1, model.index(0)));
        QVERIFY(model.removeRows(0, 1));
        QCOMPARE(model.currentId(), -1);
    }
    void timersAndSavedItems() {
        SnakeModel model;
        model.show("Saved", 60000, int(Enum::ToastFlag::TFSave));
        const auto first = model.currentId();
        auto*      timer = model.findChild<QTimer*>();
        QVERIFY(timer && timer->isActive() && timer->isSingleShot());
        model.show("Temporary", -1, 0);
        QVERIFY(! timer->isActive());
        QVERIFY(QMetaObject::invokeMethod(timer, "timeout"));
        QCOMPARE(model.rowCount(), 1);
        model.dismiss(model.currentId());
        flushInteractions();
        QCOMPARE(model.currentId(), first);
        QTimer* resumed = nullptr;
        for (auto candidate : model.findChildren<QTimer*>())
            if (candidate->isActive()) resumed = candidate;
        QVERIFY(resumed && resumed != timer);
        QVERIFY(resumed->interval() >= 1000 && resumed->interval() <= 60000);
        QVERIFY(QMetaObject::invokeMethod(resumed, "timeout"));
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(! resumed->isActive());
        flushInteractions();
        QCOMPARE(model.rowCount(), 0);
    }
    void actionReentry_data() {
        QTest::addColumn<int>("change");
        for (int change = 0; change < 4; ++change)
            QTest::newRow(qPrintable(QString::number(change))) << change;
    }
    void actionReentry() {
        QFETCH(int, change);
        auto*                model = new SnakeModel;
        QPointer<SnakeModel> guard(model);
        Action               action;
        auto                 item = model->createSnake();
        item.setAction(&action);
        model->showSnake(item);
        connect(
            &action,
            &Action::triggered,
            this,
            [&, model] {
                if (change == 0)
                    model->show("Next", -1, 0, &action);
                else if (change == 1) {
                    model->removeById(item.id);
                    model->showSnake(item);
                } else if (change == 2)
                    delete model;
                else
                    model->removeById(item.id);
            },
            Qt::SingleShotConnection);
        QVERIFY(model->performAction(item.id, model->presentationId(item.id), &action, nullptr));
        if (change == 2)
            QVERIFY(! guard);
        else {
            flushInteractions();
            if (change < 2) {
                QCOMPARE(model->rowCount(), 1);
                if (change == 0) {
                    QVERIFY(model->currentId() != item.id);
                    model->dismiss(model->currentId());
                    flushInteractions();
                    QCOMPARE(model->rowCount(), 0);
                } else
                    QCOMPARE(model->currentId(), item.id);
            } else
                QCOMPARE(model->rowCount(), 0);
            delete model;
        }
    }
    void actionLifetimeAndRejection() {
        SnakeModel     model;
        auto*          action = new Action;
        SnakeModelItem item   = model.createSnake();
        item.setAction(action);
        model.showSnake(item);
        Action other;
        QVERIFY(! model.performAction(item.id, model.presentationId(item.id), &other, nullptr));
        action->setEnabled(false);
        QVERIFY(! model.performAction(item.id, model.presentationId(item.id), action, nullptr));
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        delete action;
        QVERIFY(! item.action());
        QCOMPARE(changed.size(), 1);
        QVERIFY(! model.data(model.index(0), Qt::UserRole + 4).value<Action*>());
        QCOMPARE(model.rowCount(), 1);
    }
    void modelSignalReentry() {
        SnakeModel model;
        connect(
            &model,
            &QAbstractItemModel::rowsInserted,
            this,
            [&] {
                model.show("Queued", -1, 0);
            },
            Qt::SingleShotConnection);
        model.show("First", -1, 0);
        QCOMPARE(model.data(model.index(0), Qt::UserRole + 1).toString(), "First");
        flushInteractions();
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), Qt::UserRole + 1).toString(), "Queued");
    }
    void viewDelegateLifecycle() {
        create();
        QVERIFY(m_root);
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.SnakeView {
                width: 640; height: 420
                property MD.Action command: MD.Action { text: "Undo" }
                Component.onCompleted: show("Saved", -1, MD.Enum.TFCloseable, command)
            }
        )",
                          QUrl());
        auto* view = qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(view, qPrintable(component.errorString()));
        view->setParent(m_root);
        view->setParentItem(m_root);
        auto* model  = view->findChild<SnakeModel*>();
        auto* action = view->findChild<Action*>();
        QVERIFY(model && action);
        QVERIFY(QMetaObject::invokeMethod(view, "forceLayout"));
        const auto active = [this, view] {
            return activeBar(view);
        };
        QTRY_VERIFY(active());
        QPointer<SnakeBar> first(active());
        first->actionControl()->click();
        QCOMPARE(model->rowCount(), 0);
        QTRY_COMPARE(view->property("count").toInt(), 0);
        model->show("Another", -1, 1, action);
        QVERIFY(QMetaObject::invokeMethod(view, "forceLayout"));
        QTRY_VERIFY(active());
        auto* current = active();
        if (first) {
            QVERIFY(! first->isAvailable());
            first->actionControl()->click();
            first->iconControl()->click();
        }
        QCOMPARE(model->rowCount(), 1);
        action->trigger();
        QCOMPARE(model->rowCount(), 1);
        current->iconControl()->click();
        QCOMPARE(model->rowCount(), 0);
        QTRY_COMPARE(view->property("count").toInt(), 0);
    }
    void synchronousDestruction_data() {
        QTest::addColumn<bool>("close");
        QTest::newRow("action") << false;
        QTest::newRow("dismiss") << true;
    }
    void synchronousDestruction() {
        QFETCH(bool, close);
        create();
        QVERIFY(m_bar);
        bind();
        auto* button = close ? m_bar->iconControl() : m_bar->actionControl();
        if (close)
            connect(
                m_bar,
                &SnakeBar::dismissRequested,
                this,
                [this] {
                    delete m_root.data();
                },
                Qt::SingleShotConnection);
        else
            connect(
                m_action,
                &Action::triggered,
                this,
                [this] {
                    delete m_root.data();
                },
                Qt::SingleShotConnection);
        button->click();
        QVERIFY(! m_root);
    }
};

int run_snake_bar_input(int argc, char** argv) {
    SnakeBarInputTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "snake_bar_input.moc"
