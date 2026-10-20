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
#include "qml_material/control/popup.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/input/interaction_source.hpp"
#include "qml_material/anim/ripple_motion.hpp"
#include "qml_material/anim/spring_motion.hpp"

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

void click(QObject* button) {
    QVERIFY(button);
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
}
} // namespace

class QuickSettingsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot "
                               "assign|Cannot open|TypeError|ReferenceError|polish.*loop).*"));
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
        auto* iconButton = qobject_cast<qml_material::ButtonControl*>(iconTarget);
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
        auto* tileButton = qobject_cast<qml_material::ButtonControl*>(wifi);
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

        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
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

    void brightnessRequests() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/QuickControls.qml"));
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* slider = root->findChild<qml_material::Slider*>("brightnessSlider");
        QVERIFY(slider);
        QSignalSpy requests(slider, &qml_material::Slider::valueChangeRequested);
        slider->requestValue(75);
        QCOMPARE(requests.size(), 1);
        QCOMPARE(root->property("brightness").toReal(), 75);
        QCOMPARE(slider->value(), 75);
        root->setProperty("brightness", 30);
        QCOMPARE(slider->value(), 30);
        QCOMPARE(requests.size(), 1);
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

    void quickTileBounce_data() {
        QTest::addColumn<int>("columns");
        QTest::addColumn<bool>("rtl");
        QTest::addColumn<int>("width");
        QTest::addColumn<bool>("dark");
        for (int columns : { 2, 3, 5 }) {
            for (int width : { 320, 900 }) {
                const auto name = QString::number(columns) + "-" + QString::number(width);
                QTest::newRow(qPrintable(name + "-ltr")) << columns << false << width << false;
                QTest::newRow(qPrintable(name + "-rtl")) << columns << true << width << false;
                if (columns == 3)
                    QTest::newRow(qPrintable(name + "-dark")) << columns << false << width << true;
            }
        }
    }

    void quickTileBounce() {
        QFETCH(int, columns);
        QFETCH(bool, rtl);
        QFETCH(int, width);
        QFETCH(bool, dark);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example/compositions" as Demo
            Demo.QuickControls {
                id: root
                property bool dark: false
                MD.MProp.color: MD.MdColorMgr {
                    useSysColorSM: false
                    mode: root.dark ? MD.Enum.Dark : MD.Enum.Light
                }
                width: 720; height: implicitHeight; rows: 4
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { "columns", columns },
              { "width", width },
              { "dark", dark },
              { "layoutDirection", rtl ? Qt::RightToLeft : Qt::LeftToRight } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto*        root = qobject_cast<QQuickItem*>(object.get());
        QQuickWindow window;
        window.resize(width, 600);
        window.setColor(dark ? QColor("#101418") : QColor("#ffffff"));
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto* tile = visualChild(root, "flashlightTile");
        QVERIFY(tile);
        auto* motion = tile->findChild<qml_material::PressMotion*>("tileBounceMotion");
        QVERIFY(motion);
        QCOMPARE(tile->property("mirrored").toBool(), rtl);
        const qreal baseline     = tile->width();
        const auto  slotPosition = tile->parentItem()->position();
        auto*       previous     = columns == 2 ? nullptr : visualChild(root, "bluetoothTile");
        auto*       next         = columns == 3 ? nullptr : visualChild(root, "focusTile");
        const auto  press        = tile->mapToScene({ tile->width() / 2, 36 }).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, press);
        QTRY_COMPARE(motion->targetValue(), 1);
        motion->findChild<QAbstractAnimation*>()->setCurrentTime(100);
        const qreal bounce = tile->property("bounce").toReal();
        QVERIFY(bounce > 0 && bounce < 8);
        QCOMPARE(tile->parentItem()->position(), slotPosition);
        QCOMPARE(tile->width(), baseline + bounce * ((previous != nullptr) + (next != nullptr)));
        if (previous) QCOMPARE(previous->width(), baseline - bounce);
        if (next) QCOMPARE(next->width(), baseline - bounce);
        const auto tileBounds = tile->mapRectToItem(root, tile->boundingRect());
        if (previous) {
            const auto bounds = previous->mapRectToItem(root, previous->boundingRect());
            const auto gap =
                rtl ? bounds.left() - tileBounds.right() : tileBounds.left() - bounds.right();
            QVERIFY(std::abs(gap - 8) < 1e-6);
        }
        if (next) {
            const auto bounds = next->mapRectToItem(root, next->boundingRect());
            const auto gap =
                rtl ? tileBounds.left() - bounds.right() : bounds.left() - tileBounds.right();
            QVERIFY(std::abs(gap - 8) < 1e-6);
        }
        if (columns == 3) QCOMPARE(visualChild(root, "focusTile")->width(), baseline);
        const auto captureDirectory = qEnvironmentVariable("QM_MOTION_CAPTURE_DIR");
        if (! captureDirectory.isEmpty()) {
            motion->findChild<QAbstractAnimation*>()->pause();
            const auto frame = window.grabWindow();
            QVERIFY(! frame.isNull());
            QVERIFY(QDir().mkpath(captureDirectory));
            QVERIFY(
                frame.save(captureDirectory + "/quick-tile-" + QTest::currentDataTag() + ".png"));
        }
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, press);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(motion->targetValue(), 0);
        motion->complete();
        QCOMPARE(tile->width(), baseline);
        QVERIFY(root->property("flashlightEnabled").toBool());
        if (columns == 5) {
            auto*      last       = visualChild(root, "locationTile");
            auto*      lastMotion = last->findChild<qml_material::PressMotion*>("tileBounceMotion");
            const auto point      = last->mapToScene({ last->width() / 2, 36 }).toPoint();
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
            lastMotion->findChild<QAbstractAnimation*>()->setCurrentTime(100);
            const auto amount = last->property("bounce").toReal();
            QCOMPARE(last->width(), baseline + 2 * amount);
            QCOMPARE(visualChild(root, "hotspotTile")->width(), baseline - amount);
            QVERIFY(root->boundingRect().contains(last->mapRectToItem(root, last->boundingRect())));
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
            lastMotion->complete();
        }

        auto* wifi       = visualChild(root, "wifiTile");
        auto* wifiMotion = wifi->findChild<qml_material::PressMotion*>("tileBounceMotion");
        click(wifi->findChild<QObject*>("toggleButton"));
        QCOMPARE(wifiMotion->value(), 0);
        QVERIFY(! wifiMotion->isRunning());
        auto* alarm = visualChild(root, "alarmTile");
        click(alarm);
        auto* contentMotion = alarm->findChild<QQuickAbstractAnimation*>("tileContentBounce");
        QVERIFY(contentMotion && contentMotion->isRunning());
        contentMotion->setCurrentTime(83);
        QVERIFY(std::abs(alarm->property("textBounceScale").toReal() - (1 + .06 * 83 / 167)) <
                1e-6);
        contentMotion->setCurrentTime(167);
        QCOMPARE(alarm->property("textBounceScale").toReal(), 1.06);
        contentMotion->setCurrentTime(250);
        QEasingCurve easing(QEasingCurve::BezierSpline);
        easing.addCubicBezierSegment({ .05, 0 }, { 0, 1 }, { 1, 1 });
        QVERIFY(std::abs(alarm->property("textBounceScale").toReal() -
                         (1.06 - .06 * easing.valueForProgress(83. / 167))) < 1e-6);
        contentMotion->setCurrentTime(334);
        QCOMPARE(alarm->property("textBounceScale").toReal(), 1);
        QVERIFY(! contentMotion->isRunning());
        click(alarm);
        QVERIFY(alarm->setProperty("animationsEnabled", false));
        QCOMPARE(alarm->property("textBounceScale").toReal(), 1);
        QVERIFY(! contentMotion->isRunning());

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, press);
        motion->findChild<QAbstractAnimation*>()->setCurrentTime(40);
        root->setVisible(false);
        QCOMPARE(motion->value(), 0);
        QVERIFY(! motion->isRunning());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, press);
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
        auto* tile = qobject_cast<qml_material::ButtonControl*>(object.get());
        QVERIFY(tile);
        qml_material::InteractionAccumulator tileInput(this);
        tileInput.setSource(tile->interactionSource());
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
            QTRY_VERIFY(tileInput.values().hovered);
            QTRY_VERIFY(ripple->findChild<qml_material::RippleMotion*>()->opacity() > 0);
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
        QTRY_COMPARE(ripple->findChild<qml_material::RippleMotion*>()->opacity(), 0);
    }
};

int run_quick_settings(int argc, char** argv) {
    QuickSettingsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "quick_settings.moc"
