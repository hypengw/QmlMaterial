#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/carousel/carousel_item.hpp"
#include "qml_material/carousel/carousel_view.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/style/carousel_item_state.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class CarouselInputTest : public QObject {
    Q_OBJECT
private:
    std::unique_ptr<QQmlEngine>   m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QPointer<CarouselView>        m_view;

    void flushPointerEvents() {
        QQuickWindowPrivate::get(m_window.get())
            ->deliveryAgentPrivate()
            ->flushFrameSynchronousEvents(m_window.get());
        flushInteractions();
    }

    void create() {
        QQmlComponent component(m_engine.get());
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.CarouselView {
                id: view
                width: 320; height: 200
                layout: MD.Enum.CarouselFullScreen
                reduceMotion: true
                model: 8
                property bool intercept: false
                property bool rebuildOnClick: false
                delegate: MD.CarouselItem {
                    required property var model
                    objectName: "item" + index
                    Keys.onPressed: event => {
                        event.accepted = view.intercept &&
                            [Qt.Key_Left, Qt.Key_Right, Qt.Key_Up, Qt.Key_Down].includes(event.key)
                    }
                    onClicked: {
                        if (view.rebuildOnClick) view.model = 5
                    }
                }
            }
        )",
                          QUrl());
        m_view = qobject_cast<CarouselView*>(component.create());
        QVERIFY2(m_view, qPrintable(component.errorString()));
        m_view->setParentItem(m_window->contentItem());
        m_window->show();
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
    }
    CarouselItem* item(int index) const {
        if (! m_view) return nullptr;
        for (auto* candidate : m_view->findChildren<CarouselItem*>())
            if (candidate->carousel() == m_view &&
                candidate->objectName() == "item" + QString::number(index))
                return candidate;
        return nullptr;
    }
    CarouselAttached* attached(CarouselItem* item) const {
        return qobject_cast<CarouselAttached*>(
            qmlAttachedPropertiesObject<CarouselView>(item, false));
    }
