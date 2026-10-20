#include "qml_material/input/pull_to_refresh_state.hpp"
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquickloader_p.h>
#include <limits>

using namespace qml_material;

class PullToRefreshTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|TypeError|ReferenceError|Unable to assign).*"));
    }
    void visualLifecycle_data() {
        QTest::addColumn<bool>("shapeLoading");
        QTest::newRow("circular") << false;
        QTest::newRow("shape") << true;
    }
    void visualLifecycle() {
        QFETCH(bool, shapeLoading);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.PullToRefresh {
    width: 300; height: 400
    property Component customIndicator: Rectangle { implicitWidth: 72; implicitHeight: 24 }
})",
                          QUrl("qrc:/pull-lifecycle.qml"));
        QQuickWindow window;
        window.resize(300, 400);
        std::unique_ptr<QQuickItem> item(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(item, qPrintable(component.errorString()));
        item->setProperty("shapeLoading", shapeLoading);
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* state = item->property("refreshState").value<PullToRefreshState*>();
        QVERIFY(state);
        QQuickNumberAnimation* motion = nullptr;
        for (auto* animation : item->findChildren<QQuickNumberAnimation*>())
            if (animation->property() == "__offset") motion = animation;
        QVERIFY(motion);
        state->begin(NestedScrollConnection::Drag);
        state->postScroll({}, { 0, -200 }, NestedScrollConnection::Drag);
        QCOMPARE(item->property("presentedOffset").toReal(), 98.75);
        state->release({});
        QVERIFY(motion->isRunning());
        QVERIFY(state->settling());
        motion->setCurrentTime(motion->duration() / 2);
        const auto midway = item->property("presentedOffset").toReal();
        QVERIFY(midway > 0 && midway < 98.75);
        state->begin(NestedScrollConnection::Drag);
        QVERIFY(! state->dragging());
        QCOMPARE(state->postScroll({}, { 0, -100 }, NestedScrollConnection::Drag), QPointF());
        motion->setCurrentTime(motion->duration());
        QVERIFY(! state->settling());
        QCOMPARE(item->property("presentedOffset").toReal(), 0);
        item->setProperty("refreshing", true);
        motion->setCurrentTime(motion->duration());
        QCOMPARE(item->property("presentedOffset").toReal(), 80);
        auto* overlay = item->property("__overlay").value<QQuickItem*>();
        QVERIFY(overlay);
        auto* indicator = overlay->findChild<QQuickLoader*>();
        QVERIFY(indicator && indicator->item());
        QPointer<QObject> original = indicator->item();
        auto*             spinner  = original->findChild<QQuickLoader*>();
        QVERIFY(spinner);
        QTRY_VERIFY(spinner->item());
        QPointer<QObject> activeSpinner = spinner->item();
        QCOMPARE(indicator->size(), QSizeF(40, 40));
        QCOMPARE(spinner->size(), shapeLoading ? QSizeF(24, 24) : QSizeF(16, 16));
        QCOMPARE(spinner->position(), shapeLoading ? QPointF(8, 8) : QPointF(12, 12));
        if (shapeLoading) {
            QCOMPARE(activeSpinner->property("indicatorSize").toInt(), 24);
            QVERIFY(activeSpinner->property("running").toBool());
        } else {
            auto* content = activeSpinner->property("contentItem").value<QQuickItem*>();
            QVERIFY(content);
            QCOMPARE(content->size(), QSizeF(13.5, 13.5));
            auto* arcLoader = content->findChild<QQuickLoader*>();
            QVERIFY(arcLoader);
            QTRY_VERIFY(arcLoader->item());
            QCOMPARE(arcLoader->item()->property("radius").toReal(), 6.75);
        }
        item->setVisible(false);
        QVERIFY(! spinner->active());
        QVERIFY(! spinner->item());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(! activeSpinner);
        item->setVisible(true);
        QCOMPARE(item->isVisible(), true);
        QCOMPARE(item->property("refreshing").toBool(), true);
        QCOMPARE(item->property("presentedOffset").toReal(), 80);
        QCOMPARE(spinner->active(), true);
        QVERIFY(spinner->item());
        QPointer<QObject> previousSpinner = spinner->item();
        item->setProperty("shapeLoading", ! shapeLoading);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(! previousSpinner);
        QVERIFY(original);
        QCOMPARE(spinner->size(), shapeLoading ? QSizeF(16, 16) : QSizeF(24, 24));
        QCOMPARE(spinner->item()->property("indicatorSize").isValid(), ! shapeLoading);
        item->setProperty("indicator", item->property("customIndicator"));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(! original);
        QVERIFY(indicator->item());
        QCOMPARE(indicator->size(), QSizeF(72, 24));
        QCOMPARE(indicator->position(), QPointF(114, 56));
        item->setProperty("refreshing", false);
        QVERIFY(motion->isRunning());
        item.reset();
    }
    void windowInput_data() {
        QTest::addColumn<bool>("owned");
        QTest::addColumn<bool>("touch");
        QTest::addColumn<bool>("shortContent");
        QTest::newRow("qt-mouse") << false << false << false;
        QTest::newRow("qt-touch") << false << true << false;
        QTest::newRow("owned-mouse") << true << false << false;
        QTest::newRow("owned-touch") << true << true << false;
        QTest::newRow("qt-short-mouse") << false << false << true;
        QTest::newRow("qt-short-touch") << false << true << true;
        QTest::newRow("owned-short-mouse") << true << false << true;
        QTest::newRow("owned-short-touch") << true << true << true;
    }
    void windowInput() {
        QFETCH(bool, owned);
        QFETCH(bool, touch);
        QFETCH(bool, shortContent);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent    component(&engine);
        const QByteArray viewport = owned
                                        ? "MD.Scrollable { contentHeight: 1000; "
                                          "flickableDirection: MD.Scrollable.VerticalFlick;"
                                        : "ListView { model: 20; delegate: Rectangle {width: 300; "
                                          "height: 50} boundsBehavior: Flickable.StopAtBounds;";
        component.setData("import QtQuick\nimport Qcm.Material as MD\nMD.PullToRefresh { id: root; "
                          "width: 300; height: 400; animationsEnabled: false; " +
                              viewport + R"(
    objectName: "view"
    anchors.fill: parent
    MD.NestedScroll.enabled: true
    MD.NestedScroll.connection: root.refreshState
} })",
                          QUrl("qrc:/pull-input.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(300, 400);
        std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(root, qPrintable(component.errorString()));
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view = root->findChild<QQuickItem*>("view");
        QVERIFY(view);
        if (shortContent) view->setProperty(owned ? "contentHeight" : "model", owned ? 100 : 0);
        if (! owned) QMetaObject::invokeMethod(view, "forceLayout");
        view->setProperty("contentY", shortContent ? 0 : 60);
        auto* state = root->property("refreshState").value<PullToRefreshState*>();
        QVERIFY(state);
        QSignalSpy requests(state, &PullToRefreshState::refreshRequested);
        auto       mouse = [&](QEvent::Type type, QPointF position, ulong timestamp) {
            QMouseEvent event(type,
                              position,
                              window.mapToGlobal(position.toPoint()),
                              type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                              type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                              Qt::NoModifier);
            event.setTimestamp(timestamp);
            QCoreApplication::sendEvent(&window, &event);
        };
        static auto* device = QTest::createTouchDevice();
        const int    endY   = shortContent ? 240 : 300;
        if (touch) {
            QTest::touchEvent(&window, device).press(0, { 100, 60 }).commit();
            QTest::touchEvent(&window, device).move(0, { 100, endY }).commit();
            QQuickWindowPrivate::get(&window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
                &window);
        } else {
            mouse(QEvent::MouseButtonPress, { 100, 60 }, 1000);
            mouse(QEvent::MouseMove, { 100, qreal(endY) }, 1020);
        }
        QCOMPARE(view->property("contentY").toReal(), 0);
        QVERIFY(state->armed());
        QCOMPARE(state->offset(), 89.6875);
        QCOMPARE(requests.size(), 0);
        if (touch)
            QTest::touchEvent(&window, device).release(0, { 100, endY }).commit();
        else
            mouse(QEvent::MouseButtonRelease, { 100, qreal(endY) }, 1220);
        QCOMPARE(requests.size(), 1);
        QCOMPARE(state->offset(), 0);
        state->setRefreshing(true);
        mouse(QEvent::MouseButtonPress, { 100, 300 }, 2000);
        mouse(QEvent::MouseMove, { 100, 240 }, 2020);
        mouse(QEvent::MouseButtonRelease, { 100, 240 }, 2220);
        QCOMPARE(view->property("contentY").toReal(), shortContent ? 0 : 60);
        QCOMPARE(state->offset(), 80);
        QCOMPARE(requests.size(), 1);
        state->setRefreshing(false);
        QWheelEvent wheel({ 100, 200 },
                          window.mapToGlobal(QPoint(100, 200)),
                          { 0, 300 },
                          {},
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::NoScrollPhase,
                          false);
        QCoreApplication::sendEvent(&window, &wheel);
        QCOMPARE(view->property("contentY").toReal(), 0);
        QCOMPARE(requests.size(), 1);
        QCOMPARE(state->offset(), 0);
    }
    void qmlInitialRefresh() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.PullToRefresh {
    width: 300; height: 400
    refreshing: true
    Rectangle { objectName: "content"; anchors.fill: parent; color: "white" }
})",
                          QUrl("qrc:/pull-to-refresh.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QQuickItem> item(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(item, qPrintable(component.errorString()));
        QCOMPARE(item->property("presentedOffset").toReal(), 80);
        auto* state = item->property("refreshState").value<PullToRefreshState*>();
        QVERIFY(state);
        QVERIFY(! state->settling());
        auto* content = item->findChild<QQuickItem*>("content");
        QVERIFY(content);
        QCOMPARE(content->size(), QSizeF(300, 400));
        item->setProperty("animationsEnabled", false);
        item->setProperty("refreshing", false);
        QCOMPARE(item->property("presentedOffset").toReal(), 0);
        state->begin(NestedScrollConnection::Drag);
        state->postScroll({}, { 0, -100 }, NestedScrollConnection::Drag);
        QCOMPARE(item->property("presentedOffset").toReal(), 50);
        state->end(true);
        QCOMPARE(item->property("presentedOffset").toReal(), 0);
    }
    void threshold_data() {
        QTest::addColumn<qreal>("distance");
        QTest::addColumn<bool>("requested");
        QTest::newRow("below") << 159. << false;
        QTest::newRow("equal") << 160. << false;
        QTest::newRow("above") << 161. << true;
    }
    void threshold() {
        QFETCH(qreal, distance);
        QFETCH(bool, requested);
        PullToRefreshState state;
        QSignalSpy         requests(&state, &PullToRefreshState::refreshRequested);
        state.begin(NestedScrollConnection::Drag);
        QCOMPARE(state.postScroll({}, { 0, -distance }, NestedScrollConnection::Drag),
                 QPointF(0, -distance));
        QCOMPARE(state.armed(), requested);
        QCOMPARE(state.release({ 0, -1000 }), QPointF(0, -1000));
        QCOMPARE(requests.size(), requested ? 1 : 0);
        QCOMPARE(state.offset(), 0);
        QVERIFY(! state.refreshing());
        QVERIFY(! state.dragging());
        state.release({ 0, -1000 });
        state.end(false);
        QCOMPARE(requests.size(), requested ? 1 : 0);
    }
    void resistanceAndRetraction() {
        PullToRefreshState state;
        state.begin(NestedScrollConnection::Drag);
        state.postScroll({}, { 0, -80 }, NestedScrollConnection::Drag);
        QCOMPARE(state.offset(), 40);
        QCOMPARE(state.distanceFraction(), 0.5);
        state.postScroll({}, { 0, -160 }, NestedScrollConnection::Drag);
        QCOMPARE(state.offset(), 115);
        state.postScroll({}, { 0, -1000 }, NestedScrollConnection::Drag);
        QCOMPARE(state.offset(), 160);
        QCOMPARE(state.distanceFraction(), 2);
        QCOMPARE(state.preScroll({ 0, 1300 }, NestedScrollConnection::Drag), QPointF(0, 1240));
        QCOMPARE(state.offset(), 0);
        QVERIFY(! state.armed());
        QCOMPARE(state.release({ 0, -1000 }), QPointF());
    }
    void cancellationAndSources() {
        PullToRefreshState state;
        QSignalSpy         requests(&state, &PullToRefreshState::refreshRequested);
        for (auto source : { NestedScrollConnection::Wheel, NestedScrollConnection::Fling }) {
            state.begin(source);
            QVERIFY(! state.canConsume({ 0, -200 }, source));
            QCOMPARE(state.postScroll({}, { 0, -200 }, source), QPointF());
            QCOMPARE(state.release({ 0, -100 }), QPointF());
        }
        state.begin(NestedScrollConnection::Drag);
        QVERIFY(! state.canConsume({ 100, 0 }, NestedScrollConnection::Drag));
        state.postScroll({}, { 0, -200 }, NestedScrollConnection::Drag);
        state.end(true);
        QCOMPARE(state.offset(), 0);
        QCOMPARE(requests.size(), 0);
        state.begin(NestedScrollConnection::Drag);
        state.postScroll({}, { 0, -200 }, NestedScrollConnection::Drag);
        QCOMPARE(state.release({ 0, 1000 }), QPointF());
        QCOMPARE(requests.size(), 1);
    }
    void externallyOwnedRefresh() {
        PullToRefreshState state;
        QObject::connect(&state, &PullToRefreshState::refreshRequested, &state, [&] {
            state.setRefreshing(true);
        });
        state.begin(NestedScrollConnection::Drag);
        state.postScroll({}, { 0, -200 }, NestedScrollConnection::Drag);
        state.release({});
        QVERIFY(state.refreshing());
        QCOMPARE(state.offset(), 80);
        state.begin(NestedScrollConnection::Drag);
        QCOMPARE(state.postScroll({}, { 0, -200 }, NestedScrollConnection::Drag), QPointF());
        state.setEnabled(false);
        QCOMPARE(state.offset(), 80);
        state.setRefreshing(false);
        QCOMPARE(state.offset(), 0);
        state.setRefreshing(true);
        QCOMPARE(state.distanceFraction(), 1);
        state.setThreshold(64);
        QCOMPARE(state.offset(), 64);
    }
    void synchronousCompletionAndDeletion() {
        PullToRefreshState state;
        QObject::connect(&state, &PullToRefreshState::refreshRequested, &state, [&] {
            state.setRefreshing(true);
            state.setRefreshing(false);
        });
        state.begin(NestedScrollConnection::Drag);
        state.postScroll({}, { 0, -200 }, NestedScrollConnection::Drag);
        state.release({});
        QCOMPARE(state.offset(), 0);
        auto*                        disposable = new PullToRefreshState;
        QPointer<PullToRefreshState> guard(disposable);
        QObject::connect(
            disposable, &PullToRefreshState::refreshRequested, disposable, [disposable] {
                delete disposable;
            });
        disposable->begin(NestedScrollConnection::Drag);
        disposable->postScroll({}, { 0, -200 }, NestedScrollConnection::Drag);
        disposable->release({});
        QVERIFY(! guard);
    }
    void settlingAndInvalidValues() {
        PullToRefreshState state;
        state.setSettling(true);
        state.begin(NestedScrollConnection::Drag);
        QVERIFY(! state.dragging());
        QVERIFY(! state.canConsume({ 0, -40 }, NestedScrollConnection::Drag));
        state.setSettling(false);
        state.begin(NestedScrollConnection::Drag);
        QVERIFY(state.dragging());
        state.setThreshold(0);
        state.setThreshold(-1);
        state.setThreshold(std::numeric_limits<qreal>::infinity());
        QCOMPARE(state.threshold(), 80);
        QCOMPARE(state.postScroll({},
                                  { 0, std::numeric_limits<qreal>::quiet_NaN() },
                                  NestedScrollConnection::Drag),
                 QPointF());
        state.postScroll({}, { 0, -200 }, NestedScrollConnection::Drag);
        state.setThreshold(100);
        QVERIFY(! state.dragging());
        QCOMPARE(state.offset(), 0);
    }
};

int run_pull_to_refresh(int argc, char** argv) {
    PullToRefreshTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "pull_to_refresh.moc"
