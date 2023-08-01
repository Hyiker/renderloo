#ifndef RENDERLOO_INCLUDE_CORE_DEFERRED_HPP
#define RENDERLOO_INCLUDE_CORE_DEFERRED_HPP
#include <loo/Framebuffer.hpp>
#include <loo/Texture.hpp>
#include <memory>
struct GBuffer {
    std::unique_ptr<loo::Texture2D> position;
    // base color(3) + unused(1)
    std::unique_ptr<loo::Texture2D> bufferA;
    // metallic(1) + padding(2) + occlusion(1)
    std::unique_ptr<loo::Texture2D> bufferB;
    // normal(3) + roughness(1)
    std::unique_ptr<loo::Texture2D> bufferC;
    // emissive(3) + unused(1)
    std::unique_ptr<loo::Texture2D> bufferD;
    loo::Renderbuffer depthStencilRb;

    void init(int width, int height);
};
#endif /* RENDERLOO_INCLUDE_CORE_DEFERRED_HPP */
