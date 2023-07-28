#version 460 core

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 pixelCoord;
layout(location = 2) in vec4 offset[3];

layout(location = 0) out vec4 FragBlendingWeight;

layout(location = 0) uniform vec4 rt_metrics;
layout(location = 1, binding = 0) uniform sampler2D edgeTex;
layout(location = 2, binding = 1) uniform sampler2D areaTex;
layout(location = 3, binding = 2) uniform sampler2D searchTex;

#extension GL_GOOGLE_include_directive : enable
#define SMAA_GLSL_4
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#define SMAA_RT_METRICS rt_metrics
#include "SMAA.hlsl"

void main() {
    FragBlendingWeight = SMAABlendingWeightCalculationPS(
        texCoord, pixelCoord, offset, edgeTex, areaTex, searchTex, vec4(0.0));
}