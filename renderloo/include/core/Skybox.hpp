#ifndef RENDERLOO_INCLUDE_CORE_SKYBOX_HPP
#define RENDERLOO_INCLUDE_CORE_SKYBOX_HPP
#include <glad/glad.h>
#include <loo/Framebuffer.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>

class Skybox {

   public:
    Skybox();
    void loadTexture(const std::string& path);
    void draw(glm::mat4 view) const;
    const loo::TextureCubeMap& getEnvmap() const {
        return m_envmap ? *m_envmap : loo::TextureCubeMap::getBlackTexture();
    }
    const loo::TextureCubeMap& getDiffuseConv() const {
        return m_diffuseConv ? *m_diffuseConv
                             : loo::TextureCubeMap::getBlackTexture();
    }
    ~Skybox();

    std::string path{};

   private:
    struct {
        loo::Framebuffer framebuffer;
        loo::Renderbuffer renderbuffer;
    } helper;
    void renderEquirectangularToCubemap(const loo::Texture2D& equiTexture);
    void convolveEnvmap();
    GLuint vao, vbo;
    loo::ShaderProgram m_shader;
    loo::ShaderProgram m_equirectangularToCubemapShader, m_convolutionShader;
    std::unique_ptr<loo::TextureCubeMap> m_envmap{}, m_diffuseConv{};
};
#endif /* RENDERLOO_INCLUDE_CORE_SKYBOX_HPP */
