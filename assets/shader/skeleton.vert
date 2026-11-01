#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 shape;
layout(location = 2) in vec4 color;
layout(location = 3) in vec3 wave;
layout(location = 0) out vec4 outShape;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec3 outWave;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float phase;
    float minimumOpacity;
    float maximumOpacity;
};
void main() {
    outShape = shape;
    outColor = color;
    outWave = wave;
    gl_Position = qt_Matrix * vec4(position, 0.0, 1.0);
}
