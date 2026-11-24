#version 440

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec3 in_shadow_params;
layout(location = 3) in vec4 in_occluder;
layout(location = 4) in vec4 in_occluder_radii;

layout(location = 0) noperspective out vec3 out_shadow_params;
layout(location = 1) noperspective out vec4 out_color;
layout(location = 2) out vec2 out_pos;
layout(location = 3) flat out vec2 out_occluder_size;
layout(location = 4) flat out vec4 out_occluder_radii;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
};

void main() {
    out_shadow_params      = in_shadow_params;
    out_color              = in_color;
    out_pos                = in_occluder.xy;
    out_occluder_size      = in_occluder.zw;
    out_occluder_radii     = in_occluder_radii;
    gl_Position            = qt_Matrix * vec4(in_position, 0.0, 1.0);
}
