#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QDir>
#include "../input_test.hpp"
#include "qml_material/anim/ripple_motion.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/style/button_state.hpp"

using namespace qml_material;
namespace
{
class InputControl : public QQuickItem {
public:
    explicit InputControl(QQuickItem* parent = nullptr)
        : QQuickItem(parent), source(this), feedback(&source, this) {}
    InteractionSource      source;
    TestInteractionEmitter feedback;
};
QAbstractAnimation* clock(RippleMotion& motion) { return motion.findChild<QAbstractAnimation*>(); }
void                advance(RippleMotion& motion, int delta) {
    auto* animation = clock(motion);
    animation->setCurrentTime(animation->currentTime() + delta);
}
} // namespace
class RippleMotionTest : public QObject {
    Q_OBJECT
private slots:
    void queuedShortPress() {
        CommandButton button;
        button.setSize({ 100, 40 });
        RippleMotion motion;
        motion.setInteractionSource(button.interactionSource());
        QSignalSpy    clicks(&button, &CommandButton::clicked);
        QSignalSpy    events(button.interactionSource(), &InteractionSource::interaction);
        const QPointF point(20, 10);
        QMouseEvent   press(
            QEvent::MouseButtonPress, point, point, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(
            QEvent::MouseButtonRelease, point, point, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&button, &press);
        QCoreApplication::sendEvent(&button, &release);
        QCOMPARE(clicks.size(), 1);
        QVERIFY(events.isEmpty());
        flushInteractions();
        QCOMPARE(events.size(), 2);
        QCOMPARE(events[0][0].value<Interaction>().id, events[1][0].value<Interaction>().id);
        QVERIFY(motion.rippleOpacity() > 0);
        QCOMPARE(motion.origin(), point);
        advance(motion, 350);
        QCOMPARE(motion.rippleOpacity(), 0);
    }
    void sharedSourceCoordinates() {
        QQuickWindow window;
        window.resize(320, 160);
        QQuickItem first(window.contentItem()), second(window.contentItem()),
            surface(window.contentItem());
        first.setPosition({ 20, 30 });
        second.setPosition({ 120, 40 });
        second.setScale(2);
        surface.setSize({ 300, 120 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        InteractionSource shared;
        auto              a = shared.emitter(&first), b = shared.emitter(&second);
        RippleMotion      motion;
        motion.setTarget(&surface);
        motion.setInteractionSource(&shared);
        const auto old = a->begin(Interaction::Press, Interaction::Mouse, -1, { 3, 4 });
        flushInteractions();
        QCOMPARE(motion.origin(), first.mapToItem(&surface, { 3, 4 }));
        const auto current = b->begin(Interaction::Press, Interaction::Touch, 1, { 5, 6 });
        flushInteractions();
        const auto origin = second.mapToItem(&surface, { 5, 6 });
        QCOMPARE(motion.origin(), origin);
        a->end(old, Interaction::Cancel);
        flushInteractions();
        advance(motion, 450);
        QCOMPARE(motion.progress(), .4);
        b->end(current);
        flushInteractions();
        advance(motion, 350);
        QVERIFY(! motion.running());
        QCOMPARE(motion.origin(), origin);
    }
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }
    void presentationValues() {
        InputControl control;
        RippleMotion motion;
        motion.setInteractionSource(&control.source);
        control.feedback.set(Interaction::Hover, true);
        advance(motion, 15);
        QCOMPARE(motion.opacity(), .08);
        auto values         = motion.indication();
        values.color        = Qt::red;
        values.opacity      = .24;
        values.pressOpacity = .3;
        motion.setIndication(values);
        advance(motion, 15);
        QCOMPARE(motion.opacity(), .24);
        QCOMPARE(motion.ink(), QColor(Qt::red));
        const auto press =
            control.feedback.begin(Interaction::Press, Interaction::Mouse, -1, { 3, 4 });
        QCOMPARE(motion.rippleOpacity(), .3);
        values.pressOpacity = .2;
        motion.setIndication(values);
        QCOMPARE(motion.rippleOpacity(), .2);
        control.feedback.end(press);
        advance(motion, 350);
        motion.resetIndication();
        advance(motion, 15);
        QCOMPARE(motion.opacity(), 0);
        control.feedback.set(Interaction::Hover, false);
        control.feedback.set(Interaction::Hover, true);
        advance(motion, 15);
        QCOMPARE(motion.opacity(), .08);
        QCOMPARE(motion.findChildren<QObject*>(QString(), Qt::FindDirectChildrenOnly).size(), 1);
    }
    void hoverTiming() {
        InputControl control;
        RippleMotion motion;
        motion.setInteractionSource(&control.source);
        control.feedback.set(Interaction::Hover, true);
        QVERIFY(motion.running());
        advance(motion, 5);
        QVERIFY(qAbs(motion.opacity() - .08 / 3) < 1e-9);
        advance(motion, 10);
        QCOMPARE(motion.opacity(), .08);
        QVERIFY(! motion.running());
        control.feedback.set(Interaction::Hover, false);
        advance(motion, 15);
        QCOMPARE(motion.opacity(), 0);
        QVERIFY(! motion.running());
    }
    void orderedRecoveryAndDragTiming() {
        InputControl control;
        RippleMotion motion;
        motion.setInteractionSource(&control.source);
        auto* source = &control.feedback;
        source->set(Interaction::Focus, true);
        advance(motion, 15);
        QVERIFY(qAbs(motion.opacity() - .1 / 3) < 1e-9);
        source->set(Interaction::Hover, true);
        advance(motion, 15);
        QCOMPARE(motion.opacity(), .08);
        source->set(Interaction::Hover, false);
        advance(motion, 45);
        QCOMPARE(motion.opacity(), .1);
        source->set(Interaction::Focus, false);
        advance(motion, 15);
        const auto drag = source->begin(Interaction::Drag);
        advance(motion, 45);
        QCOMPARE(motion.opacity(), .16);
        source->end(drag);
        advance(motion, 75);
        QCOMPARE(motion.opacity(), .08);
        advance(motion, 75);
        QCOMPARE(motion.opacity(), 0);
    }
    void independentPressAndLockedOrigin() {
        InputControl control;
        RippleMotion motion;
        motion.setInteractionSource(&control.source);
        auto* source = &control.feedback;
        source->set(Interaction::Hover, true);
        advance(motion, 15);
        const auto id = source->begin(Interaction::Press, Interaction::Mouse, -1, { 7, 9 });
        QCOMPARE(motion.opacity(), .08);
        QCOMPARE(motion.rippleOpacity(), .1);
        QCOMPARE(motion.origin(), QPointF(7, 9));
        advance(motion, 60);
        QVERIFY(motion.progress() > 0);
        source->end(id, Interaction::End);
        source->set(Interaction::Hover, false);
        advance(motion, 15);
        QCOMPARE(motion.opacity(), 0);
        QVERIFY(motion.rippleOpacity() > 0);
        advance(motion, 350);
        QVERIFY(! motion.running());
        QCOMPARE(motion.rippleOpacity(), 0);
    }
    void immediateReleaseAndRepress() {
        InputControl control;
        RippleMotion motion;
        motion.setInteractionSource(&control.source);
        motion.setColor(Qt::blue);
        auto&      source = control.feedback;
        const auto first  = source.begin(Interaction::Press, Interaction::Mouse, -1, { 5, 6 });
        source.end(first);
        QVERIFY(motion.running());
        advance(motion, 100);
        QVERIFY(motion.progress() > .4);
        const auto second = source.begin(Interaction::Press, Interaction::Mouse, -1, { 5, 6 });
        QCOMPARE(motion.progress(), 0);
        source.begin(Interaction::Hover, Interaction::Mouse, -1, { 90, 80 });
        QCOMPARE(motion.origin(), QPointF(5, 6));
        advance(motion, 450);
        QCOMPARE(motion.progress(), .4);
        source.end(second);
        advance(motion, 350);
        QVERIFY(! motion.running());
    }
    void sourceReplacement() {
        InputControl first, second;
        RippleMotion motion;
        motion.setInteractionSource(&first.source);
        QCOMPARE(motion.metaObject()->indexOfProperty("pressed"), -1);
        QCOMPARE(motion.metaObject()->indexOfProperty("pressPoint"), -1);
        QCOMPARE(motion.rippleOpacity(), 0);
        const auto old = first.feedback.begin(Interaction::Press, Interaction::Mouse, -1, {});
        QVERIFY(motion.rippleOpacity() > 0);
        motion.setInteractionSource(&second.source);
        QCOMPARE(motion.rippleOpacity(), 0);
        const auto next = second.feedback.begin(Interaction::Press, Interaction::Mouse, -1, {});
        first.feedback.end(old, Interaction::End);
        advance(motion, 450);
        QCOMPARE(motion.progress(), .4);
        second.feedback.end(next, Interaction::Cancel);
        advance(motion, 350);
        QVERIFY(! motion.running());
    }
    void noHistoricalPress() {
        InputControl control;
        control.feedback.set(Interaction::Hover, true);
        control.feedback.begin(Interaction::Press, Interaction::Mouse, -1, {});
        RippleMotion motion;
        motion.setInteractionSource(&control.source);
        QCOMPARE(motion.opacity(), 0);
        QCOMPARE(motion.rippleOpacity(), 0);
        QVERIFY(! motion.running());
        motion.setEnabled(false);
        control.feedback.set(Interaction::Hover, false);
        QCOMPARE(motion.opacity(), 0);
        motion.setEnabled(true);
        QCOMPARE(motion.rippleOpacity(), 0);
    }
    void deletedInValueObserver() {
        InputControl           control;
        auto*                  motion = new RippleMotion;
        QPointer<RippleMotion> guard(motion);
        motion->setInteractionSource(&control.source);
        connect(motion, &RippleMotion::valueChanged, motion, [motion] {
            delete motion;
        });
        control.feedback.begin(Interaction::Press, Interaction::Mouse, -1, {});
        QVERIFY(! guard);
    }
    void targetDestroyedAndDisabled() {
        QQuickWindow window;
        window.resize(300, 100);
        InputControl control(window.contentItem());
        auto*        surface = new QQuickItem(window.contentItem());
        surface->setSize({ 100, 40 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        RippleMotion motion;
        motion.setTarget(surface);
        motion.setInteractionSource(&control.source);
        auto& source = control.feedback;
        auto  press  = source.begin(Interaction::Press, Interaction::Mouse, -1, {});
        QVERIFY(motion.running());
        surface->setVisible(false);
        QVERIFY(! motion.running());
        surface->setVisible(true);
        QCOMPARE(motion.rippleOpacity(), 0);
        source.end(press);
        press = source.begin(Interaction::Press, Interaction::Mouse, -1, {});
        QVERIFY(motion.running());
        delete surface;
        QVERIFY(! motion.running());
        source.end(press);
        source.begin(Interaction::Press, Interaction::Mouse, -1, {});
        QVERIFY(! motion.running());
    }
    void windowPressAndCapture() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"QML(
            import QtQuick
            import Qcm.Material as MD
            Rectangle {
                width: 320; height: 120; color: '#f6f2f7'
                MD.Button {
                    objectName: "button"
                    x: 32; y: 32; width: 200; height: 56
                    text: "State"; hoverEnabled: true
                }
            }
        )QML",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto*        root = qobject_cast<QQuickItem*>(object.get());
        QQuickWindow window;
        window.resize(320, 120);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* button = root->findChild<ButtonControl*>("button");
        QVERIFY(button);
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto* motion = button->findChild<RippleMotion*>();
        QVERIFY(motion);
        QTest::mouseMove(&window, QPoint(90, 55), 0);
        QTRY_VERIFY(button->findChild<ButtonState*>()->interactions().hovered);
        auto* state = button->findChild<ButtonState*>();
        QVERIFY(state);
        QCOMPARE(state->indication().opacity, .08);
        QCOMPARE(motion->indication().opacity, .08);
        QTRY_COMPARE(motion->opacity(), .08);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(90, 55), 0);
        QTRY_VERIFY(state->interactions().pressed);
        QVERIFY(motion->rippleOpacity() > 0);
        QCOMPARE(motion->opacity(), .08);
        const auto origin = motion->origin();
        clock(*motion)->pause();
        advance(*motion, 160);
        const auto directory = qEnvironmentVariable("QM_RIPPLE_CAPTURE_DIR");
        if (! directory.isEmpty()) {
            const auto frame = window.grabWindow();
            QVERIFY(! frame.isNull());
            QVERIFY(QDir().mkpath(directory));
            QVERIFY(frame.save(directory + "/hover-press.png"));
        }
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(90, 55), 0);
        flushInteractions();
        advance(*motion, 350);
        QCOMPARE(motion->rippleOpacity(), 0);
        QCOMPARE(motion->opacity(), .08);
        QCOMPARE(motion->origin(), origin);
        QVERIFY(! motion->running());
        button->setFocus(false);
        button->forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(button->focusVisible());
        QTest::mouseMove(&window, QPoint(300, 100), 0);
        QTRY_VERIFY(! button->findChild<ButtonState*>()->interactions().hovered);
        QTRY_COMPARE(motion->opacity(), 0);
        QCOMPARE(motion->rippleOpacity(), 0);
    }
    void qmlRenderers_data() {
        QTest::addColumn<QString>("type");
        QTest::newRow("skia") << QStringLiteral("RippleSkia");
        QTest::newRow("shape") << QStringLiteral("RippleShape");
    }
    void exampleSources() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine, QUrl("qrc:/Qcm/Material/Example/ShapeEffects.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QQuickWindow window;
        window.resize(1200, 1200);
        root->setParentItem(window.contentItem());
        root->setSize(window.size());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        int count = 0;
        for (auto* motion : root->findChildren<RippleMotion*>()) {
            auto* source = motion->interactionSource();
            if (! source || ! source->parent() || source->parent()->objectName() != "rippleInput")
                continue;
            QVERIFY(motion->indication().available);
            ++count;
        }
        QCOMPARE(count, 6);
    }
    void carouselClick() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"QML(
            import QtQuick
            import Qcm.Material as MD
            MD.Carousel {
                id: root
                width: 600; height: 220; model: 3; itemExtent: 180
                property int clicks: 0
                delegate: MD.CarouselItem {
                    required property var model
                    objectName: 'card' + index
                    onClicked: root.clicks++
                }
            }
        )QML",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto*        root = qobject_cast<QQuickItem*>(object.get());
        QQuickWindow window;
        window.resize(600, 220);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ButtonControl* button = nullptr;
        const auto     find   = [](QQuickItem* item, const auto& self) -> ButtonControl* {
            if (auto* button = qobject_cast<ButtonControl*>(item);
                button && button->objectName() == "card1")
                return button;
            for (auto* child : item->childItems())
                if (auto* button = self(child, self)) return button;
            return nullptr;
        };
        QTRY_VERIFY((button = find(root, find)));
        const auto x     = button->width() * button->property("maskStart").toReal() +
                           button->property("visibleWidth").toReal() / 2;
        const auto point = button->mapToScene({ x, button->height() / 2 }).toPoint();
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point, 0);
        QTRY_COMPARE(object->property("clicks").toInt(), 1);
    }
    void qmlRenderers() {
        QFETCH(QString, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(("import QtQuick\nimport Qcm.Material as MD\nMD." + type +
                           " { width: 100; height: 40; color: 'red' }")
                              .toUtf8(),
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* motion = object->findChild<RippleMotion*>();
        QVERIFY(motion);
        QVERIFY(! motion->running());
        QCOMPARE(object->metaObject()->indexOfProperty("pressed"), -1);
        QCOMPARE(object->metaObject()->indexOfProperty("pressX"), -1);
        QCOMPARE(object->metaObject()->indexOfProperty("pressY"), -1);
        QVERIFY(! motion->running());
    }
};
int run_ripple_motion(int argc, char** argv) {
    RippleMotionTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "ripple_motion.moc"
