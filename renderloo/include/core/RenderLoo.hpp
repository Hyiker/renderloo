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
#include "passes/ShadowMapPass.hpp"
#include "passes/TransparentPass.hpp"

#include <loo/Animation.hpp>
#include "antialias/AA.hpp"
#include "ao/AO.hpp"
#include "core/Deferred.hpp"
#include "core/FinalProcess.hpp"
#include "passes/BloomPass.hpp"
#include "passes/DebugOutputPass.hpp"

enum RenderFlag {
    RenderFlag_Opaque = 1 << 0,
    RenderFlag_Transparent = 1 << 1,
    RenderFlag_All = RenderFlag_Opaque | RenderFlag_Transparent
};
enum class CameraMode : int { FPS, ArcBall };
struct RenderInfo {
    glm::ivec2 deviceSize;
    unsigned int frameCount;
    float timeSecs;
    int enableTAA;
    glm::ivec3 padding;
};
class RenderLoo : public loo::Application {
   public:
    RenderLoo(int width, int height);
    void handleDrop(const std::string& paths);
    // only load model
    void loadModel(const std::string& filename);
    void loadSkybox(const std::string& filename);
    loo::PerspectiveCamera& getMainCamera() { return *m_mainCamera; }
    auto getMainCameraMode() const { return m_cameraMode; }
    void afterCleanup() override;
    void convertMaterial();
    void clear();

   private:
    void initGBuffers();
    void initDeferredPass();
    void initVelocity();

    void loop() override;
    void animation();
    void gui() override;
    void scene(loo::ShaderProgram& shader, RenderFlag flag = RenderFlag_All);
    void skyboxPass();
    // first pass: gbuffer
    void gbufferPass();
    // third pass: deferred pass(illumination)
    void deferredPass();
    void aoPass();
    const loo::Texture2D& smaaPass(const loo::Texture2D& input);
    const loo::Texture2D& taaPass(loo::Texture2D& input);

    const loo::Texture2D& getAOTexture() const {
        switch (m_aomethod) {
            case AOMethod::SSAO:
                return m_ssao.getAOTexture();
            case AOMethod::GTAO:
                return m_gtao.getAOTexture();
            default:
                return loo::Texture2D::getWhiteTexture();
        }
    }

    // seventh pass: merge all effects
    void finalScreenPass(const loo::Texture2D& texture);
    void keyboard();
    void mouse();
    void saveScreenshot(std::filesystem::path filename) const;

    loo::ShaderProgram m_baseshader;
    loo::Scene m_scene;
    Skybox m_skybox;

    CameraMode m_cameraMode{CameraMode::ArcBall};
    std::unique_ptr<loo::PerspectiveCamera> m_mainCamera;

    std::vector<ShaderLight> m_lights;

    ShadowMapPass m_shadowMapPass;

    // gbuffer
    GBuffer m_gbuffers;
    loo::Framebuffer m_gbufferfb;
    // ambient occlusion
    AOMethod m_aomethod{AOMethod::SSAO};
    SSAO m_ssao;
    GTAO m_gtao;
    // deferred pass
    loo::ShaderProgram m_deferredshader;
    loo::Framebuffer m_deferredfb;
    // diffuse
    std::shared_ptr<loo::Texture2D> m_deferredResult;

    TransparentPass m_transparentPass;
    BloomPass m_bloomPass;
    // antialias
    AntiAliasMethod m_antialiasmethod{AntiAliasMethod::TAA};
    SMAA m_smaa;
    TAA m_taa;

    // process
    FinalProcess m_finalprocess;
    // debug
    DebugOutputPass m_debugOutputPass;

    std::unique_ptr<loo::Texture2D> m_velocityTexture;

    bool m_wireframe{false};
    bool m_enablenormal{true};
    bool m_screenshotflag{false};
    bool m_enableDFGCompensation{true};
    bool m_enableBloom{true};

    FinalPassOptions m_finalpassoptions;

    loo::Animator m_animator{nullptr};
};

#endif /* RENDERLOO_INCLUDE_CORE_RENDER_LOO_HPP */
