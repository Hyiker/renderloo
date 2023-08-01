#version 460 core

out vec3 FragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D bloomTexture;
uniform int mipLevels;

void main() {
    vec3 color = vec3(0.0);
    for (int i = 0; i < mipLevels; i++) {
        color += textureLod(bloomTexture, texCoord, i).rgb;
    }
    FragColor = color;
}