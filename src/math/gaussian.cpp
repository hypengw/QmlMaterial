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
        out[i]        = static_cast<std::uint8_t>(
            std::max(0, std::min(255, round_to_int(v * 255.0f))));
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

int rrect_corner_blur_size(scalar sigma, scalar radius) {
    if (sigma < 0.5f || radius < 0.5f) return 0;
    const int margin = static_cast<int>(std::ceil(3.0f * sigma));
    const int r_i    = static_cast<int>(std::ceil(radius));
    return r_i + margin;
}

void fill_rrect_corner_blur(std::span<std::uint8_t> out, scalar sigma, scalar radius) {
    assert(sigma >= 0.5f && radius >= 0.5f);
    const int margin = static_cast<int>(std::ceil(3.0f * sigma));
    const int r_i    = static_cast<int>(std::ceil(radius));
    const int N      = r_i + margin;
    assert(static_cast<int>(out.size()) == N * N);

    // Extended mask frame: pixels at integer positions p = (mx + i, my + j) for
    // (i, j) in [0, Mw). We need p ∈ [-margin, r+margin] on both axes so that
    // convolving with a 1D kernel of radius `margin` produces valid output across
    // the desired window [-margin, r].
    const int Mx0 = -margin;
    const int Mx1 = r_i + margin;
    const int Mw  = Mx1 - Mx0;     // = r_i + 2*margin
    const int My0 = Mx0;
    const int Mh  = Mw;

    std::vector<float> mask(static_cast<std::size_t>(Mw) * Mh);
    const float        r2 = radius * radius;
    for (int j = 0; j < Mh; ++j) {
        const float y = static_cast<float>(j + My0) + 0.5f;
        for (int i = 0; i < Mw; ++i) {
            const float x = static_cast<float>(i + Mx0) + 0.5f;
            bool        inside;
            if (x < 0.0f || y < 0.0f) {
                inside = false;
            } else if (x >= radius && y >= radius) {
                inside = true;
            } else if (x >= radius) {
                inside = true; // right strip
            } else if (y >= radius) {
                inside = true; // bottom strip
            } else {
                const float dx = x - radius;
                const float dy = y - radius;
                inside         = (dx * dx + dy * dy) <= r2;
            }
            mask[static_cast<std::size_t>(j) * Mw + i] = inside ? 1.0f : 0.0f;
        }
    }

    std::vector<float> kernel(static_cast<std::size_t>(2 * margin + 1));
    fill_gaussian_kernel_1d(kernel, sigma);

    // Horizontal pass
    std::vector<float> tmp(static_cast<std::size_t>(Mw) * Mh, 0.0f);
    for (int j = 0; j < Mh; ++j) {
        for (int i = 0; i < Mw; ++i) {
            float acc = 0.0f;
            for (int k = -margin; k <= margin; ++k) {
                const int ii = std::clamp(i + k, 0, Mw - 1);
                acc += mask[static_cast<std::size_t>(j) * Mw + ii] * kernel[k + margin];
            }
            tmp[static_cast<std::size_t>(j) * Mw + i] = acc;
        }
    }

    // Vertical pass + extract. Output covers the window [-margin, r] on both
    // axes to match the shader's corner_sample UV mapping (UV=0 → local=-margin,
    // UV=1 → local=r). That means output (i, j) corresponds to mask index
    // (i, j) itself — NOT (i + margin, j + margin) — since mask index 0 is
    // already at grid position -margin + 0.5.
    for (int j = 0; j < N; ++j) {
        const int  mj  = j;
        std::uint8_t* row = out.data() + static_cast<std::size_t>(j) * N;
        for (int i = 0; i < N; ++i) {
            const int mi  = i;
            float     acc = 0.0f;
            for (int k = -margin; k <= margin; ++k) {
                const int jj = std::clamp(mj + k, 0, Mh - 1);
                acc += tmp[static_cast<std::size_t>(jj) * Mw + mi] * kernel[k + margin];
            }
            row[i] = static_cast<std::uint8_t>(
                std::clamp(round_to_int(acc * 255.0f), 0, 255));
        }
    }
}

} // namespace qml_material::math
