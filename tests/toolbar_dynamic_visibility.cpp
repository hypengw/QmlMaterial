#include "qml_material/layout/toolbar_layout.hpp"
#include "qml_material/util/loggingcategory.hpp"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>
#include <QUrl>

#include <cstdio>
#include <cstdlib>
#include <functional>

Q_LOGGING_CATEGORY(qml_material_logcat, "qcm.material.test")

namespace
{

auto waitFor(const std::function<bool()>& predicate, QQuickWindow& window) -> bool {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 2000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        window.requestUpdate();
        if (predicate()) {
            return true;
        }
        QThread::msleep(10);
    }
    return predicate();
}

auto fail(const char* message) -> int {
    std::fprintf(stderr, "FAIL: %s\n", message);
    return EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<qml_material::ToolBarLayout>("QmlMaterialTest", 1, 0, "ToolBarLayout");

    QQmlEngine    engine;
    QQmlComponent component(&engine, QUrl::fromLocalFile(TOOLBAR_DYNAMIC_QML_PATH));
    if (component.isError()) {
        for (const auto& error : component.errors()) {
            std::fprintf(stderr, "QML error: %s\n", qPrintable(error.toString()));
        }
        return EXIT_FAILURE;
    }

    auto* object = component.create();
    auto* root   = qobject_cast<QQuickItem*>(object);
    if (! root) {
        delete object;
        return fail("root object is not a QQuickItem");
    }

    QQuickWindow window;
    window.resize(280, 80);
    root->setParentItem(window.contentItem());
    window.show();

    const auto initialReady = waitFor(
        [root]() {
            return root->property("actionBarWidth").toReal() >= 80.0 &&
                   root->property("hiddenCount").toInt() == 0;
        },
        window);
    if (! initialReady) {
        std::fprintf(stderr,
                     "initial width=%f hidden=%d\n",
                     root->property("actionBarWidth").toReal(),
                     root->property("hiddenCount").toInt());
        delete root;
        return fail("initial toolbar layout did not settle");
    }

    root->setProperty("linkVisible", true);
    const auto expanded = waitFor(
        [root]() {
            return root->property("actionBarWidth").toReal() >= 120.0;
        },
        window);
    if (! expanded) {
        delete root;
        return fail("toolbar width did not expand for the visible action");
    }

    QThread::msleep(50);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    if (root->property("hiddenCount").toInt() != 0) {
        delete root;
        return fail("newly visible action stayed folded after the toolbar expanded");
    }

    delete root;
    std::printf("PASS toolbar_dynamic_visibility\n");
    return EXIT_SUCCESS;
}
