#include "qml_material/anim/size_transition.hpp"
#include "qml_material/control/page_stack.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtTest>

using namespace qml_material;

namespace
{
void advance(SizeTransition& transition, int time) {
    transition.findChild<QVariantAnimation*>()->setCurrentTime(time);
}
} // namespace

class PageStackTransitionTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }

    void sizeRetargetAndBounds() {
        SizeTransition transition;
        transition.setDuration(1000);
        transition.setEasing(QEasingCurve::Linear);
        transition.setTargetSize({ 400, 200 });
        QCOMPARE(transition.size(), QSizeF(400, 200));
        QVERIFY(! transition.running());
        transition.setTargetSize({ 400, 600 });
        QVERIFY(transition.running());
        advance(transition, 500);
        QCOMPARE(transition.size(), QSizeF(400, 400));
        transition.setTargetSize({ 400, 100 });
        QCOMPARE(transition.size(), QSizeF(400, 400));
        advance(transition, 500);
        QCOMPARE(transition.size(), QSizeF(400, 250));
        transition.setMaximumSize({ 300, 150 });
        QCOMPARE(transition.size(), QSizeF(300, 150));
        QCOMPARE(transition.targetSize(), QSizeF(400, 100));
        transition.complete();
        QCOMPARE(transition.size(), QSizeF(300, 100));
        QVERIFY(! transition.running());
        transition.setMaximumSize({ -1, -1 });
        QVERIFY(transition.running());
        transition.complete();
        QCOMPARE(transition.size(), QSizeF(400, 100));
        transition.setTargetSize({ 400, 800 });
        transition.setEnabled(false);
        QCOMPARE(transition.size(), QSizeF(400, 800));
        QVERIFY(! transition.running());
        transition.setEnabled(true);
        transition.setDuration(0);
        transition.setTargetSize({ 400, 50 });
        QCOMPARE(transition.size(), QSizeF(400, 50));
        QVERIFY(! transition.running());
    }

    void sizeInitialization() {
        SizeTransition transition;
        transition.classBegin();
        transition.setTargetSize({ 400, 800 });
        transition.setMaximumSize({ 400, 500 });
        transition.componentComplete();
        QCOMPARE(transition.size(), QSizeF(400, 500));
        QVERIFY(! transition.running());
    }

    void timingChangesAndCompletionReentry() {
        SizeTransition transition;
        transition.setDuration(1000);
        transition.setEasing(QEasingCurve::Linear);
        transition.setTargetSize({ 400, 200 });
        transition.setTargetSize({ 400, 600 });
        advance(transition, 500);
        QCOMPARE(transition.size(), QSizeF(400, 400));
        transition.setDuration(500);
        QCOMPARE(transition.size(), QSizeF(400, 400));
        transition.setEasing(QEasingCurve::InQuad);
        QCOMPARE(transition.size(), QSizeF(400, 400));
        connect(&transition, &SizeTransition::sizeChanged, &transition, [&] {
            if (transition.size().height() == 600) transition.setTargetSize({ 400, 100 });
        });
        advance(transition, 500);
        QVERIFY(transition.running());
        QCOMPARE(transition.targetSize(), QSizeF(400, 100));
        transition.complete();
        QCOMPARE(transition.size(), QSizeF(400, 100));
        QVERIFY(! transition.running());
    }

    void navigationRetargetsFromPresentedSize() {
        QQuickItem host;
        QQuickItem small(&host), large(&host), other(&host);
        for (auto* item : { &small, &large, &other }) item->setImplicitWidth(300);
        small.setImplicitHeight(200);
        large.setImplicitHeight(800);
        other.setImplicitHeight(100);
        PageStack stack;
        stack.setWidth(400);
        SizeTransition transition;
        transition.setDuration(1000);
        transition.setEasing(QEasingCurve::Linear);
        stack.setSizeTransition(&transition);
        connect(&stack, &Control::implicitContentHeightChanged, &transition, [&] {
            transition.setTargetSize({ stack.width(), stack.implicitContentHeight() });
        });
        connect(&transition, &SizeTransition::sizeChanged, &stack, [&] {
            stack.setHeight(transition.size().height());
        });
        QVERIFY(stack.pushItem(&small, PageStack::Immediate));
        QCOMPARE(stack.height(), 200.0);
        QVERIFY(stack.pushItem(&large));
        QCOMPARE(stack.implicitContentHeight(), 800.0);
        QCOMPARE(stack.height(), 200.0);
        QVERIFY(stack.busy());
        advance(transition, 500);
        QCOMPARE(stack.height(), 500.0);
        QVERIFY(stack.pushItem(&other));
        QCOMPARE(stack.height(), 500.0);
        QCOMPARE(stack.implicitContentHeight(), 100.0);
        advance(transition, 500);
        QCOMPARE(stack.height(), 300.0);
        stack.completeTransition();
        QCOMPARE(stack.height(), 100.0);
        QVERIFY(! stack.busy());
        small.setImplicitHeight(50);
        QCOMPARE(stack.implicitContentHeight(), 100.0);
        other.setImplicitHeight(400);
        QCOMPARE(stack.implicitContentHeight(), 400.0);
        QVERIFY(transition.running());
        stack.completeTransition();
        QCOMPARE(stack.height(), 400.0);
        QSignalSpy removed(qmlAttachedPropertiesObject<PageStack>(&other), SIGNAL(removed()));
        QVERIFY(stack.popCurrentItem());
        QCOMPARE(removed.count(), 0);
        transition.complete();
        QTRY_VERIFY(! stack.busy());
        QCOMPARE(removed.count(), 1);
        QCOMPARE(other.parentItem(), &host);
        QVERIFY(stack.popCurrentItem(PageStack::Immediate));
        QCOMPARE(stack.height(), 50.0);
        QVERIFY(! transition.running());
        QVERIFY(! stack.busy());
        QVERIFY(stack.replaceCurrentItem(&other, PageStack::Immediate));
        QCOMPARE(stack.height(), 400.0);
        QCOMPARE(stack.depth(), 1);
        QVERIFY(stack.clear());
        QCOMPARE(stack.implicitContentHeight(), 0.0);
        QVERIFY(stack.busy());
        stack.completeTransition();
        QCOMPARE(stack.height(), 0.0);
        QCOMPARE(removed.count(), 2);
    }

    void waitsForSizeAndPageMotions_data() {
        QTest::addColumn<int>("pageDuration");
        QTest::newRow("size-finishes-last") << 0;
        QTest::newRow("page-finishes-last") << 1000000;
    }
    void waitsForSizeAndPageMotions() {
        QFETCH(int, pageDuration);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.PageStack {
                width: 400
                height: sizeTransition.size.height
                property int pageDuration: 0
                pushEnter: Transition { PauseAnimation { duration: pageDuration } }
                pushExit: Transition { PauseAnimation { duration: pageDuration } }
                sizeTransition: MD.SizeTransition {
                    targetSize: Qt.size(400, implicitContentHeight)
                    duration: 1000000
                }
            }
        )",
                          QUrl("qrc:/tests/stack-resize.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* stack = qobject_cast<PageStack*>(object.get());
        stack->setProperty("pageDuration", pageDuration);
        QQuickItem first, second;
        first.setImplicitHeight(200);
        second.setImplicitHeight(600);
        QVERIFY(stack->pushItem(&first, PageStack::Immediate));
        QVERIFY(stack->pushItem(&second));
        QVERIFY(stack->busy());
        QCoreApplication::processEvents();
        QVERIFY(stack->busy());
        stack->sizeTransition()->complete();
        if (pageDuration == 0) {
            QTRY_VERIFY(! stack->busy());
        } else {
            QCoreApplication::processEvents();
            QVERIFY(stack->busy());
            stack->completeTransition();
        }
        QVERIFY(! stack->busy());
        QCOMPARE(stack->height(), 600.0);
        QCOMPARE(qobject_cast<PageStackAttached*>(qmlAttachedPropertiesObject<PageStack>(&second))
                     ->status(),
                 PageStack::Active);
    }

    void destructionDoesNotLeaveBusy() {
        PageStack  stack;
        QQuickItem first;
        first.setImplicitHeight(200);
        auto* second = new QQuickItem;
        second->setImplicitHeight(600);
        auto* transition = new SizeTransition;
        transition->setDuration(1000000);
        stack.setSizeTransition(transition);
        connect(&stack, &Control::implicitContentHeightChanged, transition, [&] {
            transition->setTargetSize({ 400, stack.implicitContentHeight() });
        });
        QVERIFY(stack.pushItem(&first, PageStack::Immediate));
        QVERIFY(stack.pushItem(second));
        QVERIFY(stack.busy());
        delete transition;
        QTRY_VERIFY(! stack.busy());
        QCOMPARE(stack.sizeTransition(), nullptr);
        SizeTransition replacement;
        replacement.setTargetSize({ 400, 600 });
        replacement.setTargetSize({ 400, 200 });
        stack.setSizeTransition(&replacement);
        QVERIFY(stack.popCurrentItem());
        QVERIFY(stack.busy());
        delete second;
        QTRY_VERIFY(! stack.busy());
        QCOMPARE(stack.currentItem(), &first);
        stack.completeTransition();
        QVERIFY(! replacement.running());
    }
};

int run_page_stack_transition(int argc, char** argv) {
    PageStackTransitionTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "page_stack_transition.moc"
