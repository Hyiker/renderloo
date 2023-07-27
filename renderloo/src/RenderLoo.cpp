#include "core/RenderLoo.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <locale>
#include <loo/glError.hpp>
#include <memory>
#include <vector>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <functional>
#include <glm/gtx/hash.hpp>
#include "core/Graphics.hpp"
#include "core/PBRMaterials.hpp"
#include "core/Transforms.hpp"

#include <imgui_impl_glfw.h>
#include "glog/logging.h"
#include "shaders/deferred.frag.hpp"
#include "shaders/deferred.vert.hpp"
#include "shaders/gbuffer.frag.hpp"
#include "shaders/gbuffer.vert.hpp"
#include "shaders/shadowmap.frag.hpp"
#include "shaders/shadowmap.vert.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
using namespace loo;
using namespace std;
using namespace glm;

namespace fs = std::filesystem;

static constexpr int SHADOWMAP_RESOLUION[2]{2048, 2048};

static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);
    static bool firstMouse = true;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        firstMouse = true;
        return;
    }
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    auto myapp = static_cast<RenderLoo*>(glfwGetWindowUserPointer(window));
    static float lastX = myapp->getWidth() / 2.0;
    static float lastY = myapp->getHeight() / 2.0;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;
    myapp->getCamera().processMouseMovement(xoffset, yoffset);
}

static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
        return;
    }
    auto myapp = static_cast<RenderLoo*>(glfwGetWindowUserPointer(window));
    myapp->getCamera().processMouseScroll(xOffset, yOffset);
}

void RenderLoo::loadModel(const std::string& filename) {
    LOG(INFO) << "Loading model from " << filename << endl;
    auto meshes = createMeshFromFile(filename);
    m_scene.addMeshes(std::move(meshes));

    m_scene.prepare();
    LOG(INFO) << "Load done" << endl;
}

void RenderLoo::loadGLTF(const std::string& filename) {
    LOG(INFO) << "Loading scene from " << filename << endl;
    // TODO: m_scene = createSceneFromFile(filename);
    auto meshes = createMeshFromFile(filename);
    m_scene.addMeshes(std::move(meshes));

    m_scene.prepare();
    LOG(INFO) << "Load done" << endl;
}

void RenderLoo::loadSkybox(const std::string& filename) {
    m_skybox.loadTexture(filename);
}

RenderLoo::RenderLoo(int width, int height)
    : Application(width, height, "High distance subsurface scattering"),
      m_baseshader{Shader(GBUFFER_VERT, ShaderType::Vertex),
                   Shader(GBUFFER_FRAG, ShaderType::Fragment)},
      m_scene(),
      m_maincam(vec3(0, 0, 0.15), vec3(0, 0, 0), glm::radians(45.f), 0.01,
                100.0),
      m_shadowmapshader{Shader(SHADOWMAP_VERT, ShaderType::Vertex),
                        Shader(SHADOWMAP_FRAG, ShaderType::Fragment)},
      m_deferredshader{Shader(DEFERRED_VERT, ShaderType::Vertex),
                       Shader(DEFERRED_FRAG, ShaderType::Fragment)},

      m_finalprocess(getWidth(), getHeight()) {

    PBRMetallicMaterial::init();

    initGBuffers();
    initShadowMap();
    initDeferredPass();

    // final pass related
    { m_finalprocess.init(); }
    if (m_lights.empty()) {
        ShaderLight sun;
        sun.setPosition(vec3(0, 10.0, 0));
        sun.setDirection(vec3(0, -1, 0));
        sun.color = vec4(1.0);
        sun.intensity = 1.0;
        sun.setType(LightType::DIRECTIONAL);
        m_lights.push_back(sun);
    }
    // init lights buffer
    ShaderProgram::initUniformBlock(std::make_unique<UniformBuffer>(
        SHADER_UB_PORT_LIGHTS, sizeof(ShaderLightBlock)));
    // init mvp uniform buffer
    ShaderProgram::initUniformBlock(
        std::make_unique<UniformBuffer>(SHADER_UB_PORT_MVP, sizeof(MVP)));
    panicPossibleGLError();
}
void RenderLoo::initGBuffers() {
    m_gbufferfb.init();

    int mipmapLevel = mipmapLevelFromSize(getWidth(), getHeight());

    m_gbuffers.position = make_unique<Texture2D>();
    m_gbuffers.position->init();
    m_gbuffers.position->setupStorage(getWidth(), getHeight(), GL_RGBA32F,
                                      mipmapLevel);
    m_gbuffers.position->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    m_gbuffers.bufferA = make_unique<Texture2D>();
    m_gbuffers.bufferA->init();
    m_gbuffers.bufferA->setupStorage(getWidth(), getHeight(), GL_RGBA32F,
                                     mipmapLevel);
    m_gbuffers.bufferA->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    m_gbuffers.bufferB = make_unique<Texture2D>();
    m_gbuffers.bufferB->init();
    m_gbuffers.bufferB->setupStorage(getWidth(), getHeight(), GL_RGBA32F,
                                     mipmapLevel);
    m_gbuffers.bufferB->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    m_gbuffers.bufferC = make_unique<Texture2D>();
    m_gbuffers.bufferC->init();
    m_gbuffers.bufferC->setupStorage(getWidth(), getHeight(), GL_RGBA32F,
                                     mipmapLevel);
    m_gbuffers.bufferC->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    panicPossibleGLError();

    m_gbuffers.depthrb.init(GL_DEPTH_COMPONENT32, getWidth(), getHeight());

    m_gbufferfb.attachTexture(*m_gbuffers.position, GL_COLOR_ATTACHMENT0, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferA, GL_COLOR_ATTACHMENT1, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferB, GL_COLOR_ATTACHMENT2, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferC, GL_COLOR_ATTACHMENT3, 0);
    m_gbufferfb.attachRenderbuffer(m_gbuffers.depthrb, GL_DEPTH_ATTACHMENT);
}

