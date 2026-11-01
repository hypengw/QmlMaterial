#ifndef QM_SHIMMER_GLSL
#define QM_SHIMMER_GLSL
float shimmerAlpha(vec2 wave, float phase, float minimum, float maximum) {
    float t = clamp(wave.x - (phase - 0.5) * wave.y + 0.5, 0.0, 1.0);
    return mix(minimum, maximum, 1.0 - abs(2.0 * t - 1.0));
}
#endif
