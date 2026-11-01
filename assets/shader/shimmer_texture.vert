#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec2 wave;
layout(location = 0) out vec2 outUv;
layout(location = 1) out vec2 outWave;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float phase;
    float minimumOpacity;
    float maximumOpacity;
};
void main() {
    outUv = uv;
    outWave = wave;
    gl_Position = qt_Matrix * vec4(position, 0.0, 1.0);
}
