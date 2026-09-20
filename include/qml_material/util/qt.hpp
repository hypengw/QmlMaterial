#pragma once

#include <QList>
#include <QObject>
#include <Qt>
#include <utility>

namespace qml_material::utils
{

inline void disconnectAll(QList<QMetaObject::Connection>& connections) {
    for (const auto& connection : std::as_const(connections)) QObject::disconnect(connection);
    connections.clear();
}

inline bool isKeyboardFocusReason(Qt::FocusReason reason) {
    return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
           reason == Qt::ShortcutFocusReason;
}

} // namespace qml_material::utils
