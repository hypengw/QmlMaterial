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
struct StateProperty {
    QPointer<QObject> owner;
    std::type_index   accessor;
    StateProperty(QObject* object, std::type_index type): owner(object), accessor(type) {}
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
    T                                  initial, savedValue {};
    bool                               covered = false, stopped = false, baseDeclared = false;

    StatePropertyOf(QObject* object, std::type_index type, std::function<QBindable<T>()> getter)
        : StateProperty(object, type), access(std::move(getter)), initial(access().value()) {
        const auto binding = access().binding();
        if (! binding.isNull())
            base.emplace(binding);
        else
            base.emplace(Qt::makePropertyBinding([value = initial] {
                return value;
            }));
    }

    void install(const QPropertyBinding<T>& binding) {
        if (! owner || stopped) return;
        access().setBinding(binding);
    }

    void reset() {
        if (! owner || stopped) return;
        if (! base)
            access().setValue(initial);
        else
            install(*base);
    }

    void apply(std::optional<qint64> state) override {
        if (! owner) {
            abandon();
            return;
        }
        if (stopped) return;
        const auto next = state ? overrides.find(*state) : overrides.end();
        if (next != overrides.end()) {
            if (! covered) {
                saved.emplace(access().binding());
                if (saved->isNull()) savedValue = access().value();
                if (! owner || stopped) return;
                covered = true;
            }
            const auto binding = next->second;
            install(binding);
        } else if (covered) {
            covered             = false;
            const auto original = std::move(*saved);
            saved.reset();
            const auto value = savedValue;
            if (! owner || stopped) return;
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
    std::vector<std::shared_ptr<StateProperty>>                   properties;
    std::map<qint64, std::vector<std::shared_ptr<StateProperty>>> states;
    std::optional<qint64>                                         current;
    bool frozen = false, switching = false, stopped = false;

    void abandon() {
        if (std::exchange(stopped, true)) return;
        for (const auto& property : properties) property->abandon();
    }

    bool select(std::optional<qint64> next) {
        if (stopped) return false;
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
                if (stopped) break;
                property->apply(next);
            }
        }
        switching = false;
        return ! stopped;
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
    bool isValid() const { return m_property && m_property->owner && ! m_property->stopped; }
    void reset() const {
        if (isValid()) m_property->reset();
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
        return ! data->stopped;
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

    template<auto Accessor, typename Object>
    auto property(Object* object) {
        using T =
            typename detail::BindableValue<std::invoke_result_t<decltype(Accessor), Object*>>::Type;
        PropertyKey<T> key;
        const auto     data = m_data;
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
        key.m_property = std::make_shared<detail::StatePropertyOf<T>>(object, type, [object] {
            return std::invoke(Accessor, object);
        });
        data->properties.push_back(key.m_property);
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
