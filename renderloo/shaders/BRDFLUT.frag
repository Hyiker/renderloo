#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"
#include "include/sampling.glsl"
// u: cosTheta, v: roughness
out vec2 ScaleBias;
in vec2 texCoord;

const int SAMPLE_COUNT = 1024;

void main() {
    float roughness = texCoord.y;
    float cosTheta = texCoord.x;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    vec3 N = vec3(0, 0, 1);
    vec3 wo = vec3(sinTheta, 0, cosTheta);
    float scale = 0;
    float bias = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGXHalfVec(Xi, roughness);
        vec3 wi = reflect(-wo, H);

        float NdotL = max(wi.z, 0);
        float HdotV = max(dot(H, wo), 0);
        float NdotH = max(H.z, 0);
        if (NdotL > 0.0) {
            float Fc = pow(1 - HdotV, 5.0);
            float G = SchlickGGXGeometryIBL(roughness, wi, wo, N);
            float T = G * HdotV / (NdotH * cosTheta);
            scale += T * (1 - Fc);
            bias += T * Fc;
        }
    }
    scale /= float(SAMPLE_COUNT);
    bias /= float(SAMPLE_COUNT);
    ScaleBias = vec2(scale, bias);
}