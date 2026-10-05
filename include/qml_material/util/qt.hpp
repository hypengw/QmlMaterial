#pragma once

#include <QList>
#include <QObject>
#include <QProperty>
#include <Qt>
#include <utility>

namespace qml_material::utils
{

template<class Owner, class Value, auto Offset>
void updateBoundValue(QObjectBindableProperty<Owner, Value, Offset>& property, const Value& value) {
    // C++ state updates must keep the caller's binding installed.
    // Owner callbacks must be detachable observers, not static property callbacks.
    // Do not batch these updates: Qt skips QML observer notifications for a
    // bound property manually notified inside a property update group.
    if (property.valueBypassingBindings() == value) return;
    property.setValueBypassingBindings(value);
    property.notify();
}

inline void disconnectAll(QList<QMetaObject::Connection>& connections) {
    for (const auto& connection : std::as_const(connections)) QObject::disconnect(connection);
    connections.clear();
}

inline bool isKeyboardFocusReason(Qt::FocusReason reason) {
    return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
           reason == Qt::ShortcutFocusReason;
}

} // namespace qml_material::utils
