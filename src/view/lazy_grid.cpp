#include "qml_material/view/lazy_grid.hpp"
#include "lazy_view_layout_p.hpp"

namespace qml_material
{
LazyGrid::LazyGrid(QQuickItem* parent): LazyView(std::make_unique<GridViewLayout>(), parent) {}
int LazyGrid::fixedSlotCount() const {
    return static_cast<GridViewLayout&>(layoutEngine()).fixedSlotCount;
}
qreal LazyGrid::minimumCellExtent() const {
    return static_cast<GridViewLayout&>(layoutEngine()).minimum;
}
qreal LazyGrid::itemExtent() const {
    return static_cast<GridViewLayout&>(layoutEngine()).itemExtent;
}
qreal LazyGrid::mainSpacing() const {
    return static_cast<GridViewLayout&>(layoutEngine()).mainSpacing;
}
qreal LazyGrid::crossSpacing() const {
    return static_cast<GridViewLayout&>(layoutEngine()).crossSpacing;
}
int LazyGrid::slotCount() const {
    return static_cast<GridViewLayout&>(layoutEngine()).grid().fixedSlotCount();
}
qreal LazyGrid::cellCrossExtent() const {
    return static_cast<GridViewLayout&>(layoutEngine()).grid().cellExtent();
}
Qt::LayoutDirection LazyGrid::layoutDirection() const {
    return static_cast<GridViewLayout&>(layoutEngine()).mirrored ? Qt::RightToLeft
                                                                 : Qt::LeftToRight;
}
void LazyGrid::setFixedSlotCount(int value) {
    auto& layout = static_cast<GridViewLayout&>(layoutEngine());
    if (value < 0 || value == layout.fixedSlotCount) return;
    layout.fixedSlotCount = value;
    invalidateLayout();
    emit fixedSlotCountChanged();
}
void LazyGrid::setMinimumCellExtent(qreal value) {
    auto& layout = static_cast<GridViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value <= 0 || value == layout.minimum) return;
    layout.minimum = value;
    invalidateLayout();
    emit minimumCellExtentChanged();
}
void LazyGrid::setItemExtent(qreal value) {
    auto& layout = static_cast<GridViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value <= 0 || value == layout.itemExtent) return;
    layout.itemExtent = value;
    invalidateLayout();
    emit itemExtentChanged();
}
void LazyGrid::setMainSpacing(qreal value) {
    auto& layout = static_cast<GridViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value < 0 || value == layout.mainSpacing) return;
    layout.mainSpacing = value;
    invalidateLayout();
    emit mainSpacingChanged();
}
void LazyGrid::setCrossSpacing(qreal value) {
    auto& layout = static_cast<GridViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value < 0 || value == layout.crossSpacing) return;
    layout.crossSpacing = value;
    invalidateLayout();
    emit crossSpacingChanged();
}
void LazyGrid::setLayoutDirection(Qt::LayoutDirection value) {
    if ((value != Qt::LeftToRight && value != Qt::RightToLeft) || value == layoutDirection())
        return;
    static_cast<GridViewLayout&>(layoutEngine()).mirrored = value == Qt::RightToLeft;
    invalidateLayout();
    emit layoutDirectionChanged();
}
} // namespace qml_material
