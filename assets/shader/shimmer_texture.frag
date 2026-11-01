#version 440
#extension GL_GOOGLE_include_directive : enable
#include "shimmer.glsl"
layout(location = 0) in vec2 outUv;
layout(location = 1) in vec2 outWave;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float phase;
    float minimumOpacity;
    float maximumOpacity;
};
layout(binding = 1) uniform sampler2D source;
void main() {
    fragColor = texture(source, outUv) * (qt_Opacity * shimmerAlpha(outWave, phase, minimumOpacity, maximumOpacity));
}
