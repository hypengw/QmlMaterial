#include <QtTest>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "../input_test.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/style/text_field_state.hpp"

using namespace qml_material;
using qmtest::events;
using qmtest::mouse;

class TextFieldInputTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }

    void textFieldNativePointerEditing() {
        QQuickWindow window;
        window.resize(400, 100);
        TextField field(window.contentItem());
        field.setSize({ 350, 60 });
        field.setText(QStringLiteral("hello world"));
        field.setSelectByMouse(true);
        QFont font;
        font.setPixelSize(24);
        field.setFont(font);
        QSignalSpy             source(field.interactionSource(), &InteractionSource::interaction);
        InteractionAccumulator state(this);
        state.setSource(field.interactionSource());
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        const auto at = [&](int index) {
            return field.positionToRectangle(index).center().toPoint();
        };
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, at(1));
        QCOMPARE(field.cursorPosition(), 1);
        QVERIFY(field.hasActiveFocus());
        QTRY_VERIFY(state.values().pressed);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, at(1));
        QTRY_VERIFY(! state.values().pressed);
        auto presses = events(source, Interaction::Press);
        QCOMPARE(presses.size(), 2);
        QCOMPARE(presses[0].id, presses[1].id);
        QCOMPARE(presses[1].phase, Interaction::End);

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, at(0));
        QTRY_VERIFY(state.values().pressed);
        QTest::mouseMove(&window, at(5));
        QTRY_COMPARE(field.selectedText(), QStringLiteral("hello"));
        QTRY_VERIFY(! state.values().pressed);
        QCOMPARE(events(source, Interaction::Press).back().phase, Interaction::Cancel);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, at(5));

        QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, at(8));
        QCOMPARE(field.selectedText(), QStringLiteral("world"));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, at(8));
        QTRY_VERIFY(! state.values().pressed);
        field.setReadOnly(true);
        field.setCursorPosition(0);
        QTest::mousePress(&window, Qt::LeftButton, Qt::ShiftModifier, at(5));
        QCOMPARE(field.selectedText(), QStringLiteral("hello"));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::ShiftModifier, at(5));
        QTest::keyClick(&window, Qt::Key_A);
        QCOMPARE(field.text(), QStringLiteral("hello world"));
        QCOMPARE(field.metaObject()->indexOfSignal("pressed(QQuickMouseEvent*)"), -1);
        QCOMPARE(field.metaObject()->indexOfSignal("released(QQuickMouseEvent*)"), -1);
        QCOMPARE(field.metaObject()->indexOfSignal("pressAndHold(QQuickMouseEvent*)"), -1);
    }

    void textFieldPressCancellation_data() {
        QTest::addColumn<int>("reason");
        QTest::newRow("ungrab") << 0;
        QTest::newRow("disabled") << 1;
        QTest::newRow("hidden") << 2;
        QTest::newRow("source") << 3;
        QTest::newRow("destroyed") << 4;
        QTest::newRow("outside") << 5;
        QTest::newRow("detached") << 6;
    }

    void textFieldPressCancellation() {
        QFETCH(int, reason);
        InteractionSource shared, replacement;
        QSignalSpy        source(&shared, &InteractionSource::interaction);
        QSignalSpy        next(&replacement, &InteractionSource::interaction);
        QQuickWindow      window;
        window.resize(300, 100);
        auto field = std::make_unique<TextField>(window.contentItem());
        field->setSize({ 200, 50 });
        field->setInteractionSource(&shared);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 10, 10 });
        QTRY_COMPARE(events(source, Interaction::Press).size(), 1);
        switch (reason) {
        case 0: field->ungrabMouse(); break;
        case 1: field->setEnabled(false); break;
        case 2: field->setVisible(false); break;
        case 3: field->setInteractionSource(&replacement); break;
        case 4: field.reset(); break;
        case 5: mouse(*field, QEvent::MouseMove, { 250, 10 }); break;
        case 6: field->setParentItem(nullptr); break;
        }
        QTRY_COMPARE(events(source, Interaction::Press).size(), 2);
        const auto presses = events(source, Interaction::Press);
        QCOMPARE(presses[1].phase, Interaction::Cancel);
        QCOMPARE(presses[0].id, presses[1].id);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 10, 10 });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(events(source, Interaction::Press).size(), 2);
        QVERIFY(events(next, Interaction::Press).isEmpty());
    }

    void textFieldTouchScroll() {
        QQuickWindow window;
        window.resize(300, 200);
        Flickable view(window.contentItem());
        view.setSize({ 300, 200 });
        view.setContentWidth(300);
        view.setContentHeight(800);
        view.setFlickableDirection(Flickable::VerticalFlick);
        TextField field(view.contentItem());
        field.setSize({ 280, 180 });
        field.setText(QStringLiteral("hello world"));
        field.setCursorPosition(0);
        field.setSelectByMouse(true);
        QSignalSpy source(field.interactionSource(), &InteractionSource::interaction);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        static auto* device = QTest::createTouchDevice();
        auto         touch  = QTest::touchEvent(&window, device, false);
        const auto   flush  = [&] {
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        };
        touch.press(0, { 200, 150 }, &window).commit();
        flush();
        QTRY_COMPARE(events(source, Interaction::Press).size(), 1);
        QCOMPARE(events(source, Interaction::Press).front().device, Interaction::Touch);
        QCOMPARE(field.cursorPosition(), 0);
        touch.move(0, { 200, 100 }, &window).commit();
        flush();
        touch.move(0, { 200, 40 }, &window).commit();
        flush();
        QTRY_VERIFY(view.isDragging());
        QTRY_COMPARE(events(source, Interaction::Press).size(), 2);
        QCOMPARE(events(source, Interaction::Press).back().phase, Interaction::Cancel);
        touch.release(0, { 200, 40 }, &window).commit();
        flush();
        QCOMPARE(field.cursorPosition(), 0);
        QVERIFY(field.selectedText().isEmpty());
        view.cancelFlick();
        view.setContentY(0);
        touch.press(0, { 200, 40 }, &window).commit();
        flush();
        touch.release(0, { 200, 40 }, &window).commit();
        flush();
        QCOMPARE(field.cursorPosition(), field.text().size());
        QTRY_COMPARE(events(source, Interaction::Press).size(), 4);
        const auto presses = events(source, Interaction::Press);
        QCOMPARE(presses[2].id, presses[3].id);
        QCOMPARE(presses[3].phase, Interaction::End);
    }

    void textFieldFocusAndEditing() {
        QQuickWindow window;
        window.resize(300, 100);
        TextField field(window.contentItem());
        field.setSize({ 200, 50 });
        InteractionAccumulator interactions(this);
        interactions.setSource(field.interactionSource());
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        field.forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(field.hasActiveFocus());
        QTRY_VERIFY(interactions.values().focused);
        QTest::keyClick(&window, Qt::Key_A);
        QCOMPARE(field.text(), QStringLiteral("a"));
        field.setFocus(false);
        QTRY_VERIFY(! interactions.values().focused);
    }

    void textFieldSourceDrivesAppearance() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"QML(
            import Qcm.Material as MD
            MD.TextField {
                width: 240; hoverEnabled: true
                placeholderText: "Name"
            }
        )QML",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* field = qobject_cast<TextField*>(object.get());
        QVERIFY(field);
        auto* state = field->findChild<TextFieldState*>();
        QVERIFY(state);
        QCOMPARE(state->interactionSource(), field->interactionSource());
        QCOMPARE(field->metaObject()->indexOfProperty("hovered"), -1);
        QCOMPARE(field->metaObject()->indexOfProperty("inputInteractions"), -1);
        QQuickWindow window;
        window.resize(320, 120);
        field->setParentItem(window.contentItem());
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::mouseMove(&window, QPoint(300, 100));
        field->forceActiveFocus(Qt::TabFocusReason);
        QTRY_COMPARE(state->indicatorHeight(), 2);

        auto replacement = std::make_unique<InteractionSource>();
        field->setInteractionSource(replacement.get());
        QCOMPARE(state->interactionSource(), replacement.get());
        QCOMPARE(state->indicatorHeight(), 1);
        QVERIFY(field->hasActiveFocus());
        field->setFocus(false);
        field->forceActiveFocus(Qt::TabFocusReason);
        QTRY_COMPARE(state->indicatorHeight(), 2);
        QTest::keyClick(&window, Qt::Key_A);
        QTest::keyClick(&window, Qt::Key_Space);
        QCOMPARE(field->text(), QStringLiteral("a "));
        QInputMethodEvent composition(QStringLiteral("ni"), {});
        QCoreApplication::sendEvent(field, &composition);
        QCOMPARE(field->preeditText(), QStringLiteral("ni"));
        QInputMethodEvent commit;
        commit.setCommitString(QStringLiteral("\u4f60"));
        QCoreApplication::sendEvent(field, &commit);
        QCOMPARE(field->text(), QStringLiteral("a \u4f60"));
        QSignalSpy accepted(field, &QQuickTextInput::accepted);
        QTest::keyClick(&window, Qt::Key_Return);
        QCOMPARE(accepted.size(), 1);
        replacement.reset();
        QVERIFY(field->interactionSource());
        QCOMPARE(state->interactionSource(), field->interactionSource());
        QCOMPARE(state->indicatorHeight(), 1);
        field->setFocus(false);
        QTRY_COMPARE(state->indicatorHeight(), 1);
    }

    void textFieldSharedHoverLifetime() {
        InteractionSource      shared;
        InteractionAccumulator interactions(this);
        interactions.setSource(&shared);
        TextField first;
        auto      second = std::make_unique<TextField>();
        for (auto* field : { &first, second.get() }) {
            field->setInteractionSource(&shared);
            field->setHoverEnabled(true);
            QHoverEvent hover(QEvent::HoverEnter, QPointF(10, 10), QPointF(10, 10), QPointF());
            QCoreApplication::sendEvent(field, &hover);
        }
        QTRY_COMPARE(interactions.active().size(), 2);
        first.setHoverEnabled(false);
        QTRY_COMPARE(interactions.active().size(), 1);
        QCOMPARE(interactions.active().front().originItem(), second.get());
        first.setEnabled(false);
        QVERIFY(interactions.values().hovered);
        second.reset();
        QTRY_VERIFY(! interactions.values().hovered);
        QVERIFY(interactions.active().isEmpty());
    }

    void textFieldWindowActivation() {
        QQuickWindow window;
        window.resize(300, 100);
        TextField field(window.contentItem());
        field.setSize({ 240, 56 });
        InteractionAccumulator interactions(this);
        interactions.setSource(field.interactionSource());
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        field.forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(interactions.values().focused);
        QQuickWindow other;
        other.resize(200, 100);
        other.setTransientParent(&window);
        other.show();
        other.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&other));
        QTRY_COMPARE(interactions.values().focused, field.hasActiveFocus());
        other.close();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTRY_VERIFY(field.hasActiveFocus());
        QTRY_VERIFY(interactions.values().focused);
    }

    void textFieldStateIndependentSource() {
        TextField         field;
        InteractionSource source;
        auto              producer = source.emitter();
        auto              state    = std::make_unique<TextFieldState>();
        state->setItem(&field);
        state->setInteractionSource(&source);
        const auto focus = producer->begin(Interaction::Focus);
        QTRY_COMPARE(state->indicatorHeight(), 2);
        QVERIFY(! field.hasFocus());
        producer->end(focus);
        QTRY_COMPARE(state->indicatorHeight(), 1);
        connect(state.get(), &TextFieldState::indicatorHeightChanged, this, [&] {
            state.reset();
        });
        producer->begin(Interaction::Focus);
        QTRY_VERIFY(! state);
    }

    void focusedTextFieldDestruction() {
        QQuickWindow window;
        window.resize(300, 100);
        auto* field = new TextField(window.contentItem());
        field->setSize({ 200, 50 });
        InteractionAccumulator state(this);
        state.setSource(field->interactionSource());
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        field->forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(field->hasActiveFocus());
        QTRY_VERIFY(state.values().focused);
        delete field;
        QCOMPARE(state.source(), nullptr);
        QVERIFY(! state.values().focused);
    }
};

int run_text_field_input(int argc, char** argv) {
    TextFieldInputTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "text_field_input.moc"
