#include "antialias/TAA.hpp"
#include <loo/Application.hpp>
#include "shaders/TAABlending.comp.hpp"
using namespace loo;
TAA::TAA() : m_blendingShader{Shader(TAABLENDING_COMP, ShaderType::Compute)} {}
void TAA::init(int width, int height) {
    m_historyFrame[0] = std::make_unique<Texture2D>();
    m_historyFrame[0]->init();
    m_historyFrame[0]->setupStorage(width, height, GL_RGBA32F, 1);
    m_historyFrame[0]->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_historyFrame[0]->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_historyFrame[1] = std::make_unique<Texture2D>();
    m_historyFrame[1]->init();
    m_historyFrame[1]->setupStorage(width, height, GL_RGBA32F, 1);
    m_historyFrame[1]->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_historyFrame[1]->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_debugTexture = std::make_unique<Texture2D>();
    m_debugTexture->init();
    m_debugTexture->setupStorage(width, height, GL_RGBA32F, 1);
    m_debugTexture->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_debugTexture->setWrapFilter(GL_CLAMP_TO_EDGE);
}
constexpr int GROUP_SIZE = 32;
const Texture2D& TAA::apply(const Texture2D& currentFrame,
                            const Texture2D& velocity,
                            const Texture2D& depthStencil) {
    Application::beginEvent("TAA");

    m_blendingShader.use();
    m_blendingShader.setRegularTexture(0, currentFrame);
    m_blendingShader.setRegularTexture(
        1, *m_historyFrame[getPreviousFrameIndex()]);
    m_blendingShader.setRegularTexture(2, depthStencil);
    m_blendingShader.setRegularTexture(3, velocity);
    m_blendingShader.setTexture(4, *m_historyFrame[m_writeInIndex], 0,
                                GL_WRITE_ONLY, GL_RGBA32F);
    // m_blendingShader.setTexture(5, *m_debugTexture, 0, GL_WRITE_ONLY,
    //                             GL_RGBA32F);
    m_blendingShader.dispatch(
        std::ceil((float)currentFrame.getWidth() / GROUP_SIZE),
        std::ceil((float)currentFrame.getHeight() / GROUP_SIZE));
    m_blendingShader.wait();
    Application::endEvent();
    m_writeInIndex = getPreviousFrameIndex();
    return *m_historyFrame[m_writeInIndex ^ 1];
}