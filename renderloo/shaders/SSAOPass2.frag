#version 460 core
layout(location = 0) out float FragColor;

layout(location = 0) in vec2 texCoords;

layout(binding = 0) uniform sampler2D SSAOResult;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(SSAOResult, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(SSAOResult, texCoords + offset).r;
        }
    }
    FragColor = result / (4.0 * 4.0);
}