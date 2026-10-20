#include <QtTest/QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QAbstractItemModel>
#include <QJSValue>
#include <algorithm>
#include <cmath>
#include <limits>
#include <QDir>
#include <QMutex>
#include <private/qquickanimation_p.h>
#include "qml_material/control/dialog.hpp"
#include "qml_material/control/overlay.hpp"
#include "qml_material/control/page_container.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/control/menu.hpp"
#include "qml_material/input/interaction_source.hpp"
#include "qml_material/input/app_bar_scroll.hpp"
#include "qml_material/anim/spring_motion.hpp"
#include "qml_material/anim/ripple_motion.hpp"

namespace
{
void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}

QQuickItem* visualChild(QQuickItem* item, const QString& name) {
    if (item->objectName() == name) return item;
    for (auto* child : item->childItems()) {
        if (auto* result = visualChild(child, name)) return result;
    }
    return nullptr;
}

qml_material::Dialog* visibleDialog(QQuickWindow& window, const QString& name) {
    for (auto item : qml_material::OverlayManager::get(&window)->childItems()) {
        auto dialog = qobject_cast<qml_material::Dialog*>(item->parent());
        if (dialog && dialog->surfaceItem() == item && dialog->objectName() == name &&
            dialog->isVisible())
            return dialog;
    }
    return nullptr;
}

void click(QObject* button) {
    QVERIFY(button);
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
}

void verifyText(QQuickItem* item) {
    if (item->isVisible() && item->property("truncated").isValid())
        QVERIFY2(! item->property("truncated").toBool(),
                 qPrintable(item->property("text").toString()));
    for (auto* child : item->childItems()) verifyText(child);
}
} // namespace

class ExampleNavigationTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot "
                               "assign|Cannot open|TypeError|ReferenceError|polish.*loop).*"));
    }

    void compactNavigation() {
        QQmlEngine   engine;
        QQuickWindow window;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example" as Demo
            Demo.Example {
                id: root
                width: 320; height: 760; pageIndex: 1
                MD.MProp.size: MD.ThemeSize { width: root.width; duration: 0 }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        polish(root);
        QCoreApplication::processEvents();
        auto* navigation = root->findChild<QQuickItem*>("compactNavigation");
        QVERIFY(navigation);
        QVERIFY(navigation->property("contentWidth").toReal() > navigation->width());
        const auto pages     = root->property("pageModel").value<QJSValue>();
        const int  pageCount = pages.property("length").toInt();
        QCOMPARE(pageCount, 4);
        for (int index = 0; index < pageCount; ++index) {
            const auto name = pages.property(index).property("name").toString();
            QVERIFY(name != "motion" && name != "about" && name != "typography");
        }
        for (int index : { pageCount - 1, 1 }) {
            auto* button =
                visualChild(navigation, "compactNavigationButton" + QString::number(index));
            click(button);
            QCOMPARE(root->property("pageIndex").toInt(), index);
            polish(root);
            QCoreApplication::processEvents();
            polish(root);
            const auto bounds = button->mapRectToItem(navigation, button->boundingRect());
            QVERIFY(bounds.left() >= 0);
            QVERIFY(bounds.right() <= navigation->width());
        }
        root->setWidth(400);
        polish(root);
        QCoreApplication::processEvents();
        verifyText(navigation);
        auto more = root->findChild<QQuickItem*>("compactMoreButton");
        QVERIFY(more && more->isVisible());
        const auto moreBounds = more->mapRectToItem(root, more->boundingRect());
        QVERIFY(root->boundingRect().contains(moreBounds));
        QVERIFY(moreBounds.left() >= navigation->mapToItem(root, { navigation->width(), 0 }).x());
    }

    void exampleRailSelection_data() {
        QTest::addColumn<int>("width");
        QTest::newRow("medium") << 840;
        QTest::newRow("large") << 1280;
    }

    void exampleRailSelection() {
        QFETCH(int, width);
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(width, 900);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example" as Demo
            Demo.Example {
                id: root
                height: 900; pageIndex: 2
                MD.MProp.size: MD.ThemeSize { width: root.width; duration: 0 }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "width", width } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* rail  = root->findChild<QQuickItem*>("exampleNavigationRail");
        auto* pages = root->findChild<qml_material::PageContainer*>("examplePages");
        QVERIFY(rail && pages);
        QTRY_VERIFY(pages->currentItem() && pages->currentItem()->property("title").isValid());
        polish(root);
        const auto findButton = [](QQuickItem*    item,
                                   const QString& text,
                                   const auto&    find) -> qml_material::SelectionButton* {
            if (auto* button = qobject_cast<qml_material::SelectionButton*>(item);
                button && button->text() == text)
                return button;
            for (auto* child : item->childItems())
                if (auto* button = find(child, text, find)) return button;
            return nullptr;
        };
        auto* previous = findButton(rail, "color", findButton);
        auto* next     = findButton(rail, "compositions", findButton);
        QVERIFY(previous && next);
        auto* motion = qobject_cast<qml_material::SpringMotion*>(
            next->property("_selectionMotion").value<QObject*>());
        auto* indicator = next->findChild<QQuickItem*>("railSelectionIndicator");
        auto* bounds    = next->findChild<QQuickItem*>("railIndicatorBounds");
        QVERIFY(motion && indicator && bounds);
        QCOMPARE(motion->value(), 0);
        QVERIFY(motion->isEnabled());
        QList<qreal> widths;
        QObject      observer;
        connect(motion, &qml_material::SpringMotion::valueChanged, &observer, [&] {
            widths.append(indicator->width());
        });
        QElapsedTimer elapsed;
        elapsed.start();
        QTest::mouseClick(
            &window,
            Qt::LeftButton,
            Qt::NoModifier,
            next->mapToScene(QPointF(next->width() / 2, next->height() / 2)).toPoint(),
            0);
        const auto clickTime = elapsed.elapsed();
        QCOMPARE(root->property("pageIndex").toInt(), 1);
        QVERIFY(! previous->isSelected() && next->isSelected());
        QTRY_VERIFY(! motion->isRunning());
        QCOMPARE(indicator->width(), bounds->width());
        QCOMPARE(indicator->opacity(), 1);
        const auto intermediate =
            std::count_if(widths.begin(), widths.end(), [bounds](qreal value) {
                return value > 0 && value < bounds->width();
            });
        QVERIFY2(intermediate > 0,
                 qPrintable(QString("no intermediate width: %1 samples, click blocked %2 ms")
                                .arg(widths.size())
                                .arg(clickTime)));
        QTRY_VERIFY(pages->currentItem() &&
                    pages->currentItem()->property("title") == "Compositions");
    }

    void examplePaletteSelection() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example" as Demo
            Demo.Example {
                width: 1280; height: 900
                MD.MProp.size: MD.ThemeSize { width: 1280; duration: 0 }
                readonly property int paletteType: MD.Token.color.paletteType
                function usePalette(value) { MD.Token.color.paletteType = value }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QQuickWindow window;
        window.resize(1280, 900);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* rail = root->findChild<QQuickItem*>("exampleNavigationRail");
        QVERIFY(rail);
        QVERIFY(rail->setProperty("expanded", true));
        QTRY_VERIFY(rail->property("useLarge").toBool());
        auto* palette = rail->findChild<QQuickItem*>("paletteChoices");
        QVERIFY(palette);
        QQuickItem* firstItem  = nullptr;
        QQuickItem* secondItem = nullptr;
        QTRY_VERIFY(
            QMetaObject::invokeMethod(
                palette, "itemAtIndex", Q_RETURN_ARG(QQuickItem*, firstItem), Q_ARG(int, 0)) &&
            firstItem);
        QTRY_VERIFY(
            QMetaObject::invokeMethod(
                palette, "itemAtIndex", Q_RETURN_ARG(QQuickItem*, secondItem), Q_ARG(int, 1)) &&
            secondItem);
        auto* first  = qobject_cast<qml_material::SelectionButton*>(firstItem);
        auto* second = qobject_cast<qml_material::SelectionButton*>(secondItem);
        QVERIFY(first && second);
        QVERIFY(first->findChildren<qml_material::Action*>().isEmpty());
        QVERIFY(second->findChildren<qml_material::Action*>().isEmpty());
        const auto usePalette = [&](int value) {
            return QMetaObject::invokeMethod(root, "usePalette", Q_ARG(QVariant, value));
        };
        const auto original = root->property("paletteType").toInt();
        QVERIFY(usePalette(0));
        QVERIFY(first->isSelected() && ! second->isSelected());
        second->click();
        QCOMPARE(root->property("paletteType").toInt(), 1);
        QVERIFY(! first->isSelected() && second->isSelected());
        QCOMPARE(palette->property("currentIndex").toInt(), 1);
        second->click();
        QVERIFY(second->isSelected());
        QVERIFY(usePalette(0));
        QVERIFY(first->isSelected() && ! second->isSelected());
        QCOMPARE(palette->property("currentIndex").toInt(), 0);
        QVERIFY(usePalette(original));
    }

    void applicationWindowRailSelection() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        const auto path = QFINDTESTDATA("../../example/main.qml");
        QVERIFY(! path.isEmpty());
        QQmlComponent            component(&engine, QUrl::fromLocalFile(path));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* window = qobject_cast<QQuickWindow*>(object.get());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QQuickItem* rail = nullptr;
        QTRY_VERIFY((rail = window->findChild<QQuickItem*>("exampleNavigationRail")));
        auto* pages = window->findChild<qml_material::PageContainer*>("examplePages");
        QVERIFY(pages);
        QTRY_VERIFY(pages->currentItem() &&
                    pages->currentItem()->property("title") == "Components");
        const auto findButton = [](QQuickItem* item,
                                   const auto& find) -> qml_material::SelectionButton* {
            if (auto* button = qobject_cast<qml_material::SelectionButton*>(item);
                button && button->text() == "color")
                return button;
            for (auto* child : item->childItems())
                if (auto* button = find(child, find)) return button;
            return nullptr;
        };
        auto* next = findButton(rail, findButton);
        QVERIFY(next);
        qml_material::InteractionAccumulator nextInput(this);
        nextInput.setSource(next->interactionSource());
        auto* motion = qobject_cast<qml_material::SpringMotion*>(
            next->property("_selectionMotion").value<QObject*>());
        auto* indicator = next->findChild<QQuickItem*>("railSelectionIndicator");
        auto* bounds    = next->findChild<QQuickItem*>("railIndicatorBounds");
        QVERIFY(motion && indicator && bounds);
        QVERIFY(motion->isEnabled());
        QCOMPARE(motion->value(), 0);
        auto* ripple = next->findChild<QQuickItem*>("railRipple");
        QVERIFY(ripple);
        const auto point =
            next->mapToScene(QPointF(next->width() / 2, next->height() / 2)).toPoint();
        QTest::mouseMove(window, point, 0);
        QTRY_VERIFY(nextInput.values().hovered);
        QTRY_VERIFY(ripple->findChild<qml_material::RippleMotion*>()->opacity() > 0);
        QCOMPARE(ripple->width(), bounds->width());
        QCOMPARE(indicator->width(), 0);
        struct Sample {
            qreal width, opacity, rippleWidth;
        };
        struct Trace {
            QMutex        mutex;
            QList<Sample> samples;
        };
        const auto trace = std::make_shared<Trace>();
        connect(
            window,
            &QQuickWindow::afterSynchronizing,
            window,
            [trace, indicator = QPointer(indicator), ripple = QPointer(ripple)] {
                if (! indicator || ! ripple) return;
                QMutexLocker lock(&trace->mutex);
                trace->samples.append(
                    { indicator->width(), indicator->opacity(), ripple->width() });
            },
            Qt::DirectConnection);
        bool       captured  = false;
        const auto directory = qEnvironmentVariable("QM_MOTION_CAPTURE_DIR");
        QObject    observer;
        connect(window, &QQuickWindow::frameSwapped, &observer, [&] {
            if (directory.isEmpty() || captured || motion->value() <= .05 || motion->value() >= .95)
                return;
            captured         = true;
            const auto frame = window->grabWindow();
            QVERIFY(! frame.isNull());
            QVERIFY(QDir().mkpath(directory));
            QVERIFY(frame.save(directory + "/application-rail-selection.png"));
        });
        QElapsedTimer elapsed;
        elapsed.start();
        QTest::mouseClick(
            window,
            Qt::LeftButton,
            Qt::NoModifier,
            next->mapToScene(QPointF(next->width() / 2, next->height() / 2)).toPoint(),
            0);
        const auto clickTime = elapsed.elapsed();
        QCOMPARE(rail->property("currentIndex").toInt(), 2);
        QVERIFY(next->isSelected());
        QTRY_VERIFY(! motion->isRunning());
        auto snapshot = [&] {
            QMutexLocker lock(&trace->mutex);
            return trace->samples;
        };
        const auto samples = snapshot();
        const auto intermediate =
            std::count_if(samples.begin(), samples.end(), [bounds](const auto& value) {
                return value.width > 0 && value.width < bounds->width() && value.opacity > 0 &&
                       value.opacity < 1 && value.rippleWidth == bounds->width();
            });
        QVERIFY2(
            intermediate > 0,
            qPrintable(
                QString("no intermediate selection geometry under hover: %1 frames, click %2 ms")
                    .arg(samples.size())
                    .arg(clickTime)));
        QCOMPARE(indicator->width(), bounds->width());
        QCOMPARE(indicator->opacity(), 1);
    }

    void utilityPopups_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<QString>("name");
        for (int width : { 320, 840, 1280 }) {
            for (const auto& name : { QStringLiteral("about"), QStringLiteral("typography") })
                QTest::newRow(qPrintable(QString::number(width) + "-" + name)) << width << name;
        }
    }

    void utilityPopups() {
        QFETCH(int, width);
        QFETCH(QString, name);
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(width, 900);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example" as Demo
            Demo.Example {
                id: root
                height: 900; pageIndex: 1
                MD.MProp.size: MD.ThemeSize { width: root.width; duration: 0 }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "width", width } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto pages = root->findChild<qml_material::PageContainer*>("examplePages");
        QVERIFY(pages);
        QTRY_VERIFY(pages->currentItem() &&
                    pages->currentItem()->property("title") == "Compositions");
        const QPointer<QQuickItem> currentPage = pages->currentItem();
        const auto                 currentKey  = pages->currentKey();
        QSignalSpy                 pageChanged(root, SIGNAL(pageIndexChanged()));
        QSignalSpy routeChanged(pages, &qml_material::PageContainer::currentKeyChanged);
        auto       mouseClick = [&](QQuickItem* item) {
            QTest::mouseClick(
                &window,
                Qt::LeftButton,
                Qt::NoModifier,
                item->mapToScene({ item->width() / 2, item->height() / 2 }).toPoint());
        };
        if (width == 320) {
            QVERIFY(! root->findChild<QQuickItem*>("exampleNavigationRail"));
            auto more = root->findChild<QQuickItem*>("compactMoreButton");
            QVERIFY(more && more->isVisible());
            auto menu = root->findChild<qml_material::Menu*>("compactUtilityMenu");
            QVERIFY(menu);
            menu->setEnter(nullptr);
            menu->setExit(nullptr);
            mouseClick(more);
            QVERIFY(menu->isOpened());
            QCOMPARE(menu->count(), 2);
            polish(menu->surfaceItem());
            auto item = visualChild(menu->surfaceItem(), name + "MenuItem");
            QVERIFY(item);
            mouseClick(item);
            QVERIFY(! menu->isVisible());
        } else {
            auto rail = root->findChild<QQuickItem*>("exampleNavigationRail");
            QVERIFY(rail);
            QVERIFY(! rail->property("useLarge").toBool());
            if (auto button = visualChild(window.contentItem(), name + "PopupButton"))
                QVERIFY(! button->isVisible());
            QVERIFY(QMetaObject::invokeMethod(rail, "open"));
            QTRY_COMPARE(rail->property("expansionProgress").toReal(), 1.0);
            polish(root);
            QTRY_VERIFY(visualChild(window.contentItem(), name + "PopupButton"));
            auto button = visualChild(window.contentItem(), name + "PopupButton");
            QVERIFY(button->isVisible());
            QCOMPARE(button->property("expansionProgress").toReal(),
                     rail->property("expansionProgress").toReal());
            mouseClick(button);
            if (rail->property("useModal").toBool()) QVERIFY(! rail->property("useLarge").toBool());
        }
        QTRY_VERIFY(visibleDialog(window, name + "Popup"));
        QPointer<qml_material::Dialog> dialog = visibleDialog(window, name + "Popup");
        QTRY_VERIFY(dialog->isOpened());
        QVERIFY(dialog->modal());
        QCOMPARE(dialog->parentItem(), root);
        QCOMPARE(pages->currentItem(), currentPage.data());
        QCOMPARE(pages->currentKey(), currentKey);
        QCOMPARE(root->property("pageIndex").toInt(), 1);
        QCOMPARE(pageChanged.count(), 0);
        QCOMPARE(routeChanged.count(), 0);
        polish(dialog->surfaceItem());
        verifyText(dialog->surfaceItem());

        window.resize(320, 360);
        root->setSize({ 320, 360 });
        polish(root);
        polish(dialog->surfaceItem());
        QTRY_VERIFY(QRectF(0, 0, 320, 360).contains(dialog->presentationRect()));
        verifyText(dialog->surfaceItem());
        if (name == "typography") {
            auto samples = dialog->contentItem();
            QCOMPARE(dialog->leftPadding(), 0.0);
            QCOMPARE(dialog->rightPadding(), 0.0);
            QCOMPARE(dialog->topPadding(), 0.0);
            QCOMPARE(dialog->bottomPadding(), 0.0);
            QCOMPARE(samples->width(), dialog->width());
            QCOMPARE(samples->property("leftMargin").toReal(), 24.0);
            QCOMPARE(samples->property("rightMargin").toReal(), 24.0);
            QCOMPARE(samples->property("topMargin").toReal(), 20.0);
            QCOMPARE(samples->property("bottomMargin").toReal(), 20.0);
            QCOMPARE(samples->property("contentWidth").toReal(), samples->width() - 48);
            QCOMPARE(samples->property("contentX").toReal(), -24.0);
            QCOMPARE(samples->property("contentY").toReal(), -20.0);
            QVERIFY(samples->property("contentHeight").toReal() > samples->height());

            auto header = dialog->header();
            QVERIFY(header);
            auto scroll = header->property("scrollBehavior").value<qml_material::AppBarScroll*>();
            QVERIFY(scroll);
            QCOMPARE(scroll->mode(), qml_material::AppBarScroll::Pinned);
            QVERIFY(! scroll->overlapped());
            auto background = header->property("background").value<QQuickItem*>();
            QVERIFY(background);
            const auto headerHeight = header->height();
            QTRY_COMPARE(background->property("color"), header->property("backgroundColor"));

            QVERIFY(samples->setProperty("contentY", 80.0));
            QVERIFY(scroll->overlapped());
            QTRY_COMPARE(background->property("color"),
                         header->property("scrolledBackgroundColor"));
            QCOMPARE(scroll->heightOffset(), 0.0);
            QCOMPARE(header->height(), headerHeight);

            QVERIFY(samples->setProperty("contentY", -samples->property("topMargin").toReal()));
            QVERIFY(! scroll->overlapped());
            QTRY_COMPARE(background->property("color"), header->property("backgroundColor"));
        }
        auto close = dialog->standardButton(qml_material::Dialog::Close);
        QVERIFY(close);
        dialog->setExit(nullptr);
        mouseClick(close);
        QTRY_VERIFY(! dialog);
        QCOMPARE(pages->currentItem(), currentPage.data());
        QCOMPARE(pages->currentKey(), currentKey);
        QCOMPARE(pageChanged.count(), 0);
        QCOMPARE(routeChanged.count(), 0);
    }
};

int run_example_navigation(int argc, char** argv) {
    ExampleNavigationTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "example_navigation.moc"
