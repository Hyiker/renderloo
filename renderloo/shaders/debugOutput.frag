#version 460 core

out vec4 FragColor;
in vec2 texCoord;

// base color(3) + unused(1)
layout(binding = 0) uniform sampler2D GBufferA;
// metallic(1) + padding(2) + occlusion(1)
layout(binding = 1) uniform sampler2D GBufferB;
// normal(3) + roughness(1)
layout(binding = 2) uniform sampler2D GBufferC;
// emissive(3) + unused(1)
layout(binding = 3) uniform sampler2D GBufferD;
// ambient occlusion(1)
layout(binding = 4) uniform sampler2D AmbientOcclusion;

uniform int mode;

const int MODE_BASE_COLOR = 1;
const int MODE_METALNESS = 2;
const int MODE_ROUGHNESS = 3;
const int MODE_NORMAL = 4;
const int MODE_EMISSION = 5;
const int MODE_AMBIENT_OCCLUSION = 6;

void main() {
    vec3 color = vec3(0.0);
    switch (mode) {
        case MODE_BASE_COLOR:
            color = texture(GBufferA, texCoord).rgb;
            break;
        case MODE_METALNESS:
            color = vec3(texture(GBufferB, texCoord).r);
            break;
        case MODE_ROUGHNESS:
            color = vec3(texture(GBufferC, texCoord).a);
            break;
        case MODE_NORMAL:
            color = texture(GBufferC, texCoord).rgb;
            break;
        case MODE_EMISSION:
            color = texture(GBufferD, texCoord).rgb;
            break;
        case MODE_AMBIENT_OCCLUSION:
            color = vec3(texture(AmbientOcclusion, texCoord).r);
            break;
    }
    FragColor = vec4(color, 1.0);
}