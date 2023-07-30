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
    void draw() const;
    const loo::TextureCubeMap& getEnvmap() const {
        return m_envmap ? *m_envmap : loo::TextureCubeMap::getBlackTexture();
    }
    const loo::TextureCubeMap& getDiffuseConv() const {
        return m_diffuseConv ? *m_diffuseConv
                             : loo::TextureCubeMap::getBlackTexture();
    }
    const loo::TextureCubeMap& getSpecularConv() const {
        return m_specularConv ? *m_specularConv
                              : loo::TextureCubeMap::getBlackTexture();
    }
    const loo::Texture2D& getBRDFLUT() const {
        return m_BRDFLUT ? *m_BRDFLUT : loo::Texture2D::getBlackTexture();
    }
    ~Skybox();

    std::string path{};

   private:
    struct {
        loo::Framebuffer framebuffer;
        loo::Renderbuffer renderbuffer;
    } helper;
    void renderEquirectangularToCubemap(const loo::Texture2D& equiTexture);
    void convolveDiffuseEnvmap();
    void convolveSpecularEnvmap();
    void initBRDFLUT();
    GLuint vao, vbo;
    loo::ShaderProgram m_shader;
    loo::ShaderProgram m_equirectangularToCubemapShader, m_diffuseConvShader,
        m_specularConvShader, m_BRDFLUTShader;
    std::unique_ptr<loo::TextureCubeMap> m_envmap{}, m_diffuseConv{},
        m_specularConv{};
    std::unique_ptr<loo::Texture2D> m_BRDFLUT{};
};
#endif /* RENDERLOO_INCLUDE_CORE_SKYBOX_HPP */
