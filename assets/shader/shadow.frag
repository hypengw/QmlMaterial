#version 410
#extension GL_ARB_shading_language_420pack : enable

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
#include "sdf.glsl"

layout(location = 0) out vec4 fragColor;

layout(location = 0) noperspective in vec2 out_pos;
layout(location = 1) noperspective in vec4 out_color;
layout(location = 2) flat in int out_op;
layout(binding = 1) uniform sampler2D strength_tex;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    vec4  innerRect[2];
    vec4  radius[2]; // tl,tr,bl,br
    vec2  blur[2];
};

void main() {
    int idx = clamp(out_op, 0, 1);

    // Occluder mask (always based on the real item geometry, i.e. op 0).
    vec4 occ = innerRect[0];
    vec2 occSize = occ.zw;
    vec2 occCenter = occ.xy + occSize * 0.5;
    vec2 pOcc = out_pos - occCenter;
    vec4 occR = vec4(radius[0].w, radius[0].y, radius[0].z, radius[0].x); // br,tr,bl,tl
    float sdfOcc = sdf_rounded_rectangle(pOcc, occSize * 0.5, occR);
    float aaOcc = fwidth(sdfOcc);
    float occMask = smoothstep(-aaOcc, 0.0, sdfOcc);

    vec4 inner = innerRect[idx];
    vec2 size  = inner.zw;
    vec2 center = inner.xy + size * 0.5;
    vec2 p = out_pos - center;

    // br,tr,bl,tl for sdf helper
    vec4 r = vec4(radius[idx].w, radius[idx].y, radius[idx].z, radius[idx].x);
    float sdf = sdf_rounded_rectangle(p, size * 0.5, r);
    float aa = fwidth(sdf);
    float outside = max(sdf, 0.0);

    float blurW = max(blur[idx].x, 1e-3);
    float t_raw = outside / blurW;
    float t = clamp(t_raw, 0.0, 1.0);
    float factor_tex = texture(strength_tex, vec2(1.0 - t, 0.5), -0.475).x;
    // Analytic equivalent of strength_tex to avoid 8-bit banding (esp. spot layer).
    float factor_ana = exp(-4.0 * t * t) - 0.018;
    factor_ana = clamp(factor_ana, 0.0, 1.0);
    float factor = mix(factor_tex, factor_ana, float(idx));

    // Smoothly remove any shadow inside the real occluder edge.
    factor *= occMask;
    // Smoothly force factor to 0 past the penumbra border to avoid boxy residuals.
    float outerAA = max(aa / blurW, 1e-3);
    float outerMask = 1.0 - smoothstep(1.0 - outerAA, 1.0 + outerAA, t_raw);
    factor *= outerMask;

    float alpha  = factor * qt_Opacity * out_color.a;
    fragColor    = vec4(out_color.rgb * alpha, alpha);
}
