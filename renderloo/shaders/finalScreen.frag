#version 460 core

out vec4 FragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D specularTexture;
layout(binding = 4) uniform sampler2D skyboxTexture;
layout(binding = 5) uniform sampler2D GBuffer3;

vec3 gammaCorrection(in vec3 color) {
    const float gamma = 2.2;
    return pow(color, vec3(1.0 / gamma));
}

void main() {
    vec3 diffuse = texture(diffuseTexture, texCoord).rgb;
    vec3 specular = texture(specularTexture, texCoord).rgb;
    vec3 skyboxTexture = texture(skyboxTexture, texCoord).rgb;
    vec3 color = specular + diffuse;
    color += skyboxTexture;
    color = gammaCorrection(color);
    FragColor = vec4(color, 1.0);
}