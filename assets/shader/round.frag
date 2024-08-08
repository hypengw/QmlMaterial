#version 440

#extension GL_GOOGLE_include_directive : enable

#include "sdf.glsl"

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    // tl,tr,bl,br
    vec4 radius_;
    vec2 size;
    // vec2  height;
    float smoothing;
};
layout(binding = 1) uniform sampler2D source;

void main() {
    vec2 p = qt_TexCoord0 - vec2(0.5);
    p *= size;
    float sdf = sdf_rounded_rectangle(p, size / 2.0, radius_);

    // fragColor = sdf_render(sdf, vec4(0.0), texture(source, qt_TexCoord0), 1.0, smoothing, -1.0);
    fragColor =
        sdf_render_uv(sdf, p, vec4(0.0), texture(source, qt_TexCoord0), 1.0, smoothing, -1.0);
    // fragColor = texture(source, qt_TexCoord0);
    // fragColor.a = sdf_alpha_uv(sdf, p, smoothing, -1.0);
}