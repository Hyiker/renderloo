#version 460 core
layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 localPos;

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;

void main() {
    localPos = aPos;
    gl_Position = projection * view * vec4(localPos * 0.5, 1.0);
}