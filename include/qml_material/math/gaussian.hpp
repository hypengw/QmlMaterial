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
/// (2 * ceil(3σ) + 1). Used in iter 5 for the rrect corner texture pre-convolution.
[[nodiscard]] int gaussian_kernel_radius(scalar sigma);

/// Populate a 1D gaussian kernel normalized to sum = 1.
/// `out.size()` must equal `2 * gaussian_kernel_radius(sigma) + 1`.
void fill_gaussian_kernel_1d(std::span<float> out, scalar sigma);

/// Edge length (in pixels) of the rrect-corner blur image for a given (sigma, radius).
/// Equal to `ceil(radius) + ceil(3*sigma)`. Zero when either parameter is ~0.
[[nodiscard]] int rrect_corner_blur_size(scalar sigma, scalar radius);

/// Populate an N×N alpha8 image (row-major, tightly packed) representing the blurred
/// top-left quadrant of a rounded rectangle. Pixel (i, j) corresponds to coverage at
/// local position (i - ceil(3σ), j - ceil(3σ)) inside a semi-infinite rrect-quarter
/// whose arc center is at (radius, radius). `out.size()` must equal `N * N` where
/// `N = rrect_corner_blur_size(sigma, radius)`.
///
/// Preconditions: `sigma >= 0.5`, `radius >= 0.5`. For degenerate inputs callers
/// should not invoke this (no corner texture needed).
void fill_rrect_corner_blur(std::span<std::uint8_t> out, scalar sigma, scalar radius);

} // namespace qml_material::math
