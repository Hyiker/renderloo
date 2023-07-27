#version 460 core

out vec4 FragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D defferedTexture;
layout(binding = 4) uniform sampler2D skyboxTexture;
layout(binding = 5) uniform sampler2D GBuffer3;

vec3 gammaCorrection(in vec3 color) {
    const float gamma = 2.2;
    return pow(color, vec3(1.0 / gamma));
}

vec3 ACESToneMapping(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 deffered = texture(defferedTexture, texCoord).rgb;
    vec3 skyboxTexture = texture(skyboxTexture, texCoord).rgb;
    vec3 color = deffered + skyboxTexture;
    color = ACESToneMapping(color);
    color = gammaCorrection(color);
    FragColor = vec4(color, 1.0);
}