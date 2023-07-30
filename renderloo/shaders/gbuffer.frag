#version 460 core
#extension GL_GOOGLE_include_directive : enable

layout(early_fragment_tests) in;

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
// tangent space -> world space
layout(location = 3) in vec3 vTangent;
layout(location = 4) in vec3 vBitangent;

layout(location = 0) out vec4 FragPosition;
// base color(3) + unused(1)
layout(location = 1) out vec4 GBufferA;
// metallic(1) + padding(2) + occlusion(1)
layout(location = 2) out vec4 GBufferB;
// normal(3) + roughness(1)
layout(location = 3) out vec4 GBufferC;

uniform vec3 uCameraPosition;
uniform bool enableNormal;
uniform bool enableParallax;
#ifdef MATERIAL_PBR
layout(std140, binding = 3) uniform PBRMetallicMaterial {
    vec4 baseColor;
    // roughness(1) + padding(3)
    vec4 metallicRoughness;
}
material;
layout(binding = 10) uniform sampler2D baseColorTex;
layout(binding = 11) uniform sampler2D occlusionTex;
layout(binding = 12) uniform sampler2D metallicTex;
layout(binding = 13) uniform sampler2D roughnessTex;

void GBufferFromPBRMaterial(in vec2 texCoord, in sampler2D baseColorTex,
                            in sampler2D occlusionTex, in sampler2D metallicTex,
                            in sampler2D roughnessTex, in vec3 baseColor,
                            in float metallic, in float roughness,
                            inout vec4 gbufferA, inout vec4 gbufferB,
                            inout vec4 gbufferC) {
    gbufferA.rgb = texture(baseColorTex, texCoord).rgb * baseColor;
    gbufferB.r = texture(metallicTex, texCoord).b * metallic;
    gbufferB.gb = texCoord;
    gbufferB.a = texture(occlusionTex, texCoord).r;
    gbufferC.a = texture(roughnessTex, texCoord).g * roughness;
}
#else
layout(std140, binding = 2) uniform SimpleMaterial {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec4 transparentIOR;
    float shininess;
}
material;
layout(binding = 3) uniform sampler2D ambientTex;
layout(binding = 4) uniform sampler2D diffuseTex;
layout(binding = 5) uniform sampler2D specularTex;
layout(binding = 6) uniform sampler2D displacementTex;
layout(binding = 8) uniform sampler2D opacityTex;
layout(binding = 9) uniform sampler2D heightTex;

void GBufferFromSimpleMaterial(in vec2 texCoord, in sampler2D diffuseTex,
                               in sampler2D specularTex, in vec3 diffuse,
                               in vec3 specular, in vec4 transparentIOR,
                               out vec4 GBufferAlbedo,
                               out vec4 GBufferTransparentIOR) {
    GBufferAlbedo.rgb = texture(diffuseTex, texCoord).rgb * diffuse.rgb;
    GBufferAlbedo.a = texture(specularTex, texCoord).a * specular.r;
    GBufferTransparentIOR.rgba = transparentIOR.rgba;
}

#endif

layout(binding = 7) uniform sampler2D normalTex;

// vec2 parallaxMapping(vec2 texCoord, vec3 viewTS) {
//     float height = texture(heightTex, texCoord).r;
//     vec2 p = viewTS.xy / viewTS.z * (height * 0.2);
//     return texCoord + p;
// }

void main() {

    vec3 color = vec3(0);
    mat3 TBN =
        mat3(normalize(vTangent), normalize(vBitangent), normalize(vNormal));
    vec2 texCoord = vTexCoord;

    // shading normal
    vec3 sNormal = texture(normalTex, texCoord).rgb;
    sNormal = length(sNormal) == 0.0 ? vNormal : (TBN * (sNormal * 2.0 - 1.0));
    sNormal = normalize(enableNormal ? sNormal : vNormal);
    FragPosition = vec4(vPos, 1);
    GBufferC.rgb = sNormal;
#ifdef MATERIAL_PBR
    GBufferFromPBRMaterial(
        texCoord, baseColorTex, occlusionTex, metallicTex, roughnessTex,
        material.baseColor.rgb, material.metallicRoughness.r,
        material.metallicRoughness.g, GBufferA, GBufferB, GBufferC);
#else
    GBufferFromSimpleMaterial(texCoord, diffuseTex, specularTex,
                              material.diffuse.rgb, material.specular.rgb,
                              material.transparentIOR, FragAlbedo, GBuffer3);
#endif
}