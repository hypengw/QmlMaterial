#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include "qml_material/carousel/carousel_control.hpp"
#include "qml_material/carousel/carousel_item.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class WheelReceiver : public QQuickItem {
public:
    explicit WheelReceiver(QQuickItem* parent): QQuickItem(parent) {}
    int calls = 0;

protected:
    void wheelEvent(QWheelEvent* event) override {
        ++calls;
        event->accept();
    }
};

class CarouselControlTest : public QObject {
    Q_OBJECT
    std::unique_ptr<QQmlEngine>   m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QPointer<CarouselControl>     m_control;

    void create() {
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Carousel {
                width: 360; height: 260
                model: 5
                layout: MD.Enum.CarouselFullScreen
                reduceMotion: true
                showNavigationButtons: true
                minimumViewportHeight: 180
                delegate: MD.CarouselItem { required property var model }
            }
        )",
                          QUrl("qrc:/tests/carousel-control.qml"));
        m_control = qobject_cast<CarouselControl*>(component.create());
        QVERIFY2(m_control, qPrintable(component.errorString()));
        m_control->setParentItem(m_window->contentItem());
        m_window->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
        QVERIFY(m_control->view());
        QVERIFY(m_control->nextControl());
        QVERIFY(m_control->previousControl());
        QTRY_VERIFY(m_control->nextControl()->isVisible());
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
    bool wheel(QPoint delta, Qt::KeyboardModifiers modifiers = Qt::NoModifier,
               bool inverted = false, QPoint pixel = {}) {
        auto*       flick    = m_control->view()->flickable();
        const auto  position = center(flick);
        QWheelEvent event(position,
                          m_window->mapToGlobal(position),
                          pixel,
                          delta,
                          Qt::NoButton,
                          modifiers,
                          Qt::NoScrollPhase,
                          inverted);
        event.setAccepted(false);
        QCoreApplication::sendEvent(flick, &event);
        return event.isAccepted();
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
        m_window->resize(420, 320);
    }
    void cleanup() {
        delete m_control.data();
        flushInteractions();
        m_window.reset();
        m_engine.reset();
    }
    void commands_data() {
        QTest::addColumn<int>("input");
        QTest::addColumn<int>("direction");
        for (int input = 0; input < 4; ++input)
            for (int direction : { -1, 1 })
                QTest::newRow(qPrintable(QString("%1-%2").arg(input).arg(direction)))
                    << input << direction;
    }
    void commands() {
        QFETCH(int, input);
        QFETCH(int, direction);
        create();
        QVERIFY(m_control);
        m_control->setCurrentIndex(2);
        auto*      button = direction > 0 ? m_control->nextControl() : m_control->previousControl();
        QSignalSpy changed(m_control, &CarouselControl::indexChanged);
        QSignalSpy clicks(m_control, &CarouselControl::clicked);
        activate(button, input);
        QTRY_COMPARE(m_control->currentIndex(), 2 + direction);
        QCOMPARE(changed.size(), 1);
        QCOMPARE(changed.first().first().toInt(), 2 + direction);
        QCOMPARE(clicks.size(), 0);
        QVERIFY(! button->property("action").isValid());
        QVERIFY(! button->property("selected").isValid());
        QVERIFY(! button->property("checked").isValid());
        m_control->setEnabled(false);
        activate(button, 0);
        QCOMPARE(m_control->currentIndex(), 2 + direction);
    }
    void programmaticIndex() {
        create();
        QVERIFY(m_control);
        m_control->setCurrentIndex(2, true);
        QCOMPARE(m_control->currentIndex(), 2);
        m_control->incrementCurrentIndex();
        QCOMPARE(m_control->currentIndex(), 3);
        m_control->decrementCurrentIndex();
        QCOMPARE(m_control->currentIndex(), 2);
        m_control->setCurrentIndex(50);
        QCOMPARE(m_control->currentIndex(), 4);
        QVERIFY(! m_control->nextControl()->isEnabled());
        m_control->view()->setModel(2);
        QCOMPARE(m_control->currentIndex(), 1);
        m_control->setEnabled(false);
        m_control->setCurrentIndex(0);
        QCOMPARE(m_control->currentIndex(), 0);
    }
    void pendingIndexAndViewReplacement() {
        CarouselControl owner;
        owner.setCurrentIndex(3);
        QCOMPARE(owner.currentIndex(), 3);
        auto first = std::make_unique<CarouselView>();
        first->setModel(5);
        owner.setView(first.get());
        QCOMPARE(first->currentIndex(), 3);
        CarouselView second;
        second.setModel(7);
        second.setCurrentIndex(4);
        QSignalSpy changes(&owner, &CarouselControl::indexChanged);
        owner.setView(&second);
        QCOMPARE(owner.currentIndex(), 4);
        first->setCurrentIndex(1);
        QCOMPARE(changes.size(), 0);
        first.reset();
        QCOMPARE(owner.view(), &second);
        owner.setCurrentIndex(2);
        QCOMPARE(second.currentIndex(), 2);
        QCOMPARE(changes.size(), 1);
        owner.setView(nullptr);
        QCOMPARE(owner.currentIndex(), 0);
    }
    void initialIndexBinding() {
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                property int chosen: 3
                function jump(index, animated) {
                    carousel.setCurrentIndex(index, animated)
                    return carousel.currentIndex
                }
                MD.Carousel {
                    id: carousel
                    objectName: "carousel"
                    width: 360; height: 240
                    currentIndex: parent.chosen
                    model: 5
                    reduceMotion: true
                    layout: MD.Enum.CarouselFullScreen
                    delegate: MD.CarouselItem { required property var model }
                }
            }
        )",
                          QUrl("qrc:/tests/carousel-index-binding.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = object->findChild<CarouselControl*>("carousel");
        QVERIFY(control);
        QCOMPARE(control->currentIndex(), 3);
        QVariant result;
        QVERIFY(QMetaObject::invokeMethod(object.get(),
                                          "jump",
                                          Q_RETURN_ARG(QVariant, result),
                                          Q_ARG(QVariant, 2),
                                          Q_ARG(QVariant, true)));
        QCOMPARE(result.toInt(), 2);
        object->setProperty("chosen", 1);
        QCOMPARE(control->currentIndex(), 1);
    }
    void wheelPolicy_data() {
        QTest::addColumn<int>("mode");
        for (int mode = 0; mode < 13; ++mode)
            QTest::newRow(qPrintable(QString::number(mode))) << mode;
    }
    void wheelPolicy() {
        QFETCH(int, mode);
        create();
        QVERIFY(m_control);
        m_control->setCurrentIndex(2);
        auto* view = m_control->view();
        view->setWheelRequiresShift(mode == 0 || mode == 1);
        if (mode == 7) m_control->setEnabled(false);
        if (mode == 8) m_control->setVisible(false);
        if (mode == 9) view->setInteractive(false);
        if (mode == 10) view->setModel(1);
        if (mode == 11) view->setModel(0);
        const int  start    = view->currentIndex();
        const bool accepted = wheel(mode == 3                ? QPoint(120, 0)
                                    : mode == 5 || mode == 6 ? QPoint()
                                                             : QPoint(0, 120),
                                    mode == 1 ? Qt::ShiftModifier : Qt::NoModifier,
                                    mode == 4,
                                    mode == 6 ? QPoint(0, 10) : QPoint());
        const bool advance  = mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 12;
        QCOMPARE(accepted, advance);
        QCOMPARE(view->currentIndex(), start + (advance ? (mode == 4 ? -1 : 1) : 0));
        if (mode == 12) {
            view->setCurrentIndex(4);
            QVERIFY(wheel({ 0, 120 }));
            QCOMPARE(view->currentIndex(), 4);
        }
    }
    void heldCommandCancelled_data() {
        QTest::addColumn<int>("change");
        for (int change = 0; change < 9; ++change)
            QTest::newRow(qPrintable(QString::number(change))) << change;
    }
    void wheelPropagation() {
        create();
        QVERIFY(m_control);
        WheelReceiver parent(m_window->contentItem());
        parent.setSize({ 420, 320 });
        m_control->setParentItem(&parent);
        m_control->view()->setWheelRequiresShift(true);
        const auto position = center(m_control->view());
        const auto send     = [&](Qt::KeyboardModifiers modifiers) {
            QWheelEvent event(position,
                              m_window->mapToGlobal(position),
                              {},
                              { 0, 120 },
                              Qt::NoButton,
                              modifiers,
                              Qt::NoScrollPhase,
                              false);
            event.setAccepted(false);
            QCoreApplication::sendEvent(m_window.get(), &event);
            QVERIFY(event.isAccepted());
        };
        send(Qt::NoModifier);
        QCOMPARE(parent.calls, 1);
        QCOMPARE(m_control->currentIndex(), 0);
        send(Qt::ShiftModifier);
        QCOMPARE(parent.calls, 1);
        QCOMPARE(m_control->currentIndex(), 1);
        m_control->setParentItem(m_window->contentItem());
    }
    void disabledWheelNavigationUsesFlickable() {
        create();
        QVERIFY(m_control);
        m_control->setCurrentIndex(2);
        m_control->setProperty("wheelNavigationEnabled", false);
        auto* view = m_control->view();
        QVERIFY(! view->wheelNavigationEnabled());
        auto* flick = qobject_cast<QQuickFlickable*>(view->flickable());
        QVERIFY(flick);
        const auto  start    = flick->contentX();
        const auto  position = center(flick);
        QWheelEvent event(position,
                          m_window->mapToGlobal(position),
                          {},
                          { 120, 0 },
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::NoScrollPhase,
                          false);
        event.setTimestamp(1000);
        QCoreApplication::sendEvent(flick, &event);
        QVERIFY(event.isAccepted());
        QTRY_VERIFY(flick->contentX() < start);
        flick->cancelFlick();
    }
    void heldCommandCancelled() {
        QFETCH(int, change);
        create();
        QVERIFY(m_control);
        m_control->setCurrentIndex(2);
        auto*                   view   = m_control->view();
        auto*                   button = m_control->nextControl();
        QPointer<CommandButton> buttonGuard(button);
        const auto              position = center(button);
        QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, position);
        flushPointerEvents();
        if (change == 0) {
            m_control->setEnabled(false);
            m_control->setEnabled(true);
        }
        if (change == 1) {
            m_control->setVisible(false);
            m_control->setVisible(true);
        }
        if (change == 2) view->setModel(6);
        if (change == 3) {
            view->setInteractive(false);
            view->setInteractive(true);
        }
        if (change == 4) {
            m_control->setNextControl(nullptr);
            m_control->setNextControl(button);
        }
        if (change == 5) {
            m_control->setView(nullptr);
            m_control->setView(view);
        }
        if (change == 6) view->setCurrentIndex(1);
        if (change == 7) view->setOrientation(Qt::Vertical);
        if (change == 8) {
            delete button;
        }
        const int  start = view->currentIndex();
        QSignalSpy changed(m_control, &CarouselControl::indexChanged);
        QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, position);
        flushPointerEvents();
        QCOMPARE(changed.size(), 0);
        QCOMPARE(view->currentIndex(), start);
        if (change != 8) {
            QVERIFY(buttonGuard);
            button->click();
            QCOMPARE(view->currentIndex(), start + 1);
        } else
            QVERIFY(! m_control->nextControl());
    }
    void focusCallbackInvalidatesCommand_data() {
        QTest::addColumn<int>("change");
        QTest::addColumn<bool>("useWheel");
        for (int change = 0; change < 7; ++change)
            for (bool useWheel : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(change).arg(useWheel)))
                    << change << useWheel;
    }
    void focusCallbackInvalidatesCommand() {
        QFETCH(int, change);
        QFETCH(bool, useWheel);
        create();
        QVERIFY(m_control);
        m_control->setCurrentIndex(2);
        const QPointer<CarouselView> view(m_control->view());
        auto*                        current = m_control->findChild<CarouselItem*>();
        for (auto* item : m_control->findChildren<CarouselItem*>())
            if (item->carousel() == view && item->property("index").toInt() == 2) current = item;
        QVERIFY(current);
        m_control->nextControl()->forceActiveFocus(Qt::TabFocusReason);
        bool called = false;
        connect(current, &QQuickItem::activeFocusChanged, m_window.get(), [&] {
            if (called || ! current->hasActiveFocus()) return;
            called = true;
            if (change == 0) m_control->setEnabled(false);
            if (change == 1) m_control->setVisible(false);
            if (change == 2) view->setModel(6);
            if (change == 3) view->setCurrentIndex(1);
            if (change == 4) view->setInteractive(false);
            if (change == 5) {
                m_control->setView(nullptr);
                m_control->setView(view);
            }
            if (change == 6) {
                auto* button = m_control->nextControl();
                m_control->setNextControl(nullptr);
                m_control->setNextControl(button);
            }
        });
        if (useWheel)
            QVERIFY(wheel({ 0, 120 }));
        else
            m_control->nextControl()->click();
        QVERIFY(called);
        QCOMPARE(view->currentIndex(), change == 3 ? 1 : useWheel && change >= 5 ? 3 : 2);
    }
    void deletedDuringFocusFeedback_data() {
        QTest::addColumn<bool>("useWheel");
        QTest::addColumn<bool>("deleteView");
        for (bool useWheel : { false, true })
            for (bool deleteView : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(useWheel).arg(deleteView)))
                    << useWheel << deleteView;
    }
    void deletedDuringFocusFeedback() {
        QFETCH(bool, useWheel);
        QFETCH(bool, deleteView);
        create();
        QVERIFY(m_control);
        auto* item = m_control->findChild<CarouselItem*>();
        QVERIFY(item);
        item->forceActiveFocus(Qt::TabFocusReason);
        auto* attached =
            qobject_cast<CarouselAttached*>(qmlAttachedPropertiesObject<CarouselView>(item, false));
        QVERIFY(attached && attached->focusRingVisible());
        const QPointer<CarouselView> view(m_control->view());
        int                          calls = 0;
        connect(attached, &CarouselAttached::focusRingChanged, m_window.get(), [&] {
            ++calls;
            if (deleteView)
                delete view.data();
            else
                delete m_control.data();
        });
        if (useWheel)
            QVERIFY(wheel({ 0, 120 }));
        else
            m_control->nextControl()->click();
        QCOMPARE(calls, 1);
        QVERIFY(! view);
    }
    void deletedInNotification_data() {
        QTest::addColumn<int>("input");
        QTest::addColumn<bool>("deleteView");
        for (int input = 0; input < 3; ++input)
            for (bool deleteView : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(input).arg(deleteView)))
                    << input << deleteView;
    }
    void deletedInNotification() {
        QFETCH(int, input);
        QFETCH(bool, deleteView);
        create();
        QVERIFY(m_control);
        const QPointer<CarouselView> view(m_control->view());
        int                          calls  = 0;
        const auto                   remove = [&] {
            ++calls;
            if (deleteView)
                delete view.data();
            else
                delete m_control.data();
        };
        if (input != 0) {
            connect(m_control, &CarouselControl::indexChanged, m_window.get(), remove);
            if (input == 1)
                m_control->nextControl()->click();
            else
                QVERIFY(wheel({ 0, 120 }));
        } else {
            connect(m_control, &CarouselControl::clicked, m_window.get(), remove);
            auto* item = m_control->findChild<CarouselItem*>();
            QVERIFY(item);
            item->click();
        }
        QCOMPARE(calls, 1);
        QVERIFY(! view);
        if (m_control) QVERIFY(! m_control->view());
    }
};

int run_carousel_control(int argc, char** argv) {
    CarouselControlTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "carousel_control.moc"
