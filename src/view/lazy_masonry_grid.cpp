#include "qml_material/view/lazy_masonry_grid.hpp"
#include "masonry_layout_p.hpp"

namespace qml_material
{
LazyMasonryGrid::LazyMasonryGrid(QQuickItem* parent)
    : LazyView(std::make_unique<MasonryViewLayout>(), parent) {}
int LazyMasonryGrid::fixedSlotCount() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).fixedSlotCount;
}
qreal LazyMasonryGrid::minimumCellExtent() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).minimum;
}
qreal LazyMasonryGrid::estimatedItemExtent() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).estimate;
}
qreal LazyMasonryGrid::mainSpacing() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).mainSpacing;
}
qreal LazyMasonryGrid::crossSpacing() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).crossSpacing;
}
int LazyMasonryGrid::slotCount() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).slotCount();
}
qreal LazyMasonryGrid::cellCrossExtent() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).cellExtent();
}
Qt::LayoutDirection LazyMasonryGrid::layoutDirection() const {
    return static_cast<MasonryViewLayout&>(layoutEngine()).mirrored ? Qt::RightToLeft
                                                                    : Qt::LeftToRight;
}
void LazyMasonryGrid::setFixedSlotCount(int value) {
    auto& layout = static_cast<MasonryViewLayout&>(layoutEngine());
    if (value < 0 || value == layout.fixedSlotCount) return;
    layout.fixedSlotCount = value;
    invalidateLayout();
    emit fixedSlotCountChanged();
}
void LazyMasonryGrid::setMinimumCellExtent(qreal value) {
    auto& layout = static_cast<MasonryViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value <= 0 || value == layout.minimum) return;
    layout.minimum = value;
    invalidateLayout();
    emit minimumCellExtentChanged();
}
void LazyMasonryGrid::setEstimatedItemExtent(qreal value) {
    auto& layout = static_cast<MasonryViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value <= 0 || value == layout.estimate) return;
    layout.estimate = value;
    invalidateLayout();
    emit estimatedItemExtentChanged();
}
void LazyMasonryGrid::setMainSpacing(qreal value) {
    auto& layout = static_cast<MasonryViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value < 0 || value == layout.mainSpacing) return;
    layout.mainSpacing = value;
    invalidateLayout();
    emit mainSpacingChanged();
}
void LazyMasonryGrid::setCrossSpacing(qreal value) {
    auto& layout = static_cast<MasonryViewLayout&>(layoutEngine());
    if (! std::isfinite(value) || value < 0 || value == layout.crossSpacing) return;
    layout.crossSpacing = value;
    invalidateLayout();
    emit crossSpacingChanged();
}
void LazyMasonryGrid::setLayoutDirection(Qt::LayoutDirection value) {
    if ((value != Qt::LeftToRight && value != Qt::RightToLeft) || value == layoutDirection())
        return;
    static_cast<MasonryViewLayout&>(layoutEngine()).mirrored = value == Qt::RightToLeft;
    invalidateLayout();
    emit layoutDirectionChanged();
}
} // namespace qml_material
