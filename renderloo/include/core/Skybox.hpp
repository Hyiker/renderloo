#ifndef LOO_LOO_SKYBOX_HPP
#define LOO_LOO_SKYBOX_HPP
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
    ~Skybox();

    std::string path{};

   private:
    struct {
        loo::Framebuffer framebuffer;
        loo::Renderbuffer renderbuffer;
    } equiangularHelper;
    void renderEquirectangularToCubemap(const loo::Texture2D& equiTexture);
    GLuint vao, vbo;
    loo::ShaderProgram m_shader;
    loo::ShaderProgram m_equirectangularToCubemapShader;
    std::unique_ptr<loo::TextureCubeMap> m_tex{};
};
#endif /* LOO_LOO_SKYBOX_HPP */
