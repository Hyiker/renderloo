#include "passes/DebugOutputPass.hpp"
#include "shaders/debugOutput.frag.hpp"
#include "shaders/finalScreen.vert.hpp"
using namespace loo;
DebugOutputPass::DebugOutputPass()
    : m_debugOutputShader{Shader(FINALSCREEN_VERT, ShaderType::Vertex),
                          Shader(DEBUGOUTPUT_FRAG, ShaderType::Fragment)} {}

void DebugOutputPass::init(int width, int height) {
    m_fb.init();

    m_outputTexture = std::make_unique<loo::Texture2D>();
    m_outputTexture->init();
    m_outputTexture->setupStorage(width, height, GL_RGB32F, 1);
    m_fb.attachTexture(*m_outputTexture, GL_COLOR_ATTACHMENT0, 0);

    m_width = width;
    m_height = height;
    panicPossibleGLError();
}

void DebugOutputPass::render(const GBuffer& gbuffer, const loo::Texture2D& ao) {
    m_fb.bind();
    m_fb.attachRenderbuffer(gbuffer.depthStencilRb,
                            GL_DEPTH_STENCIL_ATTACHMENT);

    glClearColor(0, 0, 0, 1);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    // force fill the quad in the final step
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_debugOutputShader.use();
    m_debugOutputShader.setTexture(0, *gbuffer.bufferA);
    m_debugOutputShader.setTexture(1, *gbuffer.bufferB);
    m_debugOutputShader.setTexture(2, *gbuffer.bufferC);
    m_debugOutputShader.setTexture(3, *gbuffer.bufferD);
    m_debugOutputShader.setTexture(4, ao);
    m_debugOutputShader.setTexture(5, ao);
    m_debugOutputShader.setUniform("mode", static_cast<int>(debugOutputOption));

    Quad::globalQuad().draw();

    glBlitNamedFramebuffer(m_fb.getId(), 0, 0, 0, m_width, m_height, 0, 0,
                           m_width, m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    logPossibleGLError();
    m_fb.unbind();
}