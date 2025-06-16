#include "qml_material/item/state.hpp"
#include <QQmlContext>
#include <QQmlProperty>
#include <QVariant>
#include "qml_material/theme.hpp"

namespace qml_material
{
State::State(QQuickItem* parent)
    : QQuickItem(parent),
      m_item(nullptr),
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
    connect(this, &State::itemChanged, this, &State::updateCtx, Qt::DirectConnection);
}
State::~State() {}

void State::classBegin() { QQuickItem::classBegin(); }
void State::componentComplete() {
    QQuickItem::componentComplete();
    // auto ctx = qmlContext(this);
    // Q_ASSERT(ctx);
    // auto prop = QQmlProperty(this, "item", ctx);
    // auto ok   = prop.connectNotifySignal(this, SLOT(updateCtx()));
    // Q_ASSERT(ok);
}

auto State::ctx() const -> Theme* { return m_ctx; }
void State::setCtx(Theme* v) {
    if (m_ctx != v) {
        m_ctx = v;
        ctxChanged();
    }
}

void State::updateCtx() {
    QObject const* item = nullptr;
    if (auto ctx = qmlContext(this)) {
        item = QQmlProperty(this, "item", ctx).read().value<QObject*>();
    } else {
        item = m_item;
    }

    if (item != nullptr) {
        if (auto a = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(item, true))) {
            setCtx(a);
        }
    }
}

#define X(type, name, sig)                                \
    auto State::name() const -> type { return m_##name; } \
    void State::set_##name(type val) {                    \
        if (std::exchange(m_##name, val) != val) {        \
            sig();                                        \
        }                                                 \
    }

X(QObject*, item, itemChanged)
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

StateHolder::StateHolder(QObject* parent): QObject(parent), m_state(nullptr) {}
StateHolder::~StateHolder() {}

auto StateHolder::state() const -> State* { return m_state; }

void StateHolder::setState(State* item) {
    if (auto old = std::exchange(m_state, item); old != item) {
        if (old) {
            old->setEnabled(false);
        }
        if (m_state) {
            if (! m_state->parentItem()) {
                if (auto p = qobject_cast<QQuickItem*>(this->parent()); p)
                    m_state->setParentItem(p);
            }
            m_state->setEnabled(true);
        }
        Q_EMIT stateChanged();
    }
}
} // namespace qml_material

#include <qml_material/item/moc_state.cpp>