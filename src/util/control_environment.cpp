#include "qml_material/util/control_environment.hpp"
#include <QFont>
#include <QGuiApplication>
#include <QList>
#include <QPointer>
#include <QQuickItem>
#include <QStyleHints>

namespace qml_material::utils
{
namespace
{
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
} // namespace

bool canUpdateControlEnvironment() { return qGuiApp && ! QCoreApplication::closingDown(); }

bool inheritedHoverEnabled(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem()) {
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent)) {
            if (auto enabled = owner->effectiveHoverEnabled()) return *enabled;
        }
    }
    return QGuiApplication::styleHints()->useHoverEffects();
}

QFont inheritedFont(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem())
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent)) return owner->effectiveFont();
    return QGuiApplication::font();
}

QLocale inheritedLocale(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem())
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent))
            if (auto value = owner->effectiveLocale()) return *value;
    return QLocale();
}

Qt::LayoutDirection inheritedLayoutDirection(const QQuickItem* item) {
    for (auto parent = item->parentItem(); parent; parent = parent->parentItem())
        if (auto owner = dynamic_cast<ControlEnvironment*>(parent))
            if (auto value = owner->effectiveLayoutDirection()) return *value;
    return QGuiApplication::layoutDirection();
}

QFont resolveFont(const QFont& requested, const QFont& inherited) {
    auto font = requested.resolve(inherited);
    font.setResolveMask(requested.resolveMask() | inherited.resolveMask());
    return font;
}

void propagateFont(QQuickItem* item, const QFont& font) {
    propagateEnvironmentValue(item, [&font](ControlEnvironment* owner) {
        owner->inheritFont(font);
        return true;
    });
}

void propagateHoverEnabled(QQuickItem* item, bool enabled) {
    propagateEnvironmentValue(item, [enabled](ControlEnvironment* owner) {
        if (! owner->effectiveHoverEnabled()) return false;
        owner->inheritHoverEnabled(enabled);
        return true;
    });
}

void propagateLocale(QQuickItem* item, const QLocale& locale) {
    propagateEnvironmentValue(item, [&locale](ControlEnvironment* owner) {
        if (! owner->effectiveLocale()) return false;
        owner->inheritLocale(locale);
        return true;
    });
}

void propagateLayoutDirection(QQuickItem* item, Qt::LayoutDirection direction) {
    propagateEnvironmentValue(item, [direction](ControlEnvironment* owner) {
        if (! owner->effectiveLayoutDirection()) return false;
        owner->inheritLayoutDirection(direction);
        return true;
    });
}
} // namespace qml_material::utils
