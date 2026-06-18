#include "qml_material/carousel/carousel_strategy.hpp"

// Literal values must match qml_material::token::Carousel (include/qml_material/token/token.hpp).
namespace TokenCarouselLiterals
{
constexpr qreal min_peek_px                = 16;
constexpr qreal parallax_ratio             = 0.35;
constexpr qreal parallax_ratio_uncontained = 0.5;
constexpr int   snap_duration              = 400;
constexpr qreal min_item_aspect            = 9.0 / 16.0;
constexpr qreal max_item_aspect            = 16.0 / 9.0;
} // namespace TokenCarouselLiterals

using namespace qml_material;

static_assert(CarouselEngineDefaults::min_peek_px == TokenCarouselLiterals::min_peek_px);
static_assert(CarouselEngineDefaults::parallax_ratio == TokenCarouselLiterals::parallax_ratio);
static_assert(CarouselEngineDefaults::parallax_ratio_uncontained
              == TokenCarouselLiterals::parallax_ratio_uncontained);
static_assert(CarouselEngineDefaults::snap_duration == TokenCarouselLiterals::snap_duration);
static_assert(CarouselEngineDefaults::min_item_aspect == TokenCarouselLiterals::min_item_aspect);
static_assert(CarouselEngineDefaults::max_item_aspect == TokenCarouselLiterals::max_item_aspect);

int main()
{
    return 0;
}
