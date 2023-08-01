#include "passes/TransparentPass.hpp"
#include <loo/Camera.hpp>
#include <loo/Scene.hpp>
#include "core/Graphics.hpp"
#include "shaders/gbuffer.vert.hpp"
#include "shaders/transparent.frag.hpp"

using namespace loo;

TransparentPass::TransparentPass()
    : m_transparentShader{
          Shader(GBUFFER_VERT, ShaderType::Vertex),
          Shader(TRANSPARENT_FRAG, ShaderType::Fragment),
      } {}

void TransparentPass::init(const Renderbuffer& rb, const Texture2D& output) {
    m_transparentfb.init();
    m_transparentfb.attachTexture(output, GL_COLOR_ATTACHMENT0, 0);
    m_transparentfb.attachRenderbuffer(rb, GL_DEPTH_ATTACHMENT);
    panicPossibleGLError();
}
void TransparentPass::render(const Scene& scene, const Skybox& skybox,
                             const Camera& camera,
                             const Texture2D& mainLightShadowMap,
                             const std::vector<ShaderLight>& lights) {
    Application::beginEvent("Transparent Pass");
    m_transparentfb.bind();
    glEnable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    std::vector<std::pair<Mesh*, float>> meshes;

    for (auto mesh : scene.getMeshes()) {
        if (mesh->needAlphaBlend()) {
            glm::vec4 c = glm::vec4(mesh->aabb.getCenter(), 1);
            c = scene.getModelMatrix() * mesh->objectMatrix * c;
            float dist = glm::distance(camera.position, glm::vec3(c));
            meshes.emplace_back(mesh.get(), dist);
        }
    }
    // sort transparent meshes, so that further meshes are drawn first
    std::sort(meshes.begin(), meshes.end(),
              [](auto& a, auto& b) { return a.second > b.second; });
    Application::beginEvent("Subpass1 - Alpha Test");

    m_transparentfb.enableAttachments({GL_COLOR_ATTACHMENT0});

    m_transparentShader.use();

    m_transparentShader.setTexture(20, skybox.getDiffuseConv());
    m_transparentShader.setTexture(21, skybox.getSpecularConv());
    m_transparentShader.setTexture(22, skybox.getBRDFLUT());
    m_transparentShader.setTexture(23, mainLightShadowMap);
    m_transparentShader.setUniform("cameraPosition", camera.position);
    m_transparentShader.setUniform("alphaTest", 1);
    m_transparentShader.setUniform("alphaTestThreshold", m_alphaTestThreshold);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);

    m_transparentShader.use();
    // subpass 1: alpha test
    // enable z-write and z-test, discard fragments with alpha < threshold
    for (auto p : meshes) {
        auto mesh = p.first;
        if (mesh->isDoubleSided()) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        drawMesh(*mesh, scene.getModelMatrix(), m_transparentShader);
    }
    logPossibleGLError();

    Application::endEvent();
    Application::beginEvent("Subpass2 - Alpha Blend");
    // subpass 2: alpha blend
    // disable z-write, enable blending
    glDepthMask(GL_FALSE);
    m_transparentShader.setUniform("alphaTest", 0);
    for (auto p : meshes) {
        auto mesh = p.first;
        if (mesh->isDoubleSided()) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        drawMesh(*mesh, scene.getModelMatrix(), m_transparentShader);
    }
    logPossibleGLError();
    Application::endEvent();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    Application::endEvent();
}