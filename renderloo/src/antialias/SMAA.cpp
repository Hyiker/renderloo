#include "antialias/SMAA.hpp"
#include <loo/Quad.hpp>
#include "antialias/SMAAAreaTex.h"
#include "antialias/SMAASearchTex.h"
#include "shaders/SMAABlendingWeight.frag.hpp"
#include "shaders/SMAABlendingWeight.vert.hpp"
#include "shaders/SMAAEdgeDetection.frag.hpp"
#include "shaders/SMAAEdgeDetection.vert.hpp"
#include "shaders/SMAANeighborhoodBlending.frag.hpp"
#include "shaders/SMAANeighborhoodBlending.vert.hpp"
using namespace loo;
SMAA::SMAA(int width, int height)
    : m_width(width),
      m_height(height),
      m_shaderpass1{Shader(SMAAEDGEDETECTION_VERT, GL_VERTEX_SHADER),
                    Shader(SMAAEDGEDETECTION_FRAG, GL_FRAGMENT_SHADER)},
      m_shaderpass2{Shader(SMAABLENDINGWEIGHT_VERT, GL_VERTEX_SHADER),
                    Shader(SMAABLENDINGWEIGHT_FRAG, GL_FRAGMENT_SHADER)},
      m_shaderpass3{Shader(SMAANEIGHBORHOODBLENDING_VERT, GL_VERTEX_SHADER),
                    Shader(SMAANEIGHBORHOODBLENDING_FRAG, GL_FRAGMENT_SHADER)} {
}
void SMAA::init() {
    m_fb.init();

    m_rb.init(GL_STENCIL_INDEX8, m_width, m_height);
    m_edges = std::make_unique<loo::Texture2D>();
    m_edges->init();
    m_edges->setupStorage(m_width, m_height, GL_RGBA32F, 1);
    m_edges->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_edges->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_blend = std::make_unique<loo::Texture2D>();
    m_blend->init();
    m_blend->setupStorage(m_width, m_height, GL_RGBA32F, 1);
    m_blend->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_blend->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_area = std::make_unique<loo::Texture2D>();
    m_area->init();
    m_area->setup(areaTexBytes, AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG8, GL_RG,
                  GL_UNSIGNED_BYTE);
    m_area->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_area->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_search = std::make_unique<loo::Texture2D>();
    m_search->init();
    m_search->setup(searchTexBytes, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8,
                    GL_RED, GL_UNSIGNED_BYTE);
    m_search->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_search->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_output = std::make_unique<loo::Texture2D>();
    m_output->init();
    m_output->setupStorage(m_width, m_height, GL_RGBA32F, 1);
    m_output->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_output->setWrapFilter(GL_CLAMP_TO_EDGE);

    m_fb.attachRenderbuffer(m_rb, GL_STENCIL_ATTACHMENT);
}
const loo::Texture2D& SMAA::apply(const loo::Application& app,
                                  const loo::Texture2D& src) {
    glm::vec4 metrics{1.0f / m_width, 1.0f / m_height, m_width, m_height};
    m_fb.bind();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    app.beginEvent("SMAA Edge Detection");
    // pass 1: edge detection
    m_fb.attachTexture(*m_edges, GL_COLOR_ATTACHMENT0, 0);
    m_fb.enableAttachments({GL_COLOR_ATTACHMENT0});
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    m_shaderpass1.use();
    m_shaderpass1.setUniform("rt_metrics", metrics);
    m_shaderpass1.setTexture(0, src);
    Quad::globalQuad().draw();
    app.endEvent();

    app.beginEvent("SMAA Blending Weight Calculation");
    // pass 2: blending weight calculation
    m_fb.attachTexture(*m_blend, GL_COLOR_ATTACHMENT0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    m_shaderpass2.use();
    m_shaderpass2.setUniform("rt_metrics", metrics);
    m_shaderpass2.setTexture(0, *m_edges);
    m_shaderpass2.setTexture(1, *m_area);
    m_shaderpass2.setTexture(2, *m_search);
    Quad::globalQuad().draw();
    app.endEvent();

    app.beginEvent("SMAA Neighborhood Blending");
    // pass 3: neighborhood blending
    m_fb.attachTexture(*m_output, GL_COLOR_ATTACHMENT0, 0);
    // disable stencil test
    glDisable(GL_STENCIL_TEST);
    m_shaderpass3.use();
    m_shaderpass3.setUniform("rt_metrics", metrics);
    m_shaderpass3.setTexture(0, src);
    m_shaderpass3.setTexture(1, *m_blend);
    Quad::globalQuad().draw();
    app.endEvent();

    m_fb.unbind();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    return *m_output;
}