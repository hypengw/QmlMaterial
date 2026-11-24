#version 440

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
#include "sdf.glsl"

layout(location = 0) out vec4 fragColor;

layout(location = 0) noperspective in vec3 out_shadow_params;
layout(location = 1) noperspective in vec4 out_color;
layout(location = 2) in vec2 out_pos;
layout(location = 3) flat in vec2 out_occluder_size;
layout(location = 4) flat in vec4 out_occluder_radii;
layout(binding = 1) uniform sampler2D strength_tex;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
};

void main() {
    // Take abs before length so all four corners can share the same offset
    // values without per-corner sign correction on the CPU side.
    float d      = length(abs(out_shadow_params.xy));
    vec2  uv     = vec2(out_shadow_params.z * (1.0 - d), 0.5);
    float factor = texture(strength_tex, uv, -0.475).x;
    float outside = 1.0;
    if (out_occluder_size.x > 0.0) {
        vec2 halfSize = out_occluder_size * 0.5;
        float sdf = sdf_rounded_rectangle(out_pos - halfSize, halfSize, out_occluder_radii.wyzx);
        outside = clamp(sdf, 0.0, 1.0);
    }
    fragColor = vec4(out_color.rgb * out_color.a, out_color.a) * (factor * outside * qt_Opacity);
}
