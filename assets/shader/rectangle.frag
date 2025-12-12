#version 410
#extension GL_ARB_shading_language_420pack : enable

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
#include "sdf.glsl"

layout(location = 0) out vec4 fragColor;
layout(location = 0) noperspective in vec4 out_circle_edge;
layout(location = 1) noperspective in vec4 out_color;
layout(location = 2) noperspective in vec2 out_uv;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    vec4  radius; // tl,tr,bl,br
    vec2  size;
};

void main() {
    vec2 p = (out_uv - vec2(0.5)) * size;
    vec4 r = vec4(radius.w, radius.y, radius.z, radius.x); // br,tr,bl,tl
    float sdf = sdf_rounded_rectangle(p, size * 0.5, r);
    float aa = fwidth(sdf);
    float edge_alpha = smoothstep(aa, -aa, sdf);

    float alpha = edge_alpha * qt_Opacity * out_color.a;
    fragColor = vec4(out_color.rgb * alpha, alpha);
}
