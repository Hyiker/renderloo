#include "core/RenderLoo.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <locale>
#include <loo/glError.hpp>
#include <memory>
#include <thread>
#include <vector>
#include "glm/gtx/string_cast.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <nfd.h>
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
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    static bool firstMouse = true;
    bool leftPressing =
             glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE,
         rightPressing = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) !=
                         GLFW_RELEASE;
    if (!leftPressing && !rightPressing) {
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
    if (rightPressing && myapp->getMainCameraMode() == CameraMode::FPS)
        dynamic_cast<FPSCamera&>(myapp->getMainCamera())
            .rotateCamera(xoffset, yoffset);
    else if (leftPressing && myapp->getMainCameraMode() == CameraMode::ArcBall)
        dynamic_cast<ArcBallCamera&>(myapp->getMainCamera())
            .orbitCamera(xoffset, yoffset);
}

static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
        return;
    }
    auto myapp = static_cast<RenderLoo*>(glfwGetWindowUserPointer(window));
    myapp->getMainCamera().zoomCamera(yOffset);
}
static std::unique_ptr<loo::PerspectiveCamera> placeCameraBySceneAABB(
    const AABB& aabb, CameraMode mode) {
    vec3 center = aabb.getCenter();
    vec3 diagonal = aabb.getDiagonal();
    float r = std::max(diagonal.x, diagonal.y) / 2.0;
    constexpr float EXPECTED_FOV = 90.f;
    float dist = r / sin(glm::radians(EXPECTED_FOV) / 2.0);
    float overlookAngle = glm::radians(45.f);
    float y = dist * tan(overlookAngle);
    vec3 pos = center + vec3(0, y, dist);
    if (mode == CameraMode::FPS)
        return std::make_unique<FPSCamera>(pos, center, vec3(0, 1, 0), 0.01f,
                                           std::max(100.0f, dist + r),
                                           EXPECTED_FOV);
    else if (mode == CameraMode::ArcBall)
        return std::make_unique<ArcBallCamera>(
            pos, center, vec3(0, 1, 0), 0.01f, std::max(100.0f, dist + r),
            EXPECTED_FOV);
    else
        return nullptr;
}

void RenderLoo::loadModel(const std::string& filename) {
    LOG(INFO) << "Loading model from " << filename << endl;
    m_scene.clear();
    auto meshes =
        createMeshesFromFile(m_scene.boneMap, m_scene.boneMatrices, &m_animator,
                             filename, m_scene.modelName);
    m_scene.addMeshes(std::move(meshes));
    if (m_scene.modelName.empty()) {
        fs::path p(filename);
        // filename without extension
        m_scene.modelName = p.stem().string();
    }

    m_scene.prepare();
    AABB sceneAABB = m_scene.computeAABBWorldSpace();
    glm::vec3 diagonal = sceneAABB.getDiagonal();
    LOG(INFO) << "diagnal: " << glm::to_string(diagonal) << endl;
    LOG(INFO) << "diagnal length: " << glm::length(diagonal) << endl;
    // scale the model to approximately 10x10x10
    float scale = 17.0f / glm::length(diagonal);
    m_scene.scale(glm::vec3(scale));
    LOG(INFO) << "scaled the model by: " << scale << endl;
    sceneAABB = m_scene.computeAABBWorldSpace();
    m_mainCamera = placeCameraBySceneAABB(sceneAABB, m_cameraMode);
    LOG(INFO) << "Load done" << endl;
}

void RenderLoo::loadSkybox(const std::string& filename) {
    m_skybox.loadTexture(filename);
}

