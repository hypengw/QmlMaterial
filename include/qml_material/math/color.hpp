#pragma once

#include <QColor>
#include <algorithm>
#include <array>
#include <cmath>

namespace qml_material::math
{
// QColor channels are interpreted as sRGB; alpha is kept separate from Oklab.
inline auto toOklab(QColor color) -> std::array<qreal, 3> {
    const auto linear = [](qreal channel) {
        return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
    };
    color        = color.toRgb();
    const auto r = linear(color.redF()), g = linear(color.greenF()), b = linear(color.blueF());
    const auto l = std::cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    const auto m = std::cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    const auto s = std::cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
    return { 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
             1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
             0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s };
}
inline auto fromOklab(const std::array<qreal, 3>& lab, qreal alpha) -> QColor {
    auto l = lab[0] + 0.3963377774 * lab[1] + 0.2158037573 * lab[2];
    auto m = lab[0] - 0.1055613458 * lab[1] - 0.0638541728 * lab[2];
    auto s = lab[0] - 0.0894841775 * lab[1] - 1.2914855480 * lab[2];
    l *= l * l;
    m *= m * m;
    s *= s * s;
    const auto srgb = [](qreal channel) {
        channel = std::clamp(channel, qreal(0), qreal(1));
        return channel <= 0.0031308 ? 12.92 * channel : 1.055 * std::pow(channel, 1 / 2.4) - 0.055;
    };
    return QColor::fromRgbF(srgb(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
                            srgb(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
                            srgb(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s),
                            alpha);
}
} // namespace qml_material::math
