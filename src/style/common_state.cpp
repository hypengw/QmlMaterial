#include "qml_material/style/common_state.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/style/theme.hpp"
#include "qml_material/util/qt.hpp"
#include <QQuickItem>

namespace qml_material
{
CommonState::CommonState(QObject* parent): QObject(parent) {
#define APPEARANCE(Name, Initial) m_##Name##Value = Initial;
    APPEARANCE(textColor, QColor(Qt::transparent))
    APPEARANCE(backgroundColor, QColor(Qt::transparent))
    APPEARANCE(stateLayerColor, QColor(Qt::transparent))
    APPEARANCE(outlineColor, QColor(Qt::transparent))
    APPEARANCE(supportTextColor, QColor(Qt::transparent))
    APPEARANCE(elevation, 0)
    APPEARANCE(corner, 0)
    APPEARANCE(contentOpacity, 1)
    APPEARANCE(backgroundOpacity, 1)
    APPEARANCE(stateLayerOpacity, 0)
#undef APPEARANCE
    resetColors();
    resetCorners();
    m_values.setBinding([this] {
        StateValues result;
        result.state = state();
#define VALUE(Name) result.Name = Name();
        VALUE(textColor)
        VALUE(backgroundColor)
        VALUE(stateLayerColor)
        VALUE(outlineColor)
        VALUE(supportTextColor)
        VALUE(elevation)
        VALUE(corner)
        VALUE(contentOpacity)
        VALUE(backgroundOpacity)
        VALUE(stateLayerOpacity)
#undef VALUE
        return result;
    });
}
CommonState::~CommonState() {
    disconnect(this, nullptr, this, nullptr);
    disconnect(m_targetDestroyed);
    utils::disconnectAll(m_contextConnections);
    utils::disconnectAll(m_colorConnections);
}

QObject*               CommonState::target() const { return m_target; }
MdColorMgr*            CommonState::colors() const { return m_colors.value(); }
QBindable<MdColorMgr*> CommonState::bindableColors() { return QBindable<MdColorMgr*>(&m_colors); }
void                   CommonState::setTarget(QObject* target) {
    if (m_target == target) return;
    const QScopedPropertyUpdateGroup group;
    disconnect(m_targetDestroyed);
    m_targetGeneration = m_targetGeneration.value() + 1;
    m_target           = target;
    if (target) {
        m_targetDestroyed = connect(target, &QObject::destroyed, this, [this] {
            const QScopedPropertyUpdateGroup group;
            m_targetGeneration = m_targetGeneration.value() + 1;
            m_target           = nullptr;
            const QPointer<CommonState> guard(this);
            if (! m_explicitContext) updateContext(nullptr);
            if (guard) Q_EMIT targetChanged();
        });
    }
    const QPointer<CommonState> guard(this);
    if (! m_explicitContext) resetCtx();
    if (guard) Q_EMIT targetChanged();
}
void CommonState::setColors(MdColorMgr* colors) { m_colors = colors; }

void CommonState::colorsChange() {
    utils::disconnectAll(m_colorConnections);
    auto* colors = m_colors.value();
    if (colors) {
        m_colorConnections.append(connect(colors, &QObject::destroyed, this, [this] {
            const QScopedPropertyUpdateGroup group;
            utils::disconnectAll(m_colorConnections);
            m_colors.setValueBypassingBindings(nullptr);
            m_colors.notify();
        }));
    }
    Q_EMIT colorsChanged();
}

#define INPUT(Type, Name, Setter, Bindable)                                      \
    Type            CommonState::Name() const { return m_##Name.value(); }       \
    void            CommonState::Setter(const Type& value) { m_##Name = value; } \
    QBindable<Type> CommonState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(token::State, stateTokens, setStateTokens, bindableStateTokens)
INPUT(token::Elevation, elevationTokens, setElevationTokens, bindableElevationTokens)
#undef INPUT

StateValues            CommonState::values() const { return m_values.value(); }
QBindable<StateValues> CommonState::bindableValues() const {
    return QBindable<StateValues>(&m_values);
}
QQmlListProperty<QObject> CommonState::datas() { return { this, &m_datas }; }
void                      CommonState::classBegin() { m_canApply = false; }
void                      CommonState::componentComplete() {
    m_canApply       = true;
    const auto apply = m_applyState;
    if (apply) apply();
}
void CommonState::publishState(const QString& next, std::function<void()> apply) {
    const auto                  previous = m_state.value();
    const QPointer<CommonState> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        m_state      = next;
        m_applyState = apply;
        if (m_canApply) apply();
    }
    if (! guard) return;
    if (guard && previous != next) Q_EMIT stateChanged();
}
Theme* CommonState::ctx() const { return m_context; }
void   CommonState::setCtx(Theme* context) {
    m_explicitContext = true;
    updateContext(context);
}
void CommonState::resetCtx() {
    m_explicitContext = false;
    updateContext(m_target
                      ? qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(m_target, true))
                      : nullptr);
}
void CommonState::resetColors() {
    m_colors.setBinding([this] {
        auto* context = ctx();
        return context ? context->color() : nullptr;
    });
}
void CommonState::updateContext(Theme* context) {
    if (m_context == context) return;
    utils::disconnectAll(m_contextConnections);
    if (context) {
        m_contextConnections.append(connect(context, &QObject::destroyed, this, [this] {
            utils::disconnectAll(m_contextConnections);
            const QPointer<CommonState> guard(this);
            m_context = nullptr;
            if (guard) Q_EMIT ctxChanged();
        }));
    }
    const QPointer<CommonState> guard(this);
    m_context = context;
    if (guard) Q_EMIT ctxChanged();
}

QString CommonState::state() const { return m_state.value(); }
#define APPEARANCE(Type, Name, Upper)                                               \
    Type            CommonState::Name() const { return m_##Name##Value.value(); }   \
    void            CommonState::set##Upper(const Type& v) { m_##Name##Value = v; } \
    QBindable<Type> CommonState::bindable##Upper() { return QBindable<Type>(&m_##Name##Value); }
APPEARANCE(QColor, textColor, TextColor)
APPEARANCE(QColor, backgroundColor, BackgroundColor)
APPEARANCE(QColor, stateLayerColor, StateLayerColor)
APPEARANCE(QColor, outlineColor, OutlineColor)
APPEARANCE(QColor, supportTextColor, SupportTextColor)
APPEARANCE(qreal, elevation, Elevation)
APPEARANCE(qreal, corner, Corner)
APPEARANCE(CornersGroup, corners, Corners)
APPEARANCE(qreal, contentOpacity, ContentOpacity)
APPEARANCE(qreal, backgroundOpacity, BackgroundOpacity)
APPEARANCE(qreal, stateLayerOpacity, StateLayerOpacity)
#undef APPEARANCE

#define RESET(Name, Upper) \
    void CommonState::reset##Upper() { m_appearance.Name.reset(); }
RESET(textColor, TextColor)
RESET(backgroundColor, BackgroundColor)
RESET(stateLayerColor, StateLayerColor)
RESET(outlineColor, OutlineColor)
RESET(supportTextColor, SupportTextColor)
RESET(elevation, Elevation)
RESET(corner, Corner)
RESET(contentOpacity, ContentOpacity)
RESET(backgroundOpacity, BackgroundOpacity)
RESET(stateLayerOpacity, StateLayerOpacity)
#undef RESET
void CommonState::resetCorners() {
    if (m_appearance.corners.isValid()) {
        m_appearance.corners.reset();
        return;
    }
    m_cornersValue.setBinding([this] {
        return CornersGroup(corner());
    });
}
} // namespace qml_material
