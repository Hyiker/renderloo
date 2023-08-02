#ifndef RENDERLOO_INCLUDE_AO_GTAO_HPP
#define RENDERLOO_INCLUDE_AO_GTAO_HPP
#include <loo/Application.hpp>
#include <loo/ComputeShader.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Texture.hpp>
#include <memory>
class GTAO {

   public:
    GTAO();
    void init(int width, int height);
    void render(const loo::Texture2D& position, const loo::Texture2D& normal,
                const loo::Texture2D& albedo,
                const loo::Texture2D& depthStencil);
    const loo::Texture2D& getAOTexture() const { return *m_result; }

   private:
    loo::ComputeShader m_gtaoPass1Shader, m_gtaoPass2Shader;
    // , m_gtaoPass2Shader;
    // , m_blurShader;
    // noise texture for random tangent vectors
    std::unique_ptr<loo::Texture2D> m_blurSource;
    std::unique_ptr<loo::Texture2D> m_result;
};

#endif /* RENDERLOO_INCLUDE_AO_GTAO_HPP */
