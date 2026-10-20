#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/combo_box.hpp"
#include "qml_material/control/navigation_rail.hpp"
#include "qml_material/control/menu.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "qml_material/control/range_slider.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/control/action_command.hpp"
#include "qml_material/style/slider_state.hpp"
#include "qml_material/style/combo_box_state.hpp"
#include "../input_test.hpp"

using namespace qml_material;
namespace
{
void mouse(QQuickItem* item, QEvent::Type type, QPointF point) {
    QMouseEvent event(type,
                      point,
                      point,
                      Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(item, &event);
    flushInteractions();
}
} // namespace

class ComboBoxInputTest : public QObject {
    Q_OBJECT

private slots:
    void comboRequests_data() {
        QTest::addColumn<bool>("accept");
        QTest::newRow("reject") << false;
        QTest::newRow("accept") << true;
    }

    void comboRequests() {
        QFETCH(bool, accept);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ComboBox { width: 240; model: ['One', 'Two', 'Three'] }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* combo = qobject_cast<ComboBox*>(object.get());
        QVERIFY(combo);
        QQuickWindow window;
        window.resize(400, 300);
        combo->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(combo->currentIndex(), 0);
        QSignalSpy requests(combo, &ComboBox::selectionChangeRequested);
        if (accept)
            connect(combo,
                    &ComboBox::selectionChangeRequested,
                    combo,
                    [combo](int index, const QVariant&) {
                        combo->setCurrentIndex(index);
                    });
        QKeyEvent down(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
        QCoreApplication::sendEvent(combo, &down);
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toInt(), 1);
        QCOMPARE(requests[0][1].toString(), QString("Two"));
        QCOMPARE(combo->currentIndex(), accept ? 1 : 0);
        combo->openPopup();
        QTRY_VERIFY(combo->popupVisible());
        combo->highlightIndex(2);
        combo->closePopup(true);
        QTRY_VERIFY(! combo->popupVisible());
        QCOMPARE(requests.size(), 2);
        QCOMPARE(combo->currentIndex(), accept ? 2 : 0);
    }

    void comboRetarget() {
        ComboBox combo;
        combo.setSize({ 200, 56 });
        Popup first, second;
        combo.setPopup(&first);
        mouse(&combo, QEvent::MouseButtonPress, { 20, 20 });
        InteractionAccumulator feedback(this);
        feedback.setSource(combo.interactionSource());
        combo.setPopup(&second);
        mouse(&combo, QEvent::MouseButtonRelease, { 20, 20 });
        QVERIFY(! second.isVisible());
        QVERIFY(! feedback.values().pressed);
        auto* transient = new Popup;
        combo.setPopup(transient);
        delete transient;
        QCOMPARE(combo.popup(), nullptr);
    }

    void comboModelOutlivesPopup() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD; MD.ComboBox { model: ['One', 'Two'] }",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* combo = qobject_cast<ComboBox*>(object.get());
        QVERIFY(combo && combo->popup());
        QPointer<QObject> model(combo->delegateModel());
        QVERIFY(model);
        bool modelAliveAtPopupDestruction = false;
        connect(combo->popup(), &QObject::destroyed, this, [&] {
            modelAliveAtPopupDestruction = model;
        });
        object.reset();
        QVERIFY(modelAliveAtPopupDestruction);
        QVERIFY(! model);
    }

    void comboStateOutlivesInput() {
        InteractionSource shared;
        ComboBoxState     state;
        auto*             combo = new ComboBox;
        combo->setInteractionSource(&shared);
        state.setItem(combo);
        auto producer = shared.emitter(combo);
        producer->begin(Interaction::Hover);
        flushInteractions();
        QVERIFY(state.interactions().hovered);
        delete combo;
        QCOMPARE(state.item(), nullptr);
        QCOMPARE(state.interactionSource(), nullptr);
        QVERIFY(! state.interactions().hovered);
        auto other = shared.emitter();
        other->begin(Interaction::Hover);
        flushInteractions();
        QVERIFY(! state.interactions().hovered);
    }

    void comboEditor() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ComboBox { width: 240; editable: true; model: ['One', 'Two', 'Three'] }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* combo  = qobject_cast<ComboBox*>(object.get());
        auto* editor = qobject_cast<TextField*>(combo->contentItem());
        QVERIFY(editor);
        QCOMPARE(editor->interactionSource(), combo->interactionSource());
        QQuickWindow window;
        window.resize(400, 300);
        combo->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        editor->forceActiveFocus(Qt::TabFocusReason);
        InteractionAccumulator feedback(this);
        feedback.setSource(combo->interactionSource());
        mouse(editor, QEvent::MouseButtonPress, { 10, 10 });
        QVERIFY(feedback.values().pressed);
        mouse(editor, QEvent::MouseButtonRelease, { 10, 10 });
        QVERIFY(! feedback.values().pressed);
        QSignalSpy requests(combo, &ComboBox::selectionChangeRequested);
        editor->setText("Two");
        combo->acceptInput();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toInt(), 1);
        QCOMPARE(combo->currentIndex(), 0);
        QInputMethodEvent composing("T", {});
        QCoreApplication::sendEvent(editor, &composing);
        QVERIFY(combo->inputMethodComposing());
        combo->acceptInput();
        QCOMPARE(requests.size(), 1);
        QInputMethodEvent commit;
        commit.setCommitString("wo");
        QCoreApplication::sendEvent(editor, &commit);
        QVERIFY(! combo->inputMethodComposing());
        InteractionSource replacement;
        combo->setInteractionSource(&replacement);
        QCOMPARE(editor->interactionSource(), &replacement);
    }

    void deletedInPopupNotification() {
        auto*              combo = new ComboBox;
        QPointer<ComboBox> guard(combo);
        connect(combo, &ComboBox::popupChanged, combo, [combo] {
            delete combo;
        });
        Popup popup;
        combo->setPopup(&popup);
        QVERIFY(! guard);
    }
};

int run_combo_box_input(int argc, char** argv) {
    ComboBoxInputTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "combo_box_input.moc"
