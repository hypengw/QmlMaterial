#pragma once

#include <algorithm>
#include <cmath>

namespace qml_material::math
{

using scalar = float;

inline constexpr scalar k_scalar_one    = 1.0f;
inline constexpr scalar k_nearly_zero   = k_scalar_one / (1 << 12);
inline constexpr scalar k_float_sqrt2   = 1.41421356f;

template<class T>
[[nodiscard]] constexpr const T& pin(const T& x, const T& lo, const T& hi) {
    return std::max(lo, std::min(x, hi));
}

[[nodiscard]] inline float divide_and_pin(float numer, float denom, float lo, float hi) {
    return std::max(lo, std::min(numer / denom, hi));
}

[[nodiscard]] constexpr int saturate_to_int(float x) {
    constexpr int k_max_s32_fits_in_float = 2147483520;
    constexpr int k_min_s32_fits_in_float = -k_max_s32_fits_in_float;
    x = x < k_max_s32_fits_in_float ? x : (float)k_max_s32_fits_in_float;
    x = x > k_min_s32_fits_in_float ? x : (float)k_min_s32_fits_in_float;
    return (int)x;
}

[[nodiscard]] constexpr float round(double x) { return (float)std::floor(x + 0.5); }

[[nodiscard]] constexpr int round_to_int(double x) { return saturate_to_int(round(x)); }

} // namespace qml_material::math
