#pragma once

#include <QDebug>
#include <QPointer>
#include <QProperty>
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <typeindex>
#include <utility>
#include <vector>

namespace qml_material
{
namespace detail
{
struct StateBindingOwner {
    std::function<bool()> alive;
    bool                  stopped = false;

    bool isActive() {
        if (stopped) return false;
        if (alive && ! alive()) stopped = true;
        return ! stopped;
    }
};

struct StateProperty {
    QPointer<QObject>                  owner;
    std::type_index                    accessor;
    std::shared_ptr<StateBindingOwner> lifetime;
    StateProperty(QObject* object, std::type_index type, std::shared_ptr<StateBindingOwner> guard)
        : owner(object), accessor(type), lifetime(std::move(guard)) {}
    virtual ~StateProperty()                  = default;
    virtual void apply(std::optional<qint64>) = 0;
    virtual void abandon()                    = 0;
};

template<typename T>
struct StatePropertyOf : StateProperty {
    std::function<QBindable<T>()>         access;
    std::map<qint64, QPropertyBinding<T>> overrides;
    // Reconstruct handles; Qt 6.11's copy assignment can free the replacement binding.
    std::optional<QPropertyBinding<T>> base, saved;
    T                                  initial {}, savedValue {};
    bool                               covered = false, stopped = false, baseDeclared = false;

    StatePropertyOf(QObject* object, std::type_index type, std::shared_ptr<StateBindingOwner> guard,
                    std::function<QBindable<T>()> getter)
        : StateProperty(object, type, std::move(guard)), access(std::move(getter)) {
        if (! isActive()) return;
        initial = access().value();
        if (! isActive()) return;
        const auto binding = access().binding();
        if (! binding.isNull())
            base.emplace(binding);
        else
            base.emplace(Qt::makePropertyBinding([value = initial] {
                return value;
            }));
    }

    bool isActive() {
        if (! stopped && owner && lifetime->isActive() && owner && ! stopped) return true;
        abandon();
        return false;
    }

    void install(QPropertyBinding<T> binding) {
        if (! isActive()) return;
        access().setBinding(binding);
    }

    void reset() {
        if (! isActive()) return;
        if (! base)
            access().setValue(initial);
        else
            install(*base);
    }

    void apply(std::optional<qint64> state) override {
        if (! isActive()) return;
        const auto next = state ? overrides.find(*state) : overrides.end();
        if (next != overrides.end()) {
            const auto binding = next->second;
            if (! covered) {
                const auto original = access().binding();
                if (! isActive()) return;
                const auto value = original.isNull() ? access().value() : T {};
                if (! isActive()) return;
                saved.emplace(original);
                savedValue = value;
                covered    = true;
            }
            install(binding);
        } else if (covered) {
            covered             = false;
            const auto original = std::move(*saved);
            saved.reset();
            const auto value = savedValue;
            if (! isActive()) return;
            if (original.isNull())
                access().setValue(value);
            else
                access().setBinding(original);
        }
    }

    void abandon() override {
        stopped = true;
        saved.reset();
        base.reset();
        overrides.clear();
    }
};

struct StateBindingData {
    std::shared_ptr<StateBindingOwner>          lifetime = std::make_shared<StateBindingOwner>();
    std::vector<std::shared_ptr<StateProperty>> properties;
    std::map<qint64, std::vector<std::shared_ptr<StateProperty>>> states;
    std::optional<qint64>                                         current;
    bool frozen = false, switching = false, stopped = false;

    void abandon() {
        if (std::exchange(stopped, true)) return;
        lifetime->stopped = true;
        for (const auto& property : properties) property->abandon();
    }

    bool isActive() {
        if (! stopped && lifetime->isActive() && ! stopped) return true;
        abandon();
        return false;
    }

