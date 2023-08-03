#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "include/camera.glsl"
#include "include/renderInfo.glsl"
#include "include/sampling.glsl"

in vec2 texCoord;

out float FragAO;

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferDepth;
layout(binding = 3) uniform sampler2D NoiseTex;

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

/**
* SSAO overview:
* 1. Sample random points in a hemisphere around the fragment(view space)
* 2. Fetch the linearized depth value of the sampled points(NDC space)
* 3. Compare the depth value of the sampled point with the depth value of the fragment,
*    if the sampled point is closer to the camera, then add occlusion
* 4. Repeat 1-3 for multiple samples and average the result
* 5. Output the occlusion value
*/

void main() {
    const vec2 noiseScale = framebufferSize / SSAO_NOISE_SIZE;
    vec4 positionWS = vec4(texture(GBufferPosition, texCoord).xyz, 1.0);
    vec4 positionView = view * positionWS;
    // compute linearized fragment depth
    float fragDepthLinear01 = texture(GBufferDepth, texCoord).r;
    vec2 cameraPlanes = extractZParamFromProjection(projection);
    fragDepthLinear01 =
        linear01Depth(fragDepthLinear01, cameraPlanes.x, cameraPlanes.y);

    vec3 normal = normalize(
        (view * vec4(normalize(texture(GBufferNormal, texCoord).xyz), 0.0))
            .xyz);
    vec3 randomVec = texture(NoiseTex, texCoord * noiseScale).xyz;
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    mat4 jitteredProjection = projection;
    if (_RenderInfo.enableTAA != 0) {
        vec2 jitter = Halton_2_3[_RenderInfo.frameCount % 8] /
                      vec2(_RenderInfo.deviceSize) * 0.75;
        jitteredProjection[2][0] += jitter.x;
        jitteredProjection[2][1] += jitter.y;
    }
    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        // sample a position in view space
        vec3 samplePosVS = TBN * kernelSamples[i];
        samplePosVS = positionView.xyz + samplePosVS * SSAO_RADIUS;
        vec4 offset = vec4(samplePosVS, 1.0);
        // use uv coordinates of sampled position to sample the depth texture
        offset = jitteredProjection * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        // sample the depth value of the sampled position
        float offsetDepthLinear01 = linear01Depth(
            texture(GBufferDepth, offset.xy).r, cameraPlanes.x, cameraPlanes.y);
        float rangeCheck = smoothstep(
            0.0, 1.0,
            SSAO_RADIUS / abs(offsetDepthLinear01 - fragDepthLinear01));
        occlusion +=
            ((offsetDepthLinear01 + SSAO_BIAS < fragDepthLinear01) ? 1.0
                                                                   : 0.0) *
            rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
    FragAO = occlusion;
}