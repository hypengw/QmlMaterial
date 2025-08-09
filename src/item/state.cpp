#include "qml_material/item/state.hpp"
#include <QQmlContext>
#include <QQmlProperty>
#include <QVariant>
#include "qml_material/theme.hpp"

namespace qml_material
{
State::State(QQuickItem* parent)
    : QQuickItem(parent),
      m_target(nullptr),
      m_ctx(nullptr),
      m_elevation(0),
      m_text_color(),
      m_outline_color(),
      m_background_color(),
      m_support_text_color(),
      m_state_layer_color("transparent"),
      m_state_layer_opacity(0),
      m_content_opacity(1.0),
      m_background_opacity(1.0) {
    connect(this, &State::targetChanged, this, &State::updateCtx, Qt::DirectConnection);
}
State::~State() {}

void State::classBegin() { QQuickItem::classBegin(); }
void State::componentComplete() { QQuickItem::componentComplete(); }

auto State::ctx() const -> Theme* { return m_ctx; }
void State::setCtx(Theme* v) {
    if (m_ctx != v) {
        m_ctx = v;
        ctxChanged();
    }
}

void State::updateCtx() {
    if (m_target != nullptr) {
        if (auto a = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(m_target, true))) {
            setCtx(a);
        }
    }
}

auto State::target() const noexcept -> QObject* { return m_target; }
void State::set_target(QObject* item) {
    if (m_target != item) {
        m_target = item;
        targetChanged();
    }
}

#define X(type, name, sig)                                         \
    auto State::name() const noexcept -> type { return m_##name; } \
    void State::set_##name(type val) {                             \
        if (ycore::cmp_set(m_##name, val)) {                       \
            sig();                                                 \
        }                                                          \
    }


X(qint32, elevation, elevationChanged)
X(QColor, text_color, textColorChanged)
X(QColor, outline_color, outlineColorChanged)
X(QColor, background_color, backgroundColorChanged)
X(QColor, support_text_color, supportTextColorChanged)
X(QColor, state_layer_color, stateLayerColorChanged)
X(double, state_layer_opacity, stateLayerOpacityChanged)
X(double, content_opacity, contentOpacityChanged)
X(double, background_opacity, backgroundOpacityChanged)
#undef X

} // namespace qml_material

#include <qml_material/item/moc_state.cpp>