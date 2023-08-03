#ifndef RENDERLOO_INCLUDE_AO_SSAO_HPP
#define RENDERLOO_INCLUDE_AO_SSAO_HPP
#include <loo/Application.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>
class SSAO {

   public:
    SSAO(int width, int height);
    void init();
    void render(const loo::Texture2D& position, const loo::Texture2D& normal,
                const loo::Texture2D& depthStencil);
    const loo::Texture2D& getAOTexture() const { return *m_result; }

    float bias = 0.0001f, radius = 0.5f;

   private:
    int m_width, m_height;
    loo::Framebuffer m_fb;
    loo::ShaderProgram m_ssaoPass1Shader, m_ssaoPass2Shader;
    std::unique_ptr<loo::Texture2D> m_blurSource;
    std::unique_ptr<loo::Texture2D> m_result;
};

#endif /* RENDERLOO_INCLUDE_AO_SSAO_HPP */
