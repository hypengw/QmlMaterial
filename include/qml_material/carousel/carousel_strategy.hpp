#pragma once

#include <QVector>
#include <Qt>

namespace qml_material
{

struct CarouselItemGeometry
{
    int   index         = 0;
    qreal position      = 0;
    qreal size          = 0;
    qreal mask_start    = 0;
    qreal mask_end      = 0;
    qreal parallax_shift = 0;
    int   size_class    = 2; // CarouselSizeLarge
};

struct CarouselScrollState
{
    int   focal_index      = 0;
    qreal offset_fraction  = 0; // [0, 1) progress from focal to focal+1
};

struct CarouselLayoutInput
{
    int     layout            = 0;
    int     alignment         = 0;
    Qt::Orientation orientation = Qt::Horizontal;
    qreal   viewport_size     = 0;
    qreal   cross_size        = 0;
    qreal   scroll_offset     = 0;
    qreal   item_extent       = 180;
    qreal   spacing           = 8;
    qreal   shrink_extent     = 0;
    qreal   small_item_min    = 40;
    qreal   small_item_max    = 56;
    qreal   content_padding_start = 0;
    qreal   content_padding_end   = 0;
    qreal   content_padding_cross = 0;
    qreal   min_peek_px       = 16;
    qreal   item_corner_radius = 28;
    qreal   parallax_ratio    = 0.35;
    int     count             = 0;
    QVector<int> flex_weights;
};

struct CarouselLayoutOutput
{
    QVector<CarouselItemGeometry> items;
    ///< contentX / contentY snap targets (without leading padding)
    QVector<qreal>                snap_offsets;
    qreal                         content_size       = 0;
    qreal                         max_scroll_offset  = 0;
    qreal                         end_snap_offset    = 0;
    qreal                         scroll_step        = 0;
    int                           leading_index      = 0;
};

class CarouselStrategy
{
public:
    static auto compute(const CarouselLayoutInput& input) -> CarouselLayoutOutput;
};

} // namespace qml_material
