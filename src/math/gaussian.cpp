#include "qml_material/math/gaussian.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace qml_material::math
{

float gaussian_cdf(float x) {
    constexpr float inv_sqrt2 = 0.70710678118f; // 1/sqrt(2)
    return 0.5f * (1.0f + std::erf(x * inv_sqrt2));
}

void fill_unit_cdf_profile(std::span<std::uint8_t> out) {
    const int n = static_cast<int>(out.size());
    assert(n > 1);
    for (int i = 0; i < n; ++i) {
        const float u = (static_cast<float>(i) + 0.5f) / static_cast<float>(n) * 6.0f - 3.0f;
        const float v = gaussian_cdf(u);
        out[i] = static_cast<std::uint8_t>(std::max(0, std::min(255, round_to_int(v * 255.0f))));
    }
}

int gaussian_kernel_radius(scalar sigma) {
    if (sigma <= 0.0f) return 0;
    return std::max(1, static_cast<int>(std::ceil(3.0f * sigma)));
}

void fill_gaussian_kernel_1d(std::span<float> out, scalar sigma) {
    const int radius = static_cast<int>(out.size()) / 2;
    assert(static_cast<int>(out.size()) == 2 * radius + 1);
    if (sigma <= 0.0f) {
        for (auto& v : out) v = 0.0f;
        out[radius] = 1.0f;
        return;
    }
    const float two_sigma_sqr = 2.0f * sigma * sigma;
    float       sum           = 0.0f;
    for (int i = 0; i < static_cast<int>(out.size()); ++i) {
        const float x = static_cast<float>(i - radius);
        out[i]        = std::exp(-x * x / two_sigma_sqr);
        sum += out[i];
    }
    const float inv_sum = 1.0f / sum;
    for (auto& v : out) v *= inv_sum;
}

int rrect_corner_cutout_size(scalar sigma, scalar radius) {
    if (sigma <= 0.0f || radius <= 0.0f) return 0;
    return static_cast<int>(std::ceil(radius)) + 2 * gaussian_kernel_radius(sigma);
}

void fill_rrect_corner_cutout(std::span<std::uint8_t> out, scalar sigma, scalar radius) {
    assert(sigma > 0.0f && radius > 0.0f);
    const int margin = gaussian_kernel_radius(sigma);
    const int n      = rrect_corner_cutout_size(sigma, radius);
    assert(static_cast<int>(out.size()) == n * n);

    std::vector<float> mask(static_cast<std::size_t>(n) * n, 0.0f);
    // Subpixel coverage keeps fractional radii from snapping at pixel centers.
    constexpr int samples = 4;
    const int     end     = margin + static_cast<int>(std::ceil(radius));
    for (int j = margin; j < end; ++j) {
        for (int i = margin; i < end; ++i) {
            int covered = 0;
            for (int sy = 0; sy < samples; ++sy) {
                for (int sx = 0; sx < samples; ++sx) {
                    const float x  = i - margin + (sx + 0.5f) / samples;
                    const float y  = j - margin + (sy + 0.5f) / samples;
                    const float dx = x - radius;
                    const float dy = y - radius;
                    covered += x < radius && y < radius && dx * dx + dy * dy > radius * radius;
                }
            }
            mask[static_cast<std::size_t>(j) * n + i] = float(covered) / (samples * samples);
        }
    }

    std::vector<float> kernel(static_cast<std::size_t>(2 * margin + 1));
    fill_gaussian_kernel_1d(kernel, sigma);

    std::vector<float> tmp(static_cast<std::size_t>(n) * n, 0.0f);
    for (int j = margin; j < end; ++j) {
        for (int i = 0; i < n; ++i) {
            float acc = 0.0f;
            for (int k = std::max(-margin, -i); k <= std::min(margin, n - 1 - i); ++k) {
                acc += mask[static_cast<std::size_t>(j) * n + i + k] * kernel[k + margin];
            }
            tmp[static_cast<std::size_t>(j) * n + i] = acc;
        }
    }

    for (int j = 0; j < n; ++j) {
        std::uint8_t* row = out.data() + static_cast<std::size_t>(j) * n;
        for (int i = 0; i < n; ++i) {
            float acc = 0.0f;
            for (int k = std::max(-margin, -j); k <= std::min(margin, n - 1 - j); ++k) {
                acc += tmp[static_cast<std::size_t>(j + k) * n + i] * kernel[k + margin];
            }
            row[i] = static_cast<std::uint8_t>(std::clamp(round_to_int(acc * 255.0f), 0, 255));
        }
    }
}

} // namespace qml_material::math
