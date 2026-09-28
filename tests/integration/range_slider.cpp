#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <cmath>
#include <limits>
#include "qml_material/control/range_slider.hpp"
#include "qml_material/style/slider_state.hpp"
#include "qml_material/scrollable/flickable.hpp"
using namespace qml_material;

class InputRangeSlider : public RangeSlider {
public:
    using RangeSlider::keyPressEvent;
    using RangeSlider::keyReleaseEvent;
    using RangeSlider::mouseUngrabEvent;
    using RangeSlider::touchUngrabEvent;
    using RangeSlider::wheelEvent;
    void mouse(QEvent::Type type, qreal position) {
        const qreal   visual = vertical() || mirrored() ? 1 - position : position;
        const qreal   axis   = trackStart() + visual * trackLength();
        const QPointF point =
            horizontal() ? QPointF(axis, height() / 2) : QPointF(width() / 2, axis);
        QMouseEvent event(type,
                          point,
                          point,
                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                          Qt::NoModifier);
        if (type == QEvent::MouseButtonPress)
            mousePressEvent(&event);
        else if (type == QEvent::MouseMove)
            mouseMoveEvent(&event);
        else
            mouseReleaseEvent(&event);
    }
};

class RangeSliderTest : public QObject {
    Q_OBJECT
private slots:
    void appearance() {
        InputRangeSlider slider;
        slider.setSize({ 400, 48 });
        slider.setValues(.2, .8);
        RangeSliderState first, second;
        first.setHandleIndex(0);
        second.setHandleIndex(1);
        first.setItem(&slider);
        second.setItem(&slider);
        QCOMPARE(first.handleLineWidth(), 4);
        QCOMPARE(second.handleLineWidth(), 4);
        slider.mouse(QEvent::MouseButtonPress, .2);
        QCOMPARE(first.handleLineWidth(), 2);
        QCOMPARE(second.handleLineWidth(), 4);
        slider.mouse(QEvent::MouseButtonRelease, .2);
        QCOMPARE(first.handleLineWidth(), 4);
        first.setItem(nullptr);
        QCOMPARE(first.item(), nullptr);
    }
    void qmlAppearance() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.RangeSlider {
                width: 400; height: 80; padding: 10
                first.value: 0.2; second.value: 0.8
            }
        )",
                          QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* slider = qobject_cast<RangeSlider*>(object.data());
        QVERIFY(slider);
        for (int mode = 0; mode < 3; ++mode) {
            slider->setLayoutDirection(mode == 1 ? Qt::RightToLeft : Qt::LeftToRight);
            slider->setOrientation(mode == 2 ? Qt::Vertical : Qt::Horizontal);
            for (auto* node : { slider->first(), slider->second() }) {
                QVERIFY(node->handle());
                QCOMPARE(node->handle()->parentItem(), slider);
                const auto center = slider->horizontal()
                                        ? node->handle()->x() + node->handle()->width() / 2
                                        : node->handle()->y() + node->handle()->height() / 2;
                QCOMPARE(center,
                         slider->trackStart() + node->visualPosition() * slider->trackLength());
            }
        }
        for (const auto* type : { "Slider", "SliderM2" }) {
            component.setData(QByteArray("import Qcm.Material as MD\nMD.") + type + " {}", QUrl());
            QScopedPointer<QObject> single(component.create());
            QVERIFY2(single, qPrintable(component.errorString()));
        }
    }
    void ticks() {
        RangeSlider slider;
        slider.setTo(10);
        slider.setStepSize(3);
        QCOMPARE(slider.tickPositions(20), QList<qreal>({ 0, .3, .6, .9, 1 }));
        QCOMPARE(slider.tickPositions(3), QList<qreal>({ 0, .6, 1 }));
        QCOMPARE(slider.tickPositions(0), QList<qreal>());
        slider.setStepSize(1e20);
        QCOMPARE(slider.tickPositions(20), QList<qreal>({ 0, 1 }));
    }
    void pointer_data() {
        QTest::addColumn<bool>("vertical");
        QTest::addColumn<bool>("rtl");
        QTest::newRow("horizontal") << false << false;
        QTest::newRow("rtl") << false << true;
        QTest::newRow("vertical") << true << false;
    }
    void pointer() {
        QFETCH(bool, vertical);
        QFETCH(bool, rtl);
        InputRangeSlider s;
        s.setSize({ 200, 200 });
        s.setValues(.2, .8);
        s.setOrientation(vertical ? Qt::Vertical : Qt::Horizontal);
        s.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        QSignalSpy start(&s, &RangeSlider::interactionStarted);
        QSignalSpy end(&s, &RangeSlider::interactionFinished);
        s.mouse(QEvent::MouseButtonPress, .25);
        QCOMPARE(s.activeHandle(), 0);
        QVERIFY(s.first()->pressed());
        s.mouse(QEvent::MouseMove, .95);
        QCOMPARE(s.first()->value(), .8);
        QCOMPARE(s.second()->value(), .8);
        s.mouse(QEvent::MouseMove, .3);
        QCOMPARE(s.activeHandle(), 0);
        QCOMPARE(s.first()->value(), .3);
        s.mouse(QEvent::MouseButtonRelease, .4);
        QCOMPARE(s.first()->value(), .4);
        QVERIFY(! s.pressed());
        QCOMPARE(start.size(), 1);
        QCOMPARE(end.size(), 1);
        QCOMPARE(end.at(0).at(1).toBool(), false);
        s.mouse(QEvent::MouseButtonPress, .9);
        QCOMPARE(s.activeHandle(), 1);
        s.mouse(QEvent::MouseButtonRelease, 1);
        QCOMPARE(s.second()->value(), 1.);
    }
    void overlap_data() {
        QTest::addColumn<bool>("forward");
        QTest::newRow("first") << false;
        QTest::newRow("second") << true;
    }
    void overlap() {
        QFETCH(bool, forward);
        InputRangeSlider s;
        s.setSize({ 400, 40 });
        s.setValues(.5, .5);
        s.mouse(QEvent::MouseButtonPress, .5);
        QCOMPARE(s.activeHandle(), -1);
        s.mouse(QEvent::MouseMove, forward ? .75 : .25);
        QCOMPARE(s.activeHandle(), forward ? 1 : 0);
        s.mouse(QEvent::MouseMove, forward ? .2 : .8);
        QCOMPARE(s.activeHandle(), forward ? 1 : 0);
        QCOMPARE(s.first()->value(), .5);
        QCOMPARE(s.second()->value(), .5);
        s.mouse(QEvent::MouseButtonRelease, .5);
        QVERIFY(! s.pressed());
    }
    void preview_data() {
        QTest::addColumn<bool>("live");
        QTest::addColumn<int>("snap");
        for (bool live : { false, true })
            for (int snap = 0; snap < 3; ++snap)
                QTest::newRow(qPrintable(QString("%1-%2").arg(live).arg(snap))) << live << snap;
    }
    void preview() {
        QFETCH(bool, live);
        QFETCH(int, snap);
        InputRangeSlider s;
        s.setSize({ 400, 40 });
        s.setStepSize(.1);
        s.setValues(.2, .8);
        s.setLive(live);
        s.setSnapMode(RangeSlider::SnapMode(snap));
        s.mouse(QEvent::MouseButtonPress, .36);
        QCOMPARE(s.first()->value(), live ? .4 : .2);
        QCOMPARE(s.first()->position(), snap == RangeSlider::SnapAlways ? .4 : .36);
        s.mouseUngrabEvent();
        QCOMPARE(s.first()->position(), live ? .4 : .2);
        QVERIFY(! s.pressed());
        s.mouse(QEvent::MouseButtonPress, .46);
        s.mouse(QEvent::MouseButtonRelease, .46);
        QCOMPARE(s.first()->value(), .5);
        QCOMPARE(s.first()->position(), .5);
    }
    void keyboardAndWheel() {
        InputRangeSlider s;
        s.setSize({ 200, 40 });
        s.setValues(.2, .8);
        s.setStepSize(.1);
        auto key = [&](int key, bool release = false) {
            QKeyEvent e(release ? QEvent::KeyRelease : QEvent::KeyPress, key, Qt::NoModifier);
            if (release)
                s.keyReleaseEvent(&e);
            else
                s.keyPressEvent(&e);
            return e.isAccepted();
        };
        QVERIFY(key(Qt::Key_Tab));
        QCOMPARE(s.focusedHandle(), 1);
        QVERIFY(! key(Qt::Key_Tab));
        QVERIFY(key(Qt::Key_Left));
        QCOMPARE(s.second()->value(), .7);
        key(Qt::Key_Left, true);
        QVERIFY(! s.pressed());
        s.setLive(false);
        key(Qt::Key_End);
        QCOMPARE(s.second()->value(), .7);
        QCOMPARE(s.second()->position(), 1.);
        key(Qt::Key_Escape);
        QCOMPARE(s.second()->position(), .7);
        key(Qt::Key_Home);
        key(Qt::Key_Home, true);
        QCOMPARE(s.second()->value(), .2);
        QWheelEvent wheel({ 100, 20 },
                          { 100, 20 },
                          {},
                          { 0, 120 },
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::NoScrollPhase,
                          false);
        s.wheelEvent(&wheel);
        QVERIFY(! wheel.isAccepted());
        s.setWheelEnabled(true);
        s.wheelEvent(&wheel);
        QVERIFY(wheel.isAccepted());
        QCOMPARE(s.second()->value(), .3);
        s.setLayoutDirection(Qt::RightToLeft);
        key(Qt::Key_Right);
        key(Qt::Key_Right, true);
        QCOMPARE(s.second()->value(), .2);
    }
    void cancellationAndReentry() {
        InputRangeSlider s;
        s.setSize({ 200, 40 });
        s.setValues(.2, .8);
        s.setLive(false);
        QSignalSpy end(&s, &RangeSlider::interactionFinished);
        s.mouse(QEvent::MouseButtonPress, .4);
        s.setEnabled(false);
        QCOMPARE(s.first()->position(), .2);
        QVERIFY(! s.keepMouseGrab());
        QCOMPARE(end.size(), 1);
        QVERIFY(end.at(0).at(1).toBool());
        s.setEnabled(true);
        s.mouse(QEvent::MouseButtonPress, .4);
        s.setOrientation(Qt::Vertical);
        QVERIFY(! s.pressed());
        QCOMPARE(s.first()->position(), .2);
        s.setOrientation(Qt::Horizontal);
        auto connection = connect(&s, &RangeSlider::interactionStarted, &s, [&] {
            s.setValues(.1, .9);
        });
        s.mouse(QEvent::MouseButtonPress, .4);
        QVERIFY(! s.pressed());
        QCOMPARE(s.first()->value(), .1);
        disconnect(connection);
        auto doomed = new InputRangeSlider;
        doomed->setSize({ 200, 40 });
        doomed->setValues(.2, .8);
        QPointer<InputRangeSlider> guard(doomed);
        connect(doomed, &RangeSlider::interactionStarted, doomed, [doomed] {
            delete doomed;
        });
        doomed->mouse(QEvent::MouseButtonPress, .3);
        QVERIFY(! guard);
    }
    void handles() {
        InputRangeSlider s;
        s.setSize({ 200, 40 });
        s.setValues(.2, .8);
        auto a = new QQuickItem;
        a->setSize({ 20, 40 });
        auto b = new QQuickItem;
        b->setSize({ 40, 40 });
        s.first()->setHandle(a);
        s.second()->setHandle(b);
        QCOMPARE(s.trackStart(), 10.);
        QCOMPARE(s.trackLength(), 170.);
        QCOMPARE(s.positionAt({ 10, 20 }), 0.);
        QCOMPARE(s.positionAt({ 180, 20 }), 1.);
        s.setLayoutDirection(Qt::RightToLeft);
        QCOMPARE(s.trackStart(), 20.);
        QCOMPARE(s.positionAt({ 20, 20 }), 1.);
        s.mouse(QEvent::MouseButtonPress, .3);
        delete a;
        QVERIFY(! s.pressed());
        QCOMPARE(s.first()->handle(), nullptr);
        s.second()->setHandle(nullptr);
        QCOMPARE(b->parentItem(), nullptr);
        delete b;
    }
    void windowTouch() {
        QQuickWindow window;
        window.resize(400, 200);
        InputRangeSlider s;
        s.setParentItem(window.contentItem());
        s.setSize({ 400, 80 });
        s.setValues(.2, .8);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        static auto device = QTest::createTouchDevice();
        auto        touch  = QTest::touchEvent(&window, device, false);
        auto        flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 80, 40 }, &window).commit();
        flush();
        touch.move(0, { 160, 40 }, &window).commit();
        flush();
        QVERIFY(s.first()->pressed());
        QCOMPARE(s.first()->value(), .4);
        touch.release(0, { 200, 40 }, &window).commit();
        flush();
        QVERIFY(! s.pressed());
        QCOMPARE(s.first()->value(), .5);
        touch.press(0, { 200, 40 }, &window).commit();
        flush();
        touch.move(0, { 201, 100 }, &window).commit();
        flush();
        QVERIFY(! s.keepTouchGrab());
        QCOMPARE(s.first()->value(), .5);
        touch.release(0, { 201, 100 }, &window).commit();
        flush();
        s.setParentItem(nullptr);
    }
    void handleReplacementReentry() {
        InputRangeSlider s;
        s.setSize({ 200, 40 });
        QQuickItem oldHandle, requested, replacement;
        oldHandle.setWidth(20);
        requested.setWidth(30);
        replacement.setWidth(40);
        s.first()->setHandle(&oldHandle);
        connect(&oldHandle, &QQuickItem::parentChanged, &s, [&] {
            if (! oldHandle.parentItem()) s.first()->setHandle(&replacement);
        });
        s.first()->setHandle(&requested);
        QCOMPARE(s.first()->handle(), &replacement);
        QCOMPARE(replacement.parentItem(), &s);
        QCOMPARE(requested.parentItem(), nullptr);
        QSignalSpy geometry(&s, &RangeSlider::trackGeometryChanged);
        replacement.setWidth(60);
        QCOMPARE(geometry.size(), 1);
        QCOMPARE(s.trackStart(), 30.);
        s.first()->setHandle(nullptr);
    }
    void touchInsideScrollable() {
        QQuickWindow window;
        window.resize(400, 300);
        Flickable scroll;
        scroll.setParentItem(window.contentItem());
        scroll.setSize({ 400, 300 });
        scroll.setContentHeight(900);
        scroll.setContentWidth(400);
        scroll.setContentY(100);
        InputRangeSlider s;
        s.setParentItem(scroll.contentItem());
        s.setY(100);
        s.setSize({ 400, 100 });
        s.setValues(.2, .8);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        static auto device = QTest::createTouchDevice();
        auto        touch  = QTest::touchEvent(&window, device, false);
        auto        flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 80, 40 }, &window).commit();
        flush();
        touch.move(0, { 81, 80 }, &window).commit();
        flush();
        touch.move(0, { 82, 120 }, &window).commit();
        flush();
        QVERIFY(scroll.isDragging());
        QVERIFY(! s.pressed());
        QCOMPARE(s.first()->value(), .2);
        touch.release(0, { 82, 120 }, &window).commit();
        flush();
        scroll.setContentY(100);
        touch.press(0, { 80, 40 }, &window).commit();
        flush();
        touch.move(0, { 160, 41 }, &window).commit();
        flush();
        touch.move(0, { 240, 42 }, &window).commit();
        flush();
        QVERIFY(s.pressed());
        QVERIFY(! scroll.isDragging());
        QCOMPARE(s.first()->value(), .6);
        touch.release(0, { 240, 42 }, &window).commit();
        flush();
        s.setParentItem(nullptr);
        scroll.setParentItem(nullptr);
    }
    void windowMouseAndTab() {
        QQuickWindow window;
        window.resize(400, 200);
        InputRangeSlider s;
        s.setParentItem(window.contentItem());
        s.setSize({ 400, 80 });
        s.setValues(.2, .8);
        Control after;
        after.setParentItem(window.contentItem());
        after.setFocusPolicy(Qt::StrongFocus);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 120, 40 });
        QCOMPARE(s.first()->value(), .3);
        QVERIFY(! s.pressed());
        s.forceActiveFocus(Qt::TabFocusReason);
        QCOMPARE(s.focusedHandle(), 0);
        QTest::keyClick(&window, Qt::Key_Tab);
        QVERIFY(s.hasActiveFocus());
        QCOMPARE(s.focusedHandle(), 1);
        QTest::keyClick(&window, Qt::Key_Tab);
        QVERIFY(after.hasActiveFocus());
        s.forceActiveFocus(Qt::BacktabFocusReason);
        QCOMPARE(s.focusedHandle(), 1);
        QTest::keyClick(&window, Qt::Key_Backtab);
        QCOMPARE(s.focusedHandle(), 0);
        after.setParentItem(nullptr);
        s.setParentItem(nullptr);
    }
    void values() {
        RangeSlider s;
        auto        first  = s.first();
        auto        second = s.second();
        s.setTo(100);
        s.setValues(80, 20);
        QCOMPARE(first->value(), 20.);
        QCOMPARE(second->value(), 80.);
        first->setValue(90);
        QCOMPARE(first->value(), 80.);
        QCOMPARE(second->value(), 80.);
        s.setValues(-10, 110);
        QCOMPARE(first->position(), 0.);
        QCOMPARE(second->position(), 1.);
        s.setMinimumRange(30);
        second->setValue(10);
        QCOMPARE(second->value(), 30.);
        first->setValue(20);
        QCOMPARE(first->value(), 0.);
        s.setMinimumRange(200);
        QCOMPARE(s.minimumRange(), 200.);
        QCOMPARE(s.effectiveMinimumRange(), 100.);
        QCOMPARE(second->value(), 100.);
        QCOMPARE(s.first(), first);
        QCOMPARE(s.second(), second);
    }
    void reverseAndZero() {
        RangeSlider s;
        s.setFrom(100);
        s.setTo(0);
        s.setValues(20, 80);
        QCOMPARE(s.first()->value(), 80.);
        QCOMPARE(s.second()->value(), 20.);
        QCOMPARE(s.first()->position(), .2);
        s.setMinimumRange(30);
        s.first()->setValue(0);
        QCOMPARE(s.first()->value(), 50.);
        s.second()->setValue(100);
        QCOMPARE(s.second()->value(), 20.);
        s.setTo(100);
        QCOMPARE(s.first()->value(), 100.);
        QCOMPARE(s.second()->value(), 100.);
        QCOMPARE(s.first()->position(), 0.);
        QCOMPARE(s.second()->position(), 0.);
        QCOMPARE(s.effectiveMinimumRange(), 0.);
    }
    void steps() {
        RangeSlider s;
        s.setTo(10);
        s.setStepSize(3);
        s.setMinimumRange(4);
        QCOMPARE(s.effectiveMinimumRange(), 6.);
        s.setValues(4, 8);
        QCOMPARE(s.first()->value(), 3.);
        QCOMPARE(s.second()->value(), 9.);
        s.first()->setValue(10);
        QCOMPARE(s.first()->value(), 3.);
        s.second()->setValue(10);
        QCOMPARE(s.second()->value(), 10.);
        s.first()->setValue(4);
        QCOMPARE(s.first()->value(), 3.);
        QCOMPARE(s.valueAt(.8), 9.);
        QCOMPARE(s.valueAt(1), 10.);
        s.setStepSize(20);
        QCOMPARE(s.first()->value(), 0.);
        QCOMPARE(s.second()->value(), 10.);
    }
    void finiteInputs() {
        RangeSlider s;
        const auto  nan = std::numeric_limits<qreal>::quiet_NaN();
        const auto  inf = std::numeric_limits<qreal>::infinity();
        s.setFrom(nan);
        s.setTo(inf);
        s.setStepSize(-1);
        s.setMinimumRange(-1);
        s.setValues(nan, .5);
        s.first()->setValue(inf);
        QCOMPARE(s.from(), 0.);
        QCOMPARE(s.to(), 1.);
        QCOMPARE(s.stepSize(), 0.);
        QCOMPARE(s.minimumRange(), 0.);
        QCOMPARE(s.first()->value(), 0.);
        QCOMPARE(s.second()->value(), 1.);
        QCOMPARE(s.valueAt(nan), 0.);
        s.setFrom(-std::numeric_limits<qreal>::max());
        s.setTo(std::numeric_limits<qreal>::max());
        QCOMPARE(s.to(), 1.);
        QVERIFY(std::isfinite(s.first()->position()));
    }
    void notifications() {
        RangeSlider s;
        QSignalSpy  second(s.second(), &RangeSliderNode::valueChanged);
        connect(s.first(), &RangeSliderNode::valueChanged, &s, [&] {
            QCOMPARE(s.first()->value(), .4);
            QCOMPARE(s.second()->value(), .8);
            QCOMPARE(s.second()->position(), .8);
            s.setMinimumRange(.3);
        });
        s.setValues(.4, .8);
        QCOMPARE(second.size(), 1);
        QCOMPARE(s.effectiveMinimumRange(), .3);
        auto                  doomed = new RangeSlider;
        QPointer<RangeSlider> guard(doomed);
        connect(doomed->first(), &RangeSliderNode::valueChanged, doomed, [doomed] {
            delete doomed;
        });
        doomed->setValues(.2, .7);
        QVERIFY(! guard);
    }
    void nativeBindings() {
        RangeSlider      s;
        QProperty<qreal> total;
        total.setBinding([&] {
            return s.first()->value() + s.second()->value();
        });
        QCOMPARE(total.value(), 1.);
        s.setValues(.2, .6);
        QCOMPARE(total.value(), .8);
    }
    void tinySteps() {
        RangeSlider s;
        s.setMinimumRange(.25);
        s.setStepSize(std::numeric_limits<qreal>::denorm_min());
        QCOMPARE(s.effectiveMinimumRange(), .25);
        s.setValues(.1, .6);
        QCOMPARE(s.first()->value(), .1);
        QCOMPARE(s.second()->value(), .6);
        s.first()->setValue(.5);
        QCOMPARE(s.first()->value(), .35);
        s.setStepSize(1e-20);
        QVERIFY(s.effectiveMinimumRange() >= .25 - 1e-15);
        QVERIFY(std::isfinite(s.first()->value()));
        s.setMinimumRange(std::numeric_limits<qreal>::denorm_min());
        s.setStepSize(1);
        QCOMPARE(s.effectiveMinimumRange(), 1.);
    }
    void qmlBindings() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import Qcm.Material as MD
