#version 440

layout(location = 0) out vec4 fragColor;
layout(location = 0) noperspective in vec4 out_circle_edge;
layout(location = 1) noperspective in vec4 out_color;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
};

void main() {
    // d is zero, if not in round edge
    float d                 = length(out_circle_edge.xy);
    float distance_to_outer = out_circle_edge.z * (1.0 - d);
    float edge_alpha        = clamp(distance_to_outer, 0.0, 1.0);
    vec4  final_alpha       = vec4(edge_alpha) * qt_Opacity;
    fragColor               = out_color * final_alpha;
}