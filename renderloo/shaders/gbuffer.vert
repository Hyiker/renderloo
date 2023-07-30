#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/constants.glsl"

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aWeights;

layout(location = 0) out vec3 vPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out vec3 vTangent;
layout(location = 4) out vec3 vBitangent;

layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
};

layout(std140, binding = 2) uniform BoneMatrices {
    mat4 bones[BONES_MAX_COUNT];
};

void main() {
    mat3 model3 = mat3(model);
    vTexCoord = aTexCoord;

    int influenceCount = 0;
    mat4 boneMatrix = mat4(0.0);
    for (int i = 0; i < BONES_MAX_INFLUENCE; i++) {
        if (aBoneIDs[i] == -1)
            continue;
        if (aBoneIDs[i] >= BONES_MAX_COUNT) {
            break;
        }
        influenceCount++;
        // compute the position in bone space
        boneMatrix += aWeights[i] * bones[aBoneIDs[i]];
    }
    if (influenceCount == 0) {
        boneMatrix = mat4(1.0);
    }
    vec4 normalWS = vec4(aNormal, 0.0);
    normalWS = boneMatrix * normalWS;
    vNormal = normalize((normalMatrix * normalWS).xyz);
    vPos = (model * boneMatrix * vec4(aPos, 1.0)).xyz;
    vTangent = normalize((model * boneMatrix * vec4(aTangent, 0.0)).xyz);
    vBitangent = normalize((model * boneMatrix * vec4(aBitangent, 0.0)).xyz);
    gl_Position = projection * view * vec4(vPos, 1.0);
}