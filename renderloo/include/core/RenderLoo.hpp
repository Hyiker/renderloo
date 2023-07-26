#ifndef RENDERLOO_INCLUDE_CORE_RENDER_LOO_HPP
#define RENDERLOO_INCLUDE_CORE_RENDER_LOO_HPP

#include <filesystem>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>
#include <loo/Application.hpp>
#include <loo/Camera.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Quad.hpp>
#include <loo/Scene.hpp>
#include <loo/Shader.hpp>
#include <loo/Skybox.hpp>
#include <loo/UniformBuffer.hpp>
#include <loo/loo.hpp>
#include <memory>
#include <string>
#include <vector>
#include "core/Light.hpp"

#include "core/FinalProcess.hpp"
#include "core/constants.hpp"

class RenderLoo : public loo::Application {
   public:
    RenderLoo(int width, int height, const char* skyBoxPrefix = nullptr);
    // only load model
    void loadModel(const std::string& filename);
    void loadGLTF(const std::string& filename);
    loo::Camera& getCamera() { return m_maincam; }
    void afterCleanup() override;
    void convertMaterial();
    void clear();

   private:
    void initGBuffers();
    void initShadowMap();
    void initDeferredPass();

    void loop() override;
    void gui();
    void scene();
    void skyboxPass();
    // first pass: gbuffer
    void gbufferPass();
    // second pass: shadow map
    void shadowMapPass();
    // third pass: deferred pass(illumination)
    void deferredPass();

    // seventh pass: merge all effects
    void finalScreenPass();
    void keyboard();
    void mouse();
    void saveScreenshot(std::filesystem::path filename) const;

    loo::ShaderProgram m_baseshader, m_skyboxshader;
    loo::Scene m_scene;
    std::shared_ptr<loo::TextureCubeMap> m_skyboxtex{};
    loo::Skybox m_skybox;
    loo::Camera m_maincam;
    std::vector<ShaderLight> m_lights;

    std::shared_ptr<loo::Texture2D> m_mainlightshadowmap;
    loo::Framebuffer m_mainlightshadowmapfb;
    loo::ShaderProgram m_shadowmapshader;

    // gbuffer
    struct GBuffer {
        std::shared_ptr<loo::Texture2D> position;
        std::shared_ptr<loo::Texture2D> normal;
        // blinn-phong: diffuse(3) + specular(1)
        // pbr metallic-roughness: baseColor(3) + metallic(1)
        std::shared_ptr<loo::Texture2D> albedo;
        // simple material: transparent(3)(sss mask) + IOR(1)
        // pbr: transmission(1)(sss mask) + sigma_t(3)
        std::shared_ptr<loo::Texture2D> buffer3;
        // simple material: unused
        // pbr: sigma_a(3) + roughness(1)
        std::unique_ptr<loo::Texture2D> buffer4;
        // simple material: unused
        // pbr: occlusion(1) + unused(3)
        std::unique_ptr<loo::Texture2D> buffer5;
        loo::Renderbuffer depthrb;
    } m_gbuffers;
    loo::Framebuffer m_gbufferfb;
    // deferred pass
    loo::ShaderProgram m_deferredshader;
    loo::Framebuffer m_deferredfb;
    // diffuse
    std::shared_ptr<loo::Texture2D> m_diffuseresult;
    // specular
    std::unique_ptr<loo::Texture2D> m_specularresult;
    // skybox
    std::unique_ptr<loo::Texture2D> m_skyboxresult;

    // process
    FinalProcess m_finalprocess;

    bool m_wireframe{false};
    bool m_enablenormal{true};
    bool m_screenshotflag{false};

    FinalPassOptions m_finalpassoptions;
};

#endif /* RENDERLOO_INCLUDE_CORE_RENDER_LOO_HPP */