RenderLoo::RenderLoo(int width, int height)
    : Application(width, height, "RenderLoo"),
      m_baseshader{Shader(GBUFFER_VERT, ShaderType::Vertex),
                   Shader(GBUFFER_FRAG, ShaderType::Fragment)},
      m_scene(),
      m_mainCamera(),
      m_shadowmapshader{Shader(SHADOWMAP_VERT, ShaderType::Vertex),
                        Shader(SHADOWMAP_FRAG, ShaderType::Fragment)},
      m_ssao(getWidth(), getHeight()),
      m_deferredshader{Shader(DEFERRED_VERT, ShaderType::Vertex),
                       Shader(DEFERRED_FRAG, ShaderType::Fragment)},
      m_smaa(getWidth(), getHeight()),
      m_finalprocess(getWidth(), getHeight()) {

    PBRMetallicMaterial::init();

    initGBuffers();
    initShadowMap();
    m_ssao.init(m_gbuffers.depthStencilRb);
    initDeferredPass();
    m_transparentPass.init(m_gbuffers.depthStencilRb, *m_deferredResult);
    m_smaa.init();

    // final pass related
    { m_finalprocess.init(); }
    if (m_lights.empty()) {
        ShaderLight sun;
        sun.setPosition(vec3(0, 10.0, 0));
        sun.setDirection(vec3(0, -1, 0));
        sun.color = vec4(1.0);
        sun.intensity = 0.0;
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

    // init bone uniform buffer
    ShaderProgram::initUniformBlock(std::make_unique<UniformBuffer>(
        SHADER_UB_PORT_BONES, sizeof(mat4) * BONES_MAX_COUNT));

    { glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); }

    NFD_Init();
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

    m_gbuffers.bufferD = make_unique<Texture2D>();
    m_gbuffers.bufferD->init();
    m_gbuffers.bufferD->setupStorage(getWidth(), getHeight(), GL_RGBA32F, 1);
    m_gbuffers.bufferD->setSizeFilter(GL_LINEAR, GL_LINEAR);

    panicPossibleGLError();

    m_gbuffers.depthStencilRb.init(GL_DEPTH24_STENCIL8, getWidth(),
                                   getHeight());

    m_gbufferfb.attachTexture(*m_gbuffers.position, GL_COLOR_ATTACHMENT0, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferA, GL_COLOR_ATTACHMENT1, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferB, GL_COLOR_ATTACHMENT2, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferC, GL_COLOR_ATTACHMENT3, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.bufferD, GL_COLOR_ATTACHMENT4, 0);
    m_gbufferfb.attachRenderbuffer(m_gbuffers.depthStencilRb,
                                   GL_DEPTH_ATTACHMENT);
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
    m_deferredResult = make_shared<Texture2D>();
    m_deferredResult->init();
    m_deferredResult->setupStorage(getWidth(), getHeight(), GL_RGBA32F, 1);
    m_deferredResult->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_deferredfb.attachTexture(*m_deferredResult, GL_COLOR_ATTACHMENT0, 0);
    m_deferredfb.attachRenderbuffer(m_gbuffers.depthStencilRb,
                                    GL_DEPTH_ATTACHMENT);
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

static void popupFileSelector(
    const std::vector<nfdfilteritem_t>& filter, const nfdchar_t* defaultPath,
    std::function<void(const std::string&)> cb,
    std::function<void(nfdresult_t)> cancelCb = [](nfdresult_t) {}) {
    nfdchar_t* outPath = nullptr;
    nfdresult_t result =
        NFD_OpenDialog(&outPath, filter.data(), filter.size(), defaultPath);
    if (result == NFD_OKAY) {
        cb(outPath);
    } else {
        cancelCb(result);
    }
    free(outPath);
}

void RenderLoo::gui() {
    beginEvent("GUI");
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
                bool hasAnimation = m_animator.hasAnimation();
                ImGui::TextColored(
                    ImVec4(hasAnimation ? 0.0f : 1.0f,
                           hasAnimation ? 1.0f : 0.0f, 0.0f, 1.0f),
                    "Animation: %s", hasAnimation ? "Yes" : "No");
            }
        }
    }

    {
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(300, h * 0.7));
        ImGui::SetNextWindowPos(ImVec2(0, h * 0.3), ImGuiCond_Always);
        if (ImGui::Begin("Options", nullptr, windowFlags)) {
            // Sun
            if (!m_lights.empty())
                if (ImGui::CollapsingHeader("Sun",
                                            ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::ColorEdit3("Color", (float*)&m_lights[0].color);
                    ImGui::SliderFloat3("Direction",
                                        (float*)&m_lights[0].direction, -1, 1);
                    ImGui::SliderFloat(
                        "Intensity", (float*)&m_lights[0].intensity, 0.0, 3.0);
                }
            // Camera
            if (ImGui::CollapsingHeader("Camera",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* cameraMode[] = {"FPS", "ArcBall"};
                if (ImGui::Combo("Mode", (int*)(&m_cameraMode), cameraMode,
                                 IM_ARRAYSIZE(cameraMode))) {
                    if (m_cameraMode == CameraMode::FPS) {
                        m_mainCamera = make_unique<FPSCamera>(*m_mainCamera);
                    } else if (m_cameraMode == CameraMode::ArcBall) {
                        auto arcballCam =
                            make_unique<ArcBallCamera>(*m_mainCamera);
                        arcballCam->setCenter(
                            m_scene.computeAABBWorldSpace().getCenter());
                        m_mainCamera = std::move(arcballCam);
                    }
                }
                if (m_cameraMode == CameraMode::ArcBall) {
                    static float rotationDPS = 10.f;
                    float deltaTime = getDeltaTime();
                    ImGui::SliderFloat("Rot(°/s)", &rotationDPS, 0, 360);
                    dynamic_cast<ArcBallCamera&>(*m_mainCamera)
                        .orbitCameraAroundWorldUp(rotationDPS * deltaTime);
                }
                ImGui::TextWrapped(
                    "General info: \n\tPosition: (%.2f, %.2f, "
                    "%.2f)\n\tDirection: (%.2f, %.2f, %.2f)\n\tFOV(°): %.2f",
                    m_mainCamera->position.x, m_mainCamera->position.y,
                    m_mainCamera->position.z, m_mainCamera->getDirection().x,
                    m_mainCamera->getDirection().y,
                    m_mainCamera->getDirection().z,
                    glm::degrees(m_mainCamera->getFov()));
            }
            // Model
            if (ImGui::CollapsingHeader("Model",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                static float modelRotation = 0.0f;
                if (ImGui::SliderFloat("Rotation(°)", &modelRotation, 0, 360)) {
                    float rotationRad = glm::radians(modelRotation);
                    m_scene.rotation =
                        glm::angleAxis(rotationRad, glm::vec3(0, 1, 0));
                }
            }
            // OpenGL option
            if (ImGui::CollapsingHeader("Render options",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Wire frame mode", &m_wireframe);
                ImGui::Checkbox("Normal mapping", &m_enablenormal);
                const char* antialiasmethod[] = {"None", "SMAA"};
                ImGui::Combo("Antialias", (int*)(&m_antialiasmethod),
                             antialiasmethod, IM_ARRAYSIZE(antialiasmethod));
                const char* ambientocclusionmethod[] = {"None", "SSAO", "HBAO",
                                                        "GTAO"};
                ImGui::Combo("AO", (int*)(&m_aomethod), ambientocclusionmethod,
                             IM_ARRAYSIZE(ambientocclusionmethod));
                if (m_aomethod == AOMethod::SSAO) {
                    ImGui::SliderFloat("bias", &m_ssao.bias, 0.0, 0.5);
                    ImGui::SliderFloat("radius", &m_ssao.radius, 0.0, 1.0);
                }
            }
            // Resources
            if (ImGui::CollapsingHeader("Assets",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                {
                    string path = m_skybox.path;
                    fs::path p(path);
                    string filename = p.filename().string();
                    ImGui::PushItemWidth(150);
                    ImGui::InputText("", const_cast<char*>(filename.c_str()),
                                     filename.size(),
                                     ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    if (ImGui::Button("Load HDRI")) {
                        pauseTime();
                        popupFileSelector({{"HDR Image", "hdr,exr"}}, nullptr,
                                          [this](const string& outPath) {
                                              m_skybox.loadTexture(outPath);
                                          });
                        resumeTime();
                    }
                    static glm::vec3 skyboxColor = glm::vec3(1.f);
                    ImGui::ColorEdit3("", (float*)&skyboxColor,
                                      ImGuiColorEditFlags_HDR);
                    ImGui::SameLine();
                    if (ImGui::Button("Apply")) {
                        m_skybox.loadPureColor(skyboxColor);
                    }
                }
                {
                    ImGui::PushItemWidth(150);
                    ImGui::InputText(
                        "", const_cast<char*>(m_scene.modelName.c_str()),
                        m_scene.modelName.size(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    if (ImGui::Button("Load Model")) {
                        pauseTime();
                        popupFileSelector({{"Model Files", "glb,gltf,obj,fbx"}},
                                          nullptr,
                                          [this](const string& outPath) {
                                              loadModel(outPath);
                                              convertMaterial();
                                          });
                        resumeTime();
                    }
                }
            }
        }
    }
    endEvent();
}

void RenderLoo::finalScreenPass(const loo::Texture2D& texture) {
    beginEvent("Final Screen Pass");
    m_finalprocess.render(texture);
    endEvent();
}

void RenderLoo::convertMaterial() {
    int cnt = 0;
    for (auto& mesh : m_scene.getMeshes()) {
        // Now default material is PBR material
        if (mesh->material) {
#ifdef MATERIAL_PBR
            if (!dynamic_pointer_cast<PBRMetallicMaterial>(mesh->material)) {
                auto pbrMaterial = convertPBRMetallicMaterialFromBaseMaterial(
                    *static_pointer_cast<BaseMaterial>(mesh->material));
                mesh->material = pbrMaterial;
                cnt++;
            }
#else
            LOG(INFO) << "Converting material to simple(blinn-phong) material";
            mesh->material = convertSimpleMaterialFromBaseMaterial(
                *static_pointer_cast<BaseMaterial>(mesh->material));
#endif
        }
    }
    LOG(INFO) << "Converted " << cnt << " materials to PBR materials";
}

void RenderLoo::skyboxPass() {
    Application::beginEvent("Skybox Pass");
    m_deferredfb.enableAttachments({GL_COLOR_ATTACHMENT0});
    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_GEQUAL);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_skybox.draw();
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    m_deferredfb.unbind();
    Application::endEvent();
}
void RenderLoo::gbufferPass() {
    beginEvent("GBuffer Pass");
    // render gbuffer here
    m_gbufferfb.bind();

    m_gbufferfb.enableAttachments({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                   GL_COLOR_ATTACHMENT4});

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glEnable(GL_STENCIL_TEST);

    glCullFace(GL_BACK);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0xFF);

    m_baseshader.use();
    scene(m_baseshader, RenderFlag_Opaque);

    m_gbufferfb.unbind();
    glStencilMask(0x00);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    endEvent();
}

void RenderLoo::shadowMapPass() {
    beginEvent("Shadow Map Pass");
    // render shadow map here
    m_mainlightshadowmapfb.bind();
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    glViewport(0, 0, SHADOWMAP_RESOLUION[0], SHADOWMAP_RESOLUION[1]);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glClearDepth(0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    m_shadowmapshader.use();
    // directional light matrix
    const auto& mainLight = m_lights[0];
    CHECK_EQ(mainLight.type, static_cast<int>(LightType::DIRECTIONAL));
    glm::mat4 lightSpaceMatrix = mainLight.getLightSpaceMatrix(true);

    m_shadowmapshader.setUniform("lightSpaceMatrix", lightSpaceMatrix);

    for (auto mesh : m_scene.getMeshes()) {
        if (mesh->needAlphaBlend())
            continue;
        drawMesh(*mesh, m_scene.getModelMatrix(), m_shadowmapshader);
    }

    m_mainlightshadowmapfb.unbind();
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    endEvent();

    glDepthFunc(GL_LESS);
    glClearDepth(1.0f);
}

void RenderLoo::deferredPass() {
    beginEvent("Deferred Pass");

    m_deferredfb.bind();
    m_deferredfb.enableAttachments({GL_COLOR_ATTACHMENT0});

    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    logPossibleGLError();

    m_deferredshader.use();
    m_deferredshader.setTexture(0, *m_gbuffers.position);
    m_deferredshader.setTexture(1, *m_gbuffers.bufferA);
    m_deferredshader.setTexture(2, *m_gbuffers.bufferB);
    m_deferredshader.setTexture(3, *m_gbuffers.bufferC);
    m_deferredshader.setTexture(4, *m_gbuffers.bufferD);
    m_deferredshader.setTexture(5, *m_mainlightshadowmap);
    m_deferredshader.setTexture(6, m_skybox.getDiffuseConv());
    m_deferredshader.setTexture(7, m_skybox.getSpecularConv());
    m_deferredshader.setTexture(8, m_skybox.getBRDFLUT());
    const loo::Texture2D& aoTex = m_aomethod == AOMethod::SSAO
                                      ? m_ssao.getAOTexture()
                                      : Texture2D::getWhiteTexture();
    m_deferredshader.setTexture(9, aoTex);
    m_deferredshader.setUniform("mainLightMatrix",
                                m_lights[0].getLightSpaceMatrix());

    m_deferredshader.setUniform("cameraPosition", m_mainCamera->position);

    ShaderProgram::getUniformBlock(SHADER_UB_PORT_LIGHTS)
        .mapBufferScoped<ShaderLightBlock>([&](ShaderLightBlock& lights) {
            lights.lightCount = m_lights.size();
            for (int i = 0; i < m_lights.size(); i++) {
                lights.lights[i] = m_lights[i];
            }
        });

    glDisable(GL_DEPTH_TEST);
    Quad::globalQuad().draw();
    glEnable(GL_DEPTH_TEST);

    endEvent();
}

void RenderLoo::scene(loo::ShaderProgram& shader, RenderFlag flag) {
    shader.use();

    shader.setUniform("enableNormal", m_enablenormal);
    logPossibleGLError();
    bool renderOpaque = flag & RenderFlag_Opaque,
         renderTransparent = flag & RenderFlag_Transparent;
    for (auto mesh : m_scene.getMeshes()) {
        if ((!mesh->needAlphaBlend() && renderOpaque) ||
            (mesh->needAlphaBlend() && renderTransparent)) {
            if (mesh->isDoubleSided() || renderTransparent) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }

            drawMesh(*mesh, m_scene.getModelMatrix(), m_baseshader);
        }
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
        m_mainCamera->moveCamera(CameraMovement::FORWARD, getDeltaTime());
    if (keyBackward())
        m_mainCamera->moveCamera(CameraMovement::BACKWARD, getDeltaTime());
    if (keyLeft())
        m_mainCamera->moveCamera(CameraMovement::LEFT, getDeltaTime());
    if (keyRight())
        m_mainCamera->moveCamera(CameraMovement::RIGHT, getDeltaTime());
    if (glfwGetKey(getWindow(), GLFW_KEY_R)) {
        m_mainCamera = placeCameraBySceneAABB(m_scene.aabb, m_cameraMode);
    }
}
void RenderLoo::mouse() {
    glfwSetCursorPosCallback(getWindow(), mouseCallback);
    glfwSetScrollCallback(getWindow(), scrollCallback);
}
void RenderLoo::loop() {
    m_mainCamera->setAspect(getWindowRatio());
    // render
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, getWidth(), getHeight());

    {
        animation();

        // setup camera shader uniform block
        ShaderProgram::getUniformBlock(SHADER_UB_PORT_MVP)
            .mapBufferScoped<MVP>([this](MVP& mvp) {
                m_mainCamera->getViewMatrix(mvp.view);
                m_mainCamera->getProjectionMatrix(mvp.projection, true);
            });

        gbufferPass();

        shadowMapPass();

        if (m_aomethod == AOMethod::SSAO)
            m_ssao.render(*this, *m_gbuffers.position, *m_gbuffers.bufferC);

        deferredPass();

        skyboxPass();

        m_transparentPass.render(m_scene, m_skybox, *m_mainCamera,
                                 *m_mainlightshadowmap, m_lights);

        if (m_antialiasmethod == AntiAliasMethod::SMAA) {
            beginEvent("SMAA Pass");
        }
        const Texture2D& texture = m_antialiasmethod == AntiAliasMethod::None
                                       ? *m_deferredResult
                                       : m_smaa.apply(*this, *m_deferredResult);
        if (m_antialiasmethod == AntiAliasMethod::SMAA) {
            endEvent();
        }

        finalScreenPass(texture);
    }

    keyboard();

    mouse();

    gui();
}

void RenderLoo::animation() {
    if (!m_animator.hasAnimation())
        return;
    m_animator.updateAnimation(getDeltaTime());
    auto& ub = ShaderProgram::getUniformBlock(SHADER_UB_PORT_BONES);
    ub.updateData(m_animator.finalBoneMatrices.data());
    panicPossibleGLError();
}

void RenderLoo::afterCleanup() {
    NFD_Quit();
}
