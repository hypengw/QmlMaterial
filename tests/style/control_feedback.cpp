#include <QtTest>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "../input_test.hpp"
#include "qml_material/control/combined_command_button.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/control/toggle_button.hpp"
#include "qml_material/style/rail_item_state.hpp"
#include "qml_material/style/snake_bar_state.hpp"
#include "qml_material/style/text_field_state.hpp"
#include "qml_material/style/button_state.hpp"
#include "qml_material/style/icon_button_state.hpp"
#include "qml_material/style/fab_state.hpp"
#include "qml_material/style/card_state.hpp"
#include "qml_material/style/chip_state.hpp"
#include "qml_material/token/color.hpp"

using namespace qml_material;
namespace
{
class InputButton : public CombinedCommandButton {
public:
    explicit InputButton(QQuickItem* parent = nullptr)
        : CombinedCommandButton(parent), feedback(interactionSource(), this) {}
    TestInteractionEmitter feedback;
};
class InputSelection : public SelectionButton {
public:
    InputSelection(): feedback(interactionSource(), this) {}
    TestInteractionEmitter feedback;
};
} // namespace

class ControlFeedbackTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }

    void stateDetachesFromSharedSource() {
        InteractionSource shared;
        auto*             button = new CombinedCommandButton;
        button->setInteractionSource(&shared);
        ButtonState state;
        state.setItem(button);
        auto emitter = shared.emitter(button);
        emitter->begin(Interaction::Hover);
        flushInteractions();
        QVERIFY(state.interactions().hovered);
        delete button;
        QCOMPARE(state.interactionSource(), nullptr);
        QVERIFY(! state.interactions().hovered);
    }

    void stateSourceDestroyedDuringSwitch() {
        InteractionSource           first;
        QPointer<InteractionSource> second = new InteractionSource;
        ButtonState                 state;
        state.setInteractionSource(&first);
        auto emitter = first.emitter(nullptr);
        emitter->begin(Interaction::Hover);
        flushInteractions();
        QVERIFY(state.interactions().hovered);
        connect(&state, &CommonState::interactionsChanged, this, [&] {
            delete second.data();
        });
        state.setInteractionSource(second);
        QVERIFY(! second);
        QCOMPARE(state.interactionSource(), nullptr);
        QVERIFY(! state.interactions().hovered);
    }

    void independentButtonEffects_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<qreal>("normal");
        QTest::addColumn<qreal>("hover");
        QTest::addColumn<qreal>("drag");
        QTest::newRow("button") << 0 << 1.0 << 3.0 << 3.0;
        QTest::newRow("fab") << 1 << 6.0 << 8.0 << 8.0;
        QTest::newRow("filled-card") << 2 << 0.0 << 1.0 << 6.0;
        QTest::newRow("outlined-card") << 3 << 0.0 << 1.0 << 6.0;
        QTest::newRow("elevated-card") << 4 << 1.0 << 3.0 << 8.0;
        QTest::newRow("elevated-chip") << 5 << 1.0 << 3.0 << 8.0;
    }

    void independentButtonEffects() {
        QFETCH(int, kind);
        QFETCH(qreal, normal);
        QFETCH(qreal, hover);
        QFETCH(qreal, drag);
        CommandButton                           button;
        TestInteractionEmitter                  input(button.interactionSource(), &button);
        std::unique_ptr<ButtonInteractionState> state;
        const auto                              attach = [&](auto value) {
            value->setItem(&button);
            state = std::move(value);
        };
        if (kind == 0) {
            attach(std::make_unique<ButtonState>());
        } else if (kind == 1) {
            attach(std::make_unique<FABState>());
        } else if (kind < 5) {
            auto card = std::make_unique<CardState>();
            card->setType(int(kind == 2   ? Enum::CardType::CardFilled
                              : kind == 3 ? Enum::CardType::CardOutlined
                                          : Enum::CardType::CardElevated));
            attach(std::move(card));
        } else {
            auto chip = std::make_unique<AssistChipState>();
            chip->setElevated(true);
            attach(std::move(chip));
        }
        QCOMPARE(state->elevation(), normal);
        const auto press = input.begin(Interaction::Press);
        QCOMPARE(state->elevation(), normal);
        input.set(Interaction::Hover, true);
        QCOMPARE(state->elevation(), hover);
        if (auto* shape = qobject_cast<ButtonState*>(state.get()))
            QCOMPARE(shape->corner(), shape->sizeToken().pressed_corner_size);
        input.set(Interaction::Focus, true);
        QCOMPARE(state->elevation(), normal);
        input.set(Interaction::Focus, false);
        QCOMPARE(state->elevation(), hover);
        const auto dragging = input.begin(Interaction::Drag);
        QCOMPARE(state->elevation(), drag);
        input.end(dragging, Interaction::Cancel);
        QCOMPARE(state->elevation(), hover);
        input.end(press);
        QCOMPARE(state->elevation(), hover);
        input.set(Interaction::Hover, false);
        QCOMPARE(state->elevation(), normal);
        if (auto* shape = qobject_cast<ButtonState*>(state.get())) {
            const auto corner   = shape->corner();
            const auto dragOnly = input.begin(Interaction::Drag);
            QCOMPARE(shape->corner(), corner);
            input.end(dragOnly);
        }
    }

    void cardStateOutlivesInput() {
        InteractionSource shared, replacement;
        CardState         state;
        auto*             button = new CommandButton;
        button->setInteractionSource(&shared);
        state.setItem(button);
        auto emitter = shared.emitter(button);
        emitter->begin(Interaction::Hover);
        flushInteractions();
        QVERIFY(state.interactions().hovered);
        button->setInteractionSource(&replacement);
        flushInteractions();
        QCOMPARE(state.interactionSource(), &replacement);
        QVERIFY(! state.interactions().hovered);
        auto next = replacement.emitter(button);
        next->begin(Interaction::Press);
        flushInteractions();
        QVERIFY(state.interactions().pressed);
        delete button;
        QVERIFY(! state.item() && ! state.interactionSource());
        QVERIFY(! state.interactions().pressed && ! state.interactions().hovered);
        emitter->cancel();
        next->cancel();
        flushInteractions();
        QVERIFY(! state.interactions().pressed && ! state.interactions().hovered);
    }

    void focusDoesNotBecomePress() {
        QQuickWindow window;
        window.resize(300, 100);
        InputButton button(window.contentItem());
        button.setSize({ 200, 50 });
        IconButtonState icon;
        AssistChipState chip;
        MdColorMgr      palette;
        icon.setItem(&button);
        chip.setItem(&button);
        chip.setColors(&palette);
        auto tokens                        = icon.stateTokens();
        tokens.pressed.state_layer_opacity = .31;
        tokens.focus.state_layer_opacity   = .17;
        icon.setStateTokens(tokens);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::mouseMove(&window, { 280, 80 });
        button.forceActiveFocus(Qt::TabFocusReason);
        QVERIFY(button.focusVisible());
        QCOMPARE(icon.stateLayerOpacity(), .17);
        QCOMPARE(icon.corner(), 20.0);
        const auto outline = chip.outlineColor();
        QCOMPARE(outline, chip.textColor());
        QVERIFY(outline != palette.outline_variant());
        button.feedback.set(Interaction::Hover, true);
        QCOMPARE(chip.outlineColor(), outline);
        const auto press = button.feedback.begin(Interaction::Press);
        QCOMPARE(chip.outlineColor(), outline);
        QCOMPARE(icon.corner(), icon.sizeToken().pressed_corner_size);
        QCOMPARE(icon.stateLayerOpacity(), .31);
        button.feedback.end(press);
        button.feedback.set(Interaction::Hover, false);
        QCOMPARE(icon.stateLayerOpacity(), .17);
    }

    void orderedStates() {
        InputButton      control;
        IndicationPolicy state(this);
        state.setSource(control.interactionSource());
        auto& input = control.feedback;
        input.set(Interaction::Focus, true);
        input.set(Interaction::Hover, true);
        QCOMPARE(state.values().kind, int(Interaction::Hover));
        const auto press = input.begin(Interaction::Press);
        QCOMPARE(state.values().kind, int(Interaction::Hover));
        input.end(press);
        input.set(Interaction::Hover, false);
        QCOMPARE(state.values().kind, int(Interaction::Focus));
        input.clear();
        QCOMPARE(state.values().opacity, 0);
    }

    void indicationReplacement() {
        InputSelection first, second;
        RailItemState  state;
        state.setItem(&first);
        first.feedback.set(Interaction::Hover, true);
        state.setItem(&second);
        QCOMPARE(state.target(), &second);
        QCOMPARE(state.indication().opacity, 0);
        second.feedback.set(Interaction::Hover, true);
        QCOMPARE(state.indication().opacity, .08);
    }

    void actionTargetReentry_data() {
        QTest::addColumn<bool>("iconTarget");
        QTest::newRow("action") << false;
        QTest::newRow("icon") << true;
    }

    void actionTargetReentry() {
        QFETCH(bool, iconTarget);
        InputSelection first, second;
        SnakeBarState  state;
        const auto     setTarget = [&](ButtonControl* button) {
            if (iconTarget)
                state.setIconItem(button);
            else
                state.setActionItem(button);
        };
        connect(&state,
                iconTarget ? &SnakeBarState::iconIndicationChanged
                           : &SnakeBarState::actionIndicationChanged,
                this,
                [&] {
                    if ((iconTarget ? state.iconItem() : state.actionItem()) == &first)
                        setTarget(&second);
                });
        setTarget(&first);
        QCOMPARE(iconTarget ? state.iconItem() : state.actionItem(), &second);
        second.feedback.set(Interaction::Hover, true);
        QCOMPARE((iconTarget ? state.iconIndication() : state.actionIndication()).opacity, .08);
    }

    void independentActionTargets() {
        InputSelection action, icon;
        SnakeBarState  state;
        state.setActionItem(&action);
        state.setIconItem(&icon);
        action.feedback.set(Interaction::Hover, true);
        icon.feedback.set(Interaction::Hover, true);
        QCOMPARE(state.actionIndication().opacity, .08);
        QCOMPARE(state.iconIndication().opacity, .08);
        action.feedback.set(Interaction::Hover, false);
        QCOMPARE(state.iconIndication().opacity, .08);
    }

    void indicationPolicy() {
        InputButton      control;
        IndicationPolicy state(this);
        state.setSource(control.interactionSource());
        control.feedback.set(Interaction::Focus, true);
        control.feedback.set(Interaction::Hover, true);
        state.setHoverEnabled(false);
        QCOMPARE(state.values().kind, int(Interaction::Hover));
        QCOMPARE(state.values().opacity, 0);
        control.feedback.set(Interaction::Hover, false);
        QCOMPARE(state.values().opacity, .1);
        state.setFocusEnabled(false);
        QCOMPARE(state.values().opacity, 0);
    }

    void indicationBindingReset() {
        IndicationPolicy  state(this);
        QProperty<QColor> color { Qt::red };
        state.bindDefaultColor(Qt::makePropertyBinding([&] {
            return color.value();
        }));
        QCOMPARE(state.color(), QColor(Qt::red));
        state.setColor(Qt::green);
        color = Qt::blue;
        QCOMPARE(state.color(), QColor(Qt::green));
        state.resetColor();
        QCOMPARE(state.color(), QColor(Qt::blue));
    }

    void stateOwnsIndicationPolicy() {
        RailItemState state;
        QCOMPARE(state.findChildren<QObject*>(QString(), Qt::FindDirectChildrenOnly).size(), 0);
        QCOMPARE(state.metaObject()->indexOfProperty("state"), -1);
        InputSelection button;
        state.setItem(&button);
        QProperty<QColor> color(Qt::red);
        state.bindableIndicationColor().setBinding([&] {
            return color.value();
        });
        QCOMPARE(state.indication().color, QColor(Qt::red));
        color = Qt::blue;
        QCOMPARE(state.indication().color, QColor(Qt::blue));
        button.feedback.set(Interaction::Focus, true);
        QCOMPARE(state.indication().opacity, .1);
        state.setFocusIndicationEnabled(false);
        QCOMPARE(state.indication().opacity, 0);
        auto tokens                      = state.indicationTokens();
        tokens.hover.state_layer_opacity = .21;
        state.setIndicationTokens(tokens);
        button.feedback.set(Interaction::Hover, true);
        QCOMPARE(state.indication().opacity, .21);
        state.resetIndicationTokens();
        QCOMPARE(state.indication().opacity, .08);
        state.resetIndicationColor();
        QCOMPARE(state.indication().color, state.colors()->on_surface());
    }

    void targetDeletedInIndicationObserver() {
        RailItemState             state;
        QPointer<SelectionButton> input(new SelectionButton);
        connect(&state, &CommonState::indicationChanged, &state, [&] {
            if (input) delete input.data();
        });
        state.setItem(input);
        QVERIFY(! input);
        QCOMPARE(state.item(), nullptr);
        QCOMPARE(state.target(), nullptr);
    }

    void sourceReplacementInObserver() {
        SelectionButton first, second;
        RailItemState   state;
        connect(&state, &CommonState::indicationChanged, &state, [&] {
            if (state.target() == &first) state.setItem(&second);
        });
        state.setItem(&first);
        QCOMPARE(state.item(), &second);
        QCOMPARE(state.target(), &second);
    }

    void buttonElevation_data() {
        QTest::addColumn<bool>("icon");
        QTest::addColumn<int>("type");
        QTest::addColumn<bool>("toggle");
        QTest::addColumn<qreal>("baseElevation");
        QTest::addColumn<qreal>("hoverElevation");
        const struct {
            const char* name;
            bool        icon;
            int         type;
            qreal       base, hover;
        } rows[] = {
            { "filled", false, int(Enum::ButtonType::BtFilled), 0, 1 },
            { "tonal", false, int(Enum::ButtonType::BtFilledTonal), 0, 1 },
            { "elevated", false, int(Enum::ButtonType::BtElevated), 1, 3 },
            { "outlined", false, int(Enum::ButtonType::BtOutlined), 0, 0 },
            { "text", false, int(Enum::ButtonType::BtText), 0, 0 },
            { "icon-standard", true, int(Enum::IconButtonType::IBtStandard), 0, 0 },
            { "icon-filled", true, int(Enum::IconButtonType::IBtFilled), 0, 0 },
            { "icon-tonal", true, int(Enum::IconButtonType::IBtFilledTonal), 0, 0 },
            { "icon-outlined", true, int(Enum::IconButtonType::IBtOutlined), 0, 0 },
        };
        for (const auto& row : rows) {
            for (bool toggle : { false, true }) {
                const auto name = QByteArray(row.name) + (toggle ? "-toggle" : "");
                QTest::newRow(name.constData())
                    << row.icon << row.type << toggle << row.base << row.hover;
            }
        }
    }

    void buttonElevation() {
        QFETCH(bool, icon);
        QFETCH(int, type);
        QFETCH(bool, toggle);
        QFETCH(qreal, baseElevation);
        QFETCH(qreal, hoverElevation);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick; import Qcm.Material as MD; MD." +
                              QByteArray(icon ? (toggle ? "IconToggleButton" : "IconButton")
                                              : (toggle ? "ToggleButton" : "Button")) +
                              R"( {
                id: control
                property int appearanceType: 0
                width: 180; height: 64
                hoverEnabled: true
                mdState.type: control.appearanceType
            })",
                          QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "appearanceType", type } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<ButtonControl*>(object.get());
        auto* state  = object->findChild<CommonState*>();
        QVERIFY(button && state);
        QQuickWindow window;
        window.resize(320, 160);
        button->setParentItem(window.contentItem());
        button->setPosition({ 20, 20 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        const auto verifyElevation = [&](qreal expected) {
            QCOMPARE(state->elevation(), expected);
            QTRY_COMPARE(button->background()->property("elevation").toReal(), expected);
            QTRY_COMPARE(button->background()->property("elevationVisible").toBool(),
                         expected > 0 && ! button->property("flat").toBool());
        };
        const QPoint outside(280, 120), inside(60, 40);
        for (int checked = 0; checked < (toggle ? 2 : 1); ++checked) {
            button->setFocus(false);
            if (toggle) static_cast<ToggleButton*>(button)->setChecked(checked);
            QTest::mouseMove(&window, outside);
            QTRY_VERIFY(! state->interactions().hovered);
            verifyElevation(baseElevation);
            QTest::mouseMove(&window, inside);
            QTRY_VERIFY(state->interactions().hovered);
            verifyElevation(hoverElevation);
            QVERIFY(state->stateLayerOpacity() > 0);
            if (! icon) {
                object->setProperty("appearanceType", int(Enum::ButtonType::BtText));
                verifyElevation(0);
                object->setProperty("appearanceType", type);
                verifyElevation(hoverElevation);
            }
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, inside);
            QTRY_COMPARE(state->stateLayerOpacity(),
                         state->stateTokens().pressed.state_layer_opacity);
            verifyElevation(baseElevation);
            QVERIFY(state->stateLayerOpacity() > 0);
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, inside);
            QTest::mouseMove(&window, outside);
            button->setFocus(false);
            button->forceActiveFocus(Qt::TabFocusReason);
            QTRY_VERIFY(button->focusVisible());
            flushInteractions();
            QCOMPARE(state->stateLayerOpacity(),
                     icon ? state->stateTokens().pressed.state_layer_opacity
                          : state->stateTokens().focus.state_layer_opacity);
            verifyElevation(baseElevation);
            button->setEnabled(false);
            QCOMPARE(state->contentOpacity(), state->stateTokens().disabled_content);
            verifyElevation(0);
            QCOMPARE(state->stateLayerOpacity(), 0);
            button->setEnabled(true);
            button->setFocus(false);
            QTRY_COMPARE(state->stateLayerOpacity(), 0);
            verifyElevation(baseElevation);
        }
        QTRY_VERIFY(! state->interactions().pressed && ! state->interactions().hovered &&
                    ! state->interactions().focused);
        QProperty<qreal> custom(7);
        state->bindableElevation().setBinding([&] {
            return custom.value();
        });
        QCOMPARE(state->elevation(), 7);
        QTest::mouseMove(&window, inside);
        QTRY_VERIFY(state->interactions().hovered);
        QCOMPARE(state->elevation(), hoverElevation);
        custom = 9;
        QTest::mouseMove(&window, outside);
        QTRY_VERIFY(! state->interactions().hovered && ! state->interactions().focused);
        QTRY_COMPARE(state->stateLayerOpacity(), 0);
        QCOMPARE(state->elevation(), 9);
        QVERIFY(state->bindableElevation().hasBinding());
        state->resetElevation();
        QCOMPARE(state->elevation(), baseElevation);
        custom = 11;
        QCOMPARE(state->elevation(), baseElevation);
        object.reset();
    }
};

int run_control_feedback(int argc, char** argv) {
    ControlFeedbackTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "control_feedback.moc"
