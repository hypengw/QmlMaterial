#include "../input_test.hpp"
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QDir>
#include <cmath>
#include "qml_material/anim/spring_motion.hpp"
#include "qml_material/control/switch.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/style/common_state.hpp"
#include "qml_material/style/rail_item_state.hpp"
#include "qml_material/util/qml_util.hpp"

using namespace qml_material;

namespace
{
QAbstractAnimation* clock(QObject* motion) { return motion->findChild<QAbstractAnimation*>(); }

void finish(QObject* motion) {
    auto* animation = clock(motion);
    animation->setCurrentTime(animation->duration());
}

bool capture(QQuickWindow& window, const QString& name) {
    const auto directory = qEnvironmentVariable("QM_MOTION_CAPTURE_DIR");
    if (directory.isEmpty()) return true;
    const auto frame = window.grabWindow();
    return ! frame.isNull() && QDir().mkpath(directory) &&
           frame.save(directory + "/" + name + ".png");
}

SelectionButton* railButton(QQuickItem* root, const QString& text) {
    if (auto* button = qobject_cast<SelectionButton*>(root); button && button->text() == text)
        return button;
    for (auto* child : root->childItems())
        if (auto* button = railButton(child, text)) return button;
    return nullptr;
}
} // namespace

class ControlMotionTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }

    void railSelection_data() {
        QTest::addColumn<bool>("expanded");
        QTest::addColumn<bool>("iconOnly");
        QTest::addColumn<bool>("rtl");
        for (bool expanded : { false, true })
            for (bool iconOnly : { false, true })
                for (bool rtl : { false, true })
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(expanded).arg(iconOnly).arg(rtl)))
                        << expanded << iconOnly << rtl;
    }

    void railSelection() {
        QFETCH(bool, expanded);
        QFETCH(bool, iconOnly);
        QFETCH(bool, rtl);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Rectangle {
                id: root
                width: 320; height: 180
                color: MD.MProp.color.surface
                property bool expanded: false
                property bool iconOnly: false
                property bool rtl: false
                property bool animate: true
                property int selected: 0
                Column {
                    spacing: 12
                    Repeater {
                        model: ["One", "Two"]
                        MD.RailItem {
                            required property int index
                            required property string modelData
                            objectName: modelData
                            width: root.expanded ? 280 : 96
                            expand: root.expanded
                            expansionProgress: root.expanded ? 1 : 0
                            iconStyle: root.iconOnly ? MD.Enum.IconOnly : MD.Enum.IconAndText
                            layoutDirection: root.rtl ? Qt.RightToLeft : Qt.LeftToRight
                            animationsEnabled: root.animate
                            selected: root.selected === index
                            text: modelData
                            icon.name: MD.Token.icon.star
                            onSelectionRequested: root.selected = index
                        }
                    }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { "expanded", expanded }, { "iconOnly", iconOnly }, { "rtl", rtl } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QQuickWindow window;
        window.resize(320, 180);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* first  = railButton(root, "One");
        auto* second = railButton(root, "Two");
        QVERIFY(first && second);
        auto motion = [](SelectionButton* item) {
            return qobject_cast<SpringMotion*>(
                item->property("_selectionMotion").value<QObject*>());
        };
        auto* firstMotion  = motion(first);
        auto* secondMotion = motion(second);
        QVERIFY(firstMotion && secondMotion);
        QCOMPARE(firstMotion->value(), 1);
        QCOMPARE(secondMotion->value(), 0);
        QVERIFY(! firstMotion->isRunning() && ! secondMotion->isRunning());
        QCOMPARE(firstMotion->spec(), (token::SpringSpec { 700, .9 }));
        auto* bounds       = second->findChild<QQuickItem*>("railIndicatorBounds");
        auto* indicator    = second->findChild<QQuickItem*>("railSelectionIndicator");
        auto* oldIndicator = first->findChild<QQuickItem*>("railSelectionIndicator");
        auto* ripple       = second->findChild<QQuickItem*>("railRipple");
        auto* state        = second->property("mdState").value<RailItemState*>();
        QVERIFY(bounds && indicator && oldIndicator && ripple && state);
        QCOMPARE(indicator->width(), 0);
        QCOMPARE(indicator->opacity(), 0);
        QCOMPARE(ripple->parentItem(), bounds);
        QCOMPARE(indicator->parentItem(), bounds);
        const auto  boundsSize    = bounds->size();
        const auto  itemSize      = second->size();
        const auto  selectedColor = state->selectedIndicatorColor();
        QQuickItem* label         = nullptr;
        auto*       content       = second->property("contentItem").value<QQuickItem*>();
        QVERIFY(content);
        for (auto* child : content->childItems())
            if (child->isVisible() && child->property("text").toString() == "Two") label = child;
        QCOMPARE(label != nullptr, ! iconOnly);
        const auto labelFont = label ? label->property("font").value<QFont>() : QFont();
        auto*      icon      = second->findChild<QQuickItem*>("railItemIcon");
        QVERIFY(icon);
        const auto iconLeading = expanded ? 32.0 : 36.0;
        QCOMPARE(icon->mapToItem(second, QPointF()).x(),
                 rtl ? second->width() - iconLeading - 24 : iconLeading);
        if (! qEnvironmentVariableIsEmpty("QM_MOTION_CAPTURE_DIR")) {
            ripple->setProperty("debugEnabled", true);
            ripple->setProperty("debugOpacity", 0);
            ripple->setProperty("debugBackOpacity", 0);
        }
        InteractionAccumulator secondInput(this);
        secondInput.setSource(second->interactionSource());
        const auto point =
            second->mapToScene(QPointF(second->width() / 2, second->height() / 2)).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point, 0);
        QTRY_VERIFY(secondInput.values().pressed);
        QCOMPARE(ripple->property("interactionSource").value<InteractionSource*>(),
                 second->interactionSource());
        QCOMPARE(object->property("selected").toInt(), 0);
        QCOMPARE(secondMotion->value(), 0);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point, 0);
        QCOMPARE(object->property("selected").toInt(), 1);
        QVERIFY(! first->isSelected() && second->isSelected());
        if (label) QCOMPARE(label->property("font").value<QFont>(), labelFont);
        QVERIFY(firstMotion->isRunning() && secondMotion->isRunning());
        for (auto* track : { firstMotion, secondMotion }) {
            clock(track)->setCurrentTime(40);
            clock(track)->pause();
        }
        const auto progress = anim::sampleSpring(secondMotion->spec(), 0, 1, 0, .04).value;
        QVERIFY(progress > 0 && progress < 1);
        QCOMPARE(secondMotion->value(), progress);
        QCOMPARE(indicator->width(), boundsSize.width() * progress);
        QCOMPARE(indicator->height(), boundsSize.height());
        QCOMPARE(indicator->opacity(), progress);
        QCOMPARE(indicator->x(),
                 iconOnly ? (boundsSize.width() - indicator->width()) / 2
                 : rtl    ? boundsSize.width() - indicator->width()
                          : 0);
        QCOMPARE(indicator->property("color").value<QColor>(), selectedColor);
        QCOMPARE(oldIndicator->property("color").value<QColor>(), selectedColor);
        QVERIFY(oldIndicator->opacity() > 0 && oldIndicator->opacity() < 1);
        QCOMPARE(ripple->size(), boundsSize);
        QCOMPARE(bounds->size(), boundsSize);
        QCOMPARE(second->size(), itemSize);
        for (auto* item : { first, second }) {
            for (auto* label : item->findChildren<ColorSpringMotion*>()) {
                QCOMPARE(label->spec(), (token::SpringSpec { 1600, 1 }));
                if (label->isRunning()) {
                    clock(label)->setCurrentTime(40);
                    clock(label)->pause();
                }
            }
        }
        QVERIFY(capture(window, QString("rail-%1-%2-%3").arg(expanded).arg(iconOnly).arg(rtl)));
        const auto firstValue = firstMotion->value(), firstVelocity = firstMotion->velocity();
        const auto secondValue = secondMotion->value(), secondVelocity = secondMotion->velocity();
        QVERIFY(QMetaObject::invokeMethod(first, "click"));
        QCOMPARE(object->property("selected").toInt(), 0);
        QCOMPARE(firstMotion->value(), firstValue);
        QCOMPARE(firstMotion->velocity(), firstVelocity);
        QCOMPARE(secondMotion->value(), secondValue);
        QCOMPARE(secondMotion->velocity(), secondVelocity);
        firstMotion->complete();
        secondMotion->complete();
        for (auto* item : { first, second })
            for (auto* labelMotion : item->findChildren<ColorSpringMotion*>()) {
                labelMotion->complete();
                QCOMPARE(labelMotion->color(), labelMotion->targetColor());
            }
        QCOMPARE(firstMotion->value(), 1);
        QCOMPARE(indicator->width(), 0);
        QCOMPARE(indicator->opacity(), 0);
        QVERIFY(
            capture(window, QString("rail-%1-%2-%3-final").arg(expanded).arg(iconOnly).arg(rtl)));
        QVERIFY(QMetaObject::invokeMethod(first, "click"));
        QVERIFY(! firstMotion->isRunning() && ! secondMotion->isRunning());
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point, 0);
        QTest::mouseMove(&window, QPoint(310, 170), 0);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(310, 170), 0);
        QCOMPARE(object->property("selected").toInt(), 0);
        QVERIFY(! firstMotion->isRunning() && ! secondMotion->isRunning());
        QVERIFY(object->setProperty("selected", 1));
        QVERIFY(secondMotion->isRunning());
        QVERIFY(object->setProperty("animate", false));
        QCOMPARE(firstMotion->value(), 0);
        QCOMPARE(secondMotion->value(), 1);
        QVERIFY(! firstMotion->isRunning() && ! secondMotion->isRunning());
        QVERIFY(object->setProperty("animate", true));
        QVERIFY(object->setProperty("selected", 0));
        QVERIFY(firstMotion->isRunning());
        window.hide();
        QCOMPARE(firstMotion->value(), 1);
        QCOMPARE(secondMotion->value(), 0);
        QVERIFY(! firstMotion->isRunning() && ! secondMotion->isRunning());
        for (auto* item : { first, second })
            for (auto* label : item->findChildren<ColorSpringMotion*>())
                QVERIFY(! label->isRunning());
    }

    void navigationRailSelection() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.NavigationRail {
                id: control
                width: 96; height: 300
                property int selection: 0
                currentIndex: selection
                onCurrentIndexChangeRequested: index => selection = index
                autoExpand: false
                arrangement: MD.Enum.RailTop
                header: null
                model: [{name: "One", icon: MD.Token.icon.home},
                        {name: "Two", icon: MD.Token.icon.settings}]
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QQuickWindow window;
        window.resize(320, 400);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        SelectionButton *first = nullptr, *second = nullptr;
        auto             findItems = [&] {
            first  = railButton(root, "One");
            second = railButton(root, "Two");
            return first && second;
        };
        QTRY_VERIFY(findItems());
        QVERIFY(first->isSelected() && ! second->isSelected());
        QSignalSpy requests(root, SIGNAL(currentIndexChangeRequested(int)));
        QVERIFY(requests.isValid());
        const auto point =
            second->mapToScene(QPointF(second->width() / 2, second->height() / 2)).toPoint();
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point, 0);
        QCOMPARE(root->property("currentIndex").toInt(), 1);
        QCOMPARE(requests.size(), 1);
        QVERIFY(! first->isSelected() && second->isSelected());
        for (auto* item : { first, second }) {
            auto* motion =
                qobject_cast<SpringMotion*>(item->property("_selectionMotion").value<QObject*>());
            QVERIFY(motion && motion->isRunning());
            motion->complete();
            QCOMPARE(motion->value(), item == second ? 1 : 0);
        }
    }

    void corners() {
        CornerSpringMotion motion;
        motion.classBegin();
        motion.setTargetCorners(CornersGroup(20));
        motion.componentComplete();
        QCOMPARE(motion.corners(), CornersGroup(20));
        motion.setTargetCorners(CornersGroup(4));
        clock(&motion)->setCurrentTime(40);
        const auto middle = motion.corners();
        QVERIFY(middle.topLeft() > 4 && middle.topLeft() < 20);
        motion.setTargetCorners(CornersGroup(20));
        QCOMPARE(motion.corners(), middle);
        motion.setTargetCorners(CornersGroup(8, 9, 10, 11));
        QCOMPARE(motion.corners(), middle);
        finish(&motion);
        QCOMPARE(motion.corners(), (CornersGroup(8, 9, 10, 11)));
        CornerSpringMotion small, large;
        small.setEnabled(false);
        large.setEnabled(false);
        small.setTargetCorners(CornersGroup(10));
        large.setTargetCorners(CornersGroup(100));
        small.setEnabled(true);
        large.setEnabled(true);
        small.setTargetCorners(CornersGroup(5));
        large.setTargetCorners(CornersGroup(50));
        QCOMPARE(clock(&small)->duration(), clock(&large)->duration());
    }

    void buttonPresentation_data() {
        QTest::addColumn<QString>("type");
        QTest::newRow("button") << QString("Button");
        QTest::newRow("icon") << QString("IconButton");
    }

    void buttonPresentation() {
        QFETCH(QString, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(QString("import Qcm.Material as MD\nMD.%1 {width: 120; height: 56; text: "
                                  "'Label'; icon.name: MD.Token.icon.star}")
                              .arg(type)
                              .toUtf8(),
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<ButtonControl*>(object.get());
        QVERIFY(button);
        QQuickWindow window;
        window.resize(240, 100);
        button->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* motion = button->property("_motion").value<QObject*>();
        QVERIFY(motion);
        QTRY_VERIFY(motion->property("_animate").toBool());
        auto* source = motion->property("source").value<CommonState*>();
        auto* shape  = motion->findChild<CornerSpringMotion*>();
        QVERIFY(source && shape);
        const auto initial = source->corners();
        QCOMPARE(shape->corners(), initial);
        setButtonPressed(button, true);
        const auto pressed = source->corners();
        QVERIFY(pressed != initial);
        QCOMPARE(shape->corners(), initial);
        clock(shape)->setCurrentTime(40);
        const auto middle = shape->corners();
        QVERIFY(middle != initial && middle != pressed);
        if (! qEnvironmentVariableIsEmpty("QM_MOTION_CAPTURE_DIR")) clock(shape)->pause();
        QVERIFY(capture(window, "shape-" + type));
        setButtonPressed(button, false);
        QCOMPARE(shape->corners(), middle);
        finish(shape);
        QCOMPARE(shape->corners(), initial);
        QVERIFY(button->setProperty("shapeSpringSpec",
                                    QVariant::fromValue(token::SpringSpec { 800, .6 })));
        QCOMPARE(shape->spec(), (token::SpringSpec { 800, .6 }));
        setButtonPressed(button, true);
        QVERIFY(motion->property("corners").value<CornersGroup>() != pressed);
        QVERIFY(shape->isRunning());
        finish(shape);
        QCOMPARE(motion->property("corners").value<CornersGroup>(), pressed);
        button->setProperty("animationsEnabled", false);
        setButtonPressed(button, false);
        QCOMPARE(motion->property("corners").value<CornersGroup>(), initial);
        button->setProperty("animationsEnabled", true);
        setButtonPressed(button, true);
        QVERIFY(shape->isRunning());
        window.hide();
        QVERIFY(! shape->isRunning());
        QCOMPARE(shape->corners(), source->corners());
    }

    void switchMotion_data() {
        QTest::addColumn<bool>("rtl");
        QTest::newRow("ltr") << false;
        QTest::newRow("rtl") << true;
    }

    void switchMotion() {
        QFETCH(bool, rtl);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Switch { width: 52; height: 48 }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = qobject_cast<Switch*>(object.get());
        QVERIFY(control);
        connect(control, &Switch::checkedChangeRequested, control, &Switch::setChecked);
        control->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        QQuickWindow window;
        window.resize(160, 80);
        control->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* size     = control->findChild<SpringMotion*>("switchSizeMotion");
        auto* position = control->findChild<SpringMotion*>("switchPositionMotion");
        auto* handle   = control->findChild<QQuickItem*>("switchHandle");
        QVERIFY(size && position && handle);
        QCOMPARE(size->value(), 16);
        QCOMPARE(position->value(), 16);
        control->setChecked(true);
        QCOMPARE(control->position(), 1);
        QVERIFY(position->isRunning());
        clock(size)->setCurrentTime(40);
        clock(position)->setCurrentTime(40);
        QVERIFY(size->value() > 16 && size->value() < 24);
        if (! qEnvironmentVariableIsEmpty("QM_MOTION_CAPTURE_DIR")) {
            clock(size)->pause();
            clock(position)->pause();
        }
        QVERIFY(capture(window, rtl ? "switch-rtl" : "switch-ltr"));
        const auto current = position->value();
        control->setChecked(false);
        QCOMPARE(position->value(), current);
        finish(size);
        finish(position);
        QCOMPARE(size->value(), 16);
        QCOMPARE(position->value(), 16);
        InteractionAccumulator controlInput(this);
        controlInput.setSource(control->interactionSource());
        const auto center = control->mapToScene({ 26, 24 }).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, center);
        QTRY_VERIFY(controlInput.values().pressed);
        QCOMPARE(size->value(), 28);
        QVERIFY(! size->isRunning() && ! position->isRunning());
        control->setChecked(true);
        QCOMPARE(position->value(), 16);
        QVERIFY(position->isRunning());
        QCOMPARE(position->targetValue(), 36);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, center);
        finish(size);
        finish(position);
        QCOMPARE(handle->x(),
                 (rtl ? 52 - position->value() : position->value()) - size->value() / 2);
        control->setChecked(false);
        finish(size);
        finish(position);
        const auto start = control->mapToScene({ rtl ? 44. : 8., 24 }).toPoint();
        const auto end   = control->mapToScene({ rtl ? 8. : 44., 24 }).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(&window, end);
        flushInteractions();
        QCOMPARE(control->position(), 44. / 52);
        QCOMPARE(position->value(), 16 + control->position() * 20);
        QVERIFY(control->isDragging() && controlInput.values().dragged);
        QVERIFY(! control->isChecked());
        QVERIFY(! position->isRunning());
        QVERIFY(capture(window, rtl ? "switch-drag-rtl" : "switch-drag-ltr"));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, end);
        QVERIFY(control->isChecked());
        QVERIFY(! control->isDragging());
        QVERIFY(position->isRunning());
        QCOMPARE(position->targetValue(), 36);
        control->forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Space);
        QVERIFY(! control->isChecked());
        control->setProperty("animationsEnabled", false);
        control->setChecked(! control->isChecked());
        QVERIFY(! position->isRunning());
        QVERIFY(! size->isRunning());
    }

    void switchClickMotion_data() {
        QTest::addColumn<bool>("rtl");
        QTest::addColumn<bool>("accept");
        for (bool rtl : { false, true })
            for (bool accept : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(rtl).arg(accept))) << rtl << accept;
    }

    void switchClickMotion() {
        QFETCH(bool, rtl);
        QFETCH(bool, accept);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD; MD.Switch { width: 52; height: 48 }", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = qobject_cast<Switch*>(object.get());
        QVERIFY(control);
        control->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        QQuickWindow window;
        window.resize(160, 80);
        control->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (accept) connect(control, &Switch::checkedChangeRequested, control, &Switch::setChecked);
        auto* position = control->findChild<SpringMotion*>("switchPositionMotion");
        auto* size     = control->findChild<SpringMotion*>("switchSizeMotion");
        auto* handle   = control->findChild<QQuickItem*>("switchHandle");
        QVERIFY(position && size && handle);
        QSignalSpy requests(control, &Switch::checkedChangeRequested);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 26, 24 });
        flushInteractions();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(control->isChecked(), accept);
        QCOMPARE(position->value(), 16);
        QCOMPARE(position->isRunning(), accept);
        QCOMPARE(position->targetValue(), accept ? 36 : 16);
        if (accept) {
            clock(position)->setCurrentTime(40);
            clock(size)->setCurrentTime(40);
            QVERIFY(position->value() > 16 && position->value() < 36);
            QVERIFY(capture(window, rtl ? "switch-click-rtl" : "switch-click-ltr"));
            const auto current = position->value();
            QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 26, 24 });
            flushInteractions();
            QCOMPARE(position->value(), current);
            QCOMPARE(position->targetValue(), 16);
            QVERIFY(position->isRunning());
        }
    }
};

int run_control_motion(int argc, char** argv) {
    ControlMotionTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "control_motion.moc"
