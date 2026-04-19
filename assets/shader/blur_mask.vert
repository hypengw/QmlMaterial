#version 440

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_color;

layout(location = 0) noperspective out vec2 v_pos;
layout(location = 1) noperspective out vec4 v_color;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    float sigma;
    vec2  rect_size;
    int   style;
};

void main() {
    v_pos       = in_position;
    v_color     = in_color;
    gl_Position = qt_Matrix * vec4(in_position, 0.0, 1.0);
}
