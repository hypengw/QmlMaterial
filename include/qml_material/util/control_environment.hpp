#pragma once

#include <QLocale>
#include <optional>
#include "qml_material/export.hpp"

QT_BEGIN_NAMESPACE
class QFont;
class QQuickItem;
QT_END_NAMESPACE

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
QML_MATERIAL_API bool    canUpdateControlEnvironment();
QML_MATERIAL_API bool    inheritedHoverEnabled(const QQuickItem*);
QML_MATERIAL_API QFont   inheritedFont(const QQuickItem*);
QML_MATERIAL_API QLocale inheritedLocale(const QQuickItem*);
QML_MATERIAL_API Qt::LayoutDirection inheritedLayoutDirection(const QQuickItem*);
QML_MATERIAL_API QFont               resolveFont(const QFont& requested, const QFont& inherited);
QML_MATERIAL_API void                propagateFont(QQuickItem*, const QFont&);
QML_MATERIAL_API void                propagateHoverEnabled(QQuickItem*, bool);
QML_MATERIAL_API void                propagateLocale(QQuickItem*, const QLocale&);
QML_MATERIAL_API void                propagateLayoutDirection(QQuickItem*, Qt::LayoutDirection);
} // namespace utils
} // namespace qml_material
