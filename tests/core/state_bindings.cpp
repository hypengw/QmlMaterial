#include <QtTest>
#include "qml_material/core/state_bindings.hpp"

using namespace qml_material;

class StateBindingInput : public QObject {
public:
    QProperty<int> value { 10 };
    QBindable<int> bindableValue() { return &value; }
};

enum class BindingState
{
    Base,
    Active
};

class StateBindingsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot assign|TypeError).*"));
    }

    void conditionalBindingsRestoreAndDetach() {
        QProperty<bool>               hover(false), press(false);
        QProperty<int>                source(30);
        StateBindingInput             owner, sibling;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key   = bindings.property<&StateBindingInput::bindableValue>(&owner);
        const auto other = bindings.property<&StateBindingInput::bindableValue>(&sibling);
        QVERIFY(bindings.base().bind(key, [] {
            return 10;
        }));
        QVERIFY(bindings.base().bind(other, [] {
            return 11;
        }));
        const auto hovered = bindings.when([&] {
            return hover.value();
        });
        QVERIFY(hovered.bind(key, [] {
            return 90;
        }));
        QVERIFY(bindings
                    .when([&] {
                        return press.value();
                    })
                    .bind(other, [] {
                        return 70;
                    }));
        QVERIFY(bindings
                    .when([&] {
                        return press.value();
                    })
                    .bind(key, [] {
                        return 80;
                    }));
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 100;
        }));
        owner.value.setBinding([&] {
            return source.value();
        });
        hover = true;
        QCOMPARE(owner.value.value(), 90);
        QCOMPARE(sibling.value.value(), 11);
        press = true;
        QCOMPARE(owner.value.value(), 80);
        QCOMPARE(sibling.value.value(), 70);
        QVERIFY(bindings.select(BindingState::Active));
        hover = false;
        QCOMPARE(owner.value.value(), 100);
        source = 40;
        QVERIFY(bindings.select(BindingState::Base));
        QCOMPARE(owner.value.value(), 80);
        press = false;
        QCOMPARE(owner.value.value(), 40);
        QCOMPARE(sibling.value.value(), 11);
        QVERIFY(owner.bindableValue().hasBinding());
        hover = true;
        QVERIFY(bindings.detach());
        QCOMPARE(owner.value.value(), 40);
        source = 50;
        hover  = false;
        press  = true;
        QCOMPARE(owner.value.value(), 50);
        QVERIFY(! key.isValid());
    }

    void conditionalBindingsInitialValue() {
        QProperty<bool>               active(true);
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings.base().bind(key, [] {
            return 10;
        }));
        QVERIFY(bindings
                    .when([&] {
                        return active.value();
                    })
                    .bind(key, [] {
                        return 90;
                    }));
        QCOMPARE(owner.value.value(), 90);
        active = false;
        QCOMPARE(owner.value.value(), 10);
    }

    void conditionalPredicateAbandon() {
        QProperty<bool>               active(false);
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings
                    .when([&] {
                        const bool value = active.value();
                        if (value) bindings.abandon();
                        return value;
                    })
                    .bind(key, [] {
                        return 90;
                    }));
        active = true;
        QVERIFY(! key.isValid());
        QCOMPARE(owner.value.value(), 10);
    }

    void conditionalBindingsReentry() {
        QProperty<bool>               hover(false), press(false);
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings.base().bind(key, [] {
            return 10;
        }));
        QVERIFY(bindings
                    .when([&] {
                        return hover.value();
                    })
                    .bind(key, [] {
                        return 90;
                    }));
        QVERIFY(bindings
                    .when([&] {
                        return press.value();
                    })
                    .bind(key, [] {
                        return 80;
                    }));
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 100;
        }));
        auto observer = owner.value.addNotifier([&] {
            if (owner.value.value() == 90) {
                bindings.select(BindingState::Active);
                press = true;
            }
        });
        hover         = true;
        QCOMPARE(owner.value.value(), 100);
        QVERIFY(bindings.select(BindingState::Base));
        QCOMPARE(owner.value.value(), 80);
        press = false;
        QCOMPARE(owner.value.value(), 100);
    }

    void conditionalBindingsDeletion_data() {
        QTest::addColumn<bool>("destroyBindings");
        QTest::newRow("owner") << false;
        QTest::newRow("bindings") << true;
    }

    void conditionalBindingsDeletion() {
        QFETCH(bool, destroyBindings);
        QProperty<bool> active(false);
        auto            owner = std::make_unique<StateBindingInput>();
        auto       bindings   = std::make_unique<StateBindingSet<BindingState>>(BindingState::Base);
        const auto key        = bindings->property<&StateBindingInput::bindableValue>(owner.get());
        QVERIFY(bindings
                    ->when([&] {
                        return active.value();
                    })
                    .bind(key, [] {
                        return 90;
                    }));
        auto observer = owner->value.addNotifier([&] {
            if (destroyBindings)
                bindings.reset();
            else
                owner.reset();
        });
        active        = true;
        QVERIFY(! key.isValid());
        active = false;
    }

    void stateBindingLiveOwner() {
        QProperty<int>                source(30);
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings.base().bind(key, [] {
            return 10;
        }));
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 90;
        }));
        owner.value.setBinding([&] {
            return source.value();
        });
        QVERIFY(bindings.select(BindingState::Active));
        QCOMPARE(owner.value.value(), 90);
        QVERIFY(bindings.select(BindingState::Base));
        QCOMPARE(owner.value.value(), 30);
        source = 40;
        QCOMPARE(owner.value.value(), 40);
        key.reset();
        QCOMPARE(owner.value.value(), 10);
        owner.value.setBinding([&] {
            return source.value();
        });
        QVERIFY(bindings.select(BindingState::Active));
        QVERIFY(bindings.detach());
        source = 50;
        QCOMPARE(owner.value.value(), 50);
        QVERIFY(! key.isValid());
    }

    void stateBindingReentrantSelection() {
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings.base().bind(key, [] {
            return 10;
        }));
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 90;
        }));
        auto observer = owner.value.addNotifier([&] {
            if (owner.value.value() == 90) bindings.select(BindingState::Base);
        });
        QVERIFY(bindings.select(BindingState::Active));
        QCOMPARE(owner.value.value(), 10);
    }

    void stateBindingDeadOwner_data() {
        QTest::addColumn<QString>("operation");
        for (const auto& operation : { "select", "reset", "detach" })
            QTest::newRow(operation) << QString::fromLatin1(operation);
    }

    void stateBindingDeadOwner() {
        QFETCH(QString, operation);
        bool                          alive           = true;
        int                           baseEvaluations = 0, savedEvaluations = 0;
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        QVERIFY(bindings.setOwnerAliveCheck([&] {
            return alive;
        }));
        const auto key = bindings.property<&StateBindingInput::bindableValue>(&owner);
        QVERIFY(bindings.base().bind(key, [&] {
            ++baseEvaluations;
            return 10;
        }));
        QCOMPARE(owner.value.value(), 10);
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 90;
        }));
        owner.value.setBinding([&] {
            ++savedEvaluations;
            return 30;
        });
        QCOMPARE(owner.value.value(), 30);
        QVERIFY(bindings.select(BindingState::Active));
        QCOMPARE(owner.value.value(), 90);
        const int baseBefore = baseEvaluations, savedBefore = savedEvaluations;
        alive = false;
        if (operation == "select")
            QVERIFY(! bindings.select(BindingState::Base));
        else if (operation == "reset")
            key.reset();
        else
            QVERIFY(! bindings.detach());
        QCOMPARE(owner.value.value(), 90);
        QCOMPARE(baseEvaluations, baseBefore);
        QCOMPARE(savedEvaluations, savedBefore);
        QVERIFY(! key.isValid());
        alive = true;
        QVERIFY(! bindings.select(BindingState::Base));
        key.reset();
        QCOMPARE(owner.value.value(), 90);
    }

    void stateBindingDeadOwnerDeclaration() {
        bool                          alive = true;
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        QVERIFY(bindings.setOwnerAliveCheck([&] {
            return alive;
        }));
        const auto key  = bindings.property<&StateBindingInput::bindableValue>(&owner);
        alive           = false;
        int evaluations = 0;
        QVERIFY(! bindings.base().bind(key, [&] {
            ++evaluations;
            return 20;
        }));
        QCOMPARE(owner.value.value(), 10);
        QCOMPARE(evaluations, 0);
        QVERIFY(! bindings.property<&StateBindingInput::bindableValue>(&owner).isValid());
    }

    void stateBindingReentrantAbandon_data() {
        QTest::addColumn<bool>("reset");
        QTest::newRow("restore") << false;
        QTest::newRow("reset") << true;
    }

    void stateBindingReentrantAbandon() {
        QFETCH(bool, reset);
        StateBindingInput             owner;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        const auto key       = bindings.property<&StateBindingInput::bindableValue>(&owner);
        bool       terminate = false;
        auto       original  = [&] {
            if (terminate) bindings.abandon();
            return 30;
        };
        QVERIFY(bindings.base().bind(key, original));
        QVERIFY(bindings.state(BindingState::Active).bind(key, [] {
            return 90;
        }));
        owner.value.setBinding(original);
        const auto observer = owner.value.onValueChanged([] {
        });
        QCOMPARE(owner.value.value(), 30);
        QVERIFY(bindings.select(BindingState::Active));
        QCOMPARE(owner.value.value(), 90);
        terminate = true;
        if (reset)
            key.reset();
        else
            QVERIFY(! bindings.select(BindingState::Base));
        QCOMPARE(owner.value.value(), 30);
        QVERIFY(! key.isValid());
        QVERIFY(! bindings.detach());
    }

    void stateBindingOwnerEndsBetweenProperties() {
        StateBindingInput             first, second;
        bool                          alive = true;
        StateBindingSet<BindingState> bindings(BindingState::Base);
        QVERIFY(bindings.setOwnerAliveCheck([&] {
            if (first.value.value() == 90) alive = false;
            return alive;
        }));
        const auto firstKey  = bindings.property<&StateBindingInput::bindableValue>(&first);
        const auto secondKey = bindings.property<&StateBindingInput::bindableValue>(&second);
        QVERIFY(bindings.state(BindingState::Active).bind(firstKey, [] {
            return 90;
        }));
        QVERIFY(bindings.state(BindingState::Active).bind(secondKey, [] {
            return 80;
        }));
        QVERIFY(! bindings.select(BindingState::Active));
        QCOMPARE(first.value.value(), 90);
        QCOMPARE(second.value.value(), 10);
        QVERIFY(! firstKey.isValid());
        QVERIFY(! secondKey.isValid());
    }
};

int run_state_bindings(int argc, char** argv) {
    StateBindingsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "state_bindings.moc"
