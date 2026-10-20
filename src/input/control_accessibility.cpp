#include "control_accessibility_p.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/control/number_input.hpp"
#include "qml_material/control/color_input.hpp"
#include <QAccessible>
#include <QCoreApplication>
#include <QtQuick/private/qaccessiblequickitem_p.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#include <algorithm>

namespace qml_material::detail
{
#if QT_CONFIG(accessibility)
namespace
{
class ValueAccessible : public QAccessibleQuickItem {
public:
    explicit ValueAccessible(QQuickItem* item): QAccessibleQuickItem(item) {}
    QAccessible::Role role() const override {
        const auto configured = QAccessibleQuickItem::role();
        return configured != QAccessible::NoRole    ? configured
               : qobject_cast<NumberInput*>(item()) ? QAccessible::SpinBox
                                                    : QAccessible::Slider;
    }
    QVariant currentValue() const override {
        if (auto* wheel = qobject_cast<ColorWheel*>(item())) return wheel->hue();
        return QAccessibleQuickItem::currentValue();
    }
    void setCurrentValue(const QVariant& value) override {
        bool       valid  = false;
        const auto number = value.toDouble(&valid);
        if (! valid) return;
        if (auto* slider = qobject_cast<Slider*>(item()))
            slider->requestValue(number);
        else if (auto* spin = qobject_cast<NumberInput*>(item()))
            spin->requestValue(number);
        else if (auto* wheel = qobject_cast<ColorWheel*>(item()))
            wheel->requestHue(number);
    }
    QVariant minimumValue() const override {
        if (auto* slider = qobject_cast<Slider*>(item()))
            return std::min(slider->from(), slider->to());
        if (auto* spin = qobject_cast<NumberInput*>(item()))
            return std::min(spin->from(), spin->to());
        return 0;
    }
    QVariant maximumValue() const override {
        if (auto* slider = qobject_cast<Slider*>(item()))
            return std::max(slider->from(), slider->to());
        if (auto* spin = qobject_cast<NumberInput*>(item()))
            return std::max(spin->from(), spin->to());
        return 1;
    }
    QVariant minimumStepSize() const override {
        if (auto* slider = qobject_cast<Slider*>(item())) return slider->stepSize();
        if (auto* spin = qobject_cast<NumberInput*>(item())) return spin->step();
        return 1.0 / 360;
    }
    void doAction(const QString& action) override {
        const bool increase = action == QAccessibleActionInterface::increaseAction();
        const bool decrease = action == QAccessibleActionInterface::decreaseAction();
        if (! increase && ! decrease) {
            QAccessibleQuickItem::doAction(action);
            return;
        }
        if (auto* attached = QQuickAccessibleAttached::attachedProperties(item());
            attached && attached->doAction(action))
            return;
        if (auto* slider = qobject_cast<Slider*>(item())) {
            if (increase)
                slider->increase();
            else
                slider->decrease();
        } else if (auto* spin = qobject_cast<NumberInput*>(item())) {
            if (increase)
                spin->increase();
            else
                spin->decrease();
        } else if (auto* wheel = qobject_cast<ColorWheel*>(item()))
            wheel->requestHue(wheel->hue() + (increase ? 1 : -1) / 360.0);
    }
};
QAccessibleInterface* factory(const QString&, QObject* object) {
    if (auto* item = qobject_cast<QQuickItem*>(object);
        item && (qobject_cast<Slider*>(item) || qobject_cast<NumberInput*>(item) ||
                 qobject_cast<ColorWheel*>(item)))
        return new ValueAccessible(item);
    return nullptr;
}
bool installed = false;
void removeFactory() {
    QAccessible::removeFactory(factory);
    installed = false;
}
} // namespace
#endif
void ensureControlAccessibility() {
#if QT_CONFIG(accessibility)
    if (installed) return;
    installed = true;
    QAccessible::installFactory(factory);
    qAddPostRoutine(removeFactory);
#endif
}
void notifyAccessibleValue(QQuickItem* item, const QVariant& value) {
#if QT_CONFIG(accessibility)
    if (! QAccessible::isActive()) return;
    QAccessibleValueChangeEvent event(item, value);
    QAccessible::updateAccessibility(&event);
#else
    Q_UNUSED(item);
    Q_UNUSED(value);
#endif
}
} // namespace qml_material::detail
