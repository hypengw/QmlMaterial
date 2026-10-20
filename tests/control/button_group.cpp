#include "../input_test.hpp"
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/button_group_container.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/action_command.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/style/button_state.hpp"
#include "qml_material/style/icon_button_state.hpp"

using namespace qml_material;

class CornerObserver : public QObject {
    Q_OBJECT
public:
    std::function<void()> callback;
public slots:
    void changed() {
        if (callback) callback();
    }
};

namespace
{
class TestGroup : public ButtonGroupContainer {
public:
    using ButtonGroupContainer::updatePolish;
};
ButtonGroupContainerAttached* info(QQuickItem* item) {
    return qobject_cast<ButtonGroupContainerAttached*>(
        qmlAttachedPropertiesObject<ButtonGroupContainer>(item, true));
}
void finishPress(QQuickItem* item) {
    flushInteractions();
    info(item)->findChild<PressMotion*>()->complete();
}
void setup(TestGroup& group, CommandButton& button, qreal width = 100) {
    button.setImplicitWidth(width);
    button.setImplicitHeight(40);
    group.addItem(&button);
    info(&button)->setDefaultCompressionLimit(20);
}
} // namespace

class ButtonGroupTest : public QObject {
    Q_OBJECT
private slots:
    void qmlBindings_data() {
        QTest::addColumn<QString>("source");
        QTest::newRow("example") << QFINDTESTDATA("../../example/ButtonGroups.qml");
        QTest::newRow("visual") << QFINDTESTDATA("../visual/scenes/button_group.qml");
    }
    void qmlBindings() {
        QFETCH(QString, source);
        QVERIFY(! source.isEmpty());
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine, QUrl::fromLocalFile(source));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCoreApplication::processEvents();
    }
    void exampleSingleSelection() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl::fromLocalFile(QFINDTESTDATA("../../example/ButtonGroups.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(400, 720);
        std::unique_ptr<QObject> object(component.create());
        auto*                    root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setWidth(340);
        root->setParentItem(window.contentItem());
        auto* group = root->findChild<ButtonGroupContainer*>("fileButtons");
        QVERIFY(group);
        QCOMPARE(group->count(), 3);
        QVERIFY(! group->animateWidth());
        auto* first  = qobject_cast<ToggleButton*>(group->itemAt(0));
        auto* second = qobject_cast<ToggleButton*>(group->itemAt(1));
        auto* last   = qobject_cast<ToggleButton*>(group->itemAt(2));
        QVERIFY(first && second && last);
        QVERIFY(first->isChecked());
        QVERIFY(! second->isChecked() && ! last->isChecked());
        QVERIFY(qobject_cast<ButtonGroupAttached*>(
                    qmlAttachedPropertiesObject<ButtonGroup>(first, false))
                    ->group()
                    ->isExclusive());
        window.show();

        for (const qreal width : { 340.0, 288.0 }) {
            root->setWidth(width);
            QTRY_VERIFY(std::abs(first->width() * 3 + group->spacing() * 2 - width) < .001);
            QCOMPARE(first->width(), second->width());
            QCOMPARE(first->width(), last->width());
            const QList<qreal> positions { first->x(), second->x(), last->x() };
            const qreal        buttonWidth = first->width();
            for (auto* target : { second, last, first }) {
                setButtonPressed(target, true);
                QCOMPARE(target->width(), buttonWidth);
                QCOMPARE(info(target)->pressProgress(), 0);
                setButtonPressed(target, false);
                target->click();
                QCOMPARE(qobject_cast<ButtonGroupAttached*>(
                             qmlAttachedPropertiesObject<ButtonGroup>(first, false))
                             ->group()
                             ->checkedButton(),
                         target);
                for (auto* button : { first, second, last }) {
                    QCOMPARE(button->isChecked(), button == target);
                    QCOMPARE(button->width(), buttonWidth);
                }
                QCOMPARE(QList<qreal>({ first->x(), second->x(), last->x() }), positions);
                const auto* state = target->findChild<ButtonState*>();
                QVERIFY(state);
                QCOMPARE(state->corners(), CornersGroup(target->height() / 2));
                target->click();
                QVERIFY(target->isChecked());
            }
        }
    }
    void defaults() {
        TestGroup group;
        group.updatePolish();
        QCOMPARE(group.contentWidth(), 0);
        QCOMPARE(group.spacing(), 12);
        QVERIFY(group.animateWidth());
        group.setVariant(ButtonGroupContainer::Connected);
        QCOMPARE(group.spacing(), 2);
        QVERIFY(! group.animateWidth());
        group.setSpacing(7);
        group.setAnimateWidth(true);
        group.setVariant(ButtonGroupContainer::Standard);
        group.setVariant(ButtonGroupContainer::Connected);
        QCOMPARE(group.spacing(), 7);
        QVERIFY(group.animateWidth());
        group.resetSpacing();
        group.resetAnimateWidth();
        QCOMPARE(group.spacing(), 2);
        QVERIFY(! group.animateWidth());
        group.setExpandedRatio(-1);
        QCOMPARE(group.expandedRatio(), .15);
        CommandButton only;
        setup(group, only);
        ButtonState state;
        state.setItem(&only);
        group.updatePolish();
        QCOMPARE(info(&only)->position(), int(Enum::ItemPosition::PosSingle));
        QCOMPARE(state.corners(), CornersGroup(20));
        group.setAnimateWidth(true);
        setButtonPressed(&only, true);
        finishPress(&only);
        group.updatePolish();
        QCOMPARE(only.width(), 100);
        QCOMPARE(state.corners(), CornersGroup(20));
    }
    void layoutAndVisibility() {
        TestGroup     group;
        CommandButton a, b, c;
        setup(group, a);
        setup(group, b, 80);
        setup(group, c, 60);
        group.updatePolish();
        QCOMPARE(group.contentWidth(), 264);
        QCOMPARE(b.x(), 112);
        QCOMPARE(c.x(), 204);
        QCOMPARE(info(&a)->position(), int(Enum::ItemPosition::PosFirst));
        QCOMPARE(info(&b)->position(), int(Enum::ItemPosition::PosMiddle));
        QCOMPARE(info(&c)->position(), int(Enum::ItemPosition::PosLast));
        a.setVisible(false);
        group.updatePolish();
        QCOMPARE(b.x(), 0);
        QCOMPARE(info(&b)->position(), int(Enum::ItemPosition::PosFirst));
        QCOMPARE(group.contentWidth(), 152);
        group.setVisible(false);
        group.updatePolish();
        QCOMPARE(group.contentWidth(), 152);
        group.setVisible(true);
        a.setVisible(true);
        group.moveItem(2, 0);
        group.updatePolish();
        QCOMPARE(c.x(), 0);
        QCOMPARE(info(&c)->position(), int(Enum::ItemPosition::PosFirst));
        QCOMPARE(a.metaObject()->indexOfProperty("checkable"), -1);
        QVERIFY(! qmlAttachedPropertiesObject<ButtonGroup>(&a, false));
    }
    void weightsAndMinimum() {
        TestGroup     group;
        CommandButton a, b, c;
        setup(group, a);
        setup(group, b);
        setup(group, c);
        group.setWidth(324);
        info(&a)->setWeight(1);
        info(&b)->setWeight(2);
        info(&c)->setPreferredWidth(60);
        group.updatePolish();
        QCOMPARE(a.width(), 80);
        QCOMPARE(b.width(), 160);
        QCOMPARE(c.width(), 60);
        info(&a)->setMinimumWidth(100);
        group.updatePolish();
        QCOMPARE(a.width(), 100);
        QCOMPARE(b.width(), 140);
        group.setWidth(120);
        group.updatePolish();
        QCOMPARE(a.width(), 100);
        QCOMPARE(b.width(), 0);
        QCOMPARE(c.width(), 60);
        group.setWidth(424);
        group.updatePolish();
        QVERIFY(a.width() > 100);
    }
    void widthAnimation() {
        TestGroup     group;
        CommandButton a, b, c;
        setup(group, a);
        setup(group, b);
        setup(group, c);
        group.updatePolish();
        setButtonPressed(&b, true);
        finishPress(&b);
        group.updatePolish();
        QCOMPARE(a.width(), 92.5);
        QCOMPARE(b.width(), 115);
        QCOMPARE(c.width(), 92.5);
        for (int i = 0; i < 10; ++i) group.updatePolish();
        QCOMPARE(b.width(), 115);
        group.setExpandedRatio(10);
        group.updatePolish();
        QCOMPARE(a.width(), 80);
        QCOMPARE(b.width(), 140);
        QCOMPARE(c.width(), 80);
        setButtonPressed(&a, true);
        setButtonPressed(&c, true);
        finishPress(&a);
        finishPress(&c);
        group.updatePolish();
        QCOMPARE(a.width() + b.width() + c.width(), 300);
        QVERIFY(a.width() >= 80 && b.width() >= 80 && c.width() >= 80);
        const auto before = QList<qreal> { a.width(), b.width(), c.width() };
        group.moveItem(2, 0);
        group.moveItem(2, 1);
        group.updatePolish();
        QCOMPARE(QList<qreal>({ c.width(), b.width(), a.width() }), before);
        group.setAnimateWidth(false);
        group.updatePolish();
        QCOMPARE(a.width(), 100);
        QCOMPARE(info(&b)->pressProgress(), 0);
    }
    void releaseAndRemoval() {
        TestGroup     group;
        CommandButton a, b;
        setup(group, a);
        setup(group, b);
        setButtonPressed(&a, true);
        auto* motion    = info(&a)->findChild<PressMotion*>();
        auto* animation = motion->findChild<QAbstractAnimation*>();
        animation->setCurrentTime(20);
        QVERIFY(info(&a)->pressProgress() > 0 && info(&a)->pressProgress() < .75);
        setButtonPressed(&a, false);
        finishPress(&a);
        QCOMPARE(motion->targetValue(), 0);
        finishPress(&a);
        QCOMPARE(info(&a)->pressProgress(), 0);
        setButtonPressed(&a, true);
        finishPress(&a);
        a.setEnabled(false);
        QCOMPARE(info(&a)->pressProgress(), 0);
        a.setEnabled(true);
        finishPress(&a);
        a.setVisible(false);
        QCOMPARE(info(&a)->pressProgress(), 0);
        a.setVisible(true);
        setButtonPressed(&a, true);
        finishPress(&a);
        group.updatePolish();
        QVERIFY(a.width() > 100);
        group.takeItem(group.indexOf(&a));
        QCOMPARE(a.width(), 100);
        QCOMPARE(info(&a)->pressProgress(), 0);
        QCOMPARE(info(&a)->position(), int(Enum::ItemPosition::PosSingle));
        QVERIFY(! info(&a)->connected());
        group.takeItem(group.indexOf(&b));
        b.setWidth(140);
        group.addItem(&b);
        group.updatePolish();
        group.takeItem(group.indexOf(&b));
        QCOMPARE(b.width(), 140);
    }
    void widthBindingSurvives() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ButtonGroupContainer {
                id: root
                property real itemWidth: 90
                MD.Button { objectName: "bound"; text: "Fixed"; width: root.itemWidth }
                MD.Button { text: "Natural" }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto*                    group = qobject_cast<ButtonGroupContainer*>(object.get());
        QVERIFY(group);
        QQuickWindow window;
        window.resize(400, 100);
        group->setParentItem(window.contentItem());
        window.show();
        auto* button = group->findChild<CommandButton*>("bound");
        QVERIFY(button);
        QTRY_VERIFY(group->contentWidth() > 90);
        setButtonPressed(button, true);
        finishPress(button);
        QCOMPARE(button->width(), 90);
        group->setProperty("itemWidth", 120);
        QTRY_COMPARE(button->width(), 120);
        QTRY_COMPARE(group->itemAt(1)->x(), 132);
        group->takeItem(group->indexOf(button));
        group->setProperty("itemWidth", 130);
        QCOMPARE(button->width(), 130);
        delete button;
        group->setParentItem(nullptr);
    }
    void overridesAndReentry() {
        TestGroup     group;
        CommandButton a, b;
        setup(group, a);
        setup(group, b);
        auto* data = info(&a);
        data->setCompressionLimit(3);
        data->setDefaultCompressionLimit(12);
        QCOMPARE(data->compressionLimit(), 3);
        data->resetCompressionLimit();
        QCOMPARE(data->compressionLimit(), 12);
        data->setDefaultMinimumWidth(40);
        data->setMinimumWidth(55);
        QCOMPARE(data->minimumWidth(), 55);
        data->resetMinimumWidth();
        QCOMPARE(data->minimumWidth(), 40);
        data->setPreferredWidth(120);
        group.updatePolish();
        QCOMPARE(a.width(), 120);
        data->resetPreferredWidth();
        group.updatePolish();
        QCOMPARE(a.width(), 100);
        bool removed = false;
        connect(&a, &QQuickItem::widthChanged, &group, [&] {
            if (! removed) {
                removed = true;
                group.takeItem(group.indexOf(&b));
            }
        });
        setButtonPressed(&a, true);
        finishPress(&a);
        group.updatePolish();
        group.updatePolish();
        QVERIFY(removed);
        QCOMPARE(a.width(), 100);
        QCOMPARE(group.count(), 1);
    }
    void destructionWhileAnimating() {
        auto          group = std::make_unique<TestGroup>();
        CommandButton a, b;
        setup(*group, a);
        setup(*group, b);
        setButtonPressed(&a, true);
        finishPress(&a);
        group.reset();
        QCOMPARE(info(&a)->pressProgress(), 0);
        QVERIFY(! info(&a)->grouped());
        QVERIFY(! info(&b)->grouped());
    }
    void sharedAction() {
        TestGroup     group;
        CommandButton a, b;
        Action        action;
        action.setCheckable(true);
        ActionCommand::qmlAttachedProperties(&a)->setAction(&action);
        ActionCommand::qmlAttachedProperties(&b)->setAction(&action);
        setup(group, a);
        setup(group, b);
        QSignalSpy triggers(&action, &Action::triggered);
        QSignalSpy changes(&action, &Action::toggled);
        a.click();
        QCOMPARE(triggers.size(), 1);
        QCOMPARE(changes.size(), 1);
        QVERIFY(action.isChecked());
        group.setVariant(ButtonGroupContainer::Connected);
        group.takeItem(group.indexOf(&a));
        b.click();
        QCOMPARE(triggers.size(), 2);
        QCOMPARE(changes.size(), 2);
        QVERIFY(! action.isChecked());
    }
    void qmlShapesAndSelection() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ButtonGroupContainer {
                variant: MD.ButtonGroupContainer.Connected
                MD.ButtonGroup { id: selection; objectName: "selection" }
                MD.ToggleButton {
                    objectName: "a"; text: "One"; checked: MD.ButtonGroup.checked
                    MD.ButtonGroup.group: selection
                }
                MD.IconToggleButton {
                    objectName: "b"; icon.name: MD.Token.icon.star; checked: MD.ButtonGroup.checked
                    mdState.type: MD.Enum.IBtFilled
                    MD.ButtonGroup.group: selection
                }
                MD.Button { objectName: "c"; text: "Three"; mdState.size: MD.Enum.XS }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto*                    group = qobject_cast<ButtonGroupContainer*>(object.get());
        QVERIFY(group);
        QQuickWindow window;
        window.resize(480, 100);
        group->setParentItem(window.contentItem());
        window.show();
        auto* a         = group->findChild<ToggleButton*>("a");
        auto* b         = group->findChild<ToggleButton*>("b");
        auto* c         = group->findChild<CommandButton*>("c");
        auto* selection = group->findChild<ButtonGroup*>("selection");
        QVERIFY(selection);
        auto* sa = a->findChild<ButtonState*>();
        auto* sb = b->findChild<IconButtonState*>();
        auto* sc = c->findChild<ButtonState*>();
        QVERIFY(sa && sb && sc);
        QTRY_COMPARE(info(a)->position(), int(Enum::ItemPosition::PosFirst));
        QCOMPARE(sa->corners().topLeft(), a->height() / 2);
        QCOMPARE(sa->corners().topRight(), 8);
        QCOMPARE(sb->corners(), CornersGroup(8));
        QCOMPARE(b->leftInset(), 0);
        auto* motion = a->property("_motion").value<QObject*>();
        QVERIFY(motion);
        QTRY_VERIFY(motion->property("_animate").toBool());
        QTRY_COMPARE(motion->property("corners").value<CornersGroup>(), sa->corners());
        CornerObserver observer;
        bool           reversed = false;
        observer.callback       = [&] {
            if (reversed) return;
            const auto middle = motion->property("corners").value<CornersGroup>();
            if (middle.topRight() <= 4 || middle.topRight() >= 8) return;
            reversed = true;
            setButtonPressed(a, false);
            QCOMPARE(motion->property("corners").value<CornersGroup>(), middle);
            selection->setCheckedButton(a);
            QCOMPARE(motion->property("corners").value<CornersGroup>(), middle);
        };
        QVERIFY(connect(motion, SIGNAL(cornersChanged()), &observer, SLOT(changed())));
        setButtonPressed(a, true);
        QTRY_VERIFY(reversed);
        QTRY_COMPARE(motion->property("corners").value<CornersGroup>(),
                     CornersGroup(a->height() / 2));
        selection->setCheckedButton(a);
        QCOMPARE(sa->corners(), CornersGroup(a->height() / 2));
        setButtonPressed(a, true);
        QCOMPARE(sa->corners().topRight(), 4);
        setButtonPressed(a, false);
        QCOMPARE(sa->corners().topRight(), a->height() / 2);
        selection->setCheckedButton(b);
        QVERIFY(! a->isChecked());
        QCOMPARE(c->metaObject()->indexOfProperty("checkable"), -1);
        group->setButtonSize(int(Enum::ButtonSize::M));
        QCOMPARE(sa->size(), int(Enum::ButtonSize::M));
        QCOMPARE(sb->size(), int(Enum::ButtonSize::M));
        QCOMPARE(sc->size(), int(Enum::ButtonSize::XS));
        const qreal initialImplicitWidth = a->implicitWidth();
        a->setText(QStringLiteral("A longer label"));
        QTRY_VERIFY(a->implicitWidth() > initialImplicitWidth);
        auto        font      = a->font();
        const qreal textWidth = a->implicitWidth();
        font.setPixelSize(font.pixelSize() + 4);
        a->setFont(font);
        QTRY_VERIFY(a->implicitWidth() > textWidth);
        a->setText(QStringLiteral("One"));
        sa->setCorners(CornersGroup(3));
        setButtonPressed(a, true);
        QCOMPARE(sa->corners(), CornersGroup(3));
        sa->resetCorners();
        QCOMPARE(sa->corners().topRight(), 4);
        setButtonPressed(a, false);
        group->setLayoutDirection(Qt::RightToLeft);
        QTRY_VERIFY(a->mirrored());
        QCOMPARE(sa->corners().topLeft(), 8);
        QCOMPARE(sa->corners().topRight(), a->height() / 2);
        QTRY_VERIFY(a->x() > b->x());
        group->setWidth(300);
        info(a)->setWeight(1);
        info(b)->setWeight(1);
        info(c)->setWeight(1);
        QTRY_VERIFY(std::abs(a->width() + b->width() + c->width() + 4 - 300) < .01);
        group->setWidth(480);
        QTRY_VERIFY(std::abs(a->width() + b->width() + c->width() + 4 - 480) < .01);
        window.requestActivate();
        QTRY_VERIFY(window.isActive());
        b->forceActiveFocus();
        QTRY_COMPARE(window.activeFocusItem(), b);
        QSignalSpy clicked(b, &ToggleButton::checkedChangeRequested);
        QTest::keyClick(&window, Qt::Key_Space);
        QCOMPARE(clicked.size(), 1);
        QVERIFY(b->isChecked());
        QTest::mouseClick(&window,
                          Qt::LeftButton,
                          Qt::NoModifier,
                          b->mapToScene(QPointF(b->width() / 2, b->height() / 2)).toPoint());
        QCOMPARE(clicked.size(), 2);
        group->setVariant(ButtonGroupContainer::Standard);
        InteractionAccumulator bInput(this);
        bInput.setSource(b->interactionSource());
        QTest::keyPress(&window, Qt::Key_Space);
        finishPress(b);
        QCOMPARE(info(b)->pressProgress(), 1);
        b->setFocus(false);
        QTRY_COMPARE(info(b)->findChild<PressMotion*>()->targetValue(), 0);
        finishPress(b);
        QCOMPARE(info(b)->pressProgress(), 0);
        QCOMPARE(clicked.size(), 2);
        b->forceActiveFocus(Qt::TabFocusReason);
        QTest::keyPress(&window, Qt::Key_Space);
        finishPress(b);
        QCOMPARE(info(b)->pressProgress(), 1);
        QQuickWindow otherWindow;
        otherWindow.show();
        otherWindow.requestActivate();
        QTRY_VERIFY(! window.isActive());
        finishPress(b);
        QCOMPARE(info(b)->pressProgress(), 0);
        QTRY_VERIFY(! bInput.values().pressed);
        auto* taken = group->takeItem(group->indexOf(a));
        QCOMPARE(taken, a);
        QCOMPARE(
            qobject_cast<ButtonGroupAttached*>(qmlAttachedPropertiesObject<ButtonGroup>(a, false))
                ->group(),
            selection);
        QCOMPARE(sa->size(), int(Enum::ButtonSize::S));
        QVERIFY(sa->corners().isUniform());
        group->setVariant(ButtonGroupContainer::Standard);
        QCOMPARE(b->leftInset(), 4);
        group->setParentItem(nullptr);
        delete a;
    }
};

int run_button_group(int argc, char** argv) {
    ButtonGroupTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "button_group.moc"
