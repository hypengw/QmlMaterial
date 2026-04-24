#version 440

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) out vec4 fragColor;

layout(location = 0) noperspective in vec2 v_pos;
layout(location = 1) noperspective in vec4 v_color;
layout(binding = 1) uniform sampler2D profile_tex;
layout(binding = 2) uniform sampler2D corner_tex;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    float sigma;
    vec2  rect_size;
    int   style;
    float radius;           // max of the four, used for blurred corner_sample
    float radius_tl;
    float radius_tr;
    float radius_bl;
    float radius_br;
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

// Sample the pre-convolved rrect-quarter corner texture.
// `local` is the fragment position in the corner's local frame whose origin is the
// corner's outer rrect corner (arc center at (radius, radius)). Texture spans
// [-3σ, radius]² mapped to [0, 1]² UV.
float corner_sample(vec2 local, float r, float s) {
    float margin = 3.0 * s;
    float bound  = r + margin;
    vec2  uv     = (local + vec2(margin)) / bound;
    return texture(corner_tex, uv).r;
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

    float blurred;
    if (r < 0.5 || s < 1e-4) {
        // Rect-only path (iter 4).
        blurred = separable_alpha(p, s);
    } else {
        // Corner-zone dispatch. Only rounded corners (>= 0.5 px) sample the
        // pre-convolved corner texture; square corners fall through to the
        // separable gaussian so they render a straight rect-edge blur rather
        // than a rounded one. The corner texture is shared and sized for the
        // MAX radius, so rounded corners smaller than `r` pick up an
        // approximation — acceptable for typical asymmetric shapes.
        bool in_tl = radius_tl >= 0.5
                     && p.x < radius_tl && p.y < radius_tl;
        bool in_tr = radius_tr >= 0.5
                     && p.x > rect_size.x - radius_tr && p.y < radius_tr;
        bool in_bl = radius_bl >= 0.5
                     && p.x < radius_bl && p.y > rect_size.y - radius_bl;
        bool in_br = radius_br >= 0.5
                     && p.x > rect_size.x - radius_br
                     && p.y > rect_size.y - radius_br;
        if (in_tl) {
            blurred = corner_sample(p, r, s);
        } else if (in_tr) {
            blurred = corner_sample(vec2(rect_size.x - p.x, p.y), r, s);
        } else if (in_bl) {
            blurred = corner_sample(vec2(p.x, rect_size.y - p.y), r, s);
        } else if (in_br) {
            blurred = corner_sample(vec2(rect_size.x - p.x, rect_size.y - p.y), r, s);
        } else {
            blurred = separable_alpha(p, s);
        }
    }

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

    float alpha;
    if (style == 0)      alpha = blurred;                   // Normal
    else if (style == 1) alpha = max(inside, blurred);      // Solid
    else if (style == 2) alpha = blurred * (1.0 - inside);  // Outer
    else                 alpha = blurred * inside;          // Inner

    fragColor = v_color * (alpha * qt_Opacity);
}
