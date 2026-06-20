#include "qml_material/carousel/carousel_strategy.hpp"

#include "qml_material/carousel/carousel_keylines.hpp"

namespace qml_material
{

auto CarouselStrategy::compute(const CarouselLayoutInput& input) -> CarouselLayoutOutput
{
    if (input.count <= 0 || input.viewport_size <= 0) {
        return {};
    }
    const KeylineList keylines = CarouselKeylines::buildList(input);
    return CarouselKeylines::layoutItems(input, keylines);
}

} // namespace qml_material
