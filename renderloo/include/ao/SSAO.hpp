#ifndef RENDERLOO_INCLUDE_AO_SSAO_HPP
#define RENDERLOO_INCLUDE_AO_SSAO_HPP
#include <loo/Application.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>
constexpr int SSAO_KERNEL_SIZE = 64;
class SSAO {

   public:
    SSAO(int width, int height);
    void init();
    void render(const loo::Application& app, const loo::Texture2D& position,
                const loo::Texture2D& normal,
                const loo::Texture2D& depthStencil);
    const loo::Texture2D& getAOTexture() const { return *m_result; }

    float bias = 0.0001f, radius = 0.5f;

   private:
    void initRandomTexture();
    void initKernel();

    int m_width, m_height;
    loo::Framebuffer m_fb;
    // random sample kernel, only used in this shader, no need to use uniform buffer
    struct Kernel {
        glm::vec3 samples[SSAO_KERNEL_SIZE]{};
    } m_kernel;
    loo::ShaderProgram m_ssaoPass1Shader, m_ssaoPass2Shader;
    // , m_blurShader;
    // noise texture for random tangent vectors
    std::unique_ptr<loo::Texture2D> m_noise;
    std::unique_ptr<loo::Texture2D> m_blurSource;
    std::unique_ptr<loo::Texture2D> m_result;
};

#endif /* RENDERLOO_INCLUDE_AO_SSAO_HPP */
