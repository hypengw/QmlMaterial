#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <algorithm>
#include <limits>
#include "../input_test.hpp"
#include "qml_material/control/slider.hpp"

using namespace qml_material;
namespace
{
class TestSlider : public Slider {
public:
    explicit TestSlider(QQuickItem* parent = nullptr): Slider(parent), feedback(this) {
        feedback.setSource(interactionSource());
        setSize({ 200, 40 });
        componentComplete();
    }
    InteractionAccumulator feedback;
};
void mouse(QQuickItem& item, QEvent::Type type, QPointF point) {
    QMouseEvent event(type,
                      point,
                      point,
                      type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(&item, &event);
    flushInteractions();
}
void key(QQuickItem& item, QEvent::Type type, int code) {
    QKeyEvent event(type, code, Qt::NoModifier);
    QCoreApplication::sendEvent(&item, &event);
    flushInteractions();
}
bool wheel(QQuickItem& item, int amount) {
    QWheelEvent event({ 20, 20 },
                      { 20, 20 },
                      {},
                      { 0, amount },
                      Qt::NoButton,
                      Qt::NoModifier,
                      Qt::NoScrollPhase,
                      false);
    QCoreApplication::sendEvent(&item, &event);
    flushInteractions();
    return event.isAccepted();
}
} // namespace
class SliderRequestsTest : public QObject {
    Q_OBJECT
private slots:
    void requestsDoNotCommit() {
        TestSlider slider;
        slider.setValue(.5);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        QSignalSpy changed(&slider, &Slider::valueChanged);
        slider.requestValue(.8);
        slider.requestValue(.8);
        slider.increase();
        slider.decrease();
        QCOMPARE(requests.size(), 4);
        QCOMPARE(requests[0][0].toReal(), .8);
        QCOMPARE(requests[1][0].toReal(), .8);
        QCOMPARE(requests[2][0].toReal(), .6);
        QCOMPARE(requests[3][0].toReal(), .4);
        QCOMPARE(slider.value(), .5);
        QCOMPARE(slider.position(), .5);
        QVERIFY(changed.isEmpty());
        QCOMPARE(slider.metaObject()->indexOfSignal("moved()"), -1);
        connect(&slider, &Slider::valueChangeRequested, &slider, &Slider::setValue);
        slider.increase();
        QCOMPARE(slider.value(), .6);
        slider.setValue(.3);
        QCOMPARE(requests.size(), 5);
    }
    void validationAndAvailability() {
        QQuickWindow window;
        TestSlider   slider(window.contentItem());
        slider.setFrom(100);
        slider.setTo(0);
        slider.setValue(50);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        slider.requestValue(-10);
        slider.requestValue(200);
        QCOMPARE(requests.size(), 2);
        QCOMPARE(requests[0][0].toReal(), 0);
        QCOMPARE(requests[1][0].toReal(), 100);
        slider.requestValue(50);
        slider.requestValue(std::numeric_limits<qreal>::quiet_NaN());
        slider.requestValue(std::numeric_limits<qreal>::infinity());
        slider.setEnabled(false);
        slider.increase();
        slider.setEnabled(true);
        slider.setVisible(false);
        slider.decrease();
        QCOMPARE(requests.size(), 2);
        QCOMPARE(slider.value(), 50);
    }
    void liveUsesCommittedValue() {
        TestSlider slider;
        slider.setValue(.2);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        mouse(slider, QEvent::MouseButtonPress, { 40, 20 });
        mouse(slider, QEvent::MouseMove, { 150, 20 });
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toReal(), .75);
        QCOMPARE(slider.value(), .2);
        QCOMPARE(slider.position(), .2);
        connect(&slider, &Slider::valueChangeRequested, &slider, [&](qreal value) {
            slider.setValue(std::min(value, .6));
        });
        mouse(slider, QEvent::MouseButtonRelease, { 150, 20 });
        QCOMPARE(slider.value(), .6);
        QCOMPARE(slider.position(), .6);
        QVERIFY(! (slider.feedback.values().pressed || slider.feedback.values().dragged));
    }
    void deferredPreview_data() {
        QTest::addColumn<int>("snap");
        QTest::addColumn<bool>("accept");
        for (int mode : { Slider::NoSnap, Slider::SnapAlways, Slider::SnapOnRelease }) {
            QTest::newRow(qPrintable(QString("%1-reject").arg(mode))) << mode << false;
            QTest::newRow(qPrintable(QString("%1-accept").arg(mode))) << mode << true;
        }
    }
    void deferredPreview() {
        QFETCH(int, snap);
        QFETCH(bool, accept);
        TestSlider slider;
        slider.setLive(false);
        slider.setStepSize(.2);
        slider.setSnapMode(Slider::SnapMode(snap));
        slider.setValue(.2);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        QSignalSpy finished(&slider, &Slider::interactionFinished);
        if (accept) connect(&slider, &Slider::valueChangeRequested, &slider, &Slider::setValue);
        mouse(slider, QEvent::MouseButtonPress, { 40, 20 });
        mouse(slider, QEvent::MouseMove, { 150, 20 });
        QVERIFY(requests.isEmpty());
        QCOMPARE(slider.value(), .2);
        QCOMPARE(slider.position(), snap == Slider::SnapAlways ? .8 : .75);
        mouse(slider, QEvent::MouseButtonRelease, { 150, 20 });
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toReal(), .8);
        QCOMPARE(slider.value(), accept ? .8 : .2);
        QCOMPARE(slider.position(), slider.value());
        QCOMPARE(finished.size(), 1);
        QVERIFY(! finished[0][0].toBool());
    }
    void cancelledPreview_data() {
        QTest::addColumn<bool>("escape");
        QTest::newRow("disabled") << false;
        QTest::newRow("escape") << true;
    }
    void cancelledPreview() {
        QFETCH(bool, escape);
        TestSlider slider;
        slider.setLive(false);
        slider.setValue(.2);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        QSignalSpy finished(&slider, &Slider::interactionFinished);
        mouse(slider, QEvent::MouseButtonPress, { 40, 20 });
        mouse(slider, QEvent::MouseMove, { 150, 20 });
        QCOMPARE(slider.position(), .75);
        if (escape)
            key(slider, QEvent::KeyPress, Qt::Key_Escape);
        else
            slider.setEnabled(false);
        QCOMPARE(slider.position(), .2);
        mouse(slider, QEvent::MouseButtonRelease, { 150, 20 });
        QVERIFY(requests.isEmpty());
        QCOMPARE(finished.size(), 1);
        QVERIFY(finished[0][0].toBool());
    }
    void reentrantRequest() {
        TestSlider slider;
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        QSignalSpy finished(&slider, &Slider::interactionFinished);
        connect(&slider, &Slider::valueChangeRequested, &slider, [&](qreal value) {
            if (value > .6) {
                slider.requestValue(.4);
                return;
            }
            slider.setValue(value);
        });
        mouse(slider, QEvent::MouseButtonPress, { 150, 20 });
        mouse(slider, QEvent::MouseButtonRelease, { 150, 20 });
        QCOMPARE(requests.size(), 2);
        QCOMPARE(slider.value(), .4);
        QCOMPARE(slider.position(), .4);
        QCOMPARE(finished.size(), 1);
        QVERIFY(! finished[0][0].toBool());
        QVERIFY(! (slider.feedback.values().pressed || slider.feedback.values().dragged));
    }
    void previewValueUsesOwner() {
        TestSlider slider;
        slider.setLive(false);
        slider.setStepSize(.2);
        slider.setValue(.35);
        QSignalSpy preview(&slider, &Slider::previewValueChanged);
        QCOMPARE(slider.previewValue(), .35);
        mouse(slider, QEvent::MouseButtonPress, { 70, 20 });
        QCOMPARE(slider.previewValue(), .35);
        mouse(slider, QEvent::MouseMove, { 150, 20 });
        QCOMPARE(slider.previewValue(), .8);
        connect(&slider, &Slider::valueChangeRequested, &slider, [&](qreal) {
            slider.setValue(.63);
        });
        mouse(slider, QEvent::MouseButtonRelease, { 150, 20 });
        QCOMPARE(slider.value(), .63);
        QCOMPARE(slider.previewValue(), .63);
        QVERIFY(! preview.isEmpty());
        slider.setStepSize(.6);
        mouse(slider, QEvent::MouseButtonPress, { 126, 20 });
        mouse(slider, QEvent::MouseMove, { 199, 20 });
        QCOMPARE(slider.previewValue(), 1);
        key(slider, QEvent::KeyPress, Qt::Key_Escape);
        QCOMPARE(slider.previewValue(), .63);
    }
    void keyboardAndWheel() {
        TestSlider slider;
        slider.setStepSize(.1);
        slider.setValue(.5);
        slider.setWheelEnabled(true);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        QSignalSpy finished(&slider, &Slider::interactionFinished);
        key(slider, QEvent::KeyPress, Qt::Key_Right);
        key(slider, QEvent::KeyRelease, Qt::Key_Right);
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toReal(), .6);
        QCOMPARE(slider.value(), .5);
        QVERIFY(wheel(slider, 120));
        QCOMPARE(requests.size(), 2);
        QCOMPARE(requests[1][0].toReal(), .6);
        QCOMPARE(finished.size(), 2);
        connect(&slider, &Slider::valueChangeRequested, &slider, &Slider::setValue);
        QVERIFY(wheel(slider, -120));
        QCOMPARE(slider.value(), .4);
        slider.setLayoutDirection(Qt::RightToLeft);
        key(slider, QEvent::KeyPress, Qt::Key_Right);
        key(slider, QEvent::KeyRelease, Qt::Key_Right);
        QCOMPARE(slider.value(), .3);
        slider.setOrientation(Qt::Vertical);
        key(slider, QEvent::KeyPress, Qt::Key_Up);
        key(slider, QEvent::KeyRelease, Qt::Key_Up);
        QCOMPARE(slider.value(), .4);
        slider.setEnabled(false);
        QVERIFY(! wheel(slider, 120));
        QCOMPARE(slider.value(), .4);
    }
    void wheelDoesNotInterruptPreview() {
        TestSlider slider;
        slider.setLive(false);
        slider.setValue(.2);
        slider.setWheelEnabled(true);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        mouse(slider, QEvent::MouseButtonPress, { 40, 20 });
        mouse(slider, QEvent::MouseMove, { 150, 20 });
        QVERIFY(! wheel(slider, 120));
        QVERIFY(requests.isEmpty());
        QCOMPARE(slider.position(), .75);
        mouse(slider, QEvent::MouseButtonRelease, { 150, 20 });
        QCOMPARE(requests.size(), 1);
        QCOMPARE(slider.position(), .2);
    }
    void configurationInRequest_data() {
        QTest::addColumn<int>("change");
        QTest::newRow("range") << 0;
        QTest::newRow("step") << 1;
        QTest::newRow("handle") << 2;
        QTest::newRow("orientation") << 3;
        QTest::newRow("live") << 4;
        QTest::newRow("snap") << 5;
    }
    void configurationInRequest() {
        QFETCH(int, change);
        TestSlider slider;
        QQuickItem handle;
        slider.setValue(.2);
        QSignalSpy finished(&slider, &Slider::interactionFinished);
        QSignalSpy requests(&slider, &Slider::valueChangeRequested);
        connect(&slider, &Slider::valueChangeRequested, &slider, [&](qreal) {
            switch (change) {
            case 0: slider.setFrom(-1); break;
            case 1: slider.setStepSize(.2); break;
            case 2: slider.setHandle(&handle); break;
            case 3: slider.setOrientation(Qt::Vertical); break;
            case 4: slider.setLive(false); break;
            case 5: slider.setSnapMode(Slider::SnapAlways); break;
            }
        });
        mouse(slider, QEvent::MouseButtonPress, { 40, 20 });
        mouse(slider, QEvent::MouseMove, { 150, 20 });
        mouse(slider, QEvent::MouseButtonRelease, { 150, 20 });
        QCOMPARE(requests.size(), 1);
        QCOMPARE(finished.size(), 1);
        QVERIFY(finished[0][0].toBool());
        QCOMPARE(slider.value(), .2);
        QVERIFY(! (slider.feedback.values().pressed || slider.feedback.values().dragged));
    }
    void deleteInRequest_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("tap") << 0;
        QTest::newRow("drag") << 1;
        QTest::newRow("key") << 2;
        QTest::newRow("wheel") << 3;
        QTest::newRow("semantic") << 4;
    }
    void deleteInRequest() {
        QFETCH(int, kind);
        auto slider = std::make_unique<TestSlider>();
        slider->setValue(.2);
        slider->setWheelEnabled(true);
        connect(slider.get(), &Slider::valueChangeRequested, this, [&](qreal) {
            slider.reset();
        });
        if (kind < 2) {
            mouse(*slider, QEvent::MouseButtonPress, { 40, 20 });
            mouse(*slider, kind ? QEvent::MouseMove : QEvent::MouseButtonRelease, { 150, 20 });
        } else if (kind == 2) {
            key(*slider, QEvent::KeyPress, Qt::Key_Right);
        } else if (kind == 3) {
            wheel(*slider, 120);
        } else {
            slider->increase();
        }
        QVERIFY(! slider);
    }
    void qmlOwnerFeedback() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                property bool accept: false
                property real committed: .2
                MD.Slider {
                    objectName: "slider"
                    width: 200; height: 40
                    live: false
                    stepSize: .2
                    value: parent.committed
                    onValueChangeRequested: requestedValue => {
                        if (parent.accept) parent.committed = Math.min(requestedValue, .6);
                    }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* slider = root->findChild<Slider*>("slider");
        QVERIFY(slider);
        slider->requestValue(.8);
        QCOMPARE(slider->value(), .2);
        root->setProperty("accept", true);
        mouse(*slider, QEvent::MouseButtonPress, { 40, 20 });
        mouse(*slider, QEvent::MouseButtonRelease, { 150, 20 });
        QCOMPARE(slider->value(), .6);
        QCOMPARE(slider->position(), .6);
        root->setProperty("committed", .3);
        QCOMPARE(slider->value(), .3);
        QCOMPARE(slider->position(), .3);
        QCOMPARE(slider->previewValue(), .3);
        QVERIFY(slider->handle());
        QCOMPARE(slider->handle()->property("value").toReal(), .3);
    }
};
int run_slider_requests(int argc, char** argv) {
    SliderRequestsTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "slider_requests.moc"
