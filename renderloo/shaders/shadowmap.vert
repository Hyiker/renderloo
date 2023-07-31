#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/constants.glsl"

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoord;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aWeights;

layout(location = 0) out vec2 vTexCoord;

layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
};
layout(std140, binding = 2) uniform BoneMatrices {
    mat4 bones[BONES_MAX_COUNT];
};
layout(location = 0) uniform mat4 lightSpaceMatrix;
void main() {
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
    vec3 vPos = (model * boneMatrix * vec4(aPos, 1.0)).xyz;
    vTexCoord = aTexCoord;
    gl_Position = lightSpaceMatrix * vec4(vPos, 1.0);
}