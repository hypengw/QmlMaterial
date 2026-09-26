#pragma once

#include <QCoreApplication>
#include <QPointer>
#include <QQuickWindow>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <memory>

namespace qml_material::pointer_delivery
{
inline std::unique_ptr<QPointerEvent> cloneForWindow(QPointerEvent* event) {
    return std::unique_ptr<QPointerEvent>(QQuickDeliveryAgentPrivate::clonePointerEvent(
        event, event->points().first().scenePosition()));
}

inline void send(QQuickItem* owner, QQuickWindow* window, QPointerEvent* event) {
    auto* agent = QQuickItemPrivate::get(owner)->deliveryAgent();
    if (! agent) return;
    QPointer<QQuickDeliveryAgent> guard(agent);
    auto*                         state     = QQuickItemPrivate::get(owner)->deliveryAgentPrivate();
    const bool                    filtering = state->allowChildEventFiltering;
    state->allowChildEventFiltering         = false;
    QCoreApplication::sendEvent(window, event);
    if (guard) state->allowChildEventFiltering = filtering;
}
} // namespace qml_material::pointer_delivery
