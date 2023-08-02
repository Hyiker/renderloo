#include "passes/ShadowMapPass.hpp"
#include <glog/logging.h>
#include <loo/Scene.hpp>
#include "core/Graphics.hpp"
#include "core/constants.hpp"
#include "shaders/shadowmap.frag.hpp"
#include "shaders/shadowmap.vert.hpp"
#include "shaders/shadowmapTransparent.frag.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
static constexpr int DIRECTIONAL_SHADOW_MAP_SIZE = 1024;
using namespace loo;

ShadowMapPass::ShadowMapPass()
    : m_opaqueShader{Shader(SHADOWMAP_VERT, ShaderType::Vertex),
                     Shader(SHADOWMAP_FRAG, ShaderType::Fragment)},
      m_transparentShader{
          Shader(SHADOWMAP_VERT, ShaderType::Vertex),
          Shader(SHADOWMAPTRANSPARENT_FRAG, ShaderType::Fragment)} {}
void ShadowMapPass::init() {
    m_fb.init();

    m_directionalShadowMap = std::make_unique<loo::Texture2D>();
    m_directionalShadowMap->init();
    m_directionalShadowMap->setupStorage(DIRECTIONAL_SHADOW_MAP_SIZE,
                                         DIRECTIONAL_SHADOW_MAP_SIZE,
                                         GL_DEPTH_COMPONENT32F, 1);
    m_directionalShadowMap->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_directionalShadowMap->setWrapFilter(GL_CLAMP_TO_BORDER);
    float borderDepth[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTextureParameterfv(m_directionalShadowMap->getId(),
                         GL_TEXTURE_BORDER_COLOR, borderDepth);
    // set compare mode to enable shadow comparison
    glTextureParameteri(m_directionalShadowMap->getId(),
                        GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_directionalShadowMap->getId(),
                        GL_TEXTURE_COMPARE_FUNC, GL_LESS);
    m_fb.attachTexture(*m_directionalShadowMap, GL_DEPTH_ATTACHMENT, 0);
    glNamedFramebufferDrawBuffer(m_fb.getId(), GL_NONE);
    glNamedFramebufferReadBuffer(m_fb.getId(), GL_NONE);
    panicPossibleGLError();

    // init directional shadow matrices buffer
    ShaderProgram::initUniformBlock(std::make_unique<UniformBuffer>(
        SHADER_UB_PORT_DIRECTIONAL_SHADOW_MATRICES,
        sizeof(ShaderDirectionalShadowMatricesBlock)));
}
void ShadowMapPass::render(const loo::Scene& scene,
                           const std::vector<ShaderLight>& lights,
                           float alphaTestThreshold) {

    Application::beginEvent("Shadow Map Pass");
    // render shadow map here
    m_fb.bind();
    Application::storeViewport();
    glViewport(0, 0, DIRECTIONAL_SHADOW_MAP_SIZE, DIRECTIONAL_SHADOW_MAP_SIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glClearDepth(0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    ShaderDirectionalShadowMatricesBlock block{};
    auto sceneModelMatrix = scene.getModelMatrix();
    for (auto& light : lights) {
        if (light.type == static_cast<int>(LightType::DIRECTIONAL) &&
            light.shadowData.strength > 0.f) {
            m_opaqueShader.use();
            int tileIndex = light.shadowData.tileIndex;
            // TODO set viewport to support multiple directional light shadows
            glm::mat4 lightSpaceMatrix = light.getLightSpaceMatrix(true);
            block.matrices[tileIndex] = lightSpaceMatrix;
            m_opaqueShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);
            for (auto mesh : scene.getMeshes()) {
                if (mesh->needAlphaBlend() &&
                    transparentShadowMode == TransparentShadowMode::AlphaTest)
                    continue;
                if (mesh->isDoubleSided()) {
                    glDisable(GL_CULL_FACE);
                } else {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
                drawMesh(*mesh, sceneModelMatrix, m_opaqueShader);
            }
            // if we render in solid mode, no need for special treatment of transparency
            if (transparentShadowMode == TransparentShadowMode::Solid)
                continue;
            m_transparentShader.use();
            m_transparentShader.setUniform("lightSpaceMatrix",
                                           lightSpaceMatrix);
            m_transparentShader.setUniform("alphaTestThreshold",
                                           alphaTestThreshold);
            // second pass render transparent objects using alpha test
            for (auto mesh : scene.getMeshes()) {
                if (!mesh->needAlphaBlend())
                    continue;
                if (mesh->isDoubleSided()) {
                    glDisable(GL_CULL_FACE);
                } else {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
                drawMesh(*mesh, sceneModelMatrix, m_transparentShader);
            }
        }
    }
    // update shadow matrices
    ShaderProgram::getUniformBlock(SHADER_UB_PORT_DIRECTIONAL_SHADOW_MATRICES)
        .updateData(&block);
    // first pass: render opaque objects

    m_fb.unbind();
    Application::restoreViewport();
    Application::endEvent();

    glDepthFunc(GL_LESS);
    glClearDepth(1.0f);
}