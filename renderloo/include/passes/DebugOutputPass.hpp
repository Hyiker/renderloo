#ifndef RENDERLOO_INCLUDE_PASSES_DEBUG_OUTPUT_PASS_HPP
#define RENDERLOO_INCLUDE_PASSES_DEBUG_OUTPUT_PASS_HPP
#include <loo/Application.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include <memory>
#include "core/Deferred.hpp"
enum class DebugOutputOption : int {
    None = 0,
    BaseColor = 1,
    Metalness = 2,
    Roughness = 3,
    Normal = 4,
    Emission = 5,
    AO = 6,
};
class DebugOutputPass {
   public:
    DebugOutputPass();
    void init(int width, int height);
    void render(const GBuffer& gbuffer, const loo::Texture2D& ao);

    DebugOutputOption debugOutputOption{DebugOutputOption::None};

   private:
    loo::ShaderProgram m_debugOutputShader;
    loo::Framebuffer m_fb;
    std::unique_ptr<loo::Texture2D> m_outputTexture;
    int m_width, m_height;
};

#endif /* RENDERLOO_INCLUDE_PASSES_DEBUG_OUTPUT_PASS_HPP */
