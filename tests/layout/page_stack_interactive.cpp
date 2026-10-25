#include "qml_material/control/page_stack.hpp"
#include "qml_material/style/theme.hpp"
#include "qml_material/control/page_container.hpp"
#include "qml_material/util/pool.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtTest>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
using namespace qml_material;
class PageStackInteractiveTest : public QObject {
    Q_OBJECT
private slots:
    void transactionAndSize() {
        QQuickItem host, first(&host), second(&host), third(&host);
        first.setImplicitHeight(200);
        second.setImplicitHeight(600);
        third.setImplicitHeight(100);
        first.setOpacity(.7);
        first.setScale(.8);
        first.setX(13);
        MaterialPageStack stack;
        stack.setSize({ 400, 600 });
        SizeTransition size;
        size.setSpec(Motion::tween(1000));
        size.runner().setManualClock(true);
        stack.setSizeTransition(&size);
        connect(&stack, &PageStack::destinationSizeChanged, &size, [&] {
            size.setTargetSize({ 400, stack.destinationSize().height() });
        });
        QVERIFY(stack.pushItem(&first, PageStack::Immediate));
        QVERIFY(stack.pushItem(&second, PageStack::Immediate));
        QSignalSpy removed(&stack, &PageStack::entryRemoved),
            depth(&stack, &PageStack::depthChanged),
            current(&stack, &PageStack::currentItemChanged);
        auto id = stack.beginPopPreview();
        QVERIFY(! id.isEmpty());
        auto* timeline = stack.currentMotion()->timeline();
        timeline->setManualClock(true);
        QCOMPARE(stack.depth(), 2);
        QCOMPARE(stack.currentItem(), &second);
        QCOMPARE(stack.destinationSize(), QSizeF(0, 200));
        QVERIFY(stack.updatePopPreview(id, .7));
        QCOMPARE(size.size().height(), 320.0);
        first.setImplicitHeight(300);
        QCOMPARE(size.size().height(), 320.0);
        QVERIFY(stack.updatePopPreview(id, .2));
        QCOMPARE(size.size().height(), 520.0);
        QVERIFY(stack.updatePopPreview(id, 1));
        QCOMPARE(stack.depth(), 2);
        QCOMPARE(removed.count(), 0);
        QVERIFY(stack.cancelPopPreview(id));
        timeline->advance(30);
        const auto fraction = timeline->fraction();
        const auto next     = stack.beginPopPreview();
        QVERIFY(! next.isEmpty());
        QVERIFY(next != id);
        QCOMPARE(timeline->fraction(), fraction);
        QVERIFY(! stack.updatePopPreview(id, .4));
        QVERIFY(stack.cancelPopPreview(next));
        timeline->pause();
        QCoreApplication::processEvents();
        QVERIFY(stack.busy());
        QCOMPARE(removed.count(), 0);
        timeline->resume();
        timeline->complete();
        QCoreApplication::processEvents();
        size.complete();
        QTRY_VERIFY(! stack.busy());
        QCOMPARE(stack.depth(), 2);
        QCOMPARE(depth.count(), 0);
        QCOMPARE(current.count(), 0);
        QCOMPARE(removed.count(), 0);
        QCOMPARE(size.size().height(), 600.0);
        id = stack.beginPopPreview();
        QVERIFY(stack.updatePopPreview(id, .5));
        QVERIFY(stack.commitPopPreview(id, 2));
        stack.completeTransition();
        QCOMPARE(stack.depth(), 1);
        QCOMPARE(stack.currentItem(), &first);
        QCOMPARE(removed.count(), 1);
        QCOMPARE(first.opacity(), .7);
        QCOMPARE(first.scale(), .8);
        QCOMPARE(first.x(), 13.0);
        QCOMPARE(size.size().height(), 300.0);
        QCOMPARE(second.parentItem(), &host);
        QVERIFY(stack.pushItem(&second, PageStack::Immediate));
        id = stack.beginPopPreview();
        stack.updatePopPreview(id, .7);
        QVERIFY(stack.replaceCurrentItem(&third, PageStack::Immediate));
        QCOMPARE(stack.depth(), 2);
        QCOMPARE(stack.currentItem(), &third);
        QVERIFY(! stack.commitPopPreview(id));
        QVERIFY(! stack.busy());
    }
    void destructionAndPolicy() {
        MaterialPageStack stack;
        QQuickItem        source;
        auto*             target = new QQuickItem;
        stack.pushItem(target, PageStack::Immediate);
        stack.pushItem(&source, PageStack::Immediate);
        auto id = stack.beginPopPreview();
        QVERIFY(! id.isEmpty());
        stack.updatePopPreview(id, .6);
        delete target;
        QVERIFY(! stack.busy());
        QCOMPARE(stack.currentItem(), &source);
        auto* other = new QQuickItem;
        stack.pushItem(other, PageStack::Immediate);
        id = stack.beginPopPreview();
        stack.updatePopPreview(id, .4);
        auto* theme = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(&stack, true));
        theme->motion()->setReducedMotion(true);
        QVERIFY(! stack.busy());
        QCOMPARE(stack.depth(), 2);
        QCOMPARE(stack.currentItem(), other);
        theme->motion()->setReducedMotion(false);
        id = stack.beginPopPreview();
        delete other;
        QVERIFY(! stack.busy());
        QCOMPARE(stack.currentItem(), &source);
        QCOMPARE(stack.depth(), 1);
    }
    void customMotionAndBindings() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent c(&engine);
        c.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    width: 400; height: 600
    property real base: .6
    MD.PageStack {
        id: stack; objectName: "stack"; anchors.fill: parent
        replaceMotion: Component {
            MD.PageMotion {
                timeline: MD.SeekableTransition {
                    MD.RealTrack { id: progress; values: ({from: 0, to: 1}); spec: MD.Motion.tween(1000) }
                }
                enterOpacity: progress.value
                exitOpacity: 1 - progress.value
                enterOffset: Qt.point(0, (1 - progress.value) * context.viewport.height / 2)
            }
        }
    }
    Item { objectName: "first"; opacity: base; x: base * 10; scale: base }
    Item { objectName: "second"; opacity: base; x: base * 10; scale: base }
})",
                  QUrl());
        std::unique_ptr<QObject> root(c.create());
        QVERIFY2(root, qPrintable(c.errorString()));
        auto* stack  = root->findChild<PageStack*>("stack");
        auto* first  = root->findChild<QQuickItem*>("first");
        auto* second = root->findChild<QQuickItem*>("second");
        stack->pushItem(first, PageStack::Immediate);
        stack->replaceCurrentItem(second);
        auto* timeline = stack->currentMotion()->timeline();
        timeline->setManualClock(true);
        timeline->advance(500);
        QCOMPARE(stack->currentMotion()->enterOpacity(), .5);
        QCOMPARE(stack->currentMotion()->enterOffset(), QPointF(0, 150));
        root->setProperty("base", .8);
        QCOMPARE(second->opacity(), .8);
        QCOMPARE(second->x(), 8.0);
        QCOMPARE(second->scale(), .8);
        stack->completeTransition();
        QCOMPARE(second->opacity(), .8);
        QCOMPARE(first->parentItem(), qobject_cast<QQuickItem*>(root.get()));
    }
    void containerLeases() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.PageContainer {
    width: 400; height: 600
    property Component source: Component { Item {} }
    function load() { switchTo(source, {}, true); }
})",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* stack = qobject_cast<PageContainer*>(root.get());
        QMetaObject::invokeMethod(stack, "load");
        QTRY_VERIFY(stack->currentRequest());
        stack->completeTransition();
        const QPointer<PoolRequest> lease = stack->currentRequest();
        const auto                  key   = stack->currentKey();
        QQuickItem                  extra;
        stack->pushItem(&extra, PageStack::Immediate);
        QCOMPARE(stack->currentRequest(), nullptr);
        QCOMPARE(lease->status(), PoolRequest::Ready);
        auto id = stack->beginPopPreview();
        stack->updatePopPreview(id, .8);
        stack->cancelPopPreview(id);
        stack->completeTransition();
        QCOMPARE(stack->currentRequest(), nullptr);
        QCOMPARE(lease->status(), PoolRequest::Ready);
        id = stack->beginPopPreview();
        stack->commitPopPreview(id);
        stack->completeTransition();
        QCOMPARE(stack->currentRequest(), lease.data());
        QCOMPARE(stack->currentKey(), key);
        stack->clear(PageStack::Immediate);
        QCOMPARE(stack->currentRequest(), nullptr);
        QVERIFY(! lease || lease->status() == PoolRequest::Released);
    }
    void participantCancellation() {
        MaterialPageStack stack;
        QQuickItem        first, second;
        stack.pushItem(&first, PageStack::Immediate);
        stack.pushItem(&second, PageStack::Immediate);
        const auto id = stack.beginPopPreview();
        QVERIFY(! id.isEmpty());
        stack.currentMotion()->timeline()->stop();
        QTRY_VERIFY(! stack.preview());
        QCOMPARE(stack.depth(), 2);
        QVERIFY(! stack.busy());
        SizeTransition original, replacement;
        original.setTargetSize({ 400, 600 });
        stack.setSizeTransition(&original);
        QVERIFY(! stack.beginPopPreview().isEmpty());
        QVERIFY(original.active());
        stack.setSizeTransition(&replacement);
        QVERIFY(! stack.preview());
        QVERIFY(! original.active());
        QCOMPARE(stack.sizeTransition(), &replacement);
        QVERIFY(! stack.beginPopPreview().isEmpty());
        QSignalSpy preview(&stack, &PageStack::previewChanged);
        stack.abortPreview();
        QVERIFY(preview.count() > 0);
    }
    void reentrantDestruction() {
        QQuickItem                  first, second;
        QPointer<MaterialPageStack> stack = new MaterialPageStack;
        stack->pushItem(&first, PageStack::Immediate);
        stack->pushItem(&second, PageStack::Immediate);
        connect(stack, &PageStack::previewChanged, &first, [&] {
            delete stack.data();
        });
        stack->beginPopPreview();
        QVERIFY(! stack);
    }
    void input_data() {
        QTest::addColumn<bool>("touch");
        QTest::addColumn<bool>("rtl");
        for (bool touch : { false, true })
            for (bool rtl : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(touch).arg(rtl))) << touch << rtl;
    }
    void input() {
        QFETCH(bool, touch);
        QFETCH(bool, rtl);
        QQuickWindow window;
        window.resize(400, 600);
        MaterialPageStack stack(window.contentItem());
        stack.setSize({ 400, 600 });
        stack.setBackGestureEnabled(true);
        QQuickItem first, second;
        stack.pushItem(&first, PageStack::Immediate);
        stack.pushItem(&second, PageStack::Immediate);
        stack.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* device = QTest::createTouchDevice();
        auto  send   = [&](QEvent::Type type, QPoint point) {
            if (rtl) point.setX(400 - point.x());
            if (touch) {
                auto sequence = QTest::touchEvent(&window, device, false);
                if (type == QEvent::MouseButtonPress)
                    sequence.press(0, point);
                else if (type == QEvent::MouseButtonRelease)
                    sequence.release(0, point);
                else
                    sequence.move(0, point);
                sequence.commit();
                QQuickWindowPrivate::get(&window)
                    ->deliveryAgentPrivate()
                    ->flushFrameSynchronousEvents(&window);
            } else {
                if (type == QEvent::MouseButtonPress)
                    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
                else if (type == QEvent::MouseButtonRelease)
                    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
                else
                    QTest::mouseMove(&window, point);
            }
        };
        send(QEvent::MouseButtonPress, { 10, 300 });
        send(QEvent::MouseMove, { 12, 360 });
        QVERIFY(! stack.preview());
        send(QEvent::MouseButtonRelease, { 12, 360 });
        send(QEvent::MouseButtonPress, { 10, 300 });
        send(QEvent::MouseMove, { 80, 300 });
        send(QEvent::MouseMove, { 230, 300 });
        QVERIFY(stack.preview());
        QCOMPARE(stack.depth(), 2);
        send(QEvent::MouseMove, { 380, 300 });
        send(QEvent::MouseButtonRelease, { 380, 300 });
        QVERIFY(stack.settling());
        stack.completeTransition();
        QCOMPARE(stack.depth(), 1);
        stack.pushItem(&second, PageStack::Immediate);
        send(QEvent::MouseButtonPress, { 10, 300 });
        send(QEvent::MouseMove, { 80, 300 });
        send(QEvent::MouseMove, { 180, 300 });
        QVERIFY(stack.preview());
        window.hide();
        QCoreApplication::processEvents();
        stack.completeTransition();
        QCOMPARE(stack.depth(), 2);
        QVERIFY(! stack.preview());
    }
};
int run_page_stack_interactive(int argc, char** argv) {
    PageStackInteractiveTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "page_stack_interactive.moc"
