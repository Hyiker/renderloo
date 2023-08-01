#include "passes/BloomPass.hpp"
#include <memory>
#include "shaders//finalScreen.vert.hpp"
#include "shaders/bloomPixelPicker.comp.hpp"
#include "shaders/chainedGaussianBlur.comp.hpp"
#include "shaders/mipmapSum.frag.hpp"
using namespace loo;
BloomPass::BloomPass()
    : m_pixelPicker{Shader(BLOOMPIXELPICKER_COMP, ShaderType::Compute)},
      m_chainedGaussianBlur{
          Shader(CHAINEDGAUSSIANBLUR_COMP, ShaderType::Compute)},
      m_additiveBlending{Shader(FINALSCREEN_VERT, ShaderType::Vertex),
                         Shader(MIPMAPSUM_FRAG, ShaderType::Fragment)} {}

constexpr int GROUP_SIZE = 1, MIPMAP_LAYER_MAX = 5;
void BloomPass::init(int width, int height) {
    m_width = width;
    m_height = height;
    m_fb.init();
    m_pickedPixel = std::make_unique<loo::Texture2D>();
    m_mipLevel = std::min(mipmapLevelFromSize(width, height), MIPMAP_LAYER_MAX);
    m_pickedPixel->init();
    m_pickedPixel->setupStorage(width, height, GL_RGBA32F, m_mipLevel);
    m_pickedPixel->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    m_pickedPixel->setWrapFilter(GL_CLAMP_TO_EDGE);
}

void BloomPass::render(const Texture2D& inout, const Texture2D& GBufferD) {
    Application::beginEvent("Bloom Pass");
    Application::beginEvent("Bloom Pass 1 - Pixel Picker");
    m_pixelPicker.use();
    m_pixelPicker.setUniform("brightnessThreshold", brightnessThreshold);
    m_pixelPicker.setTexture(0, inout, 0, GL_READ_ONLY, GL_RGBA32F);
    m_pixelPicker.setTexture(1, GBufferD, 0, GL_READ_ONLY, GL_RGBA32F);
    m_pixelPicker.setTexture(2, *m_pickedPixel, 0, GL_WRITE_ONLY, GL_RGBA32F);

    m_pixelPicker.dispatch(m_width / GROUP_SIZE, m_height / GROUP_SIZE);
    panicPossibleGLError();
    m_pixelPicker.wait();
    Application::endEvent();

    Application::beginEvent("Bloom Pass 2 - Chained Gaussian Blur");
    int width = m_width, height = m_height;
    for (int i = 1; i < m_mipLevel; ++i) {
        width /= 2;
        height /= 2;
        m_chainedGaussianBlur.use();
        m_chainedGaussianBlur.setTexture(0, *m_pickedPixel, i - 1, GL_READ_ONLY,
                                         GL_RGBA32F);
        m_chainedGaussianBlur.setTexture(1, *m_pickedPixel, i, GL_WRITE_ONLY,
                                         GL_RGBA32F);
        m_chainedGaussianBlur.dispatch(width / GROUP_SIZE, height / GROUP_SIZE);
        m_chainedGaussianBlur.wait();
    }
    Application::endEvent();

    Application::beginEvent("Bloom Pass 3 - Additive Blending");
    m_fb.bind();
    m_fb.attachTexture(inout, GL_COLOR_ATTACHMENT0, 0);
    m_additiveBlending.use();
    m_additiveBlending.setTexture(0, *m_pickedPixel);
    m_additiveBlending.setUniform("mipLevels", m_mipLevel);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    Quad::globalQuad().draw();

    m_fb.unbind();
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    Application::endEvent();

    Application::endEvent();
}