void RenderLoo::initShadowMap() {
    m_mainlightshadowmapfb.init();
    m_mainlightshadowmap = make_shared<Texture2D>();
    m_mainlightshadowmap->init();
    m_mainlightshadowmap->setupStorage(SHADOWMAP_RESOLUION[0],
                                       SHADOWMAP_RESOLUION[1],
                                       GL_DEPTH_COMPONENT32, 1);
    m_mainlightshadowmap->setSizeFilter(GL_NEAREST, GL_NEAREST);
    // m_mainlightshadowmap->setWrapFilter(GL_CLAMP_TO_EDGE);
    m_mainlightshadowmapfb.attachTexture(*m_mainlightshadowmap,
                                         GL_DEPTH_ATTACHMENT, 0);
    glNamedFramebufferDrawBuffer(m_mainlightshadowmapfb.getId(), GL_NONE);
    glNamedFramebufferReadBuffer(m_mainlightshadowmapfb.getId(), GL_NONE);
    panicPossibleGLError();
}
void RenderLoo::initDeferredPass() {
    m_deferredfb.init();
    m_diffuseresult = make_shared<Texture2D>();
    m_diffuseresult->init();
    m_diffuseresult->setupStorage(getWidth(), getHeight(), GL_RGB32F, 1);
    m_diffuseresult->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_specularresult = make_unique<Texture2D>();
    m_specularresult->init();
    m_specularresult->setupStorage(getWidth(), getHeight(), GL_RGB32F, 1);
    m_specularresult->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_skyboxresult = make_unique<Texture2D>();
    m_skyboxresult->init();
    m_skyboxresult->setupStorage(getWidth(), getHeight(), GL_RGB32F, 1);
    m_skyboxresult->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_deferredfb.attachTexture(*m_diffuseresult, GL_COLOR_ATTACHMENT0, 0);
    m_deferredfb.attachTexture(*m_specularresult, GL_COLOR_ATTACHMENT1, 0);
    m_deferredfb.attachTexture(*m_skyboxresult, GL_COLOR_ATTACHMENT2, 0);
    m_deferredfb.attachRenderbuffer(m_gbuffers.depthrb, GL_DEPTH_ATTACHMENT);
    panicPossibleGLError();
}
void RenderLoo::saveScreenshot(fs::path filename) const {
    std::vector<unsigned char> pixels(getWidth() * getHeight() * 3);
    glReadPixels(0, 0, getWidth(), getHeight(), GL_RGB, GL_UNSIGNED_BYTE,
                 pixels.data());
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filename.string().c_str(), getWidth(), getHeight(), 3,
                   pixels.data(), getWidth() * 3);
    stbi_flip_vertically_on_write(false);
    panicPossibleGLError();
}

