#include "qml_material/control/split_view.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtTest>
#include <memory>

using namespace qml_material;

class SplitViewTest : public QObject {
    Q_OBJECT
    QQmlEngine   m_engine;
    QQuickWindow m_window;

    std::unique_ptr<SplitView> create(const QByteArray& extra = {}, bool expanded = true) {
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.SplitView {
    id: root
    width: 800; height: 600
    property bool paneEnabled: true
    property bool paneClip: false
    property bool paneVisible: true
    property bool initiallyExpanded: true
    property int completions: 0
    property int presses: 0
    Rectangle {
        objectName: "first"
        implicitWidth: 300; implicitHeight: 240
        MD.SplitViewBase.preferredWidth: 300
        MD.SplitViewBase.preferredHeight: 240
    }
    Rectangle {
        objectName: "second"
        enabled: root.paneEnabled
        clip: root.paneClip
        visible: root.paneVisible
        MD.SplitViewBase.minimumWidth: 200
        MD.SplitViewBase.minimumHeight: 160
        MD.SplitViewBase.fillWidth: true
        MD.SplitViewBase.fillHeight: true
        MD.SplitViewBase.expanded: root.initiallyExpanded
        MD.SplitViewBase.onCollapsedCompleted: ++root.completions
        MouseArea { anchors.fill: parent; onPressed: ++root.presses }
    }
)" + extra + "\n}",
                          QUrl());
        auto* object = qobject_cast<SplitView*>(
            component.createWithInitialProperties({ { "initiallyExpanded", expanded } }));
        if (! object) qWarning() << component.errorString();
        if (object) {
            object->setParentItem(m_window.contentItem());
            object->ensurePolished();
        }
        return std::unique_ptr<SplitView>(object);
    }
    SplitViewAttached* info(QQuickItem* item) {
        return qobject_cast<SplitViewAttached*>(qmlAttachedPropertiesObject<SplitView>(item, true));
    }
    void polish(SplitView* view) {
        view->ensurePolished();
        QCoreApplication::processEvents();
        view->ensurePolished();
    }

