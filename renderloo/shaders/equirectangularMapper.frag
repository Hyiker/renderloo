#version 460 core
layout(location = 0) in vec3 localPos;
layout(location = 0) out vec4 FragColor;

layout(binding = 0, location = 3) uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = SampleSphericalMap(
        normalize(localPos));  // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, uv).rgb;
    // clamp the value to prevent NaN output
    color = clamp(color, 0.0, 1000.0);
    FragColor = vec4(color, 1.0);
}