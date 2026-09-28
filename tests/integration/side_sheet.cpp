#include "qml_material/control/side_sheet.hpp"
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include "qml_material/control/popup.hpp"
#include "qml_material/control/adaptive_presenter.hpp"

using namespace qml_material;

class SideSheetTest : public QObject {
    Q_OBJECT
private slots:
    void relocation() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: root
    width: 600; height: 400
    property Item detail: Item { parent: null; property int value: 42 }
    property MD.AdaptivePresenter coordinator: MD.AdaptivePresenter {
        objectName: "presenter"; content: root.detail
    }
    MD.SideSheet {
        anchors.fill: parent; expanded: true; animationsEnabled: false
        MD.PresentationSite {
            objectName: "embedded"; anchors.fill: parent; presenter: root.coordinator
        }
    }
    MD.SideSheetDialog {
        id: dialog
        objectName: "dialog"; animationsEnabled: false
        MD.PresentationSite {
            objectName: "modalSite"; anchors.fill: parent
            presenter: root.coordinator; popup: dialog
        }
    }
})",
                          QUrl("qrc:/tests/side-sheet-relocation.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(600, 400);
        qobject_cast<QQuickItem*>(object.data())->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* presenter = object->findChild<AdaptivePresenter*>("presenter");
        auto* embedded  = object->findChild<PresentationSite*>("embedded");
        auto* modal     = object->findChild<PresentationSite*>("modalSite");
        auto* dialog    = object->findChild<Popup*>("dialog");
        QVERIFY(presenter && embedded && modal && dialog);
        presenter->setDestination(embedded);
        QTRY_COMPARE(presenter->currentSite(), embedded);
        auto* content = presenter->content();
        presenter->setDestination(modal);
        QTRY_COMPARE(presenter->status(), AdaptivePresenter::Open);
        QCOMPARE(presenter->currentSite(), modal);
        QCOMPARE(content->width(), 256);
        presenter->setDestination(embedded);
        QTRY_COMPARE(presenter->currentSite(), embedded);
        QTRY_VERIFY(! dialog->isVisible());
        QCOMPARE(presenter->content(), content);
        QCOMPARE(content->property("value").toInt(), 42);
    }
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|TypeError|ReferenceError|Unable to assign).*"));
    }
    void input_data() {
        QTest::addColumn<bool>("touch");
        QTest::newRow("mouse") << false;
        QTest::newRow("touch") << true;
    }
    void input() {
        QFETCH(bool, touch);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.SideSheet {
    id: sheetRoot
    width: 600; height: 400; expanded: true; animationsEnabled: false
    property int clicks: 0
    ListView {
        objectName: "list"; anchors.fill: parent; model: 30
        delegate: MD.Button {
            width: 256; height: 60; text: "Click"
            onClicked: sheetRoot.clicks++
        }
    }
})",
                          QUrl("qrc:/tests/side-sheet-input.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto*        sheet = qobject_cast<SideSheet*>(object.data());
        QQuickWindow window;
        window.resize(600, 400);
        sheet->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* device = QTest::createTouchDevice();
        auto  send   = [&](QEvent::Type type, QPoint position, ulong time) {
            if (touch) {
                auto sequence = QTest::touchEvent(&window, device, false);
                if (type == QEvent::MouseButtonPress)
                    sequence.press(0, position);
                else if (type == QEvent::MouseButtonRelease)
                    sequence.release(0, position);
                else
                    sequence.move(0, position);
                sequence.commit();
                QQuickWindowPrivate::get(&window)
                    ->deliveryAgentPrivate()
                    ->flushFrameSynchronousEvents(&window);
            } else {
                QMouseEvent event(type,
                                  position,
                                  window.mapToGlobal(position),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                                  type == QEvent::MouseButtonRelease ? Qt::NoButton
                                                                     : Qt::LeftButton,
                                  Qt::NoModifier);
                event.setTimestamp(time);
                QCoreApplication::sendEvent(&window, &event);
            }
        };
        auto* list = object->findChild<QQuickItem*>("list");
        QVERIFY(list);
        QTRY_VERIFY(list->property("currentItem").value<QQuickItem*>());
        QCOMPARE(list->size(), QSizeF(256, 400));
        QCOMPARE(list->mapToScene(QPointF()), QPointF(344, 0));
        send(QEvent::MouseButtonPress, { 400, 30 }, 900);
        send(QEvent::MouseButtonRelease, { 400, 30 }, 920);
        QCOMPARE(sheet->property("clicks").toInt(), 1);
        send(QEvent::MouseButtonPress, { 400, 200 }, 1000);
        send(QEvent::MouseMove, { 401, 160 }, 1020);
        send(QEvent::MouseMove, { 402, 100 }, 1040);
        send(QEvent::MouseMove, { 403, 60 }, 1050);
        QVERIFY(! sheet->dragging());
        QVERIFY(list->property("contentY").toReal() > 0);
        send(QEvent::MouseButtonRelease, { 402, 100 }, 1060);
        QMetaObject::invokeMethod(list, "cancelFlick");
        send(QEvent::MouseButtonPress, { 400, 200 }, 1100);
        send(QEvent::MouseMove, { 440, 201 }, 1120);
        send(QEvent::MouseMove, { 490, 202 }, 1140);
        QVERIFY(sheet->dragging());
        QVERIFY(sheet->position() < 1);
        window.hide();
        QCoreApplication::processEvents();
        QVERIFY(! sheet->dragging());
        QCOMPARE(sheet->position(), 1);
    }
    void qml() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.SideSheet {
    width: 600; height: 400; expanded: true; animationsEnabled: false
    coplanar: true
    mainContent: Rectangle { color: "blue" }
    Rectangle { objectName: "child"; anchors.fill: parent; color: "red" }
})",
                          QUrl("qrc:/tests/side-sheet.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* sheet = qobject_cast<SideSheet*>(object.data());
        QVERIFY(sheet);
        QCOMPARE(sheet->position(), 1);
        QCOMPARE(sheet->state(), SideSheet::Expanded);
        auto* child = object->findChild<QQuickItem*>("child");
        QVERIFY(child);
        QCOMPARE(child->width(), 256);
        sheet->close();
        QCOMPARE(sheet->state(), SideSheet::Hidden);
        sheet->open();
        QCOMPARE(sheet->state(), SideSheet::Expanded);
    }
    void modal() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    width: 600; height: 400
    MD.SideSheetDialog {
        objectName: "dialog"; animationsEnabled: false
        Rectangle { anchors.fill: parent; color: "red" }
    }
})",
                          QUrl("qrc:/tests/side-sheet-modal.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(600, 400);
        qobject_cast<QQuickItem*>(object.data())->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* dialog = object->findChild<Popup*>("dialog");
        QVERIFY(dialog);
        dialog->open();
        QVERIFY(dialog->isOpened());
        QCOMPARE(dialog->popupItem()->width(), 256);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(30, 100));
        QTRY_VERIFY(! dialog->isVisible());
        dialog->open();
        QVERIFY(dialog->isOpened());
        QTest::keyClick(&window, Qt::Key_Escape);
        QTRY_VERIFY(! dialog->isVisible());
        auto* animation = dialog->findChild<QQuickNumberAnimation*>();
        QVERIFY(animation);
        dialog->setProperty("animationsEnabled", true);
        dialog->open();
        QVERIFY(dialog->entering());
        animation->setCurrentTime(animation->duration() / 2);
        const auto position = dialog->property("position").toReal();
        QVERIFY(position > 0 && position < 1);
        dialog->close();
        QVERIFY(dialog->closing());
        QCOMPARE(dialog->property("position").toReal(), position);
        dialog->setProperty("edge", SideSheet::Left);
        dialog->open();
        animation->setCurrentTime(animation->duration());
        QVERIFY(dialog->isOpened());
        auto* sheet = dialog->findChild<SideSheet*>();
        QVERIFY(sheet);
        QCOMPARE(sheet->effectiveEdge(), Qt::RightEdge);
        QVERIFY(sheet->beginDrag());
        sheet->setPosition(0.2);
        sheet->releaseDrag({ 600, 0 });
        QVERIFY(dialog->closing());
        QVERIFY(dialog->isVisible());
        animation->setCurrentTime(animation->duration());
        QVERIFY(! dialog->isVisible());
        dialog->open();
        QCOMPARE(sheet->effectiveEdge(), Qt::LeftEdge);
        animation->setCurrentTime(animation->duration());
        dialog->close();
        animation->setCurrentTime(animation->duration());
    }
    void geometry_data() {
        QTest::addColumn<bool>("left");
        QTest::addColumn<bool>("detached");
        for (bool left : { false, true })
            for (bool detached : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(left).arg(detached)))
                    << left << detached;
    }
    void geometry() {
        QFETCH(bool, left);
        QFETCH(bool, detached);
        SideSheet sheet;
        sheet.setSize({ 600, 400 });
        sheet.setEdge(left ? SideSheet::Left : SideSheet::Right);
        sheet.setDetached(detached);
        auto* main = new QQuickItem;
        sheet.setContentItem(main);
        sheet.setCoplanar(true);
        QCOMPARE(main->width(), 600);
        QCOMPARE(sheet.occupiedWidth(), 0);
        QVERIFY(! sheet.beginDrag());
        QSignalSpy transitions(&sheet, &SideSheet::transitionRequested);
        sheet.open();
        QCOMPARE(sheet.state(), SideSheet::Settling);
        sheet.setPosition(0.5);
        const qreal extent = 256 + (detached ? 16 : 0);
        QCOMPARE(sheet.occupiedWidth(), extent * 0.5);
        QCOMPARE(main->width(), 600 - extent * 0.5);
        QCOMPARE(main->x(), left ? extent * 0.5 : 0);
        sheet.completeTransition(transitions.last().at(0).toUInt());
        QCOMPARE(sheet.state(), SideSheet::Expanded);
        QCOMPARE(sheet.sheetItem()->width(), 256);
        QCOMPARE(sheet.sheetItem()->height(), detached ? 368 : 400);
        QCOMPARE(main->width(), 600 - extent);
        sheet.setCoplanar(false);
        QCOMPARE(main->width(), 600);
        sheet.close();
        sheet.completeTransition(transitions.last().at(0).toUInt());
        QCOMPARE(sheet.state(), SideSheet::Hidden);
        QCOMPARE(sheet.position(), 0);
    }
    void release_data() {
        QTest::addColumn<bool>("left");
        QTest::addColumn<qreal>("position");
        QTest::addColumn<QPointF>("velocity");
        QTest::addColumn<bool>("opens");
        for (bool left : { false, true }) {
            const qreal outward = left ? 1 : -1;
            QTest::newRow(left ? "left-reverse" : "right-reverse")
                << left << 0.2 << QPointF(outward, 0) << true;
            QTest::newRow(left ? "left-half" : "right-half") << left << 0.5 << QPointF() << true;
            QTest::newRow(left ? "left-close" : "right-close") << left << 0.4 << QPointF() << false;
            QTest::newRow(left ? "left-fast" : "right-fast")
                << left << 0.9 << QPointF(-501 * outward, 0) << false;
            QTest::newRow(left ? "left-limit" : "right-limit")
                << left << 0.9 << QPointF(-500 * outward, 0) << true;
            QTest::newRow(left ? "left-vertical" : "right-vertical")
                << left << 0.9 << QPointF(-600 * outward, 700) << true;
        }
    }
    void release() {
        QFETCH(bool, left);
        QFETCH(qreal, position);
        QFETCH(QPointF, velocity);
        QFETCH(bool, opens);
        SideSheet sheet;
        sheet.setSize({ 600, 400 });
        sheet.setEdge(left ? SideSheet::Left : SideSheet::Right);
        QSignalSpy transitions(&sheet, &SideSheet::transitionRequested);
        sheet.open();
        sheet.completeTransition(transitions.last().at(0).toUInt());
        QVERIFY(sheet.beginDrag());
        sheet.setPosition(position);
        sheet.releaseDrag(velocity);
        QCOMPARE(sheet.expanded(), opens);
        QCOMPARE(sheet.state(), SideSheet::Settling);
        sheet.completeTransition(transitions.last().at(0).toUInt());
        QCOMPARE(sheet.state(), opens ? SideSheet::Expanded : SideSheet::Hidden);
    }
    void interruption() {
        SideSheet sheet;
        sheet.setSize({ 600, 400 });
        QSignalSpy transitions(&sheet, &SideSheet::transitionRequested);
        QSignalSpy opened(&sheet, &SideSheet::opened);
        sheet.open();
        const auto previous = transitions.last().at(0).toUInt();
        sheet.close();
        sheet.completeTransition(previous);
        QCOMPARE(opened.size(), 0);
        sheet.completeTransition(transitions.last().at(0).toUInt());
        QCOMPARE(sheet.state(), SideSheet::Hidden);
        sheet.open();
        sheet.completeTransition(transitions.last().at(0).toUInt());
        QVERIFY(sheet.beginDrag());
        sheet.dragBy({ 100, 0 });
        QVERIFY(sheet.position() < 1);
        sheet.cancelDrag();
        sheet.completeTransition(transitions.last().at(0).toUInt());
        QCOMPARE(sheet.position(), 1);
        sheet.setLayoutDirection(Qt::RightToLeft);
        QCOMPARE(sheet.effectiveEdge(), Qt::LeftEdge);
        sheet.setDetached(true);
        sheet.setWidth(100);
        QCOMPARE(sheet.effectiveSheetWidth(), 68);
    }
};

int run_side_sheet(int argc, char** argv) {
    SideSheetTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "side_sheet.moc"
