#include "qml_material/item/state.hpp"
#include <QQmlContext>
#include <QQmlProperty>
#include "qml_material/theme.hpp"

namespace qml_material
{
State::State(QQuickItem* parent)
    : QQuickItem(parent),
      m_item(nullptr),
      m_elevation(0),
      m_text_color(),
      m_outline_color(),
      m_background_color(),
      m_support_text_color(),
      m_state_layer_color("transparent"),
      m_state_layer_opacity(0),
      m_content_opacity(1.0),
      m_background_opacity(1.0) {
    connect(this, &State::itemChanged, this, &State::ctxChanged, Qt::DirectConnection);
}
State::~State() {}

void State::classBegin() { QQuickItem::classBegin(); }
void State::componentComplete() {
    QQuickItem::componentComplete();
    QQmlProperty item(this, "item");
    auto         meta = item.propertyMetaType().metaObject();
    if (meta == nullptr || std::string_view(meta->className()) != "QObject") {
        item.connectNotifySignal(this, SLOT(on_item_changed()));
    }
}

void State::on_item_changed() { ctxChanged(); }

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

auto State::ctx() const -> Theme* {
    auto item = property("item").value<QObject*>();
    if (item != nullptr) {
        if (auto a = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(item, true))) {
            return a;
        }
    } else {
    }
    return nullptr;
}

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