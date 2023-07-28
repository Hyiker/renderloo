#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec2 pixelCoord;
layout(location = 2) out vec4 offset[3];

layout(location = 0) uniform vec4 rt_metrics;

#extension GL_GOOGLE_include_directive : enable
#define SMAA_GLSL_4
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
#define SMAA_RT_METRICS rt_metrics
#include "SMAA.hlsl"

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    texCoord = aTexCoord;
    SMAABlendingWeightCalculationVS(texCoord, pixelCoord, offset);
}