#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/control/color_picker_button.hpp"
#include "qml_material/control/action_command.hpp"
#include "qml_material/control/overlay.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class ColorPickerInputTest : public QObject {
    Q_OBJECT
private:
    std::unique_ptr<QQmlEngine>   m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QPointer<QQuickItem>          m_root;
    QPointer<ColorPickerButton>   m_button;

    void create(bool attach = true) {
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Control {
                width: 640; height: 640
                MD.ColorPickerButton {
                    objectName: "button"; x: 40; y: 40
                    color: "#123456"; showAlpha: false
                }
            }
        )",
                          QUrl());
        m_root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(m_root, qPrintable(component.errorString()));
        m_button = m_root->findChild<ColorPickerButton*>();
        QVERIFY(m_button);
        if (! attach) return;
        m_root->setParentItem(m_window->contentItem());
        m_window->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
    }
    void flushPointerEvents() {
        QQuickWindowPrivate::get(m_window.get())
            ->deliveryAgentPrivate()
            ->flushFrameSynchronousEvents(m_window.get());
        flushInteractions();
    }
    void input(int mode, bool press) {
        const auto point =
            m_button->mapToScene({ m_button->width() / 2, m_button->height() / 2 }).toPoint();
        if (mode == 0) {
            if (! press) m_button->click();
        } else if (mode == 1) {
            if (press)
                QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
            else
                QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, point);
        } else if (mode == 2) {
            if (press) {
                m_button->forceActiveFocus(Qt::TabFocusReason);
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
    QPointer<Dialog> open() {
        m_button->click();
        QPointer<Dialog> dialog(m_button->dialog());
        if (dialog) {
            dialog->setEnter(nullptr);
            dialog->setExit(nullptr);
            dialog->completeEnter();
        }
        return dialog;
    }
private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(".*"));
        m_engine = std::make_unique<QQmlEngine>();
        m_engine->addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window = std::make_unique<QQuickWindow>();
        m_window->resize(640, 640);
    }
    void cleanup() {
        delete m_root.data();
        flushInteractions();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        m_window.reset();
        m_engine.reset();
    }
    void commands_data() {
        QTest::addColumn<int>("mode");
        for (int mode = 0; mode < 4; ++mode)
            QTest::newRow(qPrintable(QString::number(mode))) << mode;
    }
    void commands() {
        QFETCH(int, mode);
        create();
        QVERIFY(m_button);
        QVERIFY(! m_button->dialog());
        QCOMPARE((m_button)->metaObject()->indexOfProperty("checkable"), -1);
        QVERIFY(! qmlAttachedPropertiesObject<ActionCommand>(m_button, false));
        for (const auto name : { "checked", "checkable", "pressed", "pressX", "pressY", "action" })
            QVERIFY(! m_button->property(name).isValid());
        QSignalSpy clicks(m_button, &CommandButton::clicked),
            accepted(m_button, &ColorPickerButton::accepted);
        input(mode, true);
        QVERIFY(! m_button->dialog());
        input(mode, false);
        auto* dialog = m_button->dialog();
        QVERIFY(dialog && dialog->isVisible());
        QCOMPARE(clicks.size(), 1);
        QCOMPARE(accepted.size(), 0);
        QCOMPARE(dialog->property("color").value<QColor>(), QColor("#123456"));
        QCOMPARE(dialog->property("selectedColor").value<QColor>(), QColor("#123456"));
        QVERIFY(! dialog->property("showAlpha").toBool());
        QCOMPARE(dialog->parentItem(), OverlayManager::get(m_window.get()));
        QVERIFY(! dialog->parent());
        QCOMPARE(dialog->presentationOwner(), m_button);
    }
    void lazyLifecycle() {
        create(false);
        QVERIFY(m_button);
        m_button->dismissPopup();
        m_button->click();
        QVERIFY(! m_button->dialog());
        QVERIFY(m_button->findChildren<Dialog*>().isEmpty());
        m_root->setParentItem(m_window->contentItem());
        auto dialog = open();
        QVERIFY(dialog && dialog->isOpened());
        m_button->click();
        QCOMPARE(m_button->dialog(), dialog);
        m_button->setColor(QColor("#abcdef"));
        m_button->setShowAlpha(true);
        QCOMPARE(dialog->property("selectedColor").value<QColor>(), QColor("#abcdef"));
        QVERIFY(dialog->property("showAlpha").toBool());
        dialog->setProperty("color", QColor("#fedcba"));
        dialog->setDeferredCompletion(true);
        dialog->reject();
        QVERIFY(dialog->closing());
        m_button->click();
        QCOMPARE(m_button->dialog(), dialog);
        dialog->completeExit();
        QVERIFY(! m_button->dialog());
        QCOMPARE(m_button->color(), QColor("#abcdef"));
        QTRY_VERIFY(! dialog);
        dialog = open();
        QVERIFY(dialog);
        QCOMPARE(dialog->property("selectedColor").value<QColor>(), QColor("#abcdef"));
        m_button->dismissPopup();
        QVERIFY(dialog && ! dialog->isVisible());
        QVERIFY(! m_button->dialog());
        dialog->open();
        QVERIFY(! dialog->isVisible());
        QTRY_VERIFY(! dialog);
    }
    void result_data() {
        QTest::addColumn<int>("finish");
        for (int finish = 0; finish < 6; ++finish)
            QTest::newRow(qPrintable(QString::number(finish))) << finish;
    }
    void result() {
        QFETCH(int, finish);
        create();
        QVERIFY(m_button);
        auto dialog = open();
        QVERIFY(dialog);
        QSignalSpy accepted(m_button, &ColorPickerButton::accepted);
        dialog->setProperty("color", QColor("#804488cc"));
        QCOMPARE(m_button->color(), QColor("#123456"));
        if (finish == 0)
            dialog->accept();
        else if (finish == 1)
            dialog->standardButton(Dialog::Ok)->click();
        else if (finish == 2)
            dialog->reject();
        else if (finish == 3)
            dialog->standardButton(Dialog::Cancel)->click();
        else if (finish == 4)
            QTest::keyClick(m_window.get(), Qt::Key_Escape);
        else
            QTest::mouseClick(m_window.get(), Qt::LeftButton, Qt::NoModifier, { 620, 620 });
        const bool committed = finish < 2;
        QCOMPARE(accepted.size(), committed ? 1 : 0);
        QCOMPARE(m_button->color(), committed ? QColor("#804488cc") : QColor("#123456"));
        if (committed) QCOMPARE(accepted.at(0).at(0).value<QColor>(), QColor("#804488cc"));
        QTRY_VERIFY(! dialog);
        QVERIFY(! m_button->dialog());
    }
    void canceledInput_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<int>("change");
        for (int mode = 1; mode < 4; ++mode)
            for (int change = 0; change < 5; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(mode).arg(change))) << mode << change;
    }
    void canceledInput() {
        QFETCH(int, mode);
        QFETCH(int, change);
        create();
        QVERIFY(m_button);
        QSignalSpy clicked(m_button, &CommandButton::clicked);
        input(mode, true);
        InteractionSource replacement;
        if (change == 0)
            m_button->setEnabled(false);
        else if (change == 1)
            m_button->setVisible(false);
        else if (change == 2)
            m_button->setInteractionSource(&replacement);
        else if (change == 3)
            m_button->dismissPopup();
        else
            m_root->setParentItem(nullptr);
        input(mode, false);
        QCOMPARE(clicked.size(), 0);
        QVERIFY(! m_button->dialog());
    }
    void clickCallback_data() {
        QTest::addColumn<int>("change");
        for (int change = 0; change < 4; ++change)
            QTest::newRow(qPrintable(QString::number(change))) << change;
    }
    void clickCallback() {
        QFETCH(int, change);
        create();
        QVERIFY(m_button);
        connect(
            m_button,
            &CommandButton::clicked,
            this,
            [this, change] {
                if (change == 0)
                    m_button->dismissPopup();
                else if (change == 1)
                    m_button->setEnabled(false);
                else if (change == 2)
                    m_root->setParentItem(nullptr);
                else
                    delete m_root.data();
            },
            Qt::SingleShotConnection);
        m_button->click();
        if (change == 3)
            QVERIFY(! m_button);
        else
            QVERIFY(! m_button->dialog());
    }
    void ownerInvalidation_data() {
        QTest::addColumn<int>("change");
        for (int change = 0; change < 4; ++change)
            QTest::newRow(qPrintable(QString::number(change))) << change;
    }
    void ownerInvalidation() {
        QFETCH(int, change);
        create();
        QVERIFY(m_button);
        auto dialog = open();
        QVERIFY(dialog);
        QSignalSpy accepted(m_button, &ColorPickerButton::accepted);
        if (change == 0)
            m_button->setVisible(false);
        else if (change == 1)
            m_root->setParentItem(nullptr);
        else if (change == 2) {
            QQuickWindow other;
            m_root->setParentItem(other.contentItem());
            m_root->setParentItem(m_window->contentItem());
        } else
            delete m_root.data();
        if (m_button) QVERIFY(! m_button->dialog());
        if (dialog) {
            QVERIFY(! dialog->isVisible());
            dialog->accept();
        }
        QCOMPARE(accepted.size(), 0);
        QTRY_VERIFY(! dialog);
    }
    void replacingClosedSession() {
        create();
        QVERIFY(m_button);
        auto first = open();
        QVERIFY(first);
        QSignalSpy accepted(m_button, &ColorPickerButton::accepted);
        first->reject();
        QVERIFY(! m_button->dialog());
        auto second = open();
        QVERIFY(second && first && second != first);
        first->setProperty("color", QColor("#ffffff"));
        first->accept();
        QCOMPARE(accepted.size(), 0);
        QCOMPARE(m_button->dialog(), second);
        QCOMPARE(m_button->color(), QColor("#123456"));
        delete first.data();
        QCOMPARE(m_button->dialog(), second);
        delete second.data();
        QVERIFY(! m_button->dialog());
        QVERIFY(open());
    }
    void dialogNotificationReentry() {
        create();
        QVERIFY(m_button);
        QPointer<Dialog> retired;
        connect(
            m_button,
            &ColorPickerButton::dialogChanged,
            this,
            [this, &retired] {
                retired = m_button->dialog();
                m_button->dismissPopup();
                m_button->click();
            },
            Qt::SingleShotConnection);
        m_button->click();
        QVERIFY(retired && ! retired->isVisible());
        QVERIFY(m_button->dialog() && m_button->dialog() != retired);
        QVERIFY(m_button->dialog()->isVisible());
    }
    void dismissNotificationReentry() {
        create();
        QVERIFY(m_button);
        auto first = open();
        QVERIFY(first);
        connect(
            first, &Popup::aboutToHide, m_button, &ButtonControl::click, Qt::SingleShotConnection);
        QSignalSpy changed(m_button, &ColorPickerButton::dialogChanged);
        m_button->dismissPopup();
        QVERIFY(first && ! first->isVisible());
        QVERIFY(m_button->dialog() && m_button->dialog() != first);
        QVERIFY(m_button->dialog()->isVisible());
        QCOMPARE(changed.size(), 1);
    }
    void duplicateAcceptance() {
        create();
        QVERIFY(m_button);
        auto dialog = open();
        QVERIFY(dialog);
        QSignalSpy accepted(m_button, &ColorPickerButton::accepted);
        connect(m_button,
                &ColorPickerButton::accepted,
                dialog,
                &Dialog::accept,
                Qt::SingleShotConnection);
        dialog->setProperty("color", QColor("#abcdef"));
        dialog->accept();
        QCOMPARE(accepted.size(), 1);
        QCOMPARE(m_button->color(), QColor("#abcdef"));
    }
    void disabledOpenerKeepsDialog() {
        create();
        QVERIFY(m_button);
        auto dialog = open();
        QVERIFY(dialog);
        QSignalSpy accepted(m_button, &ColorPickerButton::accepted);
        m_button->setEnabled(false);
        QCOMPARE(m_button->dialog(), dialog);
        QVERIFY(dialog->isOpened());
        dialog->setProperty("color", QColor("#abcdef"));
        dialog->accept();
        QCOMPARE(accepted.size(), 1);
        QCOMPARE(m_button->color(), QColor("#abcdef"));
        QVERIFY(! m_button->dialog());
    }
    void externalInteractionsDoNotOpen() {
        create();
        QVERIFY(m_button);
        InteractionSource source;
        m_button->setInteractionSource(&source);
        QSignalSpy             clicked(m_button, &CommandButton::clicked);
        TestInteractionEmitter emitter(&source, m_button);
        const auto             press = emitter.begin(Interaction::Press);
        emitter.end(press);
        QCOMPARE(clicked.size(), 0);
        QVERIFY(! m_button->dialog());
        QVERIFY(open());
    }
    void synchronousDestruction_data() {
        QTest::addColumn<int>("notification");
        for (int notification = 0; notification < 7; ++notification)
            QTest::newRow(qPrintable(QString::number(notification))) << notification;
    }
    void synchronousDestruction() {
        QFETCH(int, notification);
        create();
        QVERIFY(m_button);
        const auto destroy = [this] {
            delete m_root.data();
        };
        if (notification == 0) {
            connect(m_button,
                    &ColorPickerButton::dialogChanged,
                    this,
                    destroy,
                    Qt::SingleShotConnection);
            m_button->click();
        } else {
            auto dialog = open();
            QVERIFY(dialog);
            if (notification == 1)
                connect(m_button,
                        &ColorPickerButton::colorChanged,
                        this,
                        destroy,
                        Qt::SingleShotConnection);
            else if (notification == 2)
                connect(m_button,
                        &ColorPickerButton::accepted,
                        this,
                        destroy,
                        Qt::SingleShotConnection);
            else if (notification == 3)
                connect(dialog, &Popup::aboutToHide, this, destroy, Qt::SingleShotConnection);
            else if (notification == 4)
                connect(m_button,
                        &ColorPickerButton::dialogChanged,
                        this,
                        destroy,
                        Qt::SingleShotConnection);
            else if (notification == 5)
                connect(dialog, &QObject::destroyed, this, destroy, Qt::SingleShotConnection);
            else
                connect(m_button,
                        &ColorPickerButton::showAlphaChanged,
                        this,
                        destroy,
                        Qt::SingleShotConnection);
            if (notification < 3) {
                dialog->setProperty("color", QColor("#abcdef"));
                dialog->accept();
            } else if (notification < 5)
                m_button->dismissPopup();
            else if (notification == 5)
                delete dialog.data();
            else
                m_button->setShowAlpha(true);
        }
        QVERIFY(! m_root && ! m_button);
    }
};

int run_color_picker_input(int argc, char** argv) {
    ColorPickerInputTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "color_picker_input.moc"
