#version 460 core
#extension GL_GOOGLE_include_directive : enable

#define REVERSE_Z
#include "include/lighting.glsl"

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
// tangent space -> world space
layout(location = 3) in vec3 vTangent;
layout(location = 4) in vec3 vBitangent;

layout(location = 0) out vec4 FragResult;

layout(std140, binding = 3) uniform PBRMetallicMaterial {
    vec4 baseColor;
    // roughness(1) + padding(3)
    vec4 metallicRoughness;
    // emissive(3) + padding(1)
    vec4 emissive;
}
material;
layout(binding = 10) uniform sampler2D baseColorTex;
layout(binding = 11) uniform sampler2D occlusionTex;
layout(binding = 12) uniform sampler2D metallicTex;
layout(binding = 13) uniform sampler2D roughnessTex;
layout(binding = 14) uniform sampler2D emissiveTex;

layout(binding = 7) uniform sampler2D normalTex;

layout(binding = 20) uniform samplerCube DiffuseConvolved;
layout(binding = 21) uniform samplerCube SpecularConvolved;
layout(binding = 22) uniform sampler2D BRDFLUT;
layout(binding = 23) uniform sampler2D MainLightShadowMap;

uniform vec3 cameraPosition;
uniform mat4 mainLightMatrix;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

void main() {

    vec3 color = vec3(0);
    mat3 TBN =
        mat3(normalize(vTangent), normalize(vBitangent), normalize(vNormal));
    vec2 texCoord = vTexCoord;

    // shading normal
    vec3 sNormal = texture(normalTex, texCoord).rgb;
    sNormal = length(sNormal) == 0.0 ? vNormal : (TBN * (sNormal * 2.0 - 1.0));
    sNormal = normalize(sNormal);
    vec4 baseColor4 = texture(baseColorTex, texCoord).rgba;
    vec3 baseColor = baseColor4.rgb * material.baseColor.rgb;
    vec3 emissive = texture(emissiveTex, texCoord).rgb * material.emissive.rgb;
    float metallic =
        texture(metallicTex, texCoord).b * material.metallicRoughness.r;
    float roughness =
        texture(roughnessTex, texCoord).g * material.metallicRoughness.g;
    float alpha = baseColor4.a * material.baseColor.a;
    if (alpha == 0.0)
        discard;

    vec3 V = normalize(cameraPosition - vPos);
    SurfaceParamsPBRMetallicRoughness surface;
    surface.viewDirection = V;
    surface.normal = normalize(sNormal);
    surface.baseColor = baseColor;
    surface.metallic = metallic;
    surface.roughness = roughness;

    vec3 diffuse = vec3(0.0), specular = vec3(0.0);

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
        float shadow = computeShadow(mainLightMatrix, MainLightShadowMap, vPos);
        vec3 diff, spec;
        computePBRMetallicRoughnessLocalLighting(surface, light, V, L,
                                                 intensity, diff, spec);
        diffuse += diff * (1.0 - shadow);
        specular += spec * (1.0 - shadow);
    }
    vec3 envDiffuse, envSpecular;
    envDiffuse =
        computePBRMetallicRoughnessIBLDiffuse(surface, DiffuseConvolved, V);
    envSpecular = computePBRMetallicRoughnessIBLSpecular(
        surface, SpecularConvolved, BRDFLUT, V);
    FragResult =
        vec4(diffuse + envDiffuse + specular + envSpecular + emissive, alpha);
}