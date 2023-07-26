#include "core/Skybox.hpp"
#include <filesystem>
#include "core/Transforms.hpp"
#include "core/constants.hpp"
#include "shaders/skybox.frag.hpp"
#include "shaders/skybox.vert.hpp"

using namespace loo;
namespace fs = std::filesystem;
Skybox::Skybox()
    : m_shader{Shader(SKYBOX_VERT, ShaderType::Vertex),
               Shader(SKYBOX_FRAG, ShaderType::Fragment)} {
    constexpr float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)0);
    glBindVertexArray(0);
}

void Skybox::loadTexture(const std::string& path) {
    fs::path p = path;
    if (fs::is_directory(p)) {
        // if directory, recognize as 6 face cube map
        // skybox setup
        auto skyboxFilenames = TextureCubeMap::builder()
                                   .front("front")
                                   .back("back")
                                   .left("left")
                                   .right("right")
                                   .top("top")
                                   .bottom("bottom")
                                   .prefix(path)
                                   .build();
        m_tex = createTextureCubeMapFromFiles(
            skyboxFilenames,
            TEXTURE_OPTION_CONVERT_TO_LINEAR | TEXTURE_OPTION_MIPMAP);
    } else {
        // TODO: load spherical map
    }
}

void Skybox::draw(glm::mat4 view) const {
    m_shader.use();
    ShaderProgram::getUniformBlock(SHADER_UB_PORT_MVP)
        .mapBufferScoped<MVP>(
            [&](MVP& mvp) { mvp.view = glm::mat4(glm::mat3(view)); });
    m_shader.setTexture(SHADER_SAMPLER_PORT_SKYBOX,
                        m_tex ? *m_tex : TextureCubeMap::getBlackTexture());
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

Skybox::~Skybox() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}