#include "ao/SSAO.hpp"
#include <loo/Quad.hpp>
#include <loo/Shader.hpp>
#include <memory>
#include <random>
#include "shaders/SSAOPass1.frag.hpp"
#include "shaders/SSAOPass2.frag.hpp"
#include "shaders/finalScreen.vert.hpp"
using namespace loo;
SSAO::SSAO(int width, int height)
    : m_width(width),
      m_height(height),
      m_ssaoPass1Shader{Shader(FINALSCREEN_VERT, ShaderType::Vertex),
                        Shader(SSAOPASS1_FRAG, ShaderType::Fragment)},
      m_ssaoPass2Shader{Shader(FINALSCREEN_VERT, ShaderType::Vertex),
                        Shader(SSAOPASS2_FRAG, ShaderType::Fragment)} {}

void SSAO::init() {
    m_fb.init();
    m_result = std::make_unique<loo::Texture2D>();
    m_result->init();
    m_result->setupStorage(m_width, m_height, GL_R8, 1);
    m_result->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_result->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_blurSource = std::make_unique<loo::Texture2D>();
    m_blurSource->init();
    m_blurSource->setupStorage(m_width, m_height, GL_R8, 1);
    m_blurSource->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_blurSource->setWrapFilter(GL_CLAMP_TO_EDGE);

    initKernel();
    initRandomTexture();
}

void SSAO::render(const loo::Application& app, const Texture2D& position,
                  const Texture2D& normal, const Texture2D& depthStencil) {
    app.beginEvent("SSAO");
    app.beginEvent("Pass 1 - kernel sampling");
    m_fb.bind();
    m_fb.attachTexture(depthStencil, GL_STENCIL_ATTACHMENT, 0);
    m_fb.attachTexture(*m_blurSource, GL_COLOR_ATTACHMENT0, 0);
    m_fb.enableAttachments({GL_COLOR_ATTACHMENT0});
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    // disable stencil write
    glStencilMask(0x00);

    m_ssaoPass1Shader.use();
    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        m_ssaoPass1Shader.setUniform("kernelSamples[" + std::to_string(i) + "]",
                                     m_kernel.samples[i]);
    }
    m_ssaoPass1Shader.setUniform("framebufferSize",
                                 glm::vec2(m_width, m_height));
    m_ssaoPass1Shader.setTexture(0, position);
    m_ssaoPass1Shader.setTexture(1, normal);
    m_ssaoPass1Shader.setTexture(2, depthStencil);
    m_ssaoPass1Shader.setTexture(3, *m_noise);
    m_ssaoPass1Shader.setUniform("bias", bias);
    m_ssaoPass1Shader.setUniform("radius", radius);
    Quad::globalQuad().draw();

    app.endEvent();

    app.beginEvent("Pass 2 - blur");
    m_ssaoPass2Shader.use();
    m_fb.attachTexture(*m_result, GL_COLOR_ATTACHMENT0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    m_ssaoPass2Shader.setTexture(0, *m_blurSource);
    Quad::globalQuad().draw();
    app.endEvent();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    // enable stencil write
    glStencilMask(0xFF);
    app.endEvent();
}

void SSAO::initKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / SSAO_KERNEL_SIZE;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        m_kernel.samples[i] = sample * scale;
    }
}

constexpr int NOISE_SIZE = 4;
void SSAO::initRandomTexture() {
    std::uniform_real_distribution<float> randomFloats(-1.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; i++) {
        glm::vec3 noise(randomFloats(generator), randomFloats(generator), 0.0f);
        noise = glm::normalize(noise);
        ssaoNoise.push_back(noise);
    }
    m_noise = std::make_unique<loo::Texture2D>();
    m_noise->init();
    m_noise->setup(ssaoNoise.data(), NOISE_SIZE, NOISE_SIZE, GL_RGB16F, GL_RGB,
                   GL_FLOAT, 1);
    m_noise->setSizeFilter(GL_NEAREST, GL_NEAREST);
    m_noise->setWrapFilter(GL_REPEAT);
}