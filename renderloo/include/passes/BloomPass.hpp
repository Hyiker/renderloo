#ifndef RENDERLOO_INCLUDE_PASSES_BLOOM_PASS_HPP
#define RENDERLOO_INCLUDE_PASSES_BLOOM_PASS_HPP

#include <loo/Application.hpp>
#include <loo/ComputeShader.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include "core/Light.hpp"
#include "core/Skybox.hpp"
class BloomPass {
   public:
    BloomPass();
    void init(int width, int height);
    void render(const loo::Texture2D& inout, const loo::Texture2D& GBufferD);
    float brightnessThreshold = 1.0f;

   private:
    int m_width, m_height, m_mipLevel;
    loo::Framebuffer m_fb;
    loo::ComputeShader m_pixelPicker, m_chainedGaussianBlur;
    loo::ShaderProgram m_additiveBlending;
    std::unique_ptr<loo::Texture2D> m_pickedPixel;
};

#endif /* RENDERLOO_INCLUDE_PASSES_BLOOM_PASS_HPP */
