#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"
#include "include/sampling.glsl"
layout(location = 0) in vec3 localPos;
layout(location = 0) out vec3 FragColor;

layout(binding = 0, location = 3) uniform samplerCube envMap;
layout(location = 4) uniform float roughness;
const uint N_SAMPLES = 1024;
#ifndef PI
#define PI 3.14159265358979323846
#define PI_INV 0.31830988618379067154
#endif
void main() {
    vec3 N = normalize(localPos);
    vec3 T = vec3(0.0, 1.0, 0.0);
    if (abs(N.x) < 1e-4 && abs(N.z) < 1e-4) {
        T = vec3(1.0, 0.0, 0.0);
    }
    T = normalize(cross(T, N));
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    dvec3 irradiance = vec3(0.0);
    double weightSum = 0.0;
    float envMapSize = textureSize(envMap, 0).x;
    float solidAngleTexel = 4.0 * PI / (6.0 * envMapSize * envMapSize);
    for (int i = 0; i < N_SAMPLES; i++) {
        vec2 Xi = Hammersley(i, N_SAMPLES);
        vec3 Hlocal = ImportanceSampleGGXHalfVec(Xi, roughness);
        vec3 H = normalize(TBN * Hlocal);
        float NdotH = max(dot(N, H), 0.0);

        float pdf = GGXHalfVecPdf(NdotH, NdotH, roughness);
        float solidAngleSample = 1.0 / (float(N_SAMPLES) * pdf + 1e-4);
        float mipLevel = roughness == 0.0
                             ? 0.0
                             : 0.5 * log2(solidAngleSample / solidAngleTexel);

        vec3 wi = reflect(-N, H);
        float weight = max(dot(wi, N), 0.0);
        weightSum += weight;
        irradiance += textureLod(envMap, wi, mipLevel).rgb * weight;
    }
    if (weightSum > 0.0)
        irradiance /= weightSum;
    FragColor = vec3(irradiance);
}