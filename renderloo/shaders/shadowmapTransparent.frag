#version 460

layout(location = 0) in vec2 vTexCoord;

layout(location = 1) uniform float alphaTestThreshold;

layout(std140, binding = 3) uniform PBRMetallicMaterial {
    vec4 baseColor;
    // metallic(1) + roughness(1)
    vec4 metallicRoughness;
    // emissive(3) + padding(1)
    vec4 emissive;
}
material;

layout(binding = 10, location = 2) uniform sampler2D baseColorTex;

void main() {
    float alpha = texture(baseColorTex, vTexCoord).a * material.baseColor.a;
    if (alpha < alphaTestThreshold) {
        discard;
    }
}