    bool select(std::optional<qint64> next) {
        if (! isActive()) return false;
        if (switching) {
            qWarning("StateBindings: recursive state change");
            return false;
        }
        frozen = true;
        if (current == next) return true;
        auto affected = current ? states[*current] : std::vector<std::shared_ptr<StateProperty>> {};
        if (next) {
            for (const auto& property : states[*next]) {
                if (std::find(affected.begin(), affected.end(), property) == affected.end())
                    affected.push_back(property);
            }
        }
        switching = true;
        current   = next;
        {
            const QScopedPropertyUpdateGroup group;
            for (const auto& property : affected) {
                if (! isActive()) break;
                property->apply(next);
            }
        }
        switching = false;
        return isActive();
    }
};

template<auto Accessor>
struct AccessorTag {};
template<typename>
struct BindableValue;
template<typename T>
struct BindableValue<QBindable<T>> {
    using Type = T;
};
} // namespace detail

template<typename T>
class PropertyKey {
    template<typename>
    friend class StateBindingSet;
    friend class StateBindings;
    std::weak_ptr<detail::StateBindingData>     m_owner;
    std::shared_ptr<detail::StatePropertyOf<T>> m_property;

public:
    bool isValid() const {
        const auto property = m_property;
        return property && property->isActive();
    }
    void reset() const {
        const auto property = m_property;
        if (property) property->reset();
    }
};

class StateBindings {
    template<typename>
    friend class StateBindingSet;
    std::shared_ptr<detail::StateBindingData> m_data;
    std::optional<qint64>                     m_state;
    StateBindings(std::shared_ptr<detail::StateBindingData> data, std::optional<qint64> state)
        : m_data(std::move(data)), m_state(state) {}

public:
    template<typename T>
    bool bind(const PropertyKey<T>& key, const QPropertyBinding<T>& binding) const {
        const auto data = m_data;
        if (! data->isActive()) return false;
        if (data->frozen || data->stopped || key.m_owner.lock() != data || ! key.isValid() ||
            binding.isNull()) {
            qWarning("StateBindings: invalid or frozen declaration");
            return false;
        }
        const auto property = key.m_property;
        if (m_state) {
            if (! property->overrides.emplace(*m_state, binding).second) {
                qWarning("StateBindings: duplicate property declaration");
                return false;
            }
            data->states[*m_state].push_back(property);
        } else {
            if (property->baseDeclared) {
                qWarning("StateBindings: duplicate base declaration");
                return false;
            }
            property->base.emplace(binding);
            property->baseDeclared = true;
            property->install(binding);
        }
        return data->isActive();
    }

    template<typename T, typename F>
        requires std::is_invocable_r_v<T, F> &&
                 std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, T>
    bool bind(const PropertyKey<T>& key, F&& function) const {
        return bind(key, Qt::makePropertyBinding(std::forward<F>(function)));
    }
};

template<typename StateId>
class StateBindingSet {
    static_assert(std::is_enum_v<StateId>);
    std::shared_ptr<detail::StateBindingData> m_data = std::make_shared<detail::StateBindingData>();
    StateId                                   m_base;

public:
    explicit StateBindingSet(StateId base): m_base(base) {}
    StateBindingSet(const StateBindingSet&)            = delete;
    StateBindingSet& operator=(const StateBindingSet&) = delete;
    ~StateBindingSet() { abandon(); }

    // A false result is terminal, even while the property objects still exist.
    bool setOwnerAliveCheck(std::function<bool()> check) {
        const auto data = m_data;
        if (data->frozen || data->stopped || ! data->properties.empty()) {
            qWarning("StateBindings: owner check must precede property registration");
            return false;
        }
        data->lifetime->alive = std::move(check);
        return true;
    }

    template<auto Accessor, typename Object>
    auto property(Object* object) {
        using T =
            typename detail::BindableValue<std::invoke_result_t<decltype(Accessor), Object*>>::Type;
        PropertyKey<T> key;
        const auto     data = m_data;
        if (! data->isActive()) return key;
        if (! object || data->frozen || data->stopped) {
            qWarning("StateBindings: invalid or frozen property registration");
            return key;
        }
        const auto bindable = std::invoke(Accessor, object);
        if (! bindable.isValid() || ! bindable.isBindable() || bindable.isReadOnly()) {
            qWarning("StateBindings: property must be writable and bindable");
            return key;
        }
        const std::type_index type(typeid(detail::AccessorTag<Accessor>));
        key.m_owner = data;
        for (const auto& existing : data->properties) {
            if (existing->owner == object && existing->accessor == type) {
                key.m_property = std::static_pointer_cast<detail::StatePropertyOf<T>>(existing);
                return key;
            }
        }
        const auto property =
            std::make_shared<detail::StatePropertyOf<T>>(object, type, data->lifetime, [object] {
                return std::invoke(Accessor, object);
            });
        if (! data->isActive() || ! property->isActive()) return key;
        key.m_property = property;
        data->properties.push_back(property);
        return key;
    }

    StateBindings base() const { return { m_data, {} }; }
    StateBindings state(StateId id) const {
        return { m_data, id == m_base ? std::nullopt : std::optional<qint64>(qint64(id)) };
    }
    bool select(StateId id) {
        const auto data = m_data;
        return data->select(id == m_base ? std::nullopt : std::optional<qint64>(qint64(id)));
    }
    bool detach() {
        const auto data     = m_data;
        const bool restored = data->select({});
        data->abandon();
        return restored;
    }
    void abandon() {
        const auto data = m_data;
        data->abandon();
    }
    class Lifetime {
        std::shared_ptr<detail::StateBindingData> m_data;
        friend class StateBindingSet;
        explicit Lifetime(std::shared_ptr<detail::StateBindingData> data)
            : m_data(std::move(data)) {}

    public:
        Lifetime(const Lifetime&) = delete;
        ~Lifetime() { m_data->abandon(); }
    };
    Lifetime lifetime() const { return Lifetime(m_data); }
};
} // namespace qml_material
