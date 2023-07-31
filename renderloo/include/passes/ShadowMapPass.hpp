#ifndef RENDERLOO_INCLUDE_PASSES_SHADOW_MAP_PASS_HPP
#define RENDERLOO_INCLUDE_PASSES_SHADOW_MAP_PASS_HPP
#include <loo/Application.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include <vector>
#include "core/Light.hpp"
enum class TransparentShadowMode : int {
    Solid = 0,     // treat transparent objects as opaque ones
    AlphaTest = 1  // use alpha test to discard some of fragments
};
class ShadowMapPass {
   public:
    ShadowMapPass();
    void init();
    void render(const loo::Scene& scene, const std::vector<ShaderLight>& lights,
                float alphaTestThreshold);
    [[nodiscard]] const loo::Texture2D& getDirectionalShadowMap() const {
        return *m_directionalShadowMap;
    }

    TransparentShadowMode transparentShadowMode{
        TransparentShadowMode::AlphaTest};

   private:
    loo::Framebuffer m_fb;
    loo::ShaderProgram m_opaqueShader, m_transparentShader;
    std::unique_ptr<loo::Texture2D> m_directionalShadowMap;
};

#endif /* RENDERLOO_INCLUDE_PASSES_SHADOW_MAP_PASS_HPP */
