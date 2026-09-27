#pragma once

/*
 * Gaussian blur math: sigma<->radius, CDF profile LUT, separable kernel.
 *
 * Based on principles from Skia's SkBlurMask (BSD-3, third_party/skia/LICENSE).
 * Implementations are independent re-expressions; no Skia code is copied.
 */

#include <cstdint>
#include <span>

#include "qml_material/math/scalar.hpp"

namespace qml_material::math
{

/// sigma ↔ radius conversion. Same 1/√3 scale as Skia for Material parity.
inline constexpr scalar k_blur_sigma_scale = 0.57735f; // ≈ 1/sqrt(3)

[[nodiscard]] inline scalar radius_to_sigma(scalar radius) {
    return radius > 0 ? k_blur_sigma_scale * radius + 0.5f : 0.0f;
}

[[nodiscard]] inline scalar sigma_to_radius(scalar sigma) {
    return sigma > 0.5f ? (sigma - 0.5f) / k_blur_sigma_scale : 0.0f;
}

/// Standard-normal CDF: Φ(x) = 0.5 * (1 + erf(x / √2)).
/// Used to populate the unit blur-profile LUT.
[[nodiscard]] float gaussian_cdf(float x);

/// Populate a unit-sigma cumulative-gaussian profile spanning u ∈ [-3, 3].
/// `out[i]` = round(255 * Φ(((i + 0.5)/N) * 6 - 3)).
///
/// The LUT is sigma-independent — consumers sample as
///   `Φ(t/σ) ≈ tex((t/σ)/6 + 0.5)` in shader.
/// This lets a single texture serve all sigmas.
void fill_unit_cdf_profile(std::span<std::uint8_t> out);

/// Size of the 1D separable gaussian kernel for a given sigma
/// (2 * ceil(3σ) + 1).
[[nodiscard]] int gaussian_kernel_radius(scalar sigma);

/// Populate a 1D gaussian kernel normalized to sum = 1.
/// `out.size()` must equal `2 * gaussian_kernel_radius(sigma) + 1`.
void fill_gaussian_kernel_1d(std::span<float> out, scalar sigma);

/// Edge length of a blurred corner cutout, including the full Gaussian support.
/// Equal to ceil(radius) + 2 * ceil(3*sigma); zero for non-positive inputs.
[[nodiscard]] int rrect_corner_cutout_size(scalar sigma, scalar radius);

/// Blur the part of [0, radius]^2 outside the circle centered at (radius, radius).
/// Subtracting four such cutouts from a blurred rectangle gives its rounded mask.
/// Pixel (i, j) is centered at (i + .5 - margin, j + .5 - margin), where
/// margin = ceil(3*sigma). The output is a tightly packed size * size alpha8 image.
void fill_rrect_corner_cutout(std::span<std::uint8_t> out, scalar sigma, scalar radius);

} // namespace qml_material::math
