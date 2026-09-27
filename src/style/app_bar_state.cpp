#include "qml_material/style/app_bar_state.hpp"
#include "qml_material/control/tool_bar.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
AppBarState::AppBarState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    m_typescaleKey = m_bindings.property<&AppBarState::bindableTypescale>(this);
    m_heightKey    = m_bindings.property<&AppBarState::bindableContainerHeight>(this);
    auto base      = m_bindings.base();
    base.bind(m_typescaleKey, [] {
        return token::TypeScale::default_title_large;
    });
    base.bind(m_heightKey, [] {
        return 64;
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return showBackground() ? elevationTokens().level2 : elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.backgroundColor, [this] {
        return color(showBackground() ? &MdColorMgr::surface_container : &MdColorMgr::surface);
    });
    base.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return showBackground() ? 1 : 0;
    });
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    for (auto specification : { Specification::Medium, Specification::Large }) {
        auto active = m_bindings.state(specification);
        active.bind(m_typescaleKey, [specification] {
            return specification == Specification::Medium
                       ? token::TypeScale::default_headline_small
                       : token::TypeScale::default_headline_medium;
        });
        active.bind(m_heightKey, [specification] {
            return specification == Specification::Medium ? 112 : 152;
        });
        active.bind(m_appearance.elevation, [this]() -> qreal {
            return elevationTokens().level0;
        });
    }
    m_selection.setBinding([this] {
        Specification specification = Specification::Base;
        switch (Enum::AppBarType(type())) {
        case Enum::AppBarType::AppBarSmall: specification = Specification::Small; break;
        case Enum::AppBarType::AppBarMedium: specification = Specification::Medium; break;
        case Enum::AppBarType::AppBarLarge: specification = Specification::Large; break;
        default: break;
        }
        return Selection { specification, targetGeneration() };
    });
    m_ready = true;
}
AppBarState::~AppBarState() {
    m_bindings.abandon();
    disconnect(m_destroyed);
}
ToolBar* AppBarState::item() const { return m_item.value(); }
void     AppBarState::setItem(ToolBar* value) {
    if (m_item == value) return;
    QPointer<AppBarState> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        disconnect(m_destroyed);
        m_item = value;
        if (value)
            m_destroyed = connect(value, &QObject::destroyed, this, [this] {
                QPointer<AppBarState> guard(this);
                m_item = nullptr;
                if (guard) Q_EMIT itemChanged();
            });
        setTarget(value);
    }
    if (guard) Q_EMIT itemChanged();
}
void AppBarState::selectionChanged() {
    if (! m_ready) return;
    const auto next = m_selection.value();
    QString    name;
    switch (next.state) {
    case Specification::Small: name = QStringLiteral("small"); break;
    case Specification::Medium: name = QStringLiteral("meidium"); break;
    case Specification::Large: name = QStringLiteral("large"); break;
    default: break;
    }
    publishState(name, [this, specification = next.state] {
        m_bindings.select(specification);
    });
}
#define INPUT(Type, Name, Setter, Bindable)                                \
    Type            AppBarState::Name() const { return m_##Name.value(); } \
    void            AppBarState::Setter(Type value) { m_##Name = value; }  \
    QBindable<Type> AppBarState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(int, type, setType, bindableType)
INPUT(bool, showBackground, setShowBackground, bindableShowBackground)
INPUT(int, containerHeight, setContainerHeight, bindableContainerHeight)
#undef INPUT
token::TypeScaleItem AppBarState::typescale() const { return m_typescale.value(); }
void AppBarState::setTypescale(const token::TypeScaleItem& value) { m_typescale = value; }
QBindable<token::TypeScaleItem> AppBarState::bindableTypescale() {
    return QBindable<token::TypeScaleItem>(&m_typescale);
}
void AppBarState::resetTypescale() { m_typescaleKey.reset(); }
void AppBarState::resetContainerHeight() { m_heightKey.reset(); }
} // namespace qml_material