MD.RangeSliderBase {
    property real requested: 20
    property real observed: second.value
    first.value: requested
    second.value: 80
    from: 0
    to: 100
    stepSize: 3
    minimumRange: 10
}
)",
                          QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto s = qobject_cast<RangeSlider*>(object.data());
        QVERIFY(s);
        QCOMPARE(s->first()->value(), 21.);
        QCOMPARE(s->second()->value(), 81.);
        object->setProperty("requested", 95);
        QCOMPARE(s->first()->value(), 69.);
        object->setProperty("requested", 8);
        QCOMPARE(s->first()->value(), 9.);
        s->setValues(3, 60);
        QCOMPARE(object->property("observed").toDouble(), 60.);
    }
    void numericMatrix() {
        for (bool reverse : { false, true }) {
            for (qreal step : { 0., .1, .3, 3., 11. }) {
                for (qreal minimum : { 0., .1, .4, 2., 15. }) {
                    RangeSlider s;
                    s.setFrom(reverse ? 10 : 0);
                    s.setTo(reverse ? 0 : 10);
                    s.setStepSize(step);
                    s.setMinimumRange(minimum);
                    for (int i = 0; i <= 100; ++i) {
                        s.setValues(i * .1, 10 - i * .1);
                        s.first()->setValue(i * .1);
                        s.second()->setValue(10 - i * .1);
                        QVERIFY(s.first()->position() <= s.second()->position());
                        QVERIFY(s.first()->position() >= 0);
                        QVERIFY(s.second()->position() <= 1);
                        QVERIFY(std::abs(s.second()->value() - s.first()->value()) + 1e-12 >=
                                s.effectiveMinimumRange());
                    }
                }
            }
        }
    }
};
int run_range_slider(int argc, char** argv) {
    RangeSliderTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "range_slider.moc"
