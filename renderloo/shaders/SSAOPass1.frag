#version 460 core

in vec2 texCoord;

out float FragAO;

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D NoiseTex;

uniform vec2 framebufferSize;
uniform float radius;
uniform float bias;

#define SSAO_KERNEL_SIZE 64
#define SSAO_RADIUS radius
#define SSAO_BIAS bias
#define SSAO_NOISE_SIZE 4.0

uniform vec3 kernelSamples[SSAO_KERNEL_SIZE];
layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
};

void main() {
    const vec2 noiseScale = framebufferSize / SSAO_NOISE_SIZE;
    vec4 positionWS = vec4(texture(GBufferPosition, texCoord).xyz, 1.0);
    vec4 positionView = view * positionWS;
    vec3 normalWS = normalize(texture(GBufferNormal, texCoord).xyz);
    vec3 normal = normalize((view * vec4(normalWS, 0.0)).xyz);
    vec3 randomVec = texture(NoiseTex, texCoord * noiseScale).xyz;
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    float occlusion = 0.0;
    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        vec3 samplePos = TBN * kernelSamples[i];
        samplePos = positionView.xyz + samplePos * SSAO_RADIUS;
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        vec4 samplePositionView = view * texture(GBufferPosition, offset.xy);
        float offsetDepth = samplePositionView.z;
        float rangeCheck = smoothstep(
            0.0, 1.0, SSAO_RADIUS / abs(positionView.z - offsetDepth));
        occlusion +=
            ((offsetDepth >= samplePos.z + SSAO_BIAS) ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
    FragAO = occlusion;
}