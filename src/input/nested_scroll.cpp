#include "qml_material/input/nested_scroll.hpp"
#include "scroll_input_p.hpp"
#include "scroll_participant_p.hpp"
#include <algorithm>

namespace qml_material
{
NestedScroll::NestedScroll(QObject* parent)
    : QObject(parent), m_item(qobject_cast<QQuickItem*>(parent)) {}
NestedScroll::~NestedScroll() {
    ScrollInputAdapter::stop(this);
    if (m_connection) m_connection->m_attachment = nullptr;
    ScrollInputAdapter::destroy(this);
}
void NestedScroll::setConnection(NestedScrollConnection* connection) {
    if (m_connection == connection) return;
    if (connection && connection->m_attachment && connection->m_attachment != this) {
        qWarning("NestedScrollConnection is already attached to another viewport");
        return;
    }
    QPointer<NestedScroll>           guard(this);
    QPointer<NestedScrollConnection> next(connection);
    const auto                       previous = m_connection;
    ScrollInputAdapter::stop(this);
    if (! guard || m_connection != previous) return;
    if (next && next->m_attachment && next->m_attachment != this) {
        qWarning("NestedScrollConnection is already attached to another viewport");
        return;
    }
    disconnect(m_connectionDestroyed);
    if (m_connection) m_connection->m_attachment = nullptr;
    m_connection = next;
    if (next) {
        next->m_attachment    = this;
        m_connectionDestroyed = connect(next, &QObject::destroyed, this, [this] {
            QPointer<NestedScroll> guard(this);
            ScrollInputAdapter::stop(this);
            if (guard) emit connectionChanged();
        });
    }
    emit connectionChanged();
}
NestedScroll* NestedScroll::qmlAttachedProperties(QObject* item) { return new NestedScroll(item); }
void          NestedScroll::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    if (enabled && ! ScrollParticipant(m_item).item()) {
        qWarning("NestedScroll requires a Scrollable or Qt Flickable");
        return;
    }
    m_enabled = enabled;
    if (enabled) ScrollInputAdapter::ensure(this);
    if (! enabled) ScrollInputAdapter::stop(this);
    emit enabledChanged();
}
void NestedScroll::setAxes(Qt::Orientations axes) {
    if (m_axes == axes) return;
    m_axes = axes;
    emit axesChanged();
}
void NestedScroll::setPressDelay(int delay) {
    delay = std::max(0, delay);
    if (m_pressDelay == delay) return;
    m_pressDelay = delay;
    Q_EMIT pressDelayChanged();
}
int NestedScroll::childPressDelay(QQuickItem* receiver) {
    for (auto* item = receiver ? receiver->parentItem() : nullptr; item;
         item       = item->parentItem()) {
        const ScrollParticipant participant(item);
        if (participant.available() && participant.axes()) return participant.pressDelay();
    }
    return 0;
}
bool NestedScroll::coordinates(QQuickItem* receiver, QQuickItem* ancestor) {
    return ScrollInputAdapter::coordinates(receiver, ancestor);
}
} // namespace qml_material
