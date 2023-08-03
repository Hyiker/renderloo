#ifndef RENDERLOO_SHADERS_INCLUDE_RENDER_INFO_GLSL
#define RENDERLOO_SHADERS_INCLUDE_RENDER_INFO_GLSL

layout(std140, binding = 6) uniform RI {
    ivec2 deviceSize;
    uint frameCount;
    float timeSecs;
    int enableTAA;
    ivec3 _pad0;
}
_RenderInfo;

#endif /* RENDERLOO_SHADERS_INCLUDE_RENDER_INFO_GLSL */