void RenderLoo::gui() {
    auto& io = ImGui::GetIO();
    float h = io.DisplaySize.y;
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;
    {
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(300, h * 0.3));
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        if (ImGui::Begin("Dashboard", nullptr, windowFlags)) {
            // OpenGL option
            if (ImGui::CollapsingHeader("General info",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Press [H] to toggle GUI");
                ImGui::Text(
                    "Frame generation delay: %.3f ms/frame\n"
                    "FPS: %.1f",
                    1000.0f / io.Framerate, io.Framerate);

                const GLubyte* renderer = glGetString(GL_RENDERER);
                const GLubyte* version = glGetString(GL_VERSION);
                ImGui::TextWrapped("Renderer: %s", renderer);
                ImGui::TextWrapped("OpenGL Version: %s", version);
                int triangleCount = m_scene.countTriangle();
                const char* base = "";
                if (triangleCount > 5000) {
                    triangleCount /= 1000;
                    base = "k";
                }
                if (triangleCount > 5000) {
                    triangleCount /= 1000;
                    base = "M";
                }
                ImGui::Text(
                    "Scene meshes: %d\n"
                    "Scene triangles: %d%s",
                    (int)m_scene.countMesh(), triangleCount, base);
                ImGui::TextWrapped("Camera position: %.2f %.2f %.2f",
                                   m_maincam.getPosition().x,
                                   m_maincam.getPosition().y,
                                   m_maincam.getPosition().z);
            }
        }
    }

    {
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(300, h * 0.5));
        ImGui::SetNextWindowPos(ImVec2(0, h * 0.3), ImGuiCond_Always);
        if (ImGui::Begin("Options", nullptr, windowFlags)) {
            // Sun
            if (!m_lights.empty())
                if (ImGui::CollapsingHeader("Sun",
                                            ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::ColorPicker3("Color", (float*)&m_lights[0].color);
                    ImGui::SliderFloat3("Direction",
                                        (float*)&m_lights[0].direction, -1, 1);
                    ImGui::SliderFloat("Intensity",
                                       (float*)&m_lights[0].intensity, 0.0,
                                       100.0);
                }

            // OpenGL option
            if (ImGui::CollapsingHeader("Render options",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Wire frame mode", &m_wireframe);
                ImGui::Checkbox("Normal mapping", &m_enablenormal);
            }
        }
    }
    {
        float h_img = h * 0.2,
              w_img = h_img / io.DisplaySize.y * io.DisplaySize.x;
        ImGui::SetNextWindowBgAlpha(1.0f);
        vector<GLuint> textures{};
        ImGui::SetNextWindowSize(ImVec2(w_img * textures.size() + 40, h_img));
        ImGui::SetNextWindowPos(ImVec2(0, h * 0.8), ImGuiCond_Always);
        if (ImGui::Begin("Textures", nullptr,
                         windowFlags | ImGuiWindowFlags_NoDecoration)) {
            for (auto texId : textures) {
                ImGui::Image((void*)(intptr_t)texId, ImVec2(w_img, h_img),
                             ImVec2(0, 1), ImVec2(1, 0));
                ImGui::SameLine();
            }
        }
    }
}

void RenderLoo::finalScreenPass() {
    m_finalprocess.render(*m_diffuseresult, *m_specularresult,
                          *m_gbuffers.bufferB, *m_skyboxresult,
                          m_finalpassoptions);
}

void RenderLoo::convertMaterial() {
    for (auto& mesh : m_scene.getMeshes()) {
        // Now default material is PBR material
        if (mesh->material) {
#ifdef MATERIAL_PBR
            LOG(INFO) << "Converting material to PBR material";
            auto pbrMaterial = convertPBRMetallicMaterialFromBaseMaterial(
                *static_pointer_cast<BaseMaterial>(mesh->material));
            mesh->material = pbrMaterial;
#else
            LOG(INFO) << "Converting material to simple(blinn-phong) material";
            mesh->material = convertSimpleMaterialFromBaseMaterial(
                *static_pointer_cast<BaseMaterial>(mesh->material));
#endif
        }
    }
}

void RenderLoo::skyboxPass() {
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glm::mat4 view;
    m_maincam.getViewMatrix(view);
    m_skybox.draw(view);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}
void RenderLoo::gbufferPass() {
    // render gbuffer here
    m_gbufferfb.bind();

    m_gbufferfb.enableAttachments({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3});

    clear();

    scene();

    m_gbufferfb.unbind();
}

void RenderLoo::shadowMapPass() {
    // render shadow map here
    m_mainlightshadowmapfb.bind();
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    glViewport(0, 0, SHADOWMAP_RESOLUION[0], SHADOWMAP_RESOLUION[1]);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    m_shadowmapshader.use();
    // directional light matrix
    const auto& mainLight = m_lights[0];
    CHECK_EQ(mainLight.type, static_cast<int>(LightType::DIRECTIONAL));
    glm::mat4 lightSpaceMatrix = mainLight.getLightSpaceMatrix();

    m_shadowmapshader.setUniform("lightSpaceMatrix", lightSpaceMatrix);

    for (auto mesh : m_scene.getMeshes()) {
        drawMesh(*mesh, m_scene.getModelMatrix(), m_shadowmapshader);
    }

    m_mainlightshadowmapfb.unbind();
    glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void RenderLoo::deferredPass() {
    // render gbuffer here

    m_deferredfb.bind();
    m_deferredfb.enableAttachments(
        {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});

    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    logPossibleGLError();

    m_deferredshader.use();
    m_deferredshader.setTexture(0, *m_gbuffers.position);
    m_deferredshader.setTexture(1, *m_gbuffers.bufferA);
    m_deferredshader.setTexture(2, *m_gbuffers.bufferB);
    m_deferredshader.setTexture(3, *m_gbuffers.bufferC);
    m_deferredshader.setTexture(4, *m_mainlightshadowmap);
    m_deferredshader.setTexture(5, m_skybox.getDiffuseConv());
    m_deferredshader.setUniform("mainLightMatrix",
                                m_lights[0].getLightSpaceMatrix());

    m_deferredshader.setUniform("cameraPosition", m_maincam.getPosition());

    ShaderProgram::getUniformBlock(SHADER_UB_PORT_LIGHTS)
        .mapBufferScoped<ShaderLightBlock>([&](ShaderLightBlock& lights) {
            lights.lightCount = m_lights.size();
            for (int i = 0; i < m_lights.size(); i++) {
                lights.lights[i] = m_lights[i];
            }
        });

    glDisable(GL_DEPTH_TEST);
    Quad::globalQuad().draw();

    m_deferredfb.enableAttachments({GL_COLOR_ATTACHMENT2});
    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    skyboxPass();
    m_gbufferfb.unbind();
}

void RenderLoo::scene() {
    glEnable(GL_DEPTH_TEST);
    ShaderProgram::getUniformBlock(SHADER_UB_PORT_MVP)
        .mapBufferScoped<MVP>([this](MVP& mvp) {
            m_maincam.getViewMatrix(mvp.view);
            m_maincam.getProjectionMatrix(mvp.projection);
        });

    logPossibleGLError();
    m_baseshader.use();

    m_baseshader.setUniform("enableNormal", m_enablenormal);
    logPossibleGLError();

    for (auto mesh : m_scene.getMeshes()) {
        drawMesh(*mesh, m_scene.getModelMatrix(), m_baseshader);
    }
    logPossibleGLError();
}

void RenderLoo::clear() {
    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    logPossibleGLError();
}
void RenderLoo::keyboard() {
    if (keyForward())
        m_maincam.processKeyboard(CameraMovement::FORWARD, getFrameDeltaTime());
    if (keyBackward())
        m_maincam.processKeyboard(CameraMovement::BACKWARD,
                                  getFrameDeltaTime());
    if (keyLeft())
        m_maincam.processKeyboard(CameraMovement::LEFT, getFrameDeltaTime());
    if (keyRight())
        m_maincam.processKeyboard(CameraMovement::RIGHT, getFrameDeltaTime());
    if (glfwGetKey(getWindow(), GLFW_KEY_R)) {
        m_maincam = Camera();
    }
    static bool h_pressed = false;
    if (glfwGetKey(getWindow(), GLFW_KEY_H) == GLFW_PRESS) {
        h_pressed = true;
    } else if (glfwGetKey(getWindow(), GLFW_KEY_H) == GLFW_RELEASE &&
               h_pressed) {
        h_pressed = false;
    }
}
void RenderLoo::mouse() {
    glfwSetCursorPosCallback(getWindow(), mouseCallback);
    glfwSetScrollCallback(getWindow(), scrollCallback);
}
void RenderLoo::loop() {
    m_maincam.m_aspect = getWindowRatio();
    // render
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, getWidth(), getHeight());

    {
        gbufferPass();

        shadowMapPass();

        deferredPass();

        finalScreenPass();
    }

    keyboard();

    mouse();

    gui();
}

void RenderLoo::afterCleanup() {}
