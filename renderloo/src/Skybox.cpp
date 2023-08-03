#include "core/Skybox.hpp"
#include <glog/logging.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include "core/constants.hpp"
#include "loo/Quad.hpp"
#include "shaders/BRDFLUT.frag.hpp"
#include "shaders/envmapDiffuseConvolution.frag.hpp"
#include "shaders/envmapSpecularConvolution.frag.hpp"
#include "shaders/equirectangularMapper.frag.hpp"
#include "shaders/equirectangularMapper.vert.hpp"
#include "shaders/finalScreen.vert.hpp"
#include "shaders/skybox.frag.hpp"
#include "shaders/skybox.vert.hpp"

using namespace loo;
namespace fs = std::filesystem;
using namespace std;

static constexpr int ENVMAP_SIZE = 1024, DIFFUSECONV_SIZE = 32,
                     SPECULARCONV_SIZE = 256, SPECULARCONV_MIPLEVEL = 5,
                     BRDFLUT_SIZE = 512;
Skybox::Skybox()
    : m_shader{Shader(SKYBOX_VERT, ShaderType::Vertex),
               Shader(SKYBOX_FRAG, ShaderType::Fragment)},
      m_equirectangularToCubemapShader(
          {Shader(EQUIRECTANGULARMAPPER_VERT, ShaderType::Vertex),
           Shader(EQUIRECTANGULARMAPPER_FRAG, ShaderType::Fragment)}),
      m_diffuseConvShader{
          Shader(EQUIRECTANGULARMAPPER_VERT, ShaderType::Vertex),
          Shader(ENVMAPDIFFUSECONVOLUTION_FRAG, ShaderType::Fragment)},
      m_specularConvShader{
          Shader(EQUIRECTANGULARMAPPER_VERT, ShaderType::Vertex),
          Shader(ENVMAPSPECULARCONVOLUTION_FRAG, ShaderType::Fragment)} {
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

    helper.framebuffer.init();
    helper.renderbuffer.init(GL_DEPTH_COMPONENT24, ENVMAP_SIZE, ENVMAP_SIZE);
    initBRDFLUT();
}

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
void Skybox::renderEquirectangularToCubemap(const loo::Texture2D& equiTexture) {

    // convert HDR equirectangular environment map to cubemap equivalent
    m_equirectangularToCubemapShader.use();
    m_equirectangularToCubemapShader.setUniform("projection",
                                                captureProjection);
    m_equirectangularToCubemapShader.setTexture(0, equiTexture);

    glViewport(0, 0, ENVMAP_SIZE, ENVMAP_SIZE);
    helper.framebuffer.bind();
    helper.renderbuffer.set(GL_DEPTH_COMPONENT24, ENVMAP_SIZE, ENVMAP_SIZE);
    for (unsigned int i = 0; i < 6; ++i) {
        m_equirectangularToCubemapShader.setUniform("view", captureViews[i]);
        glNamedFramebufferTextureLayer(helper.framebuffer.getId(),
                                       GL_COLOR_ATTACHMENT0, m_envmap->getId(),
                                       0, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    helper.framebuffer.unbind();
}
void Skybox::convolveDiffuseEnvmap() {

    // convert HDR equirectangular environment map to cubemap equivalent
    m_diffuseConvShader.use();
    m_diffuseConvShader.setUniform("projection", captureProjection);
    m_diffuseConvShader.setTexture(0, getEnvmap());

    glViewport(0, 0, DIFFUSECONV_SIZE, DIFFUSECONV_SIZE);
    helper.framebuffer.bind();
    helper.renderbuffer.set(GL_DEPTH_COMPONENT24, DIFFUSECONV_SIZE,
                            DIFFUSECONV_SIZE);
    for (unsigned int i = 0; i < 6; ++i) {
        m_diffuseConvShader.setUniform("view", captureViews[i]);
        glNamedFramebufferTextureLayer(helper.framebuffer.getId(),
                                       GL_COLOR_ATTACHMENT0,
                                       m_diffuseConv->getId(), 0, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    helper.framebuffer.unbind();
}

void Skybox::convolveSpecularEnvmap() {

    // convert HDR equirectangular environment map to cubemap equivalent
    m_specularConvShader.use();
    m_specularConvShader.setUniform("projection", captureProjection);
    m_specularConvShader.setTexture(0, getEnvmap());
    helper.framebuffer.bind();
    for (int mipLevel = 0; mipLevel < SPECULARCONV_MIPLEVEL; mipLevel++) {
        int mipSize = SPECULARCONV_SIZE >> mipLevel;
        helper.renderbuffer.set(GL_DEPTH_COMPONENT24, mipSize, mipSize);
        float roughness = (float)mipLevel / (float)(SPECULARCONV_MIPLEVEL - 1);
        m_specularConvShader.setUniform("roughness", roughness);
        glViewport(0, 0, mipSize, mipSize);
        for (unsigned int i = 0; i < 6; ++i) {
            m_specularConvShader.setUniform("view", captureViews[i]);
            glNamedFramebufferTextureLayer(
                helper.framebuffer.getId(), GL_COLOR_ATTACHMENT0,
                m_specularConv->getId(), mipLevel, i);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }
    }
    helper.framebuffer.unbind();
}
static const char* BRDFLUT_FILENAME = "brdfLUT.png";
void Skybox::initBRDFLUT(bool forceRecompute) {
    m_BRDFLUT = make_unique<Texture2D>();
    m_BRDFLUT->init();
    m_BRDFLUT->setWrapFilter(GL_CLAMP_TO_EDGE);
    m_BRDFLUT->setSizeFilter(GL_LINEAR, GL_LINEAR);
    // try to load from file
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char* data =
        stbi_load(BRDFLUT_FILENAME, &width, &height, &channels, 2);
    bool success = data && width == BRDFLUT_SIZE && height == BRDFLUT_SIZE;
    if (forceRecompute || !success) {
        ShaderProgram BRDFPrecomputeShader{
            Shader(FINALSCREEN_VERT, ShaderType::Vertex),
            Shader(BRDFLUT_FRAG, ShaderType::Fragment)};
        m_BRDFLUT->setupStorage(BRDFLUT_SIZE, BRDFLUT_SIZE, GL_RGB16F, 1);

        helper.framebuffer.bind();
        helper.renderbuffer.set(GL_DEPTH_COMPONENT24, BRDFLUT_SIZE,
                                BRDFLUT_SIZE);
        BRDFPrecomputeShader.use();
        glViewport(0, 0, BRDFLUT_SIZE, BRDFLUT_SIZE);
        glNamedFramebufferTexture(helper.framebuffer.getId(),
                                  GL_COLOR_ATTACHMENT0, m_BRDFLUT->getId(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Quad::globalQuad().draw();
        helper.framebuffer.unbind();

        int width = BRDFLUT_SIZE, height = BRDFLUT_SIZE;
        std::vector<unsigned char> data(width * height * 2);
        glGetTextureImage(m_BRDFLUT->getId(), 0, GL_RG, GL_UNSIGNED_BYTE,
                          data.size(), data.data());
        stbi_flip_vertically_on_write(true);
        stbi_write_png(BRDFLUT_FILENAME, width, height, 2, data.data(),
                       width * 2);
    } else {
        m_BRDFLUT->setup(data, width, height, GL_RG16F, GL_RG, GL_UNSIGNED_BYTE,
                         1);
        stbi_image_free(data);
    }
    panicPossibleGLError();
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
        m_envmap = createTextureCubeMapFromFiles(
            skyboxFilenames,
            TEXTURE_OPTION_CONVERT_TO_LINEAR | TEXTURE_OPTION_MIPMAP);
    } else {
        // compute cubemap from equirectangular map
        auto equiMap = createTexture2DFromHDRFile(path);
        if (!equiMap) {
            LOG(ERROR) << "Failed to load equirectangular map " << path << endl;
            return;
        }
        m_envmap = make_unique<TextureCubeMap>();
        m_envmap->init();
        m_envmap->setupStorage(ENVMAP_SIZE, ENVMAP_SIZE, GL_RGB16F, -1);
        m_envmap->setWrapFilter(GL_CLAMP_TO_EDGE);
        m_envmap->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

        LOG(INFO) << "Converting equirectangular map to cubemap" << endl;
        renderEquirectangularToCubemap(*equiMap);
        m_envmap->generateMipmap();
    }
    computePrefilteredEnvmap();
    this->path = path;
}
void Skybox::loadPureColor(const glm::vec3& value) {
    m_envmap = make_unique<TextureCubeMap>();
    m_envmap->init();
    m_envmap->setupStorage(1, 1, GL_RGB32F, -1);
    float data[3] = {value.r, value.g, value.b};
    for (int i = 0; i < 6; ++i) {
        m_envmap->setupFace(i, data, GL_RGB, GL_FLOAT);
    }
    m_envmap->setWrapFilter(GL_CLAMP_TO_EDGE);
    m_envmap->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    computePrefilteredEnvmap();
    path = "";
}
void Skybox::computePrefilteredEnvmap() {
    if (!m_diffuseConv) {
        m_diffuseConv = make_unique<TextureCubeMap>();
        m_diffuseConv->init();
        m_diffuseConv->setupStorage(DIFFUSECONV_SIZE, DIFFUSECONV_SIZE,
                                    GL_RGB16F, 1);
        m_diffuseConv->setWrapFilter(GL_CLAMP_TO_EDGE);
        m_diffuseConv->setSizeFilter(GL_LINEAR, GL_LINEAR);
    }
    if (!m_specularConv) {
        m_specularConv = make_unique<TextureCubeMap>();
        m_specularConv->init();
        m_specularConv->setupStorage(SPECULARCONV_SIZE, SPECULARCONV_SIZE,
                                     GL_RGB16F, SPECULARCONV_MIPLEVEL);
        m_specularConv->setWrapFilter(GL_CLAMP_TO_EDGE);
        m_specularConv->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    }
    LOG(INFO) << "Convolving diffuse envmap" << endl;
    convolveDiffuseEnvmap();
    LOG(INFO) << "Convolving specular envmap" << endl;
    convolveSpecularEnvmap();
}

void Skybox::draw() const {
    m_shader.use();
    m_shader.setTexture(SHADER_SAMPLER_PORT_SKYBOX, getEnvmap());
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

Skybox::~Skybox() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}