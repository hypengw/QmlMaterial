#pragma once

#include <QFont>
#include <QGuiApplication>
#include <QLocale>
#include <QPointer>
#include <QQuickItem>
#include <QStyleHints>
#include <optional>
#include "qml_material/export.hpp"

namespace qml_material
{

class QML_MATERIAL_API ControlEnvironment {
public:
    virtual ~ControlEnvironment()                            = default;
    virtual QFont                  effectiveFont() const     = 0;
    virtual void                   inheritFont(const QFont&) = 0;
    virtual std::optional<bool>    effectiveHoverEnabled() const { return std::nullopt; }
    virtual void                   inheritHoverEnabled(bool) {}
    virtual std::optional<QLocale> effectiveLocale() const { return std::nullopt; }
    virtual void                   inheritLocale(const QLocale&) {}
    virtual std::optional<Qt::LayoutDirection> effectiveLayoutDirection() const {
        return std::nullopt;
    }
    virtual void inheritLayoutDirection(Qt::LayoutDirection) {}
};

namespace utils
{
inline bool canUpdateControlEnvironment() { return qGuiApp && ! QCoreApplication::closingDown(); }

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

inline QLocale inheritedLocale(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem())
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent))
            if (auto value = owner->effectiveLocale()) return *value;
    return QLocale();
}

inline Qt::LayoutDirection inheritedLayoutDirection(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem())
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent))
            if (auto value = owner->effectiveLayoutDirection()) return *value;
    return QGuiApplication::layoutDirection();
}

inline QFont resolveFont(const QFont& requested, const QFont& inherited) {
    auto font = requested.resolve(inherited);
    font.setResolveMask(requested.resolveMask() | inherited.resolveMask());
    return font;
}

template<typename Apply>
inline void propagateEnvironmentValue(QQuickItem* item, const Apply& apply) {
    QPointer<QQuickItem>        guard(item);
    QList<QPointer<QQuickItem>> children;
    for (auto child : item->childItems()) children.append(child);
    for (const auto& child : children) {
        if (! guard) return;
        if (! child || child->parentItem() != item) continue;
        if (auto owner = dynamic_cast<ControlEnvironment*>(child.data()); owner && apply(owner))
            continue;
        propagateEnvironmentValue(child, apply);
    }
}

inline void propagateFont(QQuickItem* item, const QFont& font) {
    propagateEnvironmentValue(item, [&font](ControlEnvironment* owner) {
        owner->inheritFont(font);
        return true;
    });
}

inline void propagateHoverEnabled(QQuickItem* item, bool enabled) {
    propagateEnvironmentValue(item, [enabled](ControlEnvironment* owner) {
        if (! owner->effectiveHoverEnabled()) return false;
        owner->inheritHoverEnabled(enabled);
        return true;
    });
}

inline void propagateLocale(QQuickItem* item, const QLocale& locale) {
    propagateEnvironmentValue(item, [&locale](ControlEnvironment* owner) {
        if (! owner->effectiveLocale()) return false;
        owner->inheritLocale(locale);
        return true;
    });
}

inline void propagateLayoutDirection(QQuickItem* item, Qt::LayoutDirection direction) {
    propagateEnvironmentValue(item, [direction](ControlEnvironment* owner) {
        if (! owner->effectiveLayoutDirection()) return false;
        owner->inheritLayoutDirection(direction);
        return true;
    });
}
} // namespace utils
} // namespace qml_material
