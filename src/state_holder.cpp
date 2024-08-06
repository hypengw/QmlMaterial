#include "qml_material/state_holder.h"
#include "core/log.h"

using namespace qml_material;

StateHolder::StateHolder(QObject* parent): QObject(parent), m_state(nullptr) {}
StateHolder::~StateHolder() {}

auto StateHolder::state() const -> QQuickItem* { return m_state; }

void StateHolder::setState(QQuickItem* item) {
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