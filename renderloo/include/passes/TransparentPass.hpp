#ifndef RENDERLOO_INCLUDE_PASSES_TRANSPARENT_PASS_HPP
#define RENDERLOO_INCLUDE_PASSES_TRANSPARENT_PASS_HPP
#include <loo/Application.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include "core/Light.hpp"
#include "core/Skybox.hpp"
class TransparentPass {
   public:
    TransparentPass();
    void init(const loo::Renderbuffer& rb, const loo::Texture2D& output);
    void render(const loo::Scene& scene, const Skybox& skybox,
                const loo::Camera& camera,
                const loo::Texture2D& mainLightShadowMap,
                const std::vector<ShaderLight>& lights);

   private:
    loo::Framebuffer m_transparentfb;
    loo::ShaderProgram m_transparentShader;

    float m_alphaTestThreshold{0.65f};
};

#endif /* RENDERLOO_INCLUDE_PASSES_TRANSPARENT_PASS_HPP */
