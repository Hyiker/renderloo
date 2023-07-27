#ifndef RENDERLOO_SRC_FINAL_PROCESS_HPP
#define RENDERLOO_SRC_FINAL_PROCESS_HPP
#include "core/FinalProcess.hpp"

#include <loo/glError.hpp>

#include "shaders/finalScreen.frag.hpp"
#include "shaders/finalScreen.vert.hpp"
using namespace loo;
FinalProcess::FinalProcess(int width, int height)
    : m_shader{{Shader(FINALSCREEN_VERT, GL_VERTEX_SHADER),
                Shader(FINALSCREEN_FRAG, GL_FRAGMENT_SHADER)}},
      m_width(width),
      m_height(height) {
    panicPossibleGLError();
}
void FinalProcess::init() {
    panicPossibleGLError();
}
void FinalProcess::render(const loo::Texture2D& deferredTexture,
                          const loo::Texture2D& GBuffer3,
                          const loo::Texture2D& skyboxTexture) {
    Framebuffer::bindDefault();
    glClearColor(0, 0, 0, 1);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    // force fill the quad in the final step
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_shader.use();
    m_shader.setTexture(0, deferredTexture);
    m_shader.setTexture(4, skyboxTexture);
    m_shader.setTexture(5, GBuffer3);

    Quad::globalQuad().draw();

    logPossibleGLError();
}

#endif /* RENDERLOO_SRC_FINAL_PROCESS_HPP */