private Q_SLOTS:
    void initTestCase() {
        m_engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml_modules");
        m_window.resize(800, 600);
        m_window.show();
    }
    void immediateAndBindings() {
        auto view = create();
        QVERIFY(view);
        QCOMPARE(view->count(), 2);
        auto*      first  = view->itemAt(0);
        auto*      second = view->itemAt(1);
        auto*      state  = info(second);
        QSignalSpy closed(state, &SplitViewAttached::collapsedCompleted);
        QSignalSpy opened(state, &SplitViewAttached::expandedCompleted);
        const auto saved = view->saveState();
        state->setExpanded(false);
        polish(view.get());
        QCOMPARE(closed.count(), 1);
        QCOMPARE(first->width(), 800);
        QVERIFY(! second->isVisible());
        QVERIFY(! second->isEnabled());
        QVERIFY(! second->clip());
        QCOMPARE(view->saveState(), saved);
        state->setExpanded(true);
        polish(view.get());
        QCOMPARE(opened.count(), 1);
        QCOMPARE(first->width(), 300);
        QVERIFY(second->isVisible());
        QVERIFY(second->isEnabled());
        view->setProperty("paneEnabled", false);
        QVERIFY(! second->isEnabled());
        view->setProperty("paneClip", true);
        QVERIFY(second->clip());
        view->setProperty("paneVisible", false);
        polish(view.get());
        QCOMPARE(first->width(), 800);
        QCOMPARE(closed.count(), 1);
        view->setProperty("paneVisible", true);
        polish(view.get());
        QCOMPARE(first->width(), 300);
    }
    void fixedHandleDoesNotResize() {
        auto view = create();
        QVERIFY(view);
        info(view->itemAt(0))->setFillWidth(true);
        auto* second = info(view->itemAt(1));
        second->setFillWidth(false);
        second->setMinimumWidth(280);
        second->setMaximumWidth(280);
        second->setPreferredWidth(280);
        polish(view.get());
        auto* handle = view->handleItemAt(0);
        QVERIFY(handle);
        auto* state = qobject_cast<SplitHandleAttached*>(
            qmlAttachedPropertiesObject<SplitHandleAttached>(handle, true));
        QVERIFY(state);
        QVERIFY(! state->isResizable());
        const auto saved = view->saveState();
        QTest::mousePress(&m_window,
                          Qt::LeftButton,
                          {},
                          handle->mapToScene(handle->boundingRect().center()).toPoint());
        QVERIFY(! view->isResizing());
        QVERIFY(! state->isPressed());
        QTest::mouseRelease(&m_window, Qt::LeftButton);
        QCOMPARE(view->saveState(), saved);
        QCOMPARE(view->itemAt(1)->width(), 280);
        second->setMaximumWidth(500);
        polish(view.get());
        QVERIFY(state->isResizable());
    }
    void initiallyCollapsedAndReentry() {
        auto view = create({}, false);
        QVERIFY(view);
        polish(view.get());
        auto* second = view->itemAt(1);
        auto* state  = info(second);
        QCOMPARE(view->itemAt(0)->width(), 800);
        QVERIFY(! state->isTransitioning());
        QCOMPARE(view->property("completions").toInt(), 0);
        state->setExpanded(true);
        polish(view.get());
        connect(state, &SplitViewAttached::collapsedCompleted, state, [state] {
            state->setExpanded(true);
        });
        state->setExpanded(false);
        polish(view.get());
        polish(view.get());
        QVERIFY(state->isExpanded());
        QVERIFY(second->isVisible());
        QCOMPARE(view->itemAt(0)->width(), 300);
    }
    void multiplePanesAndRemoval() {
        auto view = create(R"(
Rectangle { implicitWidth: 120; implicitHeight: 100 }
)");
        QVERIFY(view);
        QCOMPARE(view->count(), 3);
        auto* first  = view->itemAt(0);
        auto* middle = view->itemAt(1);
        info(first)->setExpanded(false);
        info(middle)->setExpanded(false);
        polish(view.get());
        polish(view.get());
        QCOMPARE(view->itemAt(2)->width(), 800);
        info(first)->setExpanded(true);
        polish(view.get());
        QCOMPARE(first->width(), 300);
        QCOMPARE(view->itemAt(2)->width(), 476);
        delete middle;
        polish(view.get());
        QCOMPARE(view->count(), 2);
        auto* logical = view->itemAt(1);
        view->moveItem(1, 0);
        QCOMPARE(view->itemAt(0), logical);
        auto* detached = view->takeItem(0);
        QCOMPARE(detached, logical);
        view->addItem(detached);
        QCOMPARE(view->count(), 2);
        polish(view.get());
        QCOMPARE(view->itemAt(1), logical);
    }
    void interrupted() {
        auto view = create(R"(
expandTransition: Transition { NumberAnimation { property: "progress"; duration: 100000 } }
collapseTransition: Transition { NumberAnimation { property: "progress"; duration: 100000 } }
)");
        QVERIFY(view);
        auto*      second = view->itemAt(1);
        auto*      state  = info(second);
        QSignalSpy closed(state, &SplitViewAttached::collapsedCompleted);
        QSignalSpy opened(state, &SplitViewAttached::expandedCompleted);
        state->setExpanded(false);
        polish(view.get());
        QVERIFY(state->isTransitioning());
        QVERIFY(! second->isEnabled());
        state->setExpanded(true);
        polish(view.get());
        view->setOrientation(Qt::Vertical);
        polish(view.get());
        QVERIFY(! state->isTransitioning());
        QCOMPARE(closed.count(), 0);
        QCOMPARE(opened.count(), 1);
        QCOMPARE(second->width(), 800);
        state->setExpanded(false);
        view->setVisible(false);
        QVERIFY(! state->isTransitioning());
        QCOMPARE(closed.count(), 0);
        view->setVisible(true);
        polish(view.get());
        QCOMPARE(view->itemAt(0)->height(), 600);
        state->setExpanded(true);
        polish(view.get());
        auto* detached = view->takeItem(1);
        QCOMPARE(view->count(), 1);
        QVERIFY(! state->isTransitioning());
        QVERIFY(detached->isEnabled());
        delete detached;
        polish(view.get());
    }
    void exitReleasesInput() {
        auto view = create(R"(
collapseTransition: Transition { NumberAnimation { property: "progress"; duration: 100000 } }
)");
        QVERIFY(view);
        polish(view.get());
        auto*        second   = view->itemAt(1);
        const QPoint position = second->mapToScene(QPointF(50, 50)).toPoint();
        second->forceActiveFocus();
        QTest::mousePress(&m_window, Qt::LeftButton, Qt::NoModifier, position);
        QCOMPARE(view->property("presses").toInt(), 1);
        info(second)->setExpanded(false);
        QVERIFY(! second->isEnabled());
        QVERIFY(! second->hasActiveFocus());
        QVERIFY(! m_window.mouseGrabberItem());
        QTest::mouseRelease(&m_window, Qt::LeftButton, Qt::NoModifier, position);
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, position);
        QCOMPARE(view->property("presses").toInt(), 1);
    }
    void anotherPaneCompletingKeepsCurrentExtent() {
        auto view = create(R"(
Rectangle { implicitWidth: 120 }
collapseTransition: Transition {
    NumberAnimation { property: "progress"; from: 0.5; to: 0.5; duration: 100000 }
}
)");
        QVERIFY(view);
        polish(view.get());
        auto* middle = view->itemAt(1);
        info(middle)->setExpanded(false);
        QTRY_COMPARE(middle->width(), 166);
        view->setCollapseTransition(nullptr);
        info(view->itemAt(2))->setExpanded(false);
        polish(view.get());
        polish(view.get());
        QCOMPARE(middle->width(), 166);
    }
    void destructionDuringCompletion() {
        auto view = create();
        QVERIFY(view);
        auto* second = view->itemAt(1);
        auto* state  = info(second);
        connect(state, &SplitViewAttached::collapsedCompleted, this, [&view] {
            view.reset();
        });
        state->setExpanded(false);
        view->ensurePolished();
        QVERIFY(! view);
    }
    void springCompletes_data() {
        QTest::addColumn<int>("orientation");
        QTest::newRow("horizontal") << int(Qt::Horizontal);
        QTest::newRow("vertical") << int(Qt::Vertical);
    }
    void springCompletes() {
        QFETCH(int, orientation);
        auto view = create(R"(
expandTransition: Transition { SpringAnimation { property: "progress"; spring: 3; damping: 0.3; epsilon: 0.001 } }
collapseTransition: Transition { SpringAnimation { property: "progress"; spring: 3; damping: 0.3; epsilon: 0.001 } }
)");
        QVERIFY(view);
        view->setOrientation(Qt::Orientation(orientation));
        polish(view.get());
        auto*      state = info(view->itemAt(1));
        QSignalSpy closed(state, &SplitViewAttached::collapsedCompleted);
        QSignalSpy opened(state, &SplitViewAttached::expandedCompleted);
        state->setExpanded(false);
        QTRY_COMPARE(closed.count(), 1);
        QVERIFY(! state->isTransitioning());
        state->setExpanded(true);
        QTRY_COMPARE(opened.count(), 1);
        QVERIFY(! state->isTransitioning());
        QCOMPARE(view->itemAt(0)->width(), orientation == Qt::Horizontal ? 300 : 800);
    }
};

int run_split_view(int argc, char** argv) {
    SplitViewTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "split_view.moc"
