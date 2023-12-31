#include "passes/BloomPass.hpp"
#include <memory>
#include "shaders/bloomAdditiveBlending.comp.hpp"
#include "shaders/bloomPixelPicker.comp.hpp"
#include "shaders/bloomUpSampling.comp.hpp"
#include "shaders/chainedGaussianBlur.comp.hpp"
using namespace loo;
BloomPass::BloomPass()
    : m_pixelPickerShader{Shader(BLOOMPIXELPICKER_COMP, ShaderType::Compute)},
      m_downSamplingShader{
          Shader(CHAINEDGAUSSIANBLUR_COMP, ShaderType::Compute)},
      m_upSamplingShader{Shader(BLOOMUPSAMPLING_COMP, ShaderType::Compute)},
      m_additiveBlendingShader{
          Shader(BLOOMADDITIVEBLENDING_COMP, ShaderType::Compute)} {}

constexpr int GROUP_SIZE = 16;
void BloomPass::init(int width, int height) {
    // base level starting from 1/2
    width /= 2;
    height /= 2;
    m_width = width;
    m_height = height;
    setBloomRange(m_mipLevelDownSample);

    m_result = std::make_unique<loo::Texture2D>();
    m_result->init();
    m_result->setupStorage(width * 2, height * 2, GL_RGBA32F, 1);
    m_result->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_result->setWrapFilter(GL_CLAMP_TO_EDGE);
}
int BloomPass::getMaxBloomRange() const {
    return mipmapLevelFromSize(m_width, m_height);
}
int BloomPass::setBloomRange(int range) {
    int width = m_width, height = m_height;
    m_mipLevelDownSample = range;
    m_mipLevelDownSample =
        std::min(mipmapLevelFromSize(width, height), m_mipLevelDownSample);
    m_downSample = std::make_unique<loo::Texture2D>();
    m_downSample->init();
    m_downSample->setupStorage(width, height, GL_RGBA32F, m_mipLevelDownSample);
    m_downSample->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    m_downSample->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_upSample = std::make_unique<loo::Texture2D>();
    m_upSample->init();
    // upSampling level is 1 less than downSampling level
    m_upSample->setupStorage(width, height, GL_RGBA32F,
                             std::max(m_mipLevelDownSample - 1, 1));
    m_upSample->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    m_upSample->setWrapFilter(GL_CLAMP_TO_EDGE);
    return m_mipLevelDownSample;
}
const Texture2D& BloomPass::render(const Texture2D& input) {
    Application::beginEvent("Bloom Pass");
    // pick bright pixels, gaussian blur it into the mipmap level 0
    Application::beginEvent("Bloom Pass 1 - Pixel Picker");
    m_pixelPickerShader.use();
    m_pixelPickerShader.setUniform("brightnessThreshold", brightnessThreshold);
    m_pixelPickerShader.setRegularTexture(0, input);
    m_pixelPickerShader.setTexture(1, *m_downSample, 0, GL_WRITE_ONLY,
                                   GL_RGBA32F);

    m_pixelPickerShader.dispatch(m_width / GROUP_SIZE, m_height / GROUP_SIZE);
    panicPossibleGLError();
    m_pixelPickerShader.wait();
    Application::endEvent();

    // forward blur the mipmap level x - 1 to level x
    Application::beginEvent("Bloom Pass 2 - Down Sampling");
    int width = m_width, height = m_height;
    for (int i = 1; i < m_mipLevelDownSample; ++i) {
        width /= 2;
        height /= 2;
        m_downSamplingShader.use();
        m_downSamplingShader.setUniform("mipLevel", i - 1);
        m_downSamplingShader.setRegularTexture(0, *m_downSample);
        m_downSamplingShader.setTexture(1, *m_downSample, i, GL_WRITE_ONLY,
                                        GL_RGBA32F);
        m_downSamplingShader.dispatch(width / GROUP_SIZE, height / GROUP_SIZE);
        m_downSamplingShader.wait();
    }
    Application::endEvent();

    // reversely upsampling the mipmap level x to level x - 1
    // upSampleLevel[MAX] = blur(downSampleLevel[MAX + 1])
    // upSampleLevel[i] = blur(upSampleLevel[i+1]) + blur(downSampleLevel[i])
    Application::beginEvent("Bloom Pass 3 - Up Sampling");
    for (int i = m_mipLevelDownSample - 2; i >= 0; i--) {
        width *= 2;
        height *= 2;
        m_upSamplingShader.use();
        m_upSamplingShader.setUniform("level", i);
        // current downSample level
        m_upSamplingShader.setRegularTexture(0, *m_downSample);
        // previous upSample level
        if (i == m_mipLevelDownSample - 2)
            m_upSamplingShader.setRegularTexture(1, *m_downSample);
        else
            m_upSamplingShader.setRegularTexture(1, *m_upSample);
        m_upSamplingShader.setTexture(2, *m_upSample, i, GL_WRITE_ONLY,
                                      GL_RGBA32F);
        m_upSamplingShader.dispatch(width / GROUP_SIZE, height / GROUP_SIZE);
        m_upSamplingShader.wait();
    }
    Application::endEvent();

    Application::beginEvent("Bloom Pass 4 - Additive Blending");
    m_additiveBlendingShader.use();
    m_additiveBlendingShader.setRegularTexture(0, input);
    m_additiveBlendingShader.setRegularTexture(1, *m_upSample);
    m_additiveBlendingShader.setTexture(2, *m_result, 0, GL_WRITE_ONLY,
                                        GL_RGBA32F);
    m_additiveBlendingShader.dispatch(m_result->getWidth() / GROUP_SIZE,
                                      m_result->getHeight() / GROUP_SIZE);
    m_additiveBlendingShader.wait();
    Application::endEvent();

    Application::endEvent();
    return *m_result;
}