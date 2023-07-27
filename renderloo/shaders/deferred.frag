#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"

layout(binding = 0) uniform sampler2D GBufferPosition;
// base color(3) + unused(1)
layout(binding = 1) uniform sampler2D GBufferA;
// metallic(1) + padding(2) + occlusion(1)
layout(binding = 2) uniform sampler2D GBufferB;
// normal(3) + roughness(1)
layout(binding = 3) uniform sampler2D GBufferC;
layout(binding = 4) uniform sampler2D MainLightShadowMap;
layout(binding = 5) uniform samplerCube DiffuseConvolved;

uniform mat4 mainLightMatrix;
uniform vec3 cameraPosition;

in vec2 texCoord;
layout(location = 0) out vec3 DiffuseResult;
layout(location = 1) out vec3 SpecularResult;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

void main() {
    vec3 positionWS;
    vec3 normalWS;
    // diffuse(rgb) + specular(a)
    vec3 baseColor;
    float metallic;
    float occlusion;
    float roughness;
    positionWS = texture(GBufferPosition, texCoord).xyz;
    normalWS = texture(GBufferC, texCoord).xyz;
    baseColor = texture(GBufferA, texCoord).rgb;
    metallic = texture(GBufferB, texCoord).r;
    roughness = texture(GBufferC, texCoord).a;
    occlusion = texture(GBufferB, texCoord).a;

    vec3 V = normalize(cameraPosition - positionWS);
    vec3 diffuse = vec3(0.0), specular = vec3(0.0);
    SurfaceParamsPBRMetallicRoughness surface;
    surface.viewDirection = V;
    surface.normal = normalize(normalWS);
    surface.baseColor = baseColor;
    surface.metallic = metallic;
    surface.roughness = roughness;
    for (int i = 0; i < nLights; i++) {
        ShaderLight light = lights[i];
        float distance = 1.0;
        vec3 L, H;
        switch (light.type) {
            case LIGHT_TYPE_DIRECTIONAL:
                L = normalize(-lights[0].direction.xyz);
                break;
            default:
                continue;
        }
        float intensity = light.intensity / (distance * distance);
        float shadow =
            computeShadow(mainLightMatrix, MainLightShadowMap, positionWS) *
            0.0;
        vec3 diff, spec;
        computePBRMetallicRoughnessLocalLighting(surface, light, V, L,
                                                 intensity, diff, spec);
        diffuse += diff * (1.0 - shadow);
        specular += spec * (1.0 - shadow);
    }
    // compute environment lighting
    vec3 envDiffuse;
    computePBRMetallicRoughnessIBL(surface, DiffuseConvolved, V, envDiffuse);
    DiffuseResult = envDiffuse;
    SpecularResult = vec3(0.0);
}