#pragma once

#include "qml_material/carousel/carousel_strategy.hpp"

namespace qml_material
{

struct Keyline
{
    qreal offset     = 0; // center position in viewport (main axis)
    qreal size       = 0;
    int   size_class = 2;
};

struct KeylineList
{
    QVector<Keyline> keylines;
    qreal            scroll_step   = 0;
    qreal            large_size    = 0;
    qreal            medium_size   = 0;
    qreal            small_size    = 0;
};

class CarouselKeylines
{
public:
    static auto buildList(const CarouselLayoutInput& input) -> KeylineList;
    static auto layoutItems(const CarouselLayoutInput& input, const KeylineList& keylines)
        -> CarouselLayoutOutput;
};

} // namespace qml_material
