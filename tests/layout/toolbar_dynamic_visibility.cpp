#include "qml_material/layout/toolbar_layout.hpp"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>
#include <QUrl>

#include <cstdio>
#include <cstdlib>
#include <functional>

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

int run_toolbar_dynamic_visibility(int argc, char** argv) {
    qmlRegisterType<qml_material::Action>("QmlMaterialTest", 1, 0, "Action");
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

    root->setProperty("maxShowActionNum", 1);
    if (! waitFor(
            [root]() {
                return root->property("hiddenCount").toInt() == 2;
            },
            window)) {
        delete root;
        return fail("changing maxShowActionNum did not fold actions");
    }
    root->setProperty("maxShowActionNum", 0);
    if (! waitFor(
            [root]() {
                return root->property("hiddenCount").toInt() == 3;
            },
            window)) {
        delete root;
        return fail("zero maxShowActionNum did not fold all actions");
    }
    root->setProperty("maxShowActionNum", 3);
    if (! waitFor(
            [root]() {
                return root->property("hiddenCount").toInt() == 0;
            },
            window)) {
        delete root;
        return fail("restoring maxShowActionNum did not restore actions");
    }
    auto* layout = root->findChild<qml_material::ToolBarLayout*>();
    if (! layout) {
        delete root;
        return fail("toolbar layout was not created");
    }
    root->setProperty("actionBarWidthLimit", layout->minimumWidth());
    if (! waitFor(
            [layout]() {
                return layout->hiddenActions().size() == 3;
            },
            window)) {
        delete root;
        return fail("narrow toolbar did not fold its actions");
    }
    root->setProperty("actionBarWidthLimit", layout->implicitWidth());
    if (! waitFor(
            [layout]() {
                return layout->hiddenActions().isEmpty();
            },
            window)) {
        delete root;
        return fail("restoring the implicit size retained folded actions");
    }
    QQuickItem cache;
    cache.setParentItem(window.contentItem());
    cache.setVisible(false);
    for (int i = 0; i < 3; ++i) {
        root->setProperty("actionBarWidthLimit", layout->minimumWidth());
        if (! waitFor(
                [layout]() {
                    return layout->hiddenActions().size() == 3;
                },
                window)) {
            delete root;
            return fail("toolbar did not fold before caching the page");
        }
        root->setParentItem(&cache);
        if (layout->isVisible()) {
            delete root;
            return fail("cached page toolbar remained visible");
        }
        layout->ensurePolished();
        root->setProperty("actionBarWidthLimit", layout->implicitWidth());
        root->setParentItem(window.contentItem());
        if (! waitFor(
                [layout]() {
                    int visible = 0;
                    for (auto* item : layout->childItems()) visible += item->isVisible();
                    return layout->hiddenActions().isEmpty() && visible == 3;
                },
                window)) {
            delete root;
            return fail("cached page did not restore visible toolbar actions");
        }
    }
    delete root;
    std::printf("PASS toolbar_dynamic_visibility\n");
    return EXIT_SUCCESS;
}
