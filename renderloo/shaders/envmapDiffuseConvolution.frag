#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/sampling.glsl"
layout(location = 0) in vec3 localPos;
layout(location = 0) out vec3 FragColor;

layout(binding = 0, location = 3) uniform samplerCube envMap;
const uint N_SAMPLES = 4096;
#define PI_INV 0.31830988618379067154
void main() {
    vec3 N = normalize(localPos);
    vec3 T = vec3(0.0, 1.0, 0.0);
    if (abs(N.x) < 1e-4 && abs(N.z) < 1e-4) {
        T = vec3(1.0, 0.0, 0.0);
    }
    T = normalize(cross(T, N));
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    vec3 irradiance = vec3(0.0);
    for (int i = 0; i < N_SAMPLES; i++) {
        float pdf = 0.0;
        vec3 dirLocal = SampleHemisphereCosineWeighted(i, N_SAMPLES, pdf);
        vec3 dirWorld = TBN * dirLocal;
        irradiance += textureLod(envMap, dirWorld, 0.0).rgb * dirLocal.z / pdf;
    }
    FragColor = irradiance / float(N_SAMPLES);
}