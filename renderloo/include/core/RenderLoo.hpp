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
#include <loo/UniformBuffer.hpp>
#include <loo/loo.hpp>
#include <memory>
#include <string>
#include <vector>
#include "core/Light.hpp"
#include "core/Skybox.hpp"

#include "core/FinalProcess.hpp"
#include "core/constants.hpp"

class RenderLoo : public loo::Application {
   public:
    RenderLoo(int width, int height);
    // only load model
    void loadModel(const std::string& filename);
    void loadGLTF(const std::string& filename);
    void loadSkybox(const std::string& filename);
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

    loo::ShaderProgram m_baseshader;
    loo::Scene m_scene;
    Skybox m_skybox;
    loo::Camera m_maincam;
    std::vector<ShaderLight> m_lights;

    std::shared_ptr<loo::Texture2D> m_mainlightshadowmap;
    loo::Framebuffer m_mainlightshadowmapfb;
    loo::ShaderProgram m_shadowmapshader;

    // gbuffer
    struct GBuffer {
        std::unique_ptr<loo::Texture2D> position;
        // base color(3) + unused(1)
        std::unique_ptr<loo::Texture2D> bufferA;
        // metallic(1) + padding(2) + occlusion(1)
        std::unique_ptr<loo::Texture2D> bufferB;
        // normal(3) + roughness(1)
        std::unique_ptr<loo::Texture2D> bufferC;
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
