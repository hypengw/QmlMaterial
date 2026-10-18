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
#include "qml_material/control/popup.hpp"
#include "qml_material/control/dialog.hpp"
#include "qml_material/control/clock.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/control/menu.hpp"
#include "qml_material/control/overlay.hpp"
#include "qml_material/control/page_container.hpp"
#include "qml_material/input/swipe_to_dismiss_state.hpp"
#include "qml_material/input/time_state.hpp"
#include "qml_material/indicator/squiggly_progress_geometry.hpp"

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

class CompositionsTest : public QObject {
    Q_OBJECT
private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot "
                               "assign|Cannot open|TypeError|ReferenceError|polish.*loop).*"));
    }
    void selectionPopupDismissal_data() {
        QTest::addColumn<QString>("componentName");
        QTest::addColumn<QString>("buttonName");
        QTest::addColumn<QString>("popupName");
        QTest::addColumn<QByteArray>("valueProperty");
        QTest::addColumn<QString>("selection");
        QTest::newRow("wifi") << QStringLiteral("QuickControls") << QStringLiteral("wifiTile")
                              << QStringLiteral("quickDetails") << QByteArray("network")
                              << QStringLiteral("Guest");
        QTest::newRow("bluetooth")
            << QStringLiteral("QuickControls") << QStringLiteral("bluetoothTile")
            << QStringLiteral("quickDetails") << QByteArray("device")
            << QStringLiteral("Desk speaker");
        QTest::newRow("media-output")
            << QStringLiteral("MediaCard") << QStringLiteral("mediaOutput")
            << QStringLiteral("mediaOutputs") << QByteArray("outputDevice")
            << QStringLiteral("This device");
    }
    void selectionPopupDismissal() {
        QFETCH(QString, componentName);
        QFETCH(QString, buttonName);
        QFETCH(QString, popupName);
        QFETCH(QByteArray, valueProperty);
        QFETCH(QString, selection);
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(600, 720);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl("qrc:/Qcm/Material/Example/compositions/" + componentName + ".qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(560);
        root->setHeight(root->implicitHeight());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        QTRY_VERIFY(visualChild(root, buttonName));
        auto button = visualChild(root, buttonName);
        if (componentName == "QuickControls") button = visualChild(button, "tileLabels");
        QVERIFY(button);
        auto popup = root->findChild<qml_material::Popup*>(popupName);
        QVERIFY(popup);
        popup->setEnter(nullptr);
        popup->setExit(nullptr);
        QSignalSpy shown(popup, &qml_material::Popup::aboutToShow);
        const auto point =
            button->mapToScene({ button->width() / 2, button->height() / 2 }).toPoint();
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(popup->isOpened());
        QCOMPARE(shown.count(), 1);
        QVERIFY(! popup->dim());
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(! popup->isVisible());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(! popup->isVisible());
        QCOMPARE(shown.count(), 1);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(popup->isOpened());
        QCOMPARE(shown.count(), 2);
        polish(popup->contentItem());
        QQuickItem* choice = nullptr;
        for (auto item : popup->contentItem()->childItems()) {
            if (qobject_cast<qml_material::AbstractButton*>(item) &&
                item->property("text") == selection) {
                choice = item;
                break;
            }
        }
        QVERIFY(choice);
        QTest::mouseClick(
            &window,
            Qt::LeftButton,
            Qt::NoModifier,
            choice->mapToScene({ choice->width() / 2, choice->height() / 2 }).toPoint());
        QVERIFY(! popup->isVisible());
        QCOMPARE(root->property(valueProperty.constData()).toString(), selection);
        QCOMPARE(shown.count(), 2);
    }
    void quickControls() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(600, 720);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/QuickControls.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(560);
        root->setHeight(root->implicitHeight());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        QTRY_VERIFY(visualChild(root, "wifiTile"));
        auto* wifi      = visualChild(root, "wifiTile");
        auto* bluetooth = visualChild(root, "bluetoothTile");
        auto* popup     = root->findChild<qml_material::Popup*>("quickDetails");
        QVERIFY(wifi && bluetooth && popup);
        QCOMPARE(wifi->height(), 72);
        auto* iconTarget = wifi->findChild<QQuickItem*>("toggleButton");
        QVERIFY(iconTarget);
        QCOMPARE(iconTarget->size(), QSizeF(56, 56));
        auto* iconButton = qobject_cast<qml_material::Button*>(iconTarget);
        QVERIFY(iconButton);
        QCOMPARE(iconButton->background()->size(), QSizeF(56, 56));
        QCOMPARE(iconButton->icon()->width(), 28);
        QCOMPARE(wifi->findChild<QQuickItem*>("tileChevron")->size(), QSizeF(32, 20));
        QVERIFY(! wifi->findChild<QQuickItem*>("tileChevron")->isVisible());
        for (const auto* name : { "tileLabel", "tileSecondaryLabel" }) {
            auto* label = wifi->findChild<QQuickItem*>(name);
            QVERIFY(label);
            const bool title = QByteArray(name) == "tileLabel";
            QCOMPARE(label->property("font").value<QFont>().pixelSize(), title ? 14 : 12);
            QCOMPARE(label->property("lineHeight").toReal(), title ? 20 : 16);
        }
        auto* tileButton = qobject_cast<qml_material::Button*>(wifi);
        QVERIFY(tileButton);
        QCOMPARE(tileButton->background()->size(), wifi->size());
        auto* tileRipple = wifi->findChild<QQuickItem*>("tileRipple");
        QVERIFY(tileRipple);
        QCOMPARE(tileRipple->size(), wifi->size());
        wifi->setProperty("available", false);
        QVERIFY(! iconTarget->isEnabled());
        wifi->setProperty("available", true);
        popup->setEnter(nullptr);
        popup->setExit(nullptr);
        auto* toggle = wifi->findChild<QQuickItem*>("toggleButton");
        QVERIFY(toggle);
        QTest::mouseClick(
            &window,
            Qt::LeftButton,
            Qt::NoModifier,
            toggle->mapToScene(QPointF(toggle->width() / 2, toggle->height() / 2)).toPoint());
        QVERIFY(! root->property("wifiEnabled").toBool());
        QVERIFY(! popup->isVisible());
        auto* details = wifi->findChild<QQuickItem*>("tileLabels");
        QVERIFY(details);
        QTest::mouseClick(
            &window,
            Qt::LeftButton,
            Qt::NoModifier,
            details->mapToScene(QPointF(details->width() / 2, details->height() / 2)).toPoint());
        QTRY_VERIFY(popup->isOpened());
        QVERIFY(! root->property("wifiEnabled").toBool());
        click(visualChild(popup->contentItem(), "detailChoice1"));
        QTRY_VERIFY(! popup->isVisible());
        QCOMPARE(root->property("network").toString(), QString("Guest"));
        QVERIFY(root->property("wifiEnabled").toBool());
        click(bluetooth->findChild<QObject*>("toggleButton"));
        QVERIFY(! root->property("bluetoothEnabled").toBool());
        click(bluetooth);
        QTRY_VERIFY(popup->isOpened());
        click(visualChild(popup->contentItem(), "detailChoice2"));
        QTRY_VERIFY(! popup->isVisible());
        QCOMPARE(root->property("device").toString(), QString("Keyboard"));
        QVERIFY(root->property("bluetoothEnabled").toBool());
        click(visualChild(root, "focusTile"));
        QVERIFY(root->property("focusEnabled").toBool());
        click(visualChild(root, "flashlightTile"));
        QVERIFY(root->property("flashlightEnabled").toBool());
        auto* slider = root->findChild<QQuickItem*>("brightnessSlider");
        QVERIFY(slider);
        QCOMPARE(slider->height(), 64);
        slider->forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Right);
        QCOMPARE(root->property("brightness").toInt(), 65);
        root->setWidth(288);
        root->setHeight(root->implicitHeight());
        polish(root);
        QCOMPARE(bluetooth->y(), wifi->y());
        QCOMPARE(wifi->width(), 140);
        auto* fill  = slider->findChild<QQuickItem*>("brightnessFill");
        auto* track = slider->findChild<QQuickItem*>("brightnessTrack");
        auto* thumb = slider->findChild<QQuickItem*>("brightnessThumb");
        auto* icon  = slider->findChild<QQuickItem*>("brightnessIcon");
        QVERIFY(fill && track && thumb && icon);
        QCOMPARE(thumb->size(), QSizeF(4, 52));
        QCOMPARE(fill->height(), 40);
        QCOMPARE(track->height(), 40);
        for (int value : { 0, 25, 50, 100 }) {
            root->setProperty("brightness", value);
            const auto center = 2 + (slider->width() - 4) * value / 100.0;
            QCOMPARE(fill->width(), std::max(0.0, center - 8));
            QCOMPARE(track->x(), std::min(slider->width(), center + 8));
            QVERIFY(
                slider->boundingRect().contains(icon->mapRectToItem(slider, icon->boundingRect())));
        }
        click(wifi);
        QTRY_VERIFY(popup->isOpened());
        QTest::keyClick(&window, Qt::Key_Escape);
        QTRY_VERIFY(! popup->isVisible());
        click(wifi);
        QTRY_VERIFY(popup->isOpened());
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(580, 680));
        QTRY_VERIFY(! popup->isVisible());
        click(wifi);
        QTRY_VERIFY(popup->isOpened());
        object.reset();
        QCoreApplication::processEvents();
    }
    void quickPaging() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(800, 720);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/QuickControls.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(720);
        root->setProperty("columns", 3);
        root->setProperty("rows", 3);
        root->setHeight(root->implicitHeight());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        QCOMPARE(root->property("pageCount").toInt(), 2);
        QTRY_VERIFY(visualChild(root, "wifiTile"));
        auto* pager = root->findChild<QQuickItem*>("quickPager");
        QVERIFY(pager);
        QVERIFY(! root->findChild<QObject*>("editQuickTiles"));
        QVERIFY(! root->findChild<QObject*>("quickTileEditor"));
        QCOMPARE(pager->height(), 232);
        QCOMPARE(visualChild(root, "wifiTile")->width(), (720 - 16) / 3.0);
        click(visualChild(root, "wifiTile")->findChild<QObject*>("toggleButton"));
        QVERIFY(! root->property("wifiEnabled").toBool());
        click(root->findChild<QObject*>("nextQuickPage"));
        QTRY_COMPARE(root->property("currentPage").toInt(), 1);
        QTRY_VERIFY(visualChild(root, "nightTile"));
        click(visualChild(root, "nightTile"));
        QVERIFY(root->property("nightLightEnabled").toBool());
        click(root->findChild<QObject*>("previousQuickPage"));
        QTRY_COMPARE(root->property("currentPage").toInt(), 0);
        QVERIFY(! root->property("wifiEnabled").toBool());

        const auto start = pager->mapToScene({ 600, 36 }).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        for (int offset : { 30, 100, 250, 450, 650 })
            QTest::mouseMove(&window, start - QPoint(offset, 0));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, start - QPoint(650, 0));
        QTRY_COMPARE(root->property("currentPage").toInt(), 1);
        QTRY_VERIFY(! pager->property("moving").toBool());

        root->setProperty("columns", 2);
        QTRY_COMPARE(root->property("currentPage").toInt(), 0);
        polish(root);
        QVERIFY(! root->property("wifiEnabled").toBool());
        QVERIFY(root->property("nightLightEnabled").toBool());
        QVERIFY(visualChild(root, "wifiTile"));
        root->setProperty("expanded", false);
        QCOMPARE(pager->height(), 72);
        QCOMPARE(root->property("currentPage").toInt(), 0);
        object.reset();
        QCoreApplication::processEvents();
    }
    void brightnessGeometry() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/ShadeSlider.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* slider = qobject_cast<qml_material::Slider*>(object.get());
        QVERIFY(slider);
        slider->setSize({ 288, 64 });
        polish(slider);
        auto* fill  = slider->findChild<QQuickItem*>("brightnessFill");
        auto* track = slider->findChild<QQuickItem*>("brightnessTrack");
        auto* thumb = slider->findChild<QQuickItem*>("brightnessThumb");
        auto* icon  = slider->findChild<QQuickItem*>("brightnessIcon");
        QVERIFY(fill && track && thumb && icon);
        for (auto direction : { Qt::LeftToRight, Qt::RightToLeft }) {
            slider->setLayoutDirection(direction);
            for (int value : { 0, 1, 50, 99, 100 }) {
                slider->setValue(value);
                const auto center = 2 + 284 * value / 100.0;
                QCOMPARE(thumb->x(), direction == Qt::LeftToRight ? center - 2 : 286 - center);
                QCOMPARE(fill->width(), std::max(0.0, center - 8));
                QCOMPARE(track->width(), std::max(0.0, 288 - center - 8));
                QCOMPARE(fill->x(), direction == Qt::LeftToRight ? 0 : 288 - fill->width());
                QCOMPARE(track->x(),
                         direction == Qt::LeftToRight ? std::min(288.0, center + 8) : 0);
                QVERIFY(slider->boundingRect().contains(
                    icon->mapRectToItem(slider, icon->boundingRect())));
            }
        }
    }
    void quickTileFeedback() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(360, 160);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example/compositions" as Demo
            Demo.QuickTile {
                width: 256; height: 72
                hoverEnabled: true
                title: "Bluetooth"; status: "Headphones"
                symbol: MD.Token.icon.bluetooth
                active: true
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* tile = qobject_cast<qml_material::Button*>(object.get());
        QVERIFY(tile);
        tile->setParentItem(window.contentItem());
        tile->setPosition({ 16, 16 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(tile);
        auto* ripple = tile->findChild<QQuickItem*>("tileRipple");
        auto* icon   = tile->findChild<QQuickItem*>("toggleButton");
        QVERIFY(ripple && icon);
        QSignalSpy details(tile, SIGNAL(detailsRequested()));
        QSignalSpy toggles(tile, SIGNAL(toggleRequested()));
        QVERIFY(details.isValid() && toggles.isValid());
        for (const auto& position : { QPointF(160, 36), QPointF(128, 4), QPointF(252, 36) }) {
            QTest::mouseMove(&window, tile->mapToScene(position).toPoint());
            QTRY_VERIFY(tile->hovered());
            QTRY_VERIFY(ripple->property("stateOpacity").toReal() > 0);
            QCOMPARE(ripple->size(), tile->size());
            QCOMPARE(ripple->position(), QPointF());
        }
        QTest::mouseClick(
            &window, Qt::LeftButton, Qt::NoModifier, tile->mapToScene({ 128, 4 }).toPoint());
        QCOMPARE(details.size(), 1);
        QCOMPARE(toggles.size(), 0);
        const auto iconCenter = icon->mapToScene({ 28, 28 }).toPoint();
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, iconCenter);
        QCOMPARE(details.size(), 1);
        QCOMPARE(toggles.size(), 1);
        tile->setProperty("dualTarget", false);
        QVERIFY(! icon->isVisible());
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, iconCenter);
        QCOMPARE(details.size(), 1);
        QCOMPARE(toggles.size(), 2);
        tile->forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Space);
        QCOMPARE(toggles.size(), 3);
        tile->setProperty("available", false);
        QTRY_COMPARE(ripple->property("stateOpacity").toReal(), 0);
    }
    void mediaLayouts() {
        QQmlEngine   engine;
        QQuickWindow window;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/MediaCard.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        click(root->findChild<QObject*>("previousTrack"));
        QCOMPARE(root->property("trackIndex").toInt(), 2);
        click(root->findChild<QObject*>("nextTrack"));
        QCOMPARE(root->property("trackIndex").toInt(), 0);
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(root->property("playing").toBool());
        click(root->findChild<QObject*>("nextTrack"));
        QCOMPARE(root->property("trackIndex").toInt(), 1);
        auto* artwork = root->findChild<QQuickItem*>("mediaArtwork");
        QVERIFY(artwork);
        QTRY_COMPARE(artwork->property("status").toInt(), 1);
        for (int width : { 288, 640 }) {
            for (bool expanded : { false, true }) {
                root->setWidth(width);
                root->setProperty("expanded", expanded);
                root->setHeight(root->implicitHeight());
                polish(root);
                QCOMPARE(root->height(), expanded ? 184 : 128);
                if (expanded) verifyText(root);
                QCoreApplication::processEvents();
                polish(root);
                QCOMPARE(root->property("trackIndex").toInt(), 1);
                QVERIFY(root->property("playing").toBool());
                for (const auto* name :
                     { "mediaArtwork", "mediaTitle", "mediaControls", "mediaTimeline" }) {
                    auto* item = root->findChild<QQuickItem*>(name);
                    QVERIFY(item);
                    const auto bounds = item->mapRectToItem(root, item->boundingRect());
                    QVERIFY2(root->boundingRect().contains(bounds), name);
                }
                auto* seek = root->findChild<QQuickItem*>("mediaSeek");
                QCOMPARE(seek->isVisible(), expanded);
                for (const auto* name : { "playPause", "previousTrack", "nextTrack" }) {
                    auto* button = root->findChild<qml_material::Button*>(name);
                    QVERIFY(button);
                    QCOMPARE(button->size(), QSizeF(48, 48));
                    QCOMPARE(button->background()->size(), QSizeF(48, 48));
                    QCOMPARE(button->icon()->width(), 24);
                    QCOMPARE(button->padding(), 12);
                }
                QCOMPARE(artwork->size(), QSizeF(width, expanded ? 184 : 128));
                QCOMPARE(artwork->property("radius").toReal(), 28);
                QCOMPARE(qobject_cast<qml_material::Control*>(root)
                             ->background()
                             ->property("radius")
                             .toReal(),
                         28);
            }
        }
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(! root->property("playing").toBool());
    }
    void mediaSeek() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(640, 400);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/MediaCard.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(640);
        root->setHeight(root->implicitHeight());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto* seek = root->findChild<qml_material::Slider*>("mediaSeek");
        auto* wave = root->findChild<QQuickItem*>("mediaWave");
        QVERIFY(seek && wave);
        QCOMPARE(root->height(), 184);
        QCOMPARE(seek->height(), 48);
        QCOMPARE(wave->height(), 48);
        QVERIFY(wave->isVisible());
        QCOMPARE(wave->width(), seek->width());
        QCOMPARE(wave->property("progress").toReal(), seek->position());
        QCOMPARE(wave->property("waveLength").toReal(), 20);
        QCOMPARE(wave->property("waveAmplitude").toReal(), 1.5);
        QCOMPARE(wave->property("waveSpeed").toReal(), 8);
        QVERIFY(! wave->property("transitionEnabled").toBool());
        QCOMPARE(seek->value(), 52);
        QCOMPARE(root->findChild<QObject*>("mediaElapsed")->property("text").toString(),
                 QString("0:52"));
        QCOMPARE(root->findChild<QObject*>("mediaDuration")->property("text").toString(),
                 QString("5:03"));
        QVERIFY(! wave->property("animate").toBool());
        QSignalSpy commits(seek, SIGNAL(seekRequested(double)));
        QVERIFY(commits.isValid());
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(wave->property("animate").toBool());
        QTRY_COMPARE(wave->property("amplitudeFraction").toReal(), 1);
        const auto phaseBeforeFrame = wave->property("phase").toReal();
        QTRY_VERIFY(wave->property("phase").toReal() != phaseBeforeFrame);
        const auto point =
            seek->mapToScene(QPointF(seek->width() * .7, seek->height() / 2)).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(seek->pressed());
        QVERIFY(! wave->property("animate").toBool());
        QCOMPARE(root->property("position").toReal(), 52);
        QCOMPARE(commits.size(), 0);
        QVERIFY(seek->position() > .65);
        QCOMPARE(wave->property("progress").toReal(), seek->position());
        QVERIFY(root->findChild<QQuickItem*>("mediaElapsed")->isVisible());
        QVERIFY(! root->findChild<QQuickItem*>("previousTrack")->isVisible());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(root->property("position").toReal() > 200);
        QCOMPARE(root->property("position").toReal(), seek->value());
        QCOMPARE(commits.size(), 1);
        QVERIFY(! root->findChild<QQuickItem*>("mediaElapsed")->isVisible());
        seek->forceActiveFocus();
        const auto before = seek->value();
        QTest::keyClick(&window, Qt::Key_Right);
        QCOMPARE(root->property("position").toReal(), before + 1);
        root->setProperty("expanded", false);
        QCOMPARE(seek->value(), before + 1);
        click(root->findChild<QObject*>("nextTrack"));
        QCOMPARE(root->property("position").toReal(), 0);
        QCOMPARE(seek->to(), 248);
        root->setProperty("expanded", true);
        seek->forceActiveFocus();
        root->setProperty("position", 248);
        QTest::keyClick(&window, Qt::Key_Right);
        QCOMPARE(root->property("position").toReal(), 248);
        root->setProperty("position", 0);
        QTest::keyClick(&window, Qt::Key_Left);
        QCOMPARE(root->property("position").toReal(), 0);
        root->setProperty("position", 52);
        QTRY_VERIFY(wave->property("animating").toBool());
        root->setVisible(false);
        QVERIFY(! wave->property("animating").toBool());
        root->setVisible(true);
        QTRY_VERIFY(wave->property("animating").toBool());
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(! wave->property("animate").toBool());
        const auto stoppedPhase = wave->property("phase").toReal();
        QTRY_COMPARE(wave->property("amplitudeFraction").toReal(), 0);
        QCOMPARE(wave->property("phase").toReal(), stoppedPhase);
        QVERIFY(! wave->property("animating").toBool());
        QCOMPARE(wave->property("progress").toReal(), seek->position());
        const auto committed = commits.size();
        const auto cancelPoint =
            seek->mapToScene(QPointF(seek->width() * .2, seek->height() / 2)).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, cancelPoint);
        QCOMPARE(root->property("position").toReal(), 52);
        QTest::keyClick(&window, Qt::Key_Escape);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, cancelPoint);
        QCOMPARE(root->property("position").toReal(), 52);
        QCOMPARE(commits.size(), committed);
        root->setProperty("seekAvailable", false);
        QVERIFY(! seek->isEnabled());
        QVERIFY(! seek->handle()->isVisible());
        QVERIFY(wave->property("transitionEnabled").toBool());
        click(root->findChild<QObject*>("playPause"));
        QTRY_COMPARE(wave->property("amplitudeFraction").toReal(), 1);
        QVERIFY(wave->property("animating").toBool());
        seek->setProperty("listening", false);
        QVERIFY(! wave->property("animate").toBool());
        QTRY_VERIFY(! wave->property("animating").toBool());
    }
    void notifications() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(400, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl("qrc:/Qcm/Material/Example/compositions/NotificationList.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setProperty("animationsEnabled", false);
        root->setParentItem(window.contentItem());
        root->setSize({ 360, 740 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto* feed = root->findChild<QObject*>("notificationFeed");
        QVERIFY(feed);
        auto* message = visualChild(root, "notification-message");
        QVERIFY(message);
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(message->property("expanded").toBool());
        QVERIFY(message->height() > 72);
        click(message->findChild<QObject*>("replyNotification"));
        QVERIFY(message->property("composing").toBool());
        auto* input = message->findChild<QQuickItem*>("notificationReplyInput");
        QVERIFY(input);
        input->forceActiveFocus();
        for (char character : QByteArrayLiteral("See you there"))
            QTest::keyClick(&window, character);
        QTest::keyClick(&window, Qt::Key_Return);
        QCOMPARE(message->property("message").toString(), QString("You: See you there"));
        QVERIFY(! message->property("composing").toBool());
        QVERIFY(! input->hasActiveFocus());
        QCOMPARE(feed->property("count").toInt(), 2);
        click(message->findChild<QObject*>("replyNotification"));
        QVERIFY(message->property("composing").toBool());
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(! message->property("expanded").toBool());
        QVERIFY(! message->property("composing").toBool());
        QVERIFY(! input->hasActiveFocus());
        click(message->findChild<QObject*>("expandNotification"));
        click(message->findChild<QObject*>("readNotification"));
        QCOMPARE(feed->property("count").toInt(), 1);
        QVERIFY(! visualChild(root, "notification-message"));
        QVERIFY(QMetaObject::invokeMethod(feed, "reset"));
        QCOMPARE(feed->property("count").toInt(), 2);
        message = visualChild(root, "notification-message");
        QVERIFY(message);
        message->setProperty("animationsEnabled", false);
        QCoreApplication::processEvents();
        polish(root);
        const auto start = message->mapToScene({ 40, 44 }).toPoint();
        QVERIFY(window.contentItem()->boundingRect().contains(start));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        for (int offset : { 20, 60, 120, 200 })
            QTest::mouseMove(&window, start + QPoint(offset, 0));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, start + QPoint(200, 0));
        QTRY_COMPARE(feed->property("count").toInt(), 1);
        QVERIFY(! visualChild(root, "notification-message"));
    }
    void notificationTemplates() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(400, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl("qrc:/Qcm/Material/Example/compositions/NotificationCard.qml"));
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { "app", "Downloads" },
              { "title", "Download complete" },
              { "message", "12 images" },
              { "detail", "12 images saved to Downloads." },
              { "age", "Now" },
              { "symbol", "download_done" },
              { "thumbnail",
                QUrl("qrc:/Qcm/Material/Example/assets/carousel/abstract-mint-square.jpg") },
              { "animationsEnabled", false } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* card = qobject_cast<QQuickItem*>(object.get());
        card->setParentItem(window.contentItem());
        card->setWidth(360);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(card);
        QCOMPARE(card->height(), 72);
        QCOMPARE(card->findChild<QQuickItem*>("notificationAppIcon")->size(), QSizeF(40, 40));
        auto* thumbnail = card->findChild<QQuickItem*>("notificationThumbnail");
        auto* summary   = card->findChild<QQuickItem*>("notificationSummary");
        QVERIFY(thumbnail && summary);
        QVERIFY(thumbnail->isVisible());
        QCOMPARE(thumbnail->size(), QSizeF(48, 48));
        QTRY_COMPARE(thumbnail->property("status").toInt(), 1);
        QCOMPARE(summary->x() + summary->width(), thumbnail->x() - 16);
        auto* expander =
            qobject_cast<qml_material::Button*>(card->findChild<QObject*>("expandNotification"));
        QVERIFY(expander);
        QCOMPARE(expander->background()->size(), QSizeF(26, 18));
        click(expander);
        auto* title  = card->findChild<QQuickItem*>("notificationExpandedTitle");
        auto* detail = card->findChild<QQuickItem*>("notificationDetail");
        QVERIFY(title && detail);
        QCOMPARE(title->y(), 38);
        QCOMPARE(detail->y(), title->y() + title->height() + 8);
        QCOMPARE(card->height(), detail->y() + detail->implicitHeight() + 16);
        card->setProperty("title", "");
        QCOMPARE(
            card->findChild<QObject*>("notificationCollapsedTitle")->property("text").toString(),
            QString("Downloads"));
        QCOMPARE(detail->y(), 46);
        card->setProperty("ongoing", true);
        auto* state = card->property("dismissState").value<qml_material::SwipeToDismissState*>();
        QVERIFY(state);
        QVERIFY(! card->property("startToEndEnabled").toBool());
        QVERIFY(! card->property("endToStartEnabled").toBool());
    }
    void notificationExpansion() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(1000, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example/compositions" as Demo
            MD.VerticalFlickable {
                property alias animationsEnabled: list.animationsEnabled
                Demo.NotificationList { id: list; width: parent.width }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setSize({ 900, 740 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto* message  = visualChild(root, "notification-message");
        auto* calendar = visualChild(root, "notification-calendar");
        auto* scroll   = root;
        QVERIFY(message && calendar && scroll);
        auto* detail    = message->findChild<QQuickItem*>("notificationDetail");
        auto* summary   = message->findChild<QQuickItem*>("notificationSummary");
        auto* animation = message->findChild<QObject*>("notificationHeightAnimation");
        QVERIFY(detail && summary && animation);
        QCOMPARE(animation->property("duration").toInt(), 360);
        QCOMPARE(message->height(), 72);
        const auto   calendarY = calendar->mapToItem(root, QPointF()).y();
        const auto   scrollY   = scroll->property("contentY").toReal();
        QList<qreal> heights;
        const auto   samples = connect(message, &QQuickItem::heightChanged, this, [&] {
            heights.append(message->height());
        });
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), 72);
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        const auto expandedHeight = message->property("expandedHeight").toReal();
        QCOMPARE(message->height(), expandedHeight);
        QVERIFY(std::any_of(heights.begin(), heights.end(), [&](qreal height) {
            return height > 72 && height < expandedHeight;
        }));
        QCOMPARE(detail->opacity(), 1);
        QCOMPARE(summary->opacity(), 0);
        polish(root);
        QCOMPARE(calendar->mapToItem(root, QPointF()).y() - calendarY, expandedHeight - 72);
        QCOMPARE(scroll->property("contentY").toReal(), scrollY);
        click(message->findChild<QObject*>("replyNotification"));
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), expandedHeight + 64);
        auto* input = message->findChild<QQuickItem*>("notificationReplyInput");
        QVERIFY(input);
        QTRY_VERIFY(input->hasActiveFocus());
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(! message->property("composing").toBool());
        QVERIFY(! input->hasActiveFocus());
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), 72);
        QCOMPARE(summary->opacity(), 1);
        QCOMPARE(detail->opacity(), 0);
        disconnect(samples);

        bool       reversed   = false;
        bool       continuous = false;
        const auto reversal   = connect(message, &QQuickItem::heightChanged, this, [&] {
            if (! reversed && message->height() > 72 && message->height() < expandedHeight) {
                reversed = true;
                QMetaObject::invokeMethod(
                    message,
                    [&] {
                        const auto before = message->height();
                        message->setProperty("expanded", false);
                        continuous = message->height() == before;
                    },
                    Qt::QueuedConnection);
            }
        });
        message->setProperty("expanded", true);
        QTRY_VERIFY(reversed && ! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), 72);
        QVERIFY(continuous);
        disconnect(reversal);

        bool       reduced   = false;
        const auto reduction = connect(message, &QQuickItem::heightChanged, this, [&] {
            if (! reduced && message->height() > 72 && message->height() < expandedHeight) {
                reduced = true;
                root->setProperty("animationsEnabled", false);
            }
        });
        message->setProperty("expanded", true);
        QTRY_VERIFY(reduced && ! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), expandedHeight);
        message->setProperty("expanded", false);
        QCOMPARE(message->height(), 72);
        disconnect(reduction);
        root->setProperty("animationsEnabled", true);
        message->setProperty("expanded", true);
        QVERIFY(message->property("expansionAnimating").toBool());
        window.hide();
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), expandedHeight);
    }
    void squigglyGeometry() {
        using namespace qml_material;
        SquigglyProgressState state;
        state.progress          = .4;
        state.amplitudeFraction = 1;
        state.transitionEnabled = false;
        const auto paths        = SquigglyProgressGeometry::render(state, { 200, 16 });
        QVERIFY(paths.valid);
        QVERIFY(std::abs(paths.active.boundingRect().left()) < .0001);
        QVERIFY(std::abs(paths.active.boundingRect().right() - 80) < .0001);
        QCOMPARE(paths.inactive.boundingRect(), QRectF(80, 8, 120, 0));
        QVERIFY(paths.active.boundingRect().height() > 2.9);
        state.phase = 20;
        QCOMPARE(SquigglyProgressGeometry::render(state, { 200, 16 }).active, paths.active);
        state.transitionEnabled = true;
        const auto transition   = SquigglyProgressGeometry::render(state, { 200, 16 });
        QVERIFY(transition.inactive.boundingRect().height() > .5);
        QVERIFY(std::abs(transition.inactive.currentPosition().y() - 8) < .0001);
        state.amplitudeFraction = 0;
        QCOMPARE(
            SquigglyProgressGeometry::render(state, { 200, 16 }).active.boundingRect().height(), 0);
        for (auto progress : { 0.0, .02, .98, 1.0 }) {
            state.progress          = progress;
            state.amplitudeFraction = 1;
            state.transitionEnabled = false;
            const auto edge         = SquigglyProgressGeometry::render(state, { 200, 16 });
            QVERIFY(edge.valid);
            if (progress > .9) QVERIFY(edge.active.boundingRect().height() > 2.9);
        }
        state.progress = std::numeric_limits<qreal>::quiet_NaN();
        QVERIFY(! SquigglyProgressGeometry::render(state, { 200, 16 }).valid);
    }
    void pickers() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(440, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/TimePickers.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(400);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* time   = root->findChild<qml_material::TimeState*>("pickerTime");
        auto* picker = root->findChild<QQuickItem*>("exampleTimePicker");
        auto* dialog = root->findChild<qml_material::Dialog*>();
        QVERIFY(time && picker && dialog);
        QCOMPARE(picker->property("time").value<qml_material::TimeState*>(), time);
        QCOMPARE(time->value(), 10 * 60 + 30);
        click(root->findChild<QObject*>("picker24HourChoice"));
        QCOMPARE(time->hourFormat(), qml_material::TimeState::Hour24);
        click(picker->findChild<QObject*>("timeModeToggle"));
        QCOMPARE(time->displayMode(), qml_material::TimeState::Input);
        click(picker->findChild<QObject*>("timeModeToggle"));
        QCOMPARE(time->displayMode(), qml_material::TimeState::Clock);
        dialog->setEnter(nullptr);
        dialog->setExit(nullptr);
        QVERIFY(QMetaObject::invokeMethod(root, "openDialog"));
        QTRY_VERIFY(dialog->isOpened());
        auto* draft = dialog->property("time").value<qml_material::TimeState*>();
        QVERIFY(draft && draft != time);
        QCOMPARE(draft->value(), time->value());
        QCOMPARE(draft->hourFormat(), time->hourFormat());
        draft->setTime(18, 45);
        QCOMPARE(time->value(), 10 * 60 + 30);
        QVERIFY(QMetaObject::invokeMethod(dialog, "reject"));
        QTRY_VERIFY(! dialog->isVisible());
        QCOMPARE(time->value(), 10 * 60 + 30);
        QVERIFY(QMetaObject::invokeMethod(root, "openDialog"));
        QTRY_VERIFY(dialog->isOpened());
        QCOMPARE(draft->value(), time->value());
        draft->setTime(18, 45);
        QVERIFY(QMetaObject::invokeMethod(dialog, "accept"));
        QTRY_VERIFY(! dialog->isVisible());
        QCOMPARE(time->value(), 18 * 60 + 45);
        time->setTime(7, 15);
        QCOMPARE(root->findChild<QObject*>("exampleTimeField")->property("value").toInt(),
                 time->value());
        root->setWidth(288);
        QCOMPARE(time->displayMode(), qml_material::TimeState::Clock);
        QCOMPARE(picker->width(), root->width());
        auto* field = root->findChild<QObject*>("exampleTimeField");
        field->setProperty("readOnly", true);
        QVERIFY(QMetaObject::invokeMethod(root, "openDialog"));
        QVERIFY(! dialog->isVisible());
        std::unique_ptr<QObject> narrow(
            component.createWithInitialProperties({ { "width", 288 } }));
        QVERIFY2(narrow, qPrintable(component.errorString()));
        QCOMPARE(narrow->findChild<qml_material::TimeState*>("pickerTime")->displayMode(),
                 qml_material::TimeState::Input);
    }
    void pickerLayout() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQuickWindow window;
        window.resize(440, 900);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        for (bool time : { true, false }) {
            const auto    url = time ? "qrc:/Qcm/Material/Example/compositions/TimePickers.qml"
                                     : "qrc:/Qcm/Material/Example/compositions/DatePickers.qml";
            QQmlComponent component(&engine, QUrl(QString::fromLatin1(url)));
            std::unique_ptr<QObject> object(component.create());
            QVERIFY2(object, qPrintable(component.errorString()));
            auto* root = qobject_cast<QQuickItem*>(object.get());
            QVERIFY(root);
            root->setParentItem(window.contentItem());
            auto* picker =
                root->findChild<QQuickItem*>(time ? "exampleTimePicker" : "exampleDatePicker");
            auto* field =
                root->findChild<QQuickItem*>(time ? "exampleTimeField" : "exampleDateField");
            auto* choice = root->findChild<QQuickItem*>(time ? "pickerHourFormatChoice"
                                                             : "pickerDateSelectionChoice");
            auto* viewport =
                time ? picker : root->findChild<QQuickItem*>("exampleDatePickerViewport");
            auto* background = root->findChild<QQuickItem*>(time ? "exampleTimePickerBackground"
                                                                 : "exampleDatePickerBackground");
            QVERIFY(picker && field && choice && viewport && background);
            for (int width : { 288, 360, 400 }) {
                root->setWidth(width);
                polish(root);
                QCoreApplication::processEvents();
                polish(root);
                for (auto* child : { field, choice, viewport }) {
                    const auto bounds =
                        child->mapRectToItem(root, QRectF(QPointF(), child->size()));
                    QVERIFY(qAbs(bounds.center().x() - width / 2.0) < .01);
                    QVERIFY(bounds.left() >= -.01 && bounds.right() <= width + .01);
                }
                QVERIFY(picker->width() <= picker->implicitWidth());
                QCOMPARE(background->size(), picker->size());
                QVERIFY(background->property("color").value<QColor>().alpha() == 255);
                QVERIFY(background->property("radius").toReal() > 0);
                const auto pickerBounds =
                    viewport->mapRectToItem(root, QRectF(QPointF(), viewport->size()));
                const auto choiceBounds =
                    choice->mapRectToItem(root, QRectF(QPointF(), choice->size()));
                QVERIFY(pickerBounds.top() >= choiceBounds.bottom() + 15.99);
            }
            if (! time) {
                root->setWidth(288);
                QVERIFY(picker->setProperty("displayMode", 0));
                polish(root);
                QCOMPARE(viewport->width(), 288.0);
                QCOMPARE(picker->width(), picker->implicitWidth());
                QCOMPARE(viewport->property("contentWidth").toReal(), picker->width());
            }
        }
    }
    void pickerSynchronization() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent dates(&engine,
                            QUrl("qrc:/Qcm/Material/Example/compositions/DatePickers.qml"));
        std::unique_ptr<QObject> dateRoot(dates.create());
        QVERIFY2(dateRoot, qPrintable(dates.errorString()));
        dateRoot->setProperty("width", 288);
        auto* picker = dateRoot->findChild<QQuickItem*>("exampleDatePicker");
        auto* field  = dateRoot->findChild<QObject*>("exampleDateField");
        QVERIFY(picker && field);
        QCOMPARE(picker->width(), 288);
        QCOMPARE(picker->property("displayMode").toInt(), 1);
        const QDateTime first(QDate(2026, 10, 20), QTime());
        field->setProperty("value", first);
        QVERIFY(QMetaObject::invokeMethod(field, "modified", Q_ARG(QDateTime, first)));
        QCOMPARE(picker->property("selectedDate").toDateTime().date(), first.date());
        const QDateTime second(QDate(2026, 11, 3), QTime());
        picker->setProperty("selectedDate", second);
        QCOMPARE(field->property("value").toDateTime().date(), second.date());
        click(dateRoot->findChild<QObject*>("pickerDateRangeChoice"));
        QCOMPARE(picker->property("selectionMode").toInt(), 1);
        QVERIFY(! qobject_cast<QQuickItem*>(field)->isVisible());
        click(dateRoot->findChild<QObject*>("pickerSingleDateChoice"));
        QCOMPARE(picker->property("selectionMode").toInt(), 0);
        QQmlComponent colors(&engine,
                             QUrl("qrc:/Qcm/Material/Example/compositions/ColorPickers.qml"));
        std::unique_ptr<QObject> colorRoot(colors.create());
        QVERIFY2(colorRoot, qPrintable(colors.errorString()));
        auto* colorPicker = colorRoot->findChild<QObject*>("exampleColorPicker");
        auto* button      = colorRoot->findChild<QObject*>("exampleColorButton");
        QVERIFY(colorPicker && button);
        const QColor blue("#2468ab");
        button->setProperty("color", blue);
        QVERIFY(QMetaObject::invokeMethod(button, "accepted", Q_ARG(QColor, blue)));
        QCOMPARE(colorPicker->property("color").value<QColor>(), blue);
        const QColor green("#25845a");
        colorPicker->setProperty("color", green);
        QCOMPARE(button->property("color").value<QColor>(), green);
    }
    void page() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(320, 900);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine, QUrl("qrc:/Qcm/Material/Example/Compositions.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setSize({ 320, 900 });
        polish(root);
        QCoreApplication::processEvents();
        polish(root);
        auto* media = root->findChild<QQuickItem*>("standaloneMedia");
        auto* quick = root->findChild<QQuickItem*>("quickControls");
        QVERIFY(media && quick);
        auto* clocks = root->findChild<QQuickItem*>("compositionClocks");
        auto* clock  = clocks->findChild<qml_material::Clock*>("clockPreview");
        QVERIFY(clock);
        auto* alarmTime = root->findChild<qml_material::TimeState*>("pickerTime");
        QVERIFY(alarmTime);
        QCOMPARE(clocks->property("alarmTime").value<qml_material::TimeState*>(), alarmTime);
        alarmTime->setTime(6, 45);
        QCOMPARE(clock->property("alarmText").toString(), alarmTime->displayText());
        clocks->setProperty("showAlarm", false);
        QVERIFY(clock->property("alarmText").toString().isEmpty());
        clocks->setProperty("showAlarm", true);
        click(clocks->findChild<QObject*>("clockFixedChoice"));
        QCOMPARE(clock->source()->mode(), qml_material::ClockSource::Fixed);
        click(clocks->findChild<QObject*>("clockLiveChoice"));
        QCOMPARE(clock->source()->mode(), qml_material::ClockSource::System);
        QVERIFY(media->width() > 0 && media->width() <= 320);
        QVERIFY(quick->width() > 0 && quick->width() <= 320);
        QVERIFY(! root->findChild<QObject*>("systemUiShade"));
        QVERIFY(! root->findChild<QObject*>("editQuickTiles"));
        auto* feed = root->findChild<QObject*>("notificationFeed");
        QVERIFY(feed);
        QCOMPARE(feed->property("count").toInt(), 2);
        QVERIFY(! root->findChild<QObject*>("notificationHistory"));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        click(root->findChild<QObject*>("mediaCompactChoice"));
        QVERIFY(! media->property("expanded").toBool());
        click(root->findChild<QObject*>("mediaExpandedChoice"));
        QVERIFY(media->property("expanded").toBool());
        for (int width : { 320, 900 }) {
            root->setWidth(width);
            polish(root);
            QCoreApplication::processEvents();
            polish(root);
            verifyText(clock);
            for (const char* name : { "quickControls",
                                      "notifications",
                                      "compositionMotion",
                                      "compositionClocks",
                                      "compositionTimePickers",
                                      "compositionDatePickers",
                                      "compositionColorPickers",
                                      "compositionFilePickers" }) {
                auto* section = root->findChild<QQuickItem*>(name);
                QVERIFY(section);
                QVERIFY(section->width() > 0 && section->width() <= root->width());
                QVERIFY(section->height() > 0);
            }
            QCOMPARE(quick->property("columns").toInt(), width >= 712 ? 3 : 2);
        }
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
        if (name == "typography")
            QVERIFY(dialog->contentItem()->property("contentHeight").toReal() >
                    dialog->contentItem()->height());
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
int run_compositions(int argc, char** argv) {
    CompositionsTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "compositions.moc"
