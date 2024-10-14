#version 440

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D qt_Texture0;
layout(location = 0) noperspective in vec3 out_shadow_params;
layout(location = 1) noperspective in vec4 out_color;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
};

void main() {
    float d      = length(out_shadow_params.xy);
    vec2  uv     = vec2(out_shadow_params.z * (1.0 - d), 0.5);
    float factor = texture(qt_Texture0, uv, -0.475).x;
    vec4  alpha  = vec4(factor) * qt_Opacity;
    fragColor    = out_color * alpha;
}
