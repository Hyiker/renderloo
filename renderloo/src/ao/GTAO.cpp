#include "ao/GTAO.hpp"
#include <loo/Quad.hpp>
#include <loo/Shader.hpp>
#include <memory>
#include <random>
#include "ao/AOHelper.hpp"
#include "shaders/GTAOPass1.comp.hpp"
#include "shaders/GTAOPass2.comp.hpp"
using namespace loo;
GTAO::GTAO()
    : m_gtaoPass1Shader{Shader(GTAOPASS1_COMP, ShaderType::Compute)},
      m_gtaoPass2Shader{Shader(GTAOPASS2_COMP, ShaderType::Compute)} {}

void GTAO::init(int width, int height) {
    initAO();

    m_blurSource = std::make_unique<loo::Texture2D>();
    m_blurSource->init();
    m_blurSource->setupStorage(width / 2, height / 2, GL_R16F, 1);
    m_blurSource->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_blurSource->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_result = std::make_unique<loo::Texture2D>();
    m_result->init();
    m_result->setupStorage(width / 2, height / 2, GL_R16F, 1);
    m_result->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_result->setWrapFilter(GL_CLAMP_TO_EDGE);
}
constexpr int GROUP_SIZE = 1, N_SLICE = 2;
void GTAO::render(const loo::Texture2D& position, const loo::Texture2D& normal,
                  const loo::Texture2D& albedo,
                  const loo::Texture2D& depthStencil) {
    Application::beginEvent("GTAO");
    int width = position.getWidth() / 2, height = position.getHeight() / 2;
    Application::beginEvent("GTAO Pass 1 - Horizontal Slice Based Integral");
    m_gtaoPass1Shader.use();
    m_gtaoPass1Shader.setUniform("NumSlices", N_SLICE);
    float sinDeltaAngle = std::sin(M_PI / N_SLICE),
          cosDeltaAngle = std::cos(M_PI / N_SLICE);
    m_gtaoPass1Shader.setUniform("SinAndCosDeltaAngle",
                                 glm::vec2(sinDeltaAngle, cosDeltaAngle));
    m_gtaoPass1Shader.setRegularTexture(0, position);
    m_gtaoPass1Shader.setRegularTexture(1, normal);
    m_gtaoPass1Shader.setRegularTexture(2, depthStencil);
    m_gtaoPass1Shader.setRegularTexture(3, albedo);
    m_gtaoPass1Shader.setTexture(4, *m_blurSource, 0, GL_WRITE_ONLY, GL_R16F);

    m_gtaoPass1Shader.dispatch(width / GROUP_SIZE, height / GROUP_SIZE);
    m_gtaoPass1Shader.wait();
    Application::endEvent();

    Application::beginEvent("GTAO Pass 1 - Horizontal Slice Based Integral");
    m_gtaoPass2Shader.use();
    m_gtaoPass2Shader.setRegularTexture(0, *m_blurSource);
    m_gtaoPass2Shader.setTexture(1, *m_result, 0, GL_WRITE_ONLY, GL_R16F);

    m_gtaoPass2Shader.dispatch(width / GROUP_SIZE, height / GROUP_SIZE);
    m_gtaoPass2Shader.wait();
    Application::endEvent();

    Application::endEvent();
}