private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(".*"));
        m_engine = std::make_unique<QQmlEngine>();
        m_engine->addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window = std::make_unique<QQuickWindow>();
        m_window->resize(360, 240);
    }
    void cleanup() {
        delete m_view.data();
        flushInteractions();
        m_window.reset();
        m_engine.reset();
    }
    void commands_data() {
        QTest::addColumn<int>("input");
        for (int input = 0; input < 4; ++input)
            QTest::newRow(qPrintable(QString::number(input))) << input;
    }
    void commands() {
        QFETCH(int, input);
        create();
        QVERIFY(m_view);
        auto* control = item(0);
        QVERIFY(control && attached(control));
        QCOMPARE((control)->metaObject()->indexOfProperty("checkable"), -1);
        for (const char* name : { "action",
                                  "checked",
                                  "checkable",
                                  "pressed",
                                  "pressX",
                                  "pressY",
                                  "longClickEnabled",
                                  "doubleClickEnabled",
                                  "_carouselView",
                                  "_carouselIndex" })
            QVERIFY(! control->property(name).isValid());
        QSignalSpy clicks(control, &CommandButton::clicked),
            forwarded(m_view, &CarouselView::clicked);
        if (input == 0)
            control->click();
        else if (input == 1)
            QTest::mouseClick(m_window.get(), Qt::LeftButton, Qt::NoModifier, { 100, 100 });
        else if (input == 2) {
            control->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(m_window.get(), Qt::Key_Space);
        } else {
            static auto* device = QTest::createTouchDevice();
            QTest::touchEvent(m_window.get(), device)
                .press(0, { 100, 100 }, m_window.get())
                .commit();
            flushPointerEvents();
            QTest::touchEvent(m_window.get(), device)
                .release(0, { 100, 100 }, m_window.get())
                .commit();
            flushPointerEvents();
        }
        flushInteractions();
        QCOMPARE(clicks.size(), 1);
        QCOMPARE(forwarded.size(), 1);
        QCOMPARE(forwarded.first().first().toInt(), 0);
        QCOMPARE(m_view->currentIndex(), 0);
    }
    void keyboard_data() {
        QTest::addColumn<bool>("vertical");
        QTest::addColumn<bool>("animated");
        QTest::addColumn<bool>("mirrored");
        for (bool vertical : { false, true })
            for (bool animated : { false, true })
                for (bool mirrored : { false, true })
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(vertical).arg(animated).arg(mirrored)))
                        << vertical << animated << mirrored;
    }
    void keyboard() {
        QFETCH(bool, vertical);
        QFETCH(bool, animated);
        QFETCH(bool, mirrored);
        create();
        QVERIFY(m_view);
        m_view->setOrientation(vertical ? Qt::Vertical : Qt::Horizontal);
        m_view->setMirrored(mirrored);
        m_view->setReduceMotion(! animated);
        auto* first = item(0);
        QVERIFY(first);
        first->forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(attached(first)->focusRingVisible());
        const auto next     = vertical ? Qt::Key_Down : mirrored ? Qt::Key_Left : Qt::Key_Right;
        const auto previous = vertical ? Qt::Key_Up : mirrored ? Qt::Key_Right : Qt::Key_Left;
        QTest::keyClick(m_window.get(), next);
        QTRY_COMPARE(m_view->currentIndex(), 1);
        QTRY_VERIFY(item(1) && item(1)->hasActiveFocus());
        QTRY_VERIFY(attached(item(1))->focusRingVisible());
        QTest::keyClick(m_window.get(), previous);
        QTRY_COMPARE(m_view->currentIndex(), 0);
        QTRY_VERIFY(first->hasActiveFocus());
        QVERIFY(attached(first)->focusRingVisible());
        QTest::keyClick(m_window.get(), previous);
        QCOMPARE(m_view->currentIndex(), 0);
        m_view->setProperty("intercept", true);
        QTest::keyClick(m_window.get(), next);
        QCOMPARE(m_view->currentIndex(), 0);
        m_view->setProperty("intercept", false);
        QTest::keyClick(m_window.get(), next);
        QTRY_COMPARE(m_view->currentIndex(), 1);
    }
    void pointerFocusDoesNotReadState() {
        create();
        QVERIFY(m_view);
        auto* first = item(0);
        QVERIFY(first);
        first->forceActiveFocus(Qt::TabFocusReason);
        QVERIFY(attached(first)->focusRingVisible());
        auto* state = first->property("mdState").value<CarouselItemState*>();
        QVERIFY(state);
        InteractionSource decoy;
        state->setInteractionSource(&decoy);
        QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, { 100, 100 });
        flushInteractions();
        QVERIFY(first->hasActiveFocus());
        QVERIFY(! state->interactions().pressed);
        QVERIFY(! attached(first)->focusRingVisible());
        QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, { 100, 100 });
    }
    void sharedSourceDoesNotNavigate() {
        create();
        QVERIFY(m_view);
        auto* first = item(0);
        QVERIFY(first);
        first->forceActiveFocus(Qt::TabFocusReason);
        QVERIFY(attached(first)->focusRingVisible());
        InteractionSource shared, replacement;
        first->setInteractionSource(&shared);
        auto emitter = shared.emitter(first);
        emitter->begin(Interaction::Press, Interaction::Mouse, -1, { 100, 100 });
        flushInteractions();
        QVERIFY(attached(first)->focusRingVisible());
        QCOMPARE(m_view->currentIndex(), 0);
        auto* state = first->property("mdState").value<CarouselItemState*>();
        QVERIFY(state && state->interactions().pressed);
        first->setInteractionSource(&replacement);
        QVERIFY(! state->interactions().pressed);
        emitter->cancel();
        flushInteractions();
        QVERIFY(attached(first)->focusRingVisible());
    }
    void cancellation_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::addColumn<int>("change");
        for (bool keyboard : { false, true })
            for (int change = 0; change < 5; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(keyboard).arg(change)))
                    << keyboard << change;
    }
    void cancellation() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        create();
        QVERIFY(m_view);
        QPointer<CarouselItem> first = item(0);
        QVERIFY(first);
        InteractionAccumulator feedback(this);
        feedback.setSource(first->interactionSource());
        QSignalSpy clicks(first, &CommandButton::clicked),
            forwarded(m_view, &CarouselView::clicked);
        if (keyboard) {
            first->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyPress(m_window.get(), Qt::Key_Space);
        } else
            QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, { 100, 100 });
        InteractionSource replacement;
        QQmlComponent     newDelegate(m_engine.get());
        if (change == 0)
            first->setInteractionSource(&replacement);
        else if (change == 1)
            first->setEnabled(false);
        else if (change == 2)
            first->setVisible(false);
        else if (change == 3)
            m_view->setModel(4);
        else {
            newDelegate.setData(
                "import Qcm.Material as MD\nMD.CarouselItem { required property var model }",
                QUrl());
            QVERIFY2(newDelegate.isReady(), qPrintable(newDelegate.errorString()));
            m_view->setDelegate(&newDelegate);
        }
        if (keyboard)
            QTest::keyRelease(m_window.get(), Qt::Key_Space);
        else
            QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, { 100, 100 });
        flushInteractions();
        QVERIFY(clicks.isEmpty() && forwarded.isEmpty());
        QVERIFY(! feedback.values().pressed);
        if (change >= 3 && first) {
            QVERIFY(! first->carousel() && ! first->isVisible() && ! first->isEnabled());
            first->click();
            QVERIFY(clicks.isEmpty() && forwarded.isEmpty());
        }
    }
    void dragCancelsCommand_data() {
        QTest::addColumn<bool>("touch");
        QTest::addColumn<bool>("mirrored");
        QTest::newRow("mouse") << false << false;
        QTest::newRow("touch") << true << false;
        QTest::newRow("mouse-rtl") << false << true;
        QTest::newRow("touch-rtl") << true << true;
    }
    void dragCancelsCommand() {
        QFETCH(bool, touch);
        QFETCH(bool, mirrored);
        create();
        QVERIFY(m_view);
        m_view->setMirrored(mirrored);
        const auto point = [mirrored](int x) {
            return QPoint(mirrored ? 320 - x : x, 100);
        };
        auto* first = item(0);
        auto* flick = qobject_cast<QQuickFlickable*>(m_view->flickable());
        QVERIFY(first && flick);
        QSignalSpy clicks(first, &CommandButton::clicked),
            forwarded(m_view, &CarouselView::clicked);
        InteractionAccumulator feedback(this);
        feedback.setSource(first->interactionSource());
        if (touch) {
            static auto* device  = QTest::createTouchDevice();
            auto         pointer = QTest::touchEvent(m_window.get(), device, false);
            pointer.press(0, point(250), m_window.get()).commit();
            flushPointerEvents();
            pointer.move(0, point(190), m_window.get()).commit();
            flushPointerEvents();
            pointer.move(0, point(70), m_window.get()).commit();
            flushPointerEvents();
            QTRY_VERIFY(flick->isDragging());
            pointer.move(0, point(30), m_window.get()).commit();
            flushPointerEvents();
            QVERIFY(flick->contentX() > 0);
            pointer.release(0, point(30), m_window.get()).commit();
            flushPointerEvents();
        } else {
            QTest::mousePress(m_window.get(), Qt::LeftButton, Qt::NoModifier, point(250));
            QTest::mouseMove(m_window.get(), point(190));
            QTest::mouseMove(m_window.get(), point(70));
            QTRY_VERIFY(flick->isDragging());
            QTest::mouseMove(m_window.get(), point(30));
            flushPointerEvents();
            QVERIFY(flick->contentX() > 0);
            QTest::mouseRelease(m_window.get(), Qt::LeftButton, Qt::NoModifier, point(30));
        }
        flushInteractions();
        QVERIFY(clicks.isEmpty() && forwarded.isEmpty() && ! feedback.values().pressed);
        flick->cancelFlick();
    }
    void forwardedIndexAfterScroll() {
        create();
        QVERIFY(m_view);
        m_view->setCurrentIndex(3);
        auto* target = item(3);
        QVERIFY(target && target->isVisible());
        QSignalSpy forwarded(m_view, &CarouselView::clicked);
        QTest::mouseClick(m_window.get(), Qt::LeftButton, Qt::NoModifier, { 100, 100 });
        QCOMPARE(forwarded.size(), 1);
        QCOMPARE(forwarded.first().first().toInt(), 3);
    }
    void rebuildInClickDoesNotForwardOldItem() {
        create();
        QVERIFY(m_view);
        m_view->setProperty("rebuildOnClick", true);
        QSignalSpy forwarded(m_view, &CarouselView::clicked);
        item(0)->click();
        QCOMPARE(m_view->count(), 5);
        QVERIFY(forwarded.isEmpty());
        m_view->setProperty("rebuildOnClick", false);
        QVERIFY(item(0));
        item(0)->click();
        QCOMPARE(forwarded.size(), 1);
    }
    void rebuildIsAtomic_data() {
        QTest::addColumn<bool>("reenter");
        QTest::newRow("retired-sibling") << false;
        QTest::newRow("new-model") << true;
    }
    void rebuildIsAtomic() {
        QFETCH(bool, reenter);
        create();
        QVERIFY(m_view);
        m_view->setLayout(CarouselLayoutId::Uncontained);
        m_view->setItemExtent(140);
        QPointer<CarouselItem> first = item(0), second = item(1);
        QVERIFY(first && second && second->isVisible());
        QSignalSpy clicks(second, &CommandButton::clicked),
            forwarded(m_view, &CarouselView::clicked);
        bool called = false;
        connect(first, &QQuickItem::visibleChanged, this, [&] {
            if (called || first->isVisible()) return;
            called = true;
            second->click();
            if (reenter) m_view->setModel(7);
        });
        m_view->setModel(4);
        QVERIFY(called && clicks.isEmpty() && forwarded.isEmpty());
        QCOMPARE(m_view->count(), reenter ? 7 : 4);
        QVERIFY(item(0));
        item(0)->click();
        QCOMPARE(forwarded.size(), 1);
    }
    void rebuildDuringKeyboard() {
        create();
        QVERIFY(m_view);
        auto* first = item(0);
        QVERIFY(first);
        first->forceActiveFocus(Qt::TabFocusReason);
        QVERIFY(attached(first)->focusRingVisible());
        connect(attached(first), &CarouselAttached::focusRingChanged, this, [&] {
            m_view->setModel(5);
        });
        QTest::keyPress(m_window.get(), Qt::Key_Right);
        QCOMPARE(m_view->count(), 5);
        QCOMPARE(m_view->currentIndex(), 0);
        QVERIFY(item(0));
        QVERIFY(! attached(item(0))->focusRingVisible());
    }
    void externallyDeletedItem() {
        create();
        QVERIFY(m_view);
        QPointer<CarouselItem> first = item(0);
        QVERIFY(first);
        delete first.data();
        QVERIFY(! first);
        m_view->setCurrentIndex(0);
        QVERIFY(item(0));
        QSignalSpy forwarded(m_view, &CarouselView::clicked);
        item(0)->click();
        QCOMPARE(forwarded.size(), 1);
    }
    void deletedInCallback_data() {
        QTest::addColumn<int>("notification");
        for (int notification = 0; notification < 6; ++notification)
            QTest::newRow(qPrintable(QString::number(notification))) << notification;
    }
    void deletedInCallback() {
        QFETCH(int, notification);
        create();
        QVERIFY(m_view);
        auto* first = item(0);
        QVERIFY(first);
        first->forceActiveFocus(Qt::TabFocusReason);
        int        calls   = 0;
        const auto destroy = [&] {
            ++calls;
            delete m_view.data();
        };
        if (notification == 0) {
            connect(m_view, &CarouselView::clicked, this, destroy);
            first->click();
        } else if (notification == 1) {
            connect(m_view, &CarouselView::currentIndexChanged, this, destroy);
            QTest::keyPress(m_window.get(), Qt::Key_Right);
        } else if (notification == 2) {
            connect(attached(first), &CarouselAttached::geometryChanged, this, destroy);
            m_view->setItemExtent(200);
            if (m_view) m_view->setLayout(CarouselLayoutId::Uncontained);
        } else if (notification == 3) {
            connect(attached(first), &CarouselAttached::contextChanged, this, destroy);
            m_view->setOrientation(Qt::Vertical);
        } else if (notification == 4) {
            connect(attached(first), &CarouselAttached::focusRingChanged, this, destroy);
            QTest::keyPress(m_window.get(), Qt::Key_Right);
        } else {
            m_view->setReduceMotion(false);
            connect(m_view, &CarouselView::currentIndexChanged, this, destroy);
            QTest::keyPress(m_window.get(), Qt::Key_Right);
            QTRY_VERIFY(! m_view);
        }
        QCOMPARE(calls, 1);
        QVERIFY(! m_view);
    }
};
int run_carousel_input(int argc, char** argv) {
    CarouselInputTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "carousel_input.moc"
