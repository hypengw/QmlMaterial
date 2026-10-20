#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <limits>
#include "qml_material/control/range_slider.hpp"

using namespace qml_material;
namespace
{
class TestRange : public RangeSlider {
public:
    explicit TestRange(QQuickItem* parent = nullptr): RangeSlider(parent) {
        setSize({ 400, 80 });
        setValues(.2, .8);
        componentComplete();
    }
};
void mouse(RangeSlider& item, QEvent::Type type, qreal position) {
    const qreal   visual = item.vertical() || item.mirrored() ? 1 - position : position;
    const qreal   axis   = item.trackStart() + visual * item.trackLength();
    const QPointF point =
        item.horizontal() ? QPointF(axis, item.height() / 2) : QPointF(item.width() / 2, axis);
    QMouseEvent event(type,
                      point,
                      point,
                      type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(&item, &event);
}
bool key(RangeSlider& item, QEvent::Type type, int code) {
    QKeyEvent event(type, code, Qt::NoModifier);
    QCoreApplication::sendEvent(&item, &event);
    return event.isAccepted();
}
bool wheel(RangeSlider& item, int amount) {
    QWheelEvent event({ 20, 20 },
                      { 20, 20 },
                      {},
                      { 0, amount },
                      Qt::NoButton,
                      Qt::NoModifier,
                      Qt::NoScrollPhase,
                      false);
    QCoreApplication::sendEvent(&item, &event);
    return event.isAccepted();
}
void acceptRequests(RangeSlider& item) {
    QObject::connect(&item,
                     &RangeSlider::valuesChangeRequested,
                     &item,
                     qOverload<const SliderRange&>(&RangeSlider::setValues));
}
SliderRange request(const QSignalSpy& spy, int index) {
    return qvariant_cast<SliderRange>(spy[index][0]);
}
} // namespace

class RangeSliderRequestsTest : public QObject {
    Q_OBJECT
private slots:
    void requestsDoNotCommit() {
        TestRange  slider;
        QSignalSpy requested(&slider, &RangeSlider::valuesChangeRequested);
        QSignalSpy changed(&slider, &RangeSlider::valuesChanged);
        slider.first()->requestValue(.4);
        slider.first()->requestValue(.4);
        slider.second()->decrease();
        slider.first()->increase();
        slider.requestValues({ .1, .9 });
        QCOMPARE(requested.size(), 5);
        QCOMPARE(request(requested, 0), (SliderRange { .4, .8 }));
        QCOMPARE(request(requested, 1), (SliderRange { .4, .8 }));
        QCOMPARE(request(requested, 2).second, .7);
        QCOMPARE(request(requested, 3).first, .3);
        QCOMPARE(request(requested, 4), (SliderRange { .1, .9 }));
        QCOMPARE(slider.values(), (SliderRange { .2, .8 }));
        QVERIFY(changed.isEmpty());
        QCOMPARE(slider.first()->metaObject()->indexOfSignal("moved()"), -1);
        acceptRequests(slider);
        slider.requestValues({ .3, .7 });
        QCOMPARE(slider.values(), (SliderRange { .3, .7 }));
        QCOMPARE(changed.size(), 1);
        slider.setValues(.1, .9);
        QCOMPARE(requested.size(), 6);
    }
    void constraints_data() {
        QTest::addColumn<bool>("reverse");
        QTest::newRow("forward") << false;
        QTest::newRow("reverse") << true;
    }
    void constraints() {
        QFETCH(bool, reverse);
        TestRange slider;
        slider.setFrom(reverse ? 100 : 0);
        slider.setTo(reverse ? 0 : 100);
        slider.setStepSize(10);
        slider.setMinimumRange(25);
        slider.setValues(reverse ? 80 : 20, reverse ? 20 : 80);
        QCOMPARE(slider.effectiveMinimumRange(), 30);
        QSignalSpy requested(&slider, &RangeSlider::valuesChangeRequested);
        slider.first()->requestValue(reverse ? -20 : 120);
        slider.second()->requestValue(reverse ? 120 : -20);
        QCOMPARE(request(requested, 0), (SliderRange { 50, reverse ? 20. : 80. }));
        QCOMPARE(request(requested, 1), (SliderRange { reverse ? 80. : 20., 50 }));
        slider.requestValues({ 120, -20 });
        QCOMPARE(request(requested, 2), (SliderRange { reverse ? 100. : 0., reverse ? 0. : 100. }));
        slider.requestValues({ std::numeric_limits<qreal>::quiet_NaN(), 50 });
        slider.first()->requestValue(std::numeric_limits<qreal>::infinity());
        slider.setEnabled(false);
        slider.requestValues({ 30, 70 });
        slider.setEnabled(true);
        slider.setVisible(false);
        slider.first()->increase();
        QCOMPARE(requested.size(), 3);
    }
    void preview_data() {
        QTest::addColumn<bool>("live");
        QTest::addColumn<int>("snap");
        QTest::addColumn<bool>("accept");
        for (bool live : { false, true })
            for (int snap = 0; snap != 3; ++snap)
                for (bool accept : { false, true })
                    QTest::newRow(qPrintable(QString("%1-%2-%3").arg(live).arg(snap).arg(accept)))
                        << live << snap << accept;
    }
    void preview() {
        QFETCH(bool, live);
        QFETCH(int, snap);
        QFETCH(bool, accept);
        TestRange slider;
        slider.setLive(live);
        slider.setStepSize(.1);
        slider.setSnapMode(RangeSlider::SnapMode(snap));
        QSignalSpy requested(&slider, &RangeSlider::valuesChangeRequested);
        QSignalSpy finished(&slider, &RangeSlider::interactionFinished);
        if (accept) acceptRequests(slider);
        mouse(slider, QEvent::MouseButtonPress, .2);
        QCOMPARE(slider.first()->previewValue(), .2);
        mouse(slider, QEvent::MouseMove, .36);
        QCOMPARE(slider.first()->value(), live && accept ? .4 : .2);
        QCOMPARE(slider.first()->position(), live ? (accept ? .4 : .2) : (snap == 1 ? .4 : .36));
        QCOMPARE(slider.first()->previewValue(), ! live || accept ? .4 : .2);
        QCOMPARE(slider.second()->previewValue(), .8);
        QCOMPARE(requested.size(), live ? 1 : 0);
        mouse(slider, QEvent::MouseButtonRelease, .36);
        QCOMPARE(slider.first()->value(), accept ? .4 : .2);
        QCOMPARE(slider.first()->position(), slider.first()->value());
        QCOMPARE(slider.first()->previewValue(), slider.first()->value());
        QCOMPARE(requested.size(), live && ! accept ? 2 : 1);
        QCOMPARE(finished.size(), 1);
        QVERIFY(! finished[0][1].toBool());
    }
    void ownerUpdatesBoth() {
        TestRange  slider;
        QSignalSpy requested(&slider, &RangeSlider::valuesChangeRequested);
        QSignalSpy finished(&slider, &RangeSlider::interactionFinished);
        int        calls = 0;
        connect(&slider, &RangeSlider::valuesChangeRequested, &slider, [&](SliderRange range) {
            slider.setValues(++calls == 1 ? SliderRange { .3, .6 } : range);
        });
        mouse(slider, QEvent::MouseButtonPress, .2);
        mouse(slider, QEvent::MouseMove, .4);
        QCOMPARE(slider.values(), (SliderRange { .3, .6 }));
        QCOMPARE(slider.first()->position(), .3);
        QVERIFY(slider.activeHandle() >= 0);
        QVERIFY(finished.isEmpty());
        mouse(slider, QEvent::MouseMove, .9);
        QCOMPARE(request(requested, 1), (SliderRange { .6, .6 }));
        mouse(slider, QEvent::MouseButtonRelease, .9);
        QCOMPARE(requested.size(), 2);
        QCOMPARE(finished.size(), 1);
        QVERIFY(! finished[0][1].toBool());
    }
    void atomicCommitAndReentry() {
        TestRange  slider;
        int        observations = 0;
        const auto check        = [&] {
            ++observations;
            QCOMPARE(slider.values(), (SliderRange { .7, .9 }));
            QCOMPARE(slider.first()->position(), .7);
            QCOMPARE(slider.second()->position(), .9);
        };
        auto first  = connect(slider.first(), &RangeSliderNode::valueChanged, &slider, check);
        auto second = connect(slider.second(), &RangeSliderNode::valueChanged, &slider, check);
        auto pair   = connect(&slider, &RangeSlider::valuesChanged, &slider, check);
        slider.setValues({ .7, .9 });
        QCOMPARE(observations, 3);
        disconnect(first);
        disconnect(second);
        disconnect(pair);
        connect(&slider, &RangeSlider::valuesChangeRequested, &slider, [&](SliderRange range) {
            if (range.first < .5)
                slider.requestValues({ .6, .8 });
            else
                slider.setValues(range);
        });
        mouse(slider, QEvent::MouseButtonPress, .7);
        mouse(slider, QEvent::MouseButtonRelease, .2);
        QCOMPARE(slider.values(), (SliderRange { .6, .8 }));
        QVERIFY(slider.activeHandle() < 0);
    }
    void keyboardAndWheel() {
        TestRange slider;
        slider.setStepSize(.1);
        slider.setWheelEnabled(true);
        QSignalSpy requested(&slider, &RangeSlider::valuesChangeRequested);
        QSignalSpy finished(&slider, &RangeSlider::interactionFinished);
        QVERIFY(key(slider, QEvent::KeyPress, Qt::Key_Right));
        QVERIFY(key(slider, QEvent::KeyPress, Qt::Key_Left));
        QVERIFY(key(slider, QEvent::KeyRelease, Qt::Key_Right));
        QVERIFY(slider.activeHandle() >= 0);
        QVERIFY(finished.isEmpty());
        QVERIFY(key(slider, QEvent::KeyRelease, Qt::Key_Left));
        QVERIFY(slider.activeHandle() < 0);
        QCOMPARE(finished.size(), 1);
        QCOMPARE(requested.size(), 2);
        QCOMPARE(slider.values(), (SliderRange { .2, .8 }));
        QVERIFY(wheel(slider, 120));
        QCOMPARE(requested.size(), 3);
        QCOMPARE(request(requested, 2).first, .3);
        acceptRequests(slider);
        QVERIFY(wheel(slider, 120));
        QCOMPARE(slider.first()->value(), .3);
        slider.setLayoutDirection(Qt::RightToLeft);
        key(slider, QEvent::KeyPress, Qt::Key_Right);
        key(slider, QEvent::KeyRelease, Qt::Key_Right);
        QCOMPARE(slider.first()->value(), .2);
        slider.setEnabled(false);
        QVERIFY(! wheel(slider, 120));
    }
    void cancellation_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("escape") << 0;
        QTest::newRow("disabled") << 1;
        QTest::newRow("hidden") << 2;
        QTest::newRow("range") << 3;
        QTest::newRow("step") << 4;
        QTest::newRow("minimum") << 5;
        QTest::newRow("orientation") << 6;
        QTest::newRow("handle") << 7;
    }
    void cancellation() {
        QFETCH(int, kind);
        TestRange  slider;
        QQuickItem handle;
        slider.setLive(false);
        QSignalSpy requested(&slider, &RangeSlider::valuesChangeRequested);
        QSignalSpy finished(&slider, &RangeSlider::interactionFinished);
        mouse(slider, QEvent::MouseButtonPress, .2);
        mouse(slider, QEvent::MouseMove, .4);
        QCOMPARE(slider.first()->previewValue(), .4);
        QVERIFY(! wheel(slider, 120));
        switch (kind) {
        case 0: key(slider, QEvent::KeyPress, Qt::Key_Escape); break;
        case 1: slider.setEnabled(false); break;
        case 2: slider.setVisible(false); break;
        case 3: slider.setTo(2); break;
        case 4: slider.setStepSize(.1); break;
        case 5: slider.setMinimumRange(.3); break;
        case 6: slider.setOrientation(Qt::Vertical); break;
        case 7: slider.first()->setHandle(&handle); break;
        }
        mouse(slider, QEvent::MouseButtonRelease, .4);
        QCOMPARE(finished.size(), 1);
        QVERIFY(finished[0][1].toBool());
        QVERIFY(requested.isEmpty());
        QCOMPARE(slider.first()->previewValue(), slider.first()->value());
        QVERIFY(slider.activeHandle() < 0);
    }
    void configureInRequest() {
        TestRange  slider;
        QSignalSpy finished(&slider, &RangeSlider::interactionFinished);
        QSignalSpy requested(&slider, &RangeSlider::valuesChangeRequested);
        connect(&slider, &RangeSlider::valuesChangeRequested, &slider, [&](SliderRange) {
            slider.setMinimumRange(.3);
        });
        mouse(slider, QEvent::MouseButtonPress, .2);
        mouse(slider, QEvent::MouseMove, .4);
        mouse(slider, QEvent::MouseButtonRelease, .5);
        QCOMPARE(requested.size(), 1);
        QCOMPARE(finished.size(), 1);
        QVERIFY(finished[0][1].toBool());
    }
    void deleteInRequest_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("tap") << 0;
        QTest::newRow("drag") << 1;
        QTest::newRow("key") << 2;
        QTest::newRow("wheel") << 3;
        QTest::newRow("node") << 4;
        QTest::newRow("range") << 5;
    }
    void deleteInRequest() {
        QFETCH(int, kind);
        auto slider = std::make_unique<TestRange>();
        slider->setWheelEnabled(true);
        connect(slider.get(), &RangeSlider::valuesChangeRequested, this, [&](SliderRange) {
            slider.reset();
        });
        if (kind < 2) {
            mouse(*slider, QEvent::MouseButtonPress, .2);
            mouse(*slider, kind ? QEvent::MouseMove : QEvent::MouseButtonRelease, .4);
        } else if (kind == 2)
            key(*slider, QEvent::KeyPress, Qt::Key_Right);
        else if (kind == 3)
            wheel(*slider, 120);
        else if (kind == 4)
            slider->first()->increase();
        else
            slider->requestValues({ .3, .7 });
        QVERIFY(! slider);
    }
    void qmlPairBinding() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                property bool accept: false
                property MD.sliderRange committed: ({first: .2, second: .8})
                MD.RangeSlider {
                    objectName: "slider"
                    width: 400; height: 80
                    values: owner.committed
                    minimumRange: .1
                    live: false
                    onValuesChangeRequested: requested => {
                        if (owner.accept) owner.committed = requested;
                    }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* slider = root->findChild<RangeSlider*>("slider");
        QVERIFY(slider);
        slider->first()->requestValue(.4);
        QCOMPARE(slider->values(), (SliderRange { .2, .8 }));
        root->setProperty("accept", true);
        slider->requestValues({ .3, .7 });
        QCOMPARE(slider->values(), (SliderRange { .3, .7 }));
        QCOMPARE(root->property("committed").value<SliderRange>(), slider->values());
        root->setProperty("committed", QVariant::fromValue(SliderRange { .8, 1 }));
        QCOMPARE(slider->values(), (SliderRange { .8, 1 }));
        root->setProperty("committed", QVariant::fromValue(SliderRange { 0, .2 }));
        QCOMPARE(slider->values(), (SliderRange { 0, .2 }));
        for (auto* node : { slider->first(), slider->second() }) {
            QVERIFY(node->handle());
            QCOMPARE(node->handle()->property("value").toReal(), node->value());
        }
    }
    void nativeTouch_data() {
        QTest::addColumn<bool>("live");
        QTest::addColumn<bool>("accept");
        for (bool live : { false, true })
            for (bool accept : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(live).arg(accept))) << live << accept;
    }
    void nativeTouch() {
        QFETCH(bool, live);
        QFETCH(bool, accept);
        QQuickWindow window;
        window.resize(400, 100);
        TestRange slider(window.contentItem());
        slider.setLive(live);
        if (accept) acceptRequests(slider);
        QSignalSpy finished(&slider, &RangeSlider::interactionFinished);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        static auto* device = QTest::createTouchDevice();
        auto         touch  = QTest::touchEvent(&window, device, false);
        const auto   flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 80, 40 }, &window).commit();
        flush();
        touch.move(0, { 160, 40 }, &window).commit();
        flush();
        QCOMPARE(slider.first()->value(), live && accept ? .4 : .2);
        touch.release(0, { 160, 40 }, &window).commit();
        flush();
        QCOMPARE(slider.first()->value(), accept ? .4 : .2);
        QCOMPARE(slider.first()->position(), slider.first()->value());
        QCOMPARE(finished.size(), 1);
        QVERIFY(! finished[0][1].toBool());
    }
};

int run_range_slider_requests(int argc, char** argv) {
    RangeSliderRequestsTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "range_slider_requests.moc"
