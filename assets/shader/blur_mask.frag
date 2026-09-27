#version 440

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) out vec4 fragColor;

layout(location = 0) noperspective in vec2 v_pos;
layout(location = 1) noperspective in vec4 v_color;
layout(binding = 1) uniform sampler2D profile_tex;
layout(binding = 2) uniform sampler2D corner_tl;
layout(binding = 3) uniform sampler2D corner_tr;
layout(binding = 4) uniform sampler2D corner_bl;
layout(binding = 5) uniform sampler2D corner_br;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    float sigma;
    vec2  rect_size;
    int   style;
    float radius;
    float radius_tl;
    float radius_tr;
    float radius_bl;
    float radius_br;
    vec4 corner_uv[4];
};

// Unit-sigma cumulative normal: sample Φ(u) with u ∈ [-3, 3] mapped to [0, 1].
float cdf(float u) {
    float t = clamp(u / 6.0 + 0.5, 0.0, 1.0);
    return texture(profile_tex, vec2(t, 0.5)).r;
}

// Coverage of a 1D step function [0, w] blurred by σ, sampled at x.
// = Φ(x/σ) - Φ((x-w)/σ)
float coverage_1d(float x, float w, float s) {
    if (s < 1e-4) {
        return (x >= 0.0 && x <= w) ? 1.0 : 0.0;
    }
    return cdf(x / s) - cdf((x - w) / s);
}

float corner_sample(sampler2D tex, vec2 local, vec4 mapping) {
    if (mapping.y <= 0.0) return 0.0;
    vec2 uv = (local + vec2(mapping.x)) / mapping.y;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) return 0.0;
    return texture(tex, uv).r;
}

float separable_alpha(vec2 p, float s) {
    float cx = coverage_1d(p.x, rect_size.x, s);
    float cy = coverage_1d(p.y, rect_size.y, s);
    return cx * cy;
}

void main() {
    float s = sigma;
    vec2  p = v_pos;
    float r = radius;

    // Inside mask: per-quadrant SDF rounded rectangle (handles non-uniform corners).
    float inside;
    if (r < 0.5) {
        inside = step(0.0, p.x) * step(p.x, rect_size.x)
               * step(0.0, p.y) * step(p.y, rect_size.y);
    } else {
        vec2  hs  = rect_size * 0.5;
        vec2  ctr = p - hs;
        float sx  = step(0.0, ctr.x);
        float sy  = step(0.0, ctr.y);
        float r_q = mix(mix(radius_tl, radius_tr, sx),
                        mix(radius_bl, radius_br, sx),
                        sy);
        vec2  q   = abs(ctr) - (hs - vec2(r_q));
        float sdf = min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r_q;
        inside    = 1.0 - clamp(sdf, 0.0, 1.0);
    }

    // Gaussian convolution is linear: subtract all four finite corner cutouts.
    // Their blur tails overlap edge regions, so do not switch at the arc radius.
    float blurred = inside;
    if (s >= 0.5) {
        blurred = separable_alpha(p, s)
            - corner_sample(corner_tl, p, corner_uv[0])
            - corner_sample(corner_tr, vec2(rect_size.x - p.x, p.y), corner_uv[1])
            - corner_sample(corner_bl, vec2(p.x, rect_size.y - p.y), corner_uv[2])
            - corner_sample(corner_br, rect_size - p, corner_uv[3]);
        blurred = clamp(blurred, 0.0, 1.0);
    }

    float alpha;
    if (style == 0)      alpha = blurred;                   // Normal
    else if (style == 1) alpha = max(inside, blurred);      // Solid
    else if (style == 2) alpha = blurred * (1.0 - inside);  // Outer
    else                 alpha = blurred * inside;          // Inner

    fragColor = v_color * (alpha * qt_Opacity);
}
