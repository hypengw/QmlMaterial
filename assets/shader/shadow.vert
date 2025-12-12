#version 410
#extension GL_ARB_shading_language_420pack : enable

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec3 in_shadow_params;

layout(location = 0) noperspective out vec2 out_pos;
layout(location = 1) noperspective out vec4 out_color;
layout(location = 2) flat out int out_op;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    vec4  innerRect[2];
    vec4  radius[2];
    vec2  blur[2];
};

void main() {
    out_pos   = in_position;
    out_color = in_color;
    out_op    = int(in_shadow_params.x + 0.5);
    gl_Position = qt_Matrix * vec4(in_position, 0.0, 1.0);
}
