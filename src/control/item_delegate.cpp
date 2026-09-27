#include "qml_material/control/item_delegate.hpp"
#include "qml_material/control/icon_spec.hpp"
#include "qml_material/style/theme.hpp"
#include <QQmlEngine>
#include <QQmlProperty>
#include <QtQml/private/qqmlanybinding_p.h>
#include <QtQml/private/qqmlpropertytopropertybinding_p.h>

namespace qml_material
{
ItemDelegate::ItemDelegate(QQuickItem* parent): AbstractButton(parent) {
    setFocusPolicy(Qt::NoFocus);
    setPadding(12);
    setSpacing(8);
    icon()->setWidth(24);
    icon()->setHeight(24);
    connect(this,
            &Control::implicitLayoutHeightChanged,
            this,
            &ItemDelegate::implicitDelegateHeightChanged);
    connect(this,
            &AbstractButton::implicitIndicatorHeightChanged,
            this,
            &ItemDelegate::implicitDelegateHeightChanged);
    connect(this, &Control::topPaddingChanged, this, &ItemDelegate::implicitDelegateHeightChanged);
    connect(
        this, &Control::bottomPaddingChanged, this, &ItemDelegate::implicitDelegateHeightChanged);
}
qreal ItemDelegate::implicitDelegateHeight() const {
    return std::max(implicitLayoutHeight(),
                    implicitIndicatorHeight() + topPadding() + bottomPadding());
}
void ItemDelegate::classBegin() {
    AbstractButton::classBegin();
    auto* theme = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(this, true));
    m_defaultIconColor.setBinding([theme] {
        auto* colors = theme->color();
        return colors ? colors->on_background() : QColor(Qt::transparent);
    });
    for (const auto& properties :
         { std::pair { QQmlProperty(this, "implicitLayoutWidth"),
                       QQmlProperty(this, "implicitWidth") },
           std::pair { QQmlProperty(this, "implicitDelegateHeight"),
                       QQmlProperty(this, "implicitHeight") },
           std::pair { QQmlProperty(this, "defaultIconColor"), QQmlProperty(icon(), "color") } }) {
        const auto& source = properties.first;
        const auto& target = properties.second;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        auto binding = QQmlPropertyToPropertyBinding::create(qmlEngine(this), source, target);
#else
        QQmlAnyBinding binding;
        binding = new QQmlPropertyToPropertyBinding(qmlEngine(this),
                                                    source.object(),
#    if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                                                    QQmlPropertyIndex(source.index()),
#    else
                                                    source.index(),
#    endif
                                                    target.object(),
                                                    target.index());
#endif
        binding.installOn(target);
    }
}
} // namespace qml_material
