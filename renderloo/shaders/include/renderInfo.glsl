#ifndef RENDERLOO_SHADERS_INCLUDE_RENDER_INFO_HPP
#define RENDERLOO_SHADERS_INCLUDE_RENDER_INFO_HPP

layout(std140, binding = 6) uniform RI {
    ivec2 deviceSize;
    uint frameCount;
    float timeSecs;
    int enableTAA;
    int _pad0;
    int _pad1;
    int _pad2;
}
_RenderInfo;

#endif /* RENDERLOO_SHADERS_INCLUDE_RENDER_INFO_HPP */
