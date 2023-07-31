#ifndef RENDERLOO_INCLUDE_PASSES_SHADOW_MAP_PASS_HPP
#define RENDERLOO_INCLUDE_PASSES_SHADOW_MAP_PASS_HPP
#include <loo/Application.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include "core/Light.hpp"
#include "core/Skybox.hpp"
class ShadowMapPass {
   public:
    ShadowMapPass();
    // void render(const loo::Scene& scene,
    //             const loo::Texture2D& mainLightShadowMap,
    //             const std::vector<ShaderLight>& lights);
    // [[nodiscard]] auto getAlphaTestThreshold() const {
    //     return m_alphaTestThreshold;
    // }

   private:
    loo::Framebuffer m_fb;
    loo::ShaderProgram m_opaqueShader, m_transparentShader;
};

#endif /* RENDERLOO_INCLUDE_PASSES_SHADOW_MAP_PASS_HPP */
