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
    const loo::Texture2D& render(const loo::Texture2D& input);
    int setBloomRange(int range);
    int getMaxBloomRange() const;
    int getBloomRange() const { return m_mipLevelDownSample; }
    float brightnessThreshold = 1.0f;

   private:
    int m_width, m_height;
    // namely, the bloom range
    int m_mipLevelDownSample{6};
    loo::ComputeShader m_pixelPickerShader, m_downSamplingShader,
        m_upSamplingShader, m_additiveBlendingShader;
    std::unique_ptr<loo::Texture2D> m_downSample, m_upSample, m_result;
};

#endif /* RENDERLOO_INCLUDE_PASSES_BLOOM_PASS_HPP */
