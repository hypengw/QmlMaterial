#include "qml_material/view/lazy_list.hpp"
#include "lazy_view_layout_p.hpp"

namespace qml_material
{
LazyList::LazyList(QQuickItem* parent): LazyView(std::make_unique<LinearViewLayout>(), parent) {}
qreal LazyList::spacing() const { return static_cast<LinearViewLayout&>(layoutEngine()).spacing; }
qreal LazyList::estimatedItemExtent() const {
    return static_cast<LinearViewLayout&>(layoutEngine()).estimate;
}
void LazyList::setSpacing(qreal value) {
    auto& layout = static_cast<LinearViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value < 0 || value == layout.spacing) return;
    layout.spacing = value;
    invalidateLayout();
    emit spacingChanged();
}
void LazyList::setEstimatedItemExtent(qreal value) {
    auto& layout = static_cast<LinearViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value <= 0 || value == layout.estimate) return;
    layout.estimate = value;
    invalidateLayout();
    emit estimatedItemExtentChanged();
}
} // namespace qml_material
