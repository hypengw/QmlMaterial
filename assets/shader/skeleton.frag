#version 440
#extension GL_GOOGLE_include_directive : enable
#include "shimmer.glsl"
layout(location = 0) in vec4 outShape;
layout(location = 1) in vec4 outColor;
layout(location = 2) in vec3 outWave;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float phase;
    float minimumOpacity;
    float maximumOpacity;
};
void main() {
    vec2 q = abs(outShape.xy) - outShape.zw + outWave.z;
    float d = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - outWave.z;
    float coverage = clamp(0.5 - d / max(fwidth(d), 0.0001), 0.0, 1.0);
    fragColor = outColor * (coverage * qt_Opacity * shimmerAlpha(outWave.xy, phase, minimumOpacity, maximumOpacity));
}
