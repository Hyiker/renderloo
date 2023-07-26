#include "core/Skybox.hpp"
#include <glog/logging.h>
#include <stb/stb_image.h>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include "core/Transforms.hpp"
#include "core/constants.hpp"
#include "shaders/equirectangularMapper.frag.hpp"
#include "shaders/equirectangularMapper.vert.hpp"
#include "shaders/skybox.frag.hpp"
#include "shaders/skybox.vert.hpp"

using namespace loo;
namespace fs = std::filesystem;
using namespace std;

static shared_ptr<Texture2D> createTexture2DFromHDRFile(
    const std::string& filename) {

    shared_ptr<Texture2D> tex = make_shared<Texture2D>();
    int width, height, nchannel;
    stbi_set_flip_vertically_on_load(true);
    auto data = stbi_loadf(filename.c_str(), &width, &height, &nchannel, 3);
    CHECK_NOTNULL(data);
    CHECK_EQ(nchannel, 3);
    if (!data) {
        return nullptr;
    }
    tex->init();
    logPossibleGLError();
    // attention, mismatch between internalformat and format may casue
    // GL_INVALID_OPERATION
    tex->setup(data, width, height, GL_RGB16F, GL_RGB, GL_FLOAT);
    panicPossibleGLError();
    tex->setWrapFilter(GL_CLAMP_TO_EDGE);
    tex->setSizeFilter(GL_LINEAR, GL_LINEAR);
    panicPossibleGLError();

    stbi_image_free(data);
    LOG(INFO) << "2D HDR Texture " << filename << " loaded.";
    return tex;
}

static constexpr int CUBEMAP_SIZE = 1024;
Skybox::Skybox()
    : m_shader{Shader(SKYBOX_VERT, ShaderType::Vertex),
               Shader(SKYBOX_FRAG, ShaderType::Fragment)},
      m_equirectangularToCubemapShader(
          {Shader(EQUIRECTANGULARMAPPER_VERT, ShaderType::Vertex),
           Shader(EQUIRECTANGULARMAPPER_FRAG, ShaderType::Fragment)}) {
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

    equiangularHelper.framebuffer.init();
    equiangularHelper.renderbuffer.init(GL_DEPTH_COMPONENT24, CUBEMAP_SIZE,
                                        CUBEMAP_SIZE);
}
void Skybox::renderEquirectangularToCubemap(const loo::Texture2D& equiTexture) {
    glm::mat4 captureProjection =
        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f))};

    // convert HDR equirectangular environment map to cubemap equivalent
    m_equirectangularToCubemapShader.use();
    m_equirectangularToCubemapShader.setUniform("equirectangularMap", 0);
    m_equirectangularToCubemapShader.setUniform("projection",
                                                captureProjection);
    m_equirectangularToCubemapShader.setTexture(0, equiTexture);

    glViewport(0, 0, CUBEMAP_SIZE, CUBEMAP_SIZE);
    equiangularHelper.framebuffer.bind();
    for (unsigned int i = 0; i < 6; ++i) {
        m_equirectangularToCubemapShader.setUniform("view", captureViews[i]);
        glNamedFramebufferTextureLayer(equiangularHelper.framebuffer.getId(),
                                       GL_COLOR_ATTACHMENT0, m_tex->getId(), 0,
                                       i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    equiangularHelper.framebuffer.unbind();
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
        // compute cubemap from equirectangular map
        auto equiMap = createTexture2DFromHDRFile(path);
        if (!m_tex) {
            m_tex = make_unique<TextureCubeMap>();
            m_tex->init();
            m_tex->setupStorage(CUBEMAP_SIZE, CUBEMAP_SIZE, GL_RGB16F, 1);
            m_tex->setWrapFilter(GL_CLAMP_TO_EDGE);
            m_tex->setSizeFilter(GL_LINEAR, GL_LINEAR);
        }
        LOG(INFO) << "Converting equirectangular map to cubemap" << endl;
        renderEquirectangularToCubemap(*equiMap);
    }
    this->path = path;
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