#version 460 core
layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 vTexCoord;

layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
}
mvp;

void main() {
    vTexCoord = aPos;
    mat4 viewMod = mat4(mat3(mvp.view));
    gl_Position = (mvp.projection * viewMod * vec4(aPos, 1.0)).xyww;
    gl_Position.z = 0.0;
}