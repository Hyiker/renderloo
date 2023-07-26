#ifndef LOO_LOO_SKYBOX_HPP
#define LOO_LOO_SKYBOX_HPP
#include <glad/glad.h>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>

class Skybox {

   public:
    Skybox();
    void loadTexture(const std::string& path);
    void draw(glm::mat4 view) const;
    ~Skybox();

   private:
    GLuint vao, vbo;
    loo::ShaderProgram m_shader;
    std::unique_ptr<loo::TextureCubeMap> m_tex{};
};
#endif /* LOO_LOO_SKYBOX_HPP */
