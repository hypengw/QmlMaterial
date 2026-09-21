#pragma once

#include <QFont>
#include <QGuiApplication>
#include <QPointer>
#include <QQuickItem>
#include <QStyleHints>
#include <optional>
#include "qml_material/export.hpp"

namespace qml_material
{

class QML_MATERIAL_API ControlEnvironment {
public:
    virtual ~ControlEnvironment()                             = default;
    virtual QFont               effectiveFont() const         = 0;
    virtual void                refreshInheritedEnvironment() = 0;
    virtual std::optional<bool> effectiveHoverEnabled() const { return std::nullopt; }
};

namespace utils
{
inline bool inheritedHoverEnabled(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem()) {
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent)) {
            if (auto enabled = owner->effectiveHoverEnabled()) return *enabled;
        }
    }
    return QGuiApplication::styleHints()->useHoverEffects();
}

inline QFont inheritedFont(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem())
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent)) return owner->effectiveFont();
    return QGuiApplication::font();
}

inline QFont resolveFont(const QFont& requested, const QFont& inherited) {
    auto font = requested.resolve(inherited);
    font.setResolveMask(requested.resolveMask() | inherited.resolveMask());
    return font;
}

inline void propagateControlEnvironment(QQuickItem* item) {
    QList<QPointer<QQuickItem>> children;
    for (auto child : item->childItems()) children.append(child);
    for (const auto& child : children) {
        if (! child || child->parentItem() != item) continue;
        if (auto owner = dynamic_cast<ControlEnvironment*>(child.data()))
            owner->refreshInheritedEnvironment();
        else
            propagateControlEnvironment(child);
    }
}
} // namespace utils
} // namespace qml_